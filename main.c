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

#define SHOW(x) do { \
        printf("->%s->> Memory Usage: %.4lf\n->> Memory Usage(prog): %.4lf\n", \
            x, get_mempool_usage(mp), get_mempool_prog_usage(mp)); \
        printf("->> "); \
        get_mempool_list_count(mp, &free_cnt, &alloc_cnt); \
        printf("[list_count] free_list(%llu)  alloc_list(%llu)\n", free_cnt, alloc_cnt); \
        printf("\n"); \
}while (0)

// ATTENTION !!!!!!
// You can modify this to match your computer.
#define MEM_SIZE (4*GB+500*MB)
#define DATA_N 30000000

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
    // -------- clock start ---------
    clock_t start, finish;
    double total_time;
    start = clock();
    // -------- clock start ----------
#ifdef _z_memorypool_h_
    mem_size_t total_size = 0, cur_size = 0, cnt = 0;
#else
    unsigned long long total_size = 0, cur_size = 0, cnt = 0;
#endif

#ifndef _z_memorypool_h_
    printf("System malloc:\n");
#else
    printf("Memory Pool:\n");
    mem_size_t free_cnt = 0, alloc_cnt = 0;
    MemoryPool *mp = MemoryPool_Init(MEM_SIZE);
    printf("Memory Pool Size: %.4lf MB\n", (double)mp->mem_pool_size / 1024 / 1024);
#endif

    for (int i = 0; i < 3; ++i)
    {
        printf("-------------------------\n");
#ifdef _z_memorypool_h_
        SHOW("Alloc Before: \n");
#endif

        total_size = 0; cnt = 0;
        for (int j = 0; j < DATA_N; ++j)
        {
            cur_size = random_uint(200);
            total_size += cur_size;
            if (total_size >= MEM_SIZE) break;
            mem[j] = My_Malloc(cur_size);
            cnt++;
        }

#ifdef _z_memorypool_h_
        SHOW("Free Before: \n");
#endif

        for (int j = 0; j < cnt; ++j)
            My_Free(mem[j]);

#ifdef _z_memorypool_h_
        SHOW("Free After: \n");
#endif
        printf("total_size: %.4lf MB\n", (double)total_size/1024/1024);
    }


    // -------- clock end ----------
    finish = clock();
    total_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("\nTotal time: %f seconds.\n", total_time);
    // -------- clock end ----------

    return 0;
}