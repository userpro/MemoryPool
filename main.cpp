#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "memorypool.h" // 注释这行比较系统malloc与memory pool的性能
#include <algorithm>

// 开启ENABLE_SHOW输出内部信息 会极大的影响性能
#define ENABLE_SHOW 
// HARD_MODE模式更接近随机分配释放内存的情景, 测试代码Total Usage Size输出失效.
// #define HARD_MODE 

/* -------- 测试数据参数 -------- */
#define MEM_SIZE (0.5*GB) // 内存池管理的每个内存块大小
#define DATA_N   100000   // 数据条数
#define DATA_MAX_SIZE (16*KB) // 每条数据最大尺寸
#define MAX_N    3        // 总测试次数
/* -------- 测试数据参数 -------- */


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
    printf("-> %s\n->> Memory Usage: %.4lf\n->> Memory Usage(prog): %.4lf\n", \
        x, get_mempool_usage(mp), get_mempool_prog_usage(mp)); \
    get_memory_list_count(mp, &mlist_cnt); \
    printf("->> [memorypool_list_count] mlist(%llu)\n", mlist_cnt); \
    Memory *mlist = mp->mlist; \
    while (mlist) \
    { \
        Memory *mm = mlist; \
        get_memory_info(mlist, &free_cnt, &alloc_cnt); \
        printf("->>> id: %d [list_count] free_list(%llu)  alloc_list(%llu)\n", get_memory_id(mlist), free_cnt, alloc_cnt); \
        mlist = mlist->next; \
    } \
    printf("\n"); \
}while (0)

#ifdef _z_memorypool_h_ // 全局变量记录内存池使用信息
    mem_size_t total_size = 0, cur_size = 0;
#else
    unsigned long long total_size = 0, cur_size = 0, cnt = 0;
#endif

struct Node
{
    int size;
    char *data;
    bool operator <(const Node& n) const { return size < n.size; }
} mem[DATA_N];

unsigned int random_uint(unsigned int maxn)
{
    unsigned int ret = abs(rand()) % maxn;
    return ret > 0 ? ret : 32;
}

int main()
{
    srand((unsigned)time(NULL));
    clock_t start, finish;
    double total_time;
    start = clock();

#ifndef _z_memorypool_h_ // 区分系统malloc和内存池实现
    printf("System malloc:\n");
#else
    printf("Memory Pool:\n");
    mem_size_t mlist_cnt = 0, free_cnt = 0, alloc_cnt = 0;
    MemoryPool *mp = MemoryPool_Init(MEM_SIZE, 1);
#endif

    for (int i = 0; i < 3; ++i)
    {
        printf("-------------------------\n");
#if (defined _z_memorypool_h_) && (defined ENABLE_SHOW)
        SHOW("Alloc Before: ");
#endif

        total_size = 0;
        for (int j = 0; j < DATA_N; ++j)
        {
            cur_size = random_uint(DATA_MAX_SIZE);
            total_size += cur_size;
            mem[j].data = (char *)My_Malloc(cur_size);
            mem[j].size = cur_size;
            *(int *)mem[j].data = 123;
        }
        // 排序进一步打乱内存释放顺序 模拟实际中随机释放内存
        std::sort(mem, mem + DATA_N);

#ifdef HARD_MODE
        // 释放前一半管理的内存
        for (int j = 0; j < DATA_N/2; ++j)
            My_Free(mem[j].data);
        // 重新分配前一半的内存
        for (int j = 0; j < DATA_N/2; ++j)
        {
            cur_size = random_uint(DATA_MAX_SIZE);
            total_size += cur_size;
            mem[j].data = (char *)My_Malloc(cur_size);
            mem[j].size = cur_size;
            *(int *)mem[j].data = 456;
        }
        std::sort(mem, mem + DATA_N);
#endif

#if (defined _z_memorypool_h_) && (defined ENABLE_SHOW)
        SHOW("Free Before: ");
#endif

        for (int j = 0; j < DATA_N; ++j) {
            // printf("%d ", *(int *)mem[j].data);
            My_Free(mem[j].data);
        }
        printf("\n");
        // MemoryPool_Clear(mp);

#ifdef _z_memorypool_h_ // 统计信息
        #ifdef ENABLE_SHOW 
            SHOW("Free After: ");
        #endif
        printf("Memory Pool Size: %.4lf MB\n", (double)mp->total_mem_pool_size / 1024 / 1024);
#endif
        printf("Total Usage Size: %.4lf MB\n", (double)total_size/1024/1024);
    }

    finish = clock();
    total_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("\nTotal time: %f seconds.\n", total_time);

    return 0;
}