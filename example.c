#include <stdio.h>
#include "memorypool.h"

struct TAT {
    int T_T;
};

mem_size_t max_mem = 2 * GB + 1000 * MB + 1000 * KB;
mem_size_t mempool_size = 1 * GB + 500 * MB + 500 * KB;

int main() {
    MemoryPool* mp = MemoryPoolInit(max_mem, mempool_size);
    struct TAT* tat = (struct TAT*) MemoryPoolAlloc(mp, sizeof(struct TAT));
    tat->T_T = 2333;
    printf("%d\n", tat->T_T);
    MemoryPoolFree(mp, tat);
    MemoryPoolClear(mp);
    MemoryPoolDestroy(mp);
    return 0;
}