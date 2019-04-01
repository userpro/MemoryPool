#ifndef _Z_MEMORYPOOL_H_
#define _Z_MEMORYPOOL_H_
#include <stdlib.h>
#include <pthread.h>

#define mem_size_t  unsigned long long
#define MP_CHUNKHEADER sizeof(struct _chunk)
#define MP_CHUNKEND    sizeof(struct _chunk *)

#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

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
    mm->free_list = (_Chunk *)mm->start; \
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

typedef struct _chunk
{
    mem_size_t alloc_mem;
    struct _chunk *prev, *next;
    int is_free;
} _Chunk;

typedef struct _mem_pool_list
{
    char *start;
    unsigned int    id;
    mem_size_t mem_pool_size;
    mem_size_t alloc_mem;
    mem_size_t alloc_prog_mem;
    _Chunk *free_list, *alloc_list;
    struct _mem_pool_list *next;
    // pthread_mutex_t lock;
} _Memory;

typedef struct _mem_pool
{
    unsigned int last_id;
    int          auto_extend;
    int          thread_safe;
    mem_size_t   mem_pool_size, total_mem_pool_size, max_mem_pool_size;
    struct _mem_pool_list *mlist;
    pthread_mutex_t lock;
} MemoryPool;

/*
 *  内部工具函数
 */

// 所有Memory的数量
void get_memory_list_count(MemoryPool *mp, mem_size_t *mlist_len);
// 每个Memory的统计信息
void get_memory_info(MemoryPool *mp, _Memory *mm, mem_size_t *free_list_len, mem_size_t *alloc_list_len);
int  get_memory_id(_Memory *mm);

/*
 *  内存池API
 */

MemoryPool* MemoryPool_Init   (mem_size_t maxmempoolsize, mem_size_t mempoolsize, int thread_safe);
void*       MemoryPool_Alloc  (MemoryPool *mp, mem_size_t wantsize);
int         MemoryPool_Free   (MemoryPool *mp, void *p);
MemoryPool* MemoryPool_Clear  (MemoryPool *mp);
int         MemoryPool_Destroy(MemoryPool *mp);

/*
 *  内存池信息API
 */

// 实际分配空间
mem_size_t get_used_memory  (MemoryPool *mp);
float      get_mempool_usage(MemoryPool *mp);
// 数据占用空间
mem_size_t get_prog_memory  (MemoryPool *mp);
float      get_mempool_prog_usage(MemoryPool *mp);


#endif // !_Z_MEMORYPOOL_H_

