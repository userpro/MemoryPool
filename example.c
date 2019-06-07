#include <stdio.h>
#include "memorypool.h"

struct TAT {
    int T_T;
};

mem_size_t max_mem = 2 * GB + 1000 * MB + 1000 * KB;
mem_size_t mem_pool_size = 1 * GB + 500 * MB + 500 * KB;

int main() {
    MemoryPool* mp = MemoryPool_Init(max_mem, mem_pool_size, 0);
    struct TAT* tat = (struct TAT*) MemoryPool_Alloc(mp, sizeof(struct TAT));
    tat->T_T = 2333;
    printf("%d\n", tat->T_T);
    int* a = (int*) MemoryPool_Calloc(mp, sizeof(int));
    printf("%d\n", *a);
    MemoryPool_Free(mp, tat);
    MemoryPool_Clear(mp);
    MemoryPool_Destroy(mp);
    return 0;
}