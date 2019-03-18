#ifndef _z_memorypool_h_
#define _z_memorypool_h_

#include <stdlib.h>

#define MAX_MEM_SIZE (32 * GB)

#ifndef mem_size_t
    #define mem_size_t unsigned long long
#endif

#ifndef CHUNKHEADER
    #define CHUNKHEADER sizeof(struct _chunk)
#endif

#ifndef CHUNKEND
    #define CHUNKEND sizeof(struct _chunk *)
#endif


#define init_Memory(mm) do { \
    mm->alloc_mem = 0; \
    mm->alloc_prog_mem = 0; \
    mm->free_list = (Chunk *)mm->start; \
    mm->free_list->is_free = 1; \
    mm->free_list->alloc_mem = mp->mem_pool_size; \
    mm->free_list->prev = NULL; \
    mm->free_list->next = NULL; \
    mm->alloc_list = NULL; \
} while (0)

#define dlinklist_insert_front(head,x) do { \
    x->prev = NULL; \
    x->next = head; \
    if (head) \
        head->prev = x; \
    head = x; \
} while(0)

#define dlinklist_delete(head,x) do { \
    if (!x->prev) { \
        head = x->next; \
        if (x->next) x->next->prev = NULL; \
    } else { \
        x->prev->next = x->next; \
        if (x->next) x->next->prev = x->prev; \
    } \
} while(0)


#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

typedef struct _chunk
{
    mem_size_t alloc_mem;
    struct _chunk *prev, *next;
    int is_free;
}Chunk;

typedef struct _mem_pool_list
{
    char *start;
    unsigned int id;
    mem_size_t alloc_mem;
    mem_size_t alloc_prog_mem;
    Chunk *free_list, *alloc_list;
    struct _mem_pool_list *next;
}Memory;

typedef struct _mem_pool
{
    unsigned int last_id;
    int          auto_extend;
    mem_size_t   mem_pool_size, total_mem_pool_size;
    struct _mem_pool_list *mlist;
}MemoryPool;

/*
 *  内部工具函数
 */

// 所有Memory的数量
void get_memory_list_count(MemoryPool *mp, mem_size_t *mlist_len);

// 每个Memory的统计信息
void get_memory_info(Memory *mm, mem_size_t *free_list_len, mem_size_t *alloc_list_len);

int  get_memory_id(Memory *mm);

/*
 *  内存池API
 */

MemoryPool *MemoryPool_Init(mem_size_t mempoolsize, int auto_extend);

void *MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize);

int  MemoryPool_Free(MemoryPool *mp, void *p);

MemoryPool *MemoryPool_Clear(MemoryPool *mp);

int  MemoryPool_Destroy(MemoryPool *mp);

/*
 *  获取内存池信息
 */

// 实际分配空间
double get_mempool_usage(MemoryPool *mp);

// 真实使用空间
double get_mempool_prog_usage(MemoryPool *mp);


#endif // !_z_memorypool_h_

