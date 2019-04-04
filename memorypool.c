#include "memorypool.h"

#define MP_CHUNKHEADER sizeof(struct _mp_chunk)
#define MP_CHUNKEND    sizeof(struct _mp_chunk *)

#define MP_LOCK(flag, lockobj) do { \
    if (flag) pthread_mutex_lock(&lockobj->lock); \
} while (0)
#define MP_UNLOCK(flag, lockobj) do { \
    if (flag) pthread_mutex_unlock(&lockobj->lock); \
} while (0)

#define MP_ALIGN_SIZE(_n) (_n + sizeof(long) - ((sizeof(long)-1)&_n))

#define MP_INIT_MEMORY_STRUCT(mm, mem_sz) do { \
    mm->mem_pool_size = mem_sz; \
    mm->alloc_mem = 0; \
    mm->alloc_prog_mem = 0; \
    mm->free_list = (_MP_Chunk *)mm->start; \
    mm->free_list->is_free = 1; \
    mm->free_list->alloc_mem = mem_sz; \
    mm->free_list->prev = NULL; \
    mm->free_list->next = NULL; \
    mm->alloc_list = NULL; \
} while (0)

#define MP_DLINKLIST_INS_FRT(head,x) do { \
    x->prev = NULL; \
    x->next = head; \
    if (head) \
        head->prev = x; \
    head = x; \
} while(0)

#define MP_DLINKLIST_DEL(head,x) do { \
    if (!x->prev) { \
        head = x->next; \
        if (x->next) x->next->prev = NULL; \
    } else { \
        x->prev->next = x->next; \
        if (x->next) x->next->prev = x->prev; \
    } \
} while(0)


void 
get_memory_list_count(MemoryPool *mp, mem_size_t *mlist_len) {
    MP_LOCK(mp->thread_safe, mp);
    mem_size_t mlist_l = 0;
    _MP_Memory *mm = mp->mlist;
    while (mm) {
        mlist_l++;
        mm = mm->next;
    }
    MP_UNLOCK(mp->thread_safe, mp);
    *mlist_len = mlist_l;
}

void 
get_memory_info(MemoryPool *mp, _MP_Memory *mm, mem_size_t *free_list_len, mem_size_t *alloc_list_len) {
    MP_LOCK(mp->thread_safe, mp);
    mem_size_t free_l = 0, alloc_l = 0;
    _MP_Chunk *p = mm->free_list;
    while (p) {
        free_l++;
        p = p->next;
    }

    p = mm->alloc_list;
    while (p) {
        alloc_l++;
        p = p->next;
    }
    MP_UNLOCK(mp->thread_safe, mp);
    *free_list_len  = free_l;
    *alloc_list_len = alloc_l;
}

int 
get_memory_id(_MP_Memory *mm) {
    return mm->id;
}


static _MP_Memory *
extend_memory_list(MemoryPool *mp, mem_size_t new_mem_sz) {
    char *s = (char *)malloc(sizeof(_MP_Memory) 
            + new_mem_sz * sizeof(char));
    if (!s) return NULL;

    _MP_Memory *mm = (_MP_Memory *)s;
    mm->start = s + sizeof(_MP_Memory);

    MP_INIT_MEMORY_STRUCT(mm, new_mem_sz);
    mm->id    = mp->last_id++;
    mm->next  = mp->mlist;
    mp->mlist = mm;
    // pthread_mutex_init(&mp->mlist->lock, NULL);
    return mm;
}

static _MP_Memory *
find_memory_list(MemoryPool *mp, void *p) {
    _MP_Memory *tmp = mp->mlist;
    while (tmp) {
        if (tmp->start <= (char *)p 
            && tmp->start + mp->mem_pool_size > (char *)p)
            break;
        tmp = tmp->next;
    }

    return tmp;
}

static int 
merge_free_chunk(MemoryPool *mp, _MP_Memory *mm, _MP_Chunk *c) {
    if (mp == NULL || mm == NULL || c == NULL || !c->is_free)
        return 1;

    _MP_Chunk *p0 = c, *p1 = c;
    while (p0->is_free) {
        p1 = p0;
        if ((char *)p0 - MP_CHUNKEND - MP_CHUNKHEADER <= mm->start)
            break;
        p0 = *(_MP_Chunk **)((char *)p0 - MP_CHUNKEND);
    }

    p0 = (_MP_Chunk *)((char *)p1 + p1->alloc_mem);
    while ((char *)p0 < mm->start + mp->mem_pool_size && p0->is_free) {
        MP_DLINKLIST_DEL(mm->free_list, p0);
        p1->alloc_mem += p0->alloc_mem;
        p0 = (_MP_Chunk *)((char *)p0 + p0->alloc_mem);
    }

    *(_MP_Chunk **)((char *)p1 + p1->alloc_mem - MP_CHUNKEND) = p1;
    MP_UNLOCK(mp->thread_safe, mp);
    return 0;
}

