#ifndef _Z_MEMORYPOOL_H_
#define _Z_MEMORYPOOL_H_

#ifdef _Z_MEMORYPOOL_THREAD_
#include <pthread.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define mem_size_t unsigned long long
#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

typedef struct _mp_chunk {
    mem_size_t alloc_mem;
    struct _mp_chunk *prev, *next;
    int is_free;
} _MP_Chunk;

typedef struct _mp_mempool_list {
    char* start;
    unsigned int id;
    mem_size_t mempool_size;       // 固定值 每个内存池最大内存
    mem_size_t alloc_mem;          // 统计值 当前池内已分配的内存总大小
    mem_size_t alloc_prog_mem;     // 统计值 当前池内实际分配给应用程序的内存总大小(减去内存管理元信息)
    _MP_Chunk *free_list, *alloc_list;
    struct _mp_mempool_list* next;
} _MP_Memory;

typedef struct _mp_mempool {
    unsigned int last_id;
    int auto_extend;
    mem_size_t mempool_size;       // 固定值 每个内存池最大内存
    mem_size_t max_mempool_size;   // 固定值 所有内存池加和总上限
    mem_size_t alloc_mempool_size; // 统计值 当前已分配的内存池总大小
    struct _mp_mempool_list* mlist;
#ifdef _Z_MEMORYPOOL_THREAD_
    pthread_mutex_t lock;
#endif
} MemoryPool;

/*
 *  内部工具函数(调试用)
 */

// 所有Memory的数量
void get_memory_list_count(MemoryPool* mp, mem_size_t* mlist_len);
// 每个Memory的统计信息
void get_memory_info(MemoryPool* mp,
                     _MP_Memory* mm,
                     mem_size_t* free_list_len,
                     mem_size_t* alloc_list_len);
int get_memory_id(_MP_Memory* mm);

/*
 *  内存池API
 */

MemoryPool* MemoryPoolInit(mem_size_t maxmempoolsize, mem_size_t mempoolsize);
void* MemoryPoolAlloc(MemoryPool* mp, mem_size_t wantsize);
int MemoryPoolFree(MemoryPool* mp, void* p);
MemoryPool* MemoryPoolClear(MemoryPool* mp);
int MemoryPoolDestroy(MemoryPool* mp);
int MemoryPoolSetThreadSafe(MemoryPool* mp, int thread_safe);

/*
 *  内存池信息API
 */

// 总空间
mem_size_t GetTotalMemory(MemoryPool* mp);
// 实际分配空间
mem_size_t GetUsedMemory(MemoryPool* mp);
float MemoryPoolGetUsage(MemoryPool* mp);
// 数据占用空间
mem_size_t GetProgMemory(MemoryPool* mp);
float MemoryPoolGetProgUsage(MemoryPool* mp);

#endif  // !_Z_MEMORYPOOL_H_
