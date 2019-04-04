#ifndef _Z_MEMORYPOOL_H_
#define _Z_MEMORYPOOL_H_
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define mem_size_t  unsigned long long
#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

typedef struct _mp_chunk
{
    mem_size_t alloc_mem;
    struct _mp_chunk *prev, *next;
    int is_free;
} _MP_Chunk;

typedef struct _mp_mem_pool_list
{
    char *start;
    unsigned int    id;
    mem_size_t mem_pool_size;
    mem_size_t alloc_mem;
    mem_size_t alloc_prog_mem;
    _MP_Chunk *free_list, *alloc_list;
    struct _mp_mem_pool_list *next;
    // pthread_mutex_t lock;
} _MP_Memory;

typedef struct _mp_mem_pool
{
    unsigned int last_id;
    int          auto_extend;
    int          thread_safe;
    mem_size_t   mem_pool_size, total_mem_pool_size, max_mem_pool_size;
    struct _mp_mem_pool_list *mlist;
    pthread_mutex_t lock;
} MemoryPool;

/*
 *  内部工具函数(调试用)
 */

// 所有Memory的数量
void get_memory_list_count(MemoryPool *mp, mem_size_t *mlist_len);
// 每个Memory的统计信息
void get_memory_info(MemoryPool *mp, _MP_Memory *mm, mem_size_t *free_list_len, mem_size_t *alloc_list_len);
int  get_memory_id(_MP_Memory *mm);

/*
 *  内存池API
 */

MemoryPool* MemoryPool_Init   (mem_size_t maxmempoolsize, mem_size_t mempoolsize, int thread_safe);
void*       MemoryPool_Alloc  (MemoryPool *mp, mem_size_t wantsize);
void*       MemoryPool_Calloc (MemoryPool *mp, mem_size_t wantsize);
int         MemoryPool_Free   (MemoryPool *mp, void *p);
MemoryPool* MemoryPool_Clear  (MemoryPool *mp);
int         MemoryPool_Destroy(MemoryPool *mp);
int         MemoryPool_SetThreadSafe(MemoryPool *mp, int thread_safe);

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