MemoryPool *
MemoryPool_Init(mem_size_t maxmempoolsize, mem_size_t mempoolsize, int thread_safe) {
    if (mempoolsize > maxmempoolsize) {
        // printf("[MemoryPool_Init] MemPool Init ERROR! Mempoolsize is too big! \n");
        return NULL;
    }

    MemoryPool *mp = (MemoryPool *)malloc(sizeof(MemoryPool));
    if (!mp) return NULL;

    mp->last_id = 0;
    if (mempoolsize < maxmempoolsize) 
        mp->auto_extend = 1;
    mp->thread_safe = thread_safe;
    mp->max_mem_pool_size   = maxmempoolsize;
    mp->total_mem_pool_size = mp->mem_pool_size = mempoolsize;
    pthread_mutex_init(&mp->lock, NULL);

    char *s = (char *)malloc(sizeof(_MP_Memory) 
            + sizeof(char) * mp->mem_pool_size);
    if (!s) return NULL;

    mp->mlist = (_MP_Memory *)s;
    mp->mlist->start = s + sizeof(_MP_Memory);
    MP_INIT_MEMORY_STRUCT(mp->mlist, mp->mem_pool_size);
    mp->mlist->next = NULL;
    mp->mlist->id   = mp->last_id++;
    // pthread_mutex_init(&mp->mlist->lock, NULL);

    return mp;
}

