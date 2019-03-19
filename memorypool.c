#include "memorypool.h"

void 
get_memory_list_count(MemoryPool *mp, mem_size_t *mlist_len)
{
    mem_size_t mlist_l = 0;
    Memory *mm = mp->mlist;
    while (mm)
    {
        mlist_l++;
        mm = mm->next;
    }

    *mlist_len = mlist_l;
}

void 
get_memory_info(Memory *mm, mem_size_t *free_list_len, mem_size_t *alloc_list_len)
{
    mem_size_t free_l = 0, alloc_l = 0;
    Chunk *p = mm->free_list;
    while (p)
    {
        free_l++;
        p = p->next;
    }

    p = mm->alloc_list;
    while (p)
    {
        alloc_l++;
        p = p->next;
    }

    *free_list_len  = free_l;
    *alloc_list_len = alloc_l;
}

int 
get_memory_id(Memory *mm)
{
    return mm->id;
}


static Memory *
extend_memory_list(MemoryPool *mp)
{
    Memory *mm = (Memory *)malloc(sizeof(Memory));
    if (!mm)
        return NULL;

    mm->start = (char *)malloc(mp->mem_pool_size * sizeof(char));
    if (!mm->start)
        return NULL;

    init_Memory(mm);
    mm->id    = mp->last_id++;
    mm->next  = mp->mlist;
    mp->mlist = mm;
    return mm;
}

static Memory *
find_memory_list(MemoryPool *mp, void *p)
{
    Memory *tmp = mp->mlist;
    while (tmp)
    {
        if (tmp->start <= (char *)p 
            && tmp->start + mp->mem_pool_size > (char *)p)
            break;
        tmp = tmp->next;
    }

    return tmp;
}

static int 
merge_free_chunk(MemoryPool *mp, Memory *mm, Chunk *c)
{
    if (mp == NULL || mm == NULL || c == NULL)
        return 1;

    Chunk *p0 = c, *p1 = c;
    if ((char *)c - CHUNKEND > mm->start) {
        p0 = *(Chunk **)((char *)c - CHUNKEND);
        p1 = p0;
    }
    while ((char *)p0 > mm->start && p0->is_free)
    {
        p1 = p0;
        p0 = *(Chunk **)((char *)p0 - CHUNKEND);
    }

    p0 = (Chunk *)((char *)p1 + p1->alloc_mem);
    while ((char *)p0 < mm->start + mp->mem_pool_size && p0->is_free)
    {
        dlinklist_delete(mm->free_list, p0);

        p1->alloc_mem += p0->alloc_mem;
        p0 = (Chunk *)((char *)p0 + p0->alloc_mem);
    }

    *(Chunk **)((char *)p1 + p1->alloc_mem - CHUNKEND) = p1;

    return 0;
}

MemoryPool *
MemoryPool_Init(mem_size_t mempoolsize, int auto_extend)
{
    if (mempoolsize > MAX_MEM_SIZE)
    {
        // printf("[MemoryPool_Init] MemPool Init ERROR! Mempoolsize is too big! \n");
        return NULL;
    }

    MemoryPool *mp = (MemoryPool *)malloc(sizeof(MemoryPool));
    if (!mp)
    {
        // printf("[MemoryPool_Init] Alloc memorypool struct error!\n");
        return NULL;
    }

    mp->total_mem_pool_size = mp->mem_pool_size = mempoolsize;
    mp->auto_extend = auto_extend;
    mp->last_id     = 0;

    mp->mlist = (Memory*)malloc(sizeof(Memory));
    if (!mp->mlist)
        return NULL;

    char *s = (char *)malloc(sizeof(char) * mp->mem_pool_size); // memory pool
    if (!s)
    {
        // printf("[MemoryPool_Init] No memory! \n");
        return NULL;
    }
    mp->mlist->start = s;
    init_Memory(mp->mlist);
    mp->mlist->next = NULL;
    mp->mlist->id   = mp->last_id++;

    return mp;
}

void *
MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize)
{
    mem_size_t total_needed_size = wantsize + CHUNKHEADER + CHUNKEND;
    if (total_needed_size > mp->mem_pool_size) return NULL;

    Memory *mm    = NULL, *mm1       = NULL;
    Chunk  *_free = NULL, *_not_free = NULL;

FIND_FREE_CHUNK:
    mm = mp->mlist;
    while (mm)
    {
        if (mp->mem_pool_size - mm->alloc_mem < total_needed_size)
        {
            mm = mm->next;
            continue;
        }

        _free = mm->free_list;
        _not_free = NULL;

        while (_free)
        {
            if (_free->alloc_mem >= total_needed_size)
            {
                // 如果free块分割后剩余内存足够大 则进行分割
                if (_free->alloc_mem - total_needed_size > CHUNKHEADER + CHUNKEND)
                {
                    // 从free块头开始分割出alloc块
                    // nf指向分割后的free块
                    _not_free = _free;

                    _free = (Chunk *)((char *)_not_free + total_needed_size);
                    *_free    = *_not_free;
                    _free->alloc_mem -= total_needed_size;
                    *(Chunk **)((char *)_free + _free->alloc_mem - CHUNKEND) = _free;
                    
                    // update free_list
                    if (!_free->prev)
                    {
                        mm->free_list = _free;
                        if (_free->next) _free->next->prev = _free;
                    } else {
                        _free->prev->next = _free;
                        if (_free->next) _free->next->prev = _free;
                    }

                    _not_free->is_free   = 0;
                    _not_free->alloc_mem = total_needed_size;

                    *(Chunk **)((char *)_not_free + total_needed_size - CHUNKEND) = _not_free;
                }
                // 不够 则整块分配为alloc
                else
                {
                    _not_free = _free;
                    dlinklist_delete(mm->free_list, _not_free);
                    _not_free->is_free = 0;
                }

                dlinklist_insert_front(mm->alloc_list, _not_free);

                mm->alloc_mem      += _not_free->alloc_mem;
                mm->alloc_prog_mem += (_not_free->alloc_mem - CHUNKHEADER - CHUNKEND);

                return (void *)((char *)_not_free + CHUNKHEADER);
            }
            _free = _free->next;
        }

        mm = mm->next;
    }

    if (mp->auto_extend)
    {
        // 超过总内存限制
        if (mp->total_mem_pool_size + mp->mem_pool_size > MAX_MEM_SIZE)
            return NULL;
        mm1 = extend_memory_list(mp);
        if (!mm1)
            return NULL;
        mp->total_mem_pool_size += mp->mem_pool_size;
        goto FIND_FREE_CHUNK;
    }

    // printf("[MemoryPool_Alloc] No enough memory! \n");
    return NULL;
}

int 
MemoryPool_Free(MemoryPool *mp, void *p)
{
    if (p == NULL || mp == NULL)
        return 1;

    Memory *mm = mp->mlist;
    if (mp->auto_extend)
        mm = find_memory_list(mp, p);

    Chunk *ck = (Chunk *)((char *)p - CHUNKHEADER);
    dlinklist_delete(mm->alloc_list, ck);
    dlinklist_insert_front(mm->free_list, ck);
    ck->is_free = 1;

    mm->alloc_mem      -= ck->alloc_mem;
    mm->alloc_prog_mem -= (ck->alloc_mem - CHUNKHEADER - CHUNKEND);

    return merge_free_chunk(mp, mm, ck);
}

MemoryPool *
MemoryPool_Clear(MemoryPool *mp)
{
    if (mp == NULL)
    {
        // printf("[MemoryPool_Clear] Memory Pool is NULL! \n");
        return NULL;
    }

    Memory *mm = mp->mlist;
    while (mm)
    {
        init_Memory(mm);
        mm = mm->next;
    }

    return mp;
}

int 
MemoryPool_Destroy(MemoryPool *mp)
{
    if (mp == NULL)
    {
        // printf("[MemoryPool_Destroy] Memory Pool is NULL! \n");
        return 1;
    }

    Memory *mm = mp->mlist, *mm1 = NULL;
    while (mm)
    {
        free(mm->start);
        mm1 = mm;
        mm  = mm->next;
        free(mm1);
    }
    free(mp);

    return 0;
}

double 
get_mempool_usage(MemoryPool *mp)
{
    mem_size_t total_alloc = 0;
    Memory *mm = mp->mlist;
    while (mm)
    {
        total_alloc += mm->alloc_mem;
        mm = mm->next;
    }
    return (double)total_alloc / mp->total_mem_pool_size;
}

double 
get_mempool_prog_usage(MemoryPool *mp)
{
    mem_size_t total_alloc_prog = 0;
    Memory *mm = mp->mlist;
    while (mm)
    {
        total_alloc_prog += mm->alloc_prog_mem;
        mm = mm->next;
    }
    return (double)total_alloc_prog / mp->total_mem_pool_size;
}

