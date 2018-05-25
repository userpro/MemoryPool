#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "memorypool.h"

#ifdef _z_memorypool_h_
    #define My_Malloc(x) MemoryPool_Alloc(mp, x)
    #define My_Free(x)   MemoryPool_Free(mp, x)
#else
    #define KB (unsigned long long)(1<<10)
    #define MB (unsigned long long)(1<<20)
    #define GB (unsigned long long)(1<<30)
    #define My_Malloc(x) malloc(x)
    #define My_Free(x)   free(x)
#endif

#ifdef _z_memorypool_h_
    mem_size_t total_size = 0, cur_size = 0;
#else
    unsigned long long total_size = 0, cur_size = 0, cnt = 0;
#endif

#define SHOW(x) do { \
    printf("-> %s\n->> Memory Usage: %.4lf\n->> Memory Usage(prog): %.4lf\n", \
        x, get_mempool_usage(mp), get_mempool_prog_usage(mp)); \
    get_memory_list_count(mp, &mlist_cnt); \
    printf("->> [memorypool_list_count] mlist(%llu)\n", mlist_cnt); \
    Memory *mlist = mp->mlist; \
    while (mlist) \
    { \
        Memory *mm = mlist; \
        get_every_memory_info(mlist, &free_cnt, &alloc_cnt); \
        printf("->>> id: %d [list_count] free_list(%llu)  alloc_list(%llu)\n", get_memory_id(mlist), free_cnt, alloc_cnt); \
        mlist = mlist->next; \
    } \
    printf("\n"); \
}while (0)

#define MEM_SIZE (50*MB)
#define DATA_N 10
#define DATA_MAX_SIZE (30*MB)
#define uint unsigned int

char *mem[DATA_N] = {0};

void _init()
{
    srand((unsigned)time(NULL));
}

uint random_uint(uint maxn)
{
    uint ret = abs(rand()) % maxn;
    return ret > 0 ? ret : 10;
}

int main()
{
    _init();
    // -------- clock start ---------
    clock_t start, finish;
    double total_time;
    start = clock();
    // -------- clock start ----------

    // -------- pre --------
#ifndef _z_memorypool_h_
    printf("System malloc:\n");
#else
    printf("Memory Pool:\n");
    mem_size_t mlist_cnt = 0, free_cnt = 0, alloc_cnt = 0;
    MemoryPool *mp = MemoryPool_Init(MEM_SIZE, 1);
#endif
    // -------- pre --------

    // -------- main --------
    for (int i = 0; i < 3; ++i)
    {
        printf("-------------------------\n");
#ifdef _z_memorypool_h_
        SHOW("Alloc Before: ");
#endif

        total_size = 0;
        for (int j = 0; j < DATA_N; ++j)
        {
            cur_size = random_uint(DATA_MAX_SIZE);
            total_size += cur_size;
            mem[j] = My_Malloc(cur_size);
        }

#ifdef _z_memorypool_h_
        SHOW("Free Before: ");
#endif

        for (int j = 0; j < DATA_N; ++j)
            My_Free(mem[j]);
        // MemoryPool_Clear(mp);

#ifdef _z_memorypool_h_
        SHOW("Free After: ");
        printf("Memory Pool Size: %.4lf MB\n", (double)mp->total_mem_pool_size / 1024 / 1024);
#endif
        printf("total_size: %.4lf MB\n", (double)total_size/1024/1024);
    }
    // -------- main --------

    // -------- clock end ----------
    finish = clock();
    total_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("\nTotal time: %f seconds.\n", total_time);
    // -------- clock end ----------

    return 0;
}