void *
MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize)
{
    mem_size_t total_needed_size = MP_ALIGN_SIZE(wantsize + MP_CHUNKHEADER + MP_CHUNKEND);
    if (total_needed_size > mp->mem_pool_size) return NULL;

    _MP_Memory *mm    = NULL, *mm1       = NULL;
    _MP_Chunk  *_free = NULL, *_not_free = NULL;

    MP_LOCK(mp->thread_safe, mp);
FIND_FREE_CHUNK:
    mm = mp->mlist;
    while (mm) {
        if (mp->mem_pool_size - mm->alloc_mem < total_needed_size) {
            mm = mm->next;
            continue;
        }

        _free = mm->free_list;
        _not_free = NULL;

        while (_free) {
            if (_free->alloc_mem >= total_needed_size) {
                // 如果free块分割后剩余内存足够大 则进行分割
                if (_free->alloc_mem - total_needed_size > MP_CHUNKHEADER + MP_CHUNKEND) {
                    // 从free块头开始分割出alloc块
                    _not_free = _free;

                    _free   = (_MP_Chunk *)((char *)_not_free + total_needed_size);
                    *_free  = *_not_free;
                    _free->alloc_mem -= total_needed_size;
                    *(_MP_Chunk **)((char *)_free + _free->alloc_mem - MP_CHUNKEND) = _free;
                    
                    // update free_list
                    if (!_free->prev) {
                        mm->free_list = _free;
                        if (_free->next) _free->next->prev = _free;
                    } else {
                        _free->prev->next = _free;
                        if (_free->next) _free->next->prev = _free;
                    }

                    _not_free->is_free   = 0;
                    _not_free->alloc_mem = total_needed_size;

                    *(_MP_Chunk **)((char *)_not_free + total_needed_size - MP_CHUNKEND) = _not_free;
                }
                // 不够 则整块分配为alloc
                else {
                    _not_free = _free;
                    MP_DLINKLIST_DEL(mm->free_list, _not_free);
                    _not_free->is_free = 0;
                }
                MP_DLINKLIST_INS_FRT(mm->alloc_list, _not_free);

                mm->alloc_mem      += _not_free->alloc_mem;
                mm->alloc_prog_mem += (_not_free->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

                MP_UNLOCK(mp->thread_safe, mp);
                return (void *)((char *)_not_free + MP_CHUNKHEADER);
            }
            _free = _free->next;
        }

        mm = mm->next;
    }

    if (mp->auto_extend) {
        // 超过总内存限制
        if (mp->total_mem_pool_size >= mp->max_mem_pool_size)
            return NULL;
        mem_size_t add_mem_sz = mp->max_mem_pool_size - mp->mem_pool_size;
        add_mem_sz = add_mem_sz >= mp->mem_pool_size ? mp->mem_pool_size : add_mem_sz;
        mm1 = extend_memory_list(mp, add_mem_sz);
        if (!mm1) return NULL;
        mp->total_mem_pool_size += add_mem_sz;
        
        goto FIND_FREE_CHUNK;
    }

    // printf("[MemoryPool_Alloc] No enough memory! \n");
    MP_UNLOCK(mp->thread_safe, mp);
    return NULL;
}

void * 
MemoryPool_Calloc (MemoryPool *mp, mem_size_t wantsize) {
    char *m = (char *)MemoryPool_Alloc(mp, wantsize);
    if (!m) return NULL;
    _MP_Chunk *ck = (_MP_Chunk *)(m - MP_CHUNKHEADER);
    memset((void *)m, ck->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND, 0);
    return m;
}

int 
MemoryPool_Free(MemoryPool *mp, void *p)
{
    if (p == NULL || mp == NULL)
        return 1;
    MP_LOCK(mp->thread_safe, mp);
    _MP_Memory *mm = mp->mlist;
    if (mp->auto_extend)
        mm = find_memory_list(mp, p);

    _MP_Chunk *ck = (_MP_Chunk *)((char *)p - MP_CHUNKHEADER);
    
    MP_DLINKLIST_DEL(mm->alloc_list, ck);
    MP_DLINKLIST_INS_FRT(mm->free_list,  ck);
    ck->is_free = 1;

    mm->alloc_mem      -= ck->alloc_mem;
    mm->alloc_prog_mem -= (ck->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

    return merge_free_chunk(mp, mm, ck);
}

MemoryPool *
MemoryPool_Clear(MemoryPool *mp) {
    if (!mp) return NULL;
    MP_LOCK(mp->thread_safe, mp);
    _MP_Memory *mm = mp->mlist;
    while (mm) {
        MP_INIT_MEMORY_STRUCT(mm, mm->mem_pool_size);
        mm = mm->next;
    }
    MP_UNLOCK(mp->thread_safe, mp);
    return mp;
}

int 
MemoryPool_Destroy(MemoryPool *mp) {
    if (mp == NULL) return 1;
    MP_LOCK(mp->thread_safe, mp);
    _MP_Memory *mm = mp->mlist, *mm1 = NULL;
    while (mm) {
        mm1 = mm;
        mm  = mm->next;
        // pthread_mutex_destroy(&mm1->lock);
        free(mm1);
    }
    MP_UNLOCK(mp->thread_safe, mp);
    pthread_mutex_destroy(&mp->lock);
    free(mp);

    return 0;
}

int 
MemoryPool_SetThreadSafe(MemoryPool *mp, int thread_safe) {
    MP_LOCK(mp->thread_safe, mp);
    if (mp->thread_safe) {
        mp->thread_safe = thread_safe;
        MP_UNLOCK(1, mp);
    } else {
        mp->thread_safe = thread_safe;
    }
    return 0;
}

mem_size_t
get_used_memory(MemoryPool *mp) {
    MP_LOCK(mp->thread_safe, mp);
    mem_size_t total_alloc = 0;
    _MP_Memory *mm = mp->mlist;
    while (mm) {
        total_alloc += mm->alloc_mem;
        mm = mm->next;
    }
    MP_UNLOCK(mp->thread_safe, mp);
    return total_alloc;
}

mem_size_t
get_prog_memory(MemoryPool *mp) {
    MP_LOCK(mp->thread_safe, mp);
    mem_size_t total_alloc_prog = 0;
    _MP_Memory *mm = mp->mlist;
    while (mm) {
        total_alloc_prog += mm->alloc_prog_mem;
        mm = mm->next;
    }
    MP_UNLOCK(mp->thread_safe, mp);
    return total_alloc_prog;
}

float 
get_mempool_usage(MemoryPool *mp) {
    return (float)get_used_memory(mp) / mp->total_mem_pool_size;
}

float 
get_mempool_prog_usage(MemoryPool *mp) {
    return (float)get_prog_memory(mp) / mp->total_mem_pool_size;
}

#undef MP_CHUNKHEADER
#undef MP_CHUNKEND
#undef MP_LOCK
#undef MP_UNLOCK
#undef MP_ALIGN_SIZE
#undef MP_INIT_MEMORY_STRUCT
#undef MP_DLINKLIST_INS_FRT
#undef MP_DLINKLIST_DEL