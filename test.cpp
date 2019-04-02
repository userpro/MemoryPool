#include "memorypool.h" // 注释这行比较系统malloc与memory pool的性能
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <pthread.h>

// 开启ENABLE_SHOW输出内部信息 会极大的影响性能
#define ENABLE_SHOW 
// HARD_MODE模式更接近随机分配释放内存的情景, 测试代码Total Usage Size输出失效.
// #define HARD_MODE 

/* -------- 测试数据参数 -------- */
#define MAX_MEM_SIZE  (3*GB)   // 内存池管理的每个内存块大小
#define MEM_SIZE      (0.6*GB) // 内存池管理的每个内存块大小
#define DATA_N        (50000) // 数据条数
#define DATA_MAX_SIZE (16*KB)  // 每条数据最大尺寸
#define MAX_N         (3)      // 总测试次数
/* -------- 测试数据参数 -------- */


#ifdef _Z_MEMORYPOOL_H_
    #define My_Malloc(x) MemoryPool_Alloc(mp, x)
    #define My_Free(x)   MemoryPool_Free(mp, x)
#else
    #define KB (unsigned long long)(1<<10)
    #define MB (unsigned long long)(1<<20)
    #define GB (unsigned long long)(1<<30)
    #define My_Malloc(x) malloc(x)
    #define My_Free(x)   free(x)
#endif

#define SHOW(x, mp) do { \
    printf("============ %lu ============\n", (unsigned long)pthread_self()); \
    mem_size_t mlist_cnt = 0, free_cnt = 0, alloc_cnt = 0; \
    printf("-> %s\n->> Memory Usage: %.4lf\n->> Memory Usage(prog): %.4lf\n", \
        x, get_mempool_usage(mp), get_mempool_prog_usage(mp)); \
    get_memory_list_count(mp, &mlist_cnt); \
    printf("->> [memorypool_list_count] mlist(%llu)\n", mlist_cnt); \
    _MP_Memory *mlist = mp->mlist; \
    while (mlist) { \
        get_memory_info(mp, mlist, &free_cnt, &alloc_cnt); \
        printf("->>> id: %d [list_count] free_list(%llu)  alloc_list(%llu)\n", get_memory_id(mlist), free_cnt, alloc_cnt); \
        mlist = mlist->next; \
    } \
    printf("============ %lu ============\n\n", (unsigned long)pthread_self()); \
}while (0)

#ifdef _Z_MEMORYPOOL_H_ // 全局变量记录内存池使用信息
    mem_size_t total_size = 0, cur_size = 0;
#else
    unsigned long long total_size = 0, cur_size = 0, cnt = 0;
#endif

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct Node
{
    int size;
    char *data;
    bool operator <(const Node& n) const { return size < n.size; }
};

unsigned int 
random_uint(unsigned int maxn)
{
    unsigned int ret = abs(rand()) % maxn;
    return ret > 0 ? ret : 32;
}

void *
test_fn(void *arg) {
    Node mem[DATA_N];
#ifdef _Z_MEMORYPOOL_H_
    MemoryPool *mp = (MemoryPool *)arg;
    mem_size_t cur_total_size = 0;
#else
    unsigned long long cur_total_size = 0;
#endif

#if (defined _Z_MEMORYPOOL_H_) && (defined ENABLE_SHOW)
    pthread_mutex_lock(&mutex);
    SHOW("Alloc Before: ", mp);
    pthread_mutex_unlock(&mutex);
#endif

    for (int i = 0; i < DATA_N; ++i)
    {
        cur_size = random_uint(DATA_MAX_SIZE);
        cur_total_size += cur_size;
        mem[i].data = (char *)My_Malloc(cur_size);
        if (mem[i].data == NULL) {
            printf("Memory overflow!\n");
            exit(0);
        }
        mem[i].size = cur_size;
        *(int *)mem[i].data = 123;
    }
    // 排序进一步打乱内存释放顺序 模拟实际中随机释放内存
    std::sort(mem, mem + DATA_N);

#ifdef HARD_MODE
    // 释放前一半管理的内存
    for (int i = 0; i < DATA_N/2; ++i)
        My_Free(mem[i].data);
    // 重新分配前一半的内存
    for (int i = 0; i < DATA_N/2; ++i)
    {
        cur_size = random_uint(DATA_MAX_SIZE);
        cur_total_size += cur_size;
        mem[i].data = (char *)My_Malloc(cur_size);
        if (mem[i].data == NULL) {
            printf("Memory overflow!\n");
            exit(0);
        }
        mem[i].size = cur_size;
        *(int *)mem[i].data = 456;
    }
    std::sort(mem, mem + DATA_N);
#endif

#if (defined _Z_MEMORYPOOL_H_) && (defined ENABLE_SHOW)
    pthread_mutex_lock(&mutex);
    SHOW("Free Before: ", mp);
    pthread_mutex_unlock(&mutex);
#endif

    for (int i = 0; i < DATA_N; ++i) {
        // printf("%d ", *(int *)mem[i].data);
        My_Free(mem[i].data);
        mem[i].data = NULL;
    }
    printf("\n");

    pthread_mutex_lock(&mutex);
    total_size += cur_total_size;
#ifdef _Z_MEMORYPOOL_H_ // 统计信息
    #ifdef ENABLE_SHOW 
        SHOW("Free After: ", mp);
    #endif
    printf("Memory Pool Size: %.4lf MB\n", (double)mp->total_mem_pool_size/1024/1024);
#endif
    printf("Total Usage Size: %.4lf MB\n", (double)total_size/1024/1024);

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main()
{
    srand((unsigned)time(NULL));
    clock_t start, finish;
    double total_time;
    start = clock();

#ifndef _Z_MEMORYPOOL_H_ // 区分系统malloc和内存池实现
    printf("System malloc:\n");
#else
    printf("Memory Pool:\n");
    MemoryPool *mp = MemoryPool_Init(MAX_MEM_SIZE, MEM_SIZE, 1);
#endif

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32*MB);
    pthread_t pid1, pid2, pid3;

// 第一次执行
#ifdef _Z_MEMORYPOOL_H_
    pthread_create(&pid1, &attr, test_fn, mp);
    pthread_create(&pid2, &attr, test_fn, mp);
    pthread_create(&pid3, &attr, test_fn, mp);
#else
    pthread_create(&pid1, &attr, test_fn, NULL);
    pthread_create(&pid2, &attr, test_fn, NULL);
    pthread_create(&pid3, &attr, test_fn, NULL);
#endif
    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);
    pthread_join(pid3, NULL);

    // 第二次执行
    printf("\n>\n>\n>\n\n");
    total_size = 0;
    MemoryPool_SetThreadSafe(mp, 0);

#ifdef _Z_MEMORYPOOL_H_
    pthread_create(&pid1, &attr, test_fn, mp);
    // pthread_create(&pid2, &attr, test_fn, mp);
    // pthread_create(&pid3, &attr, test_fn, mp);
#else
    pthread_create(&pid1, &attr, test_fn, NULL);
    // pthread_create(&pid2, &attr, test_fn, NULL);
    // pthread_create(&pid3, &attr, test_fn, NULL);
#endif
    pthread_join(pid1, NULL);
    // pthread_join(pid2, NULL);
    // pthread_join(pid3, NULL);


#ifdef _Z_MEMORYPOOL_H_
    MemoryPool_Destroy(mp);
#endif

    finish = clock();
    total_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("\nTotal time: %f seconds.\n", total_time);

    return 0;
}