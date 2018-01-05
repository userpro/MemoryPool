#include "memorypool.h"

int main()
{
    MemoryPool *mp = MemoryPool_Init(1*GB + 500*MB + 500*KB);
    char *t = (char *)MemoryPool_Alloc(mp, 20*MB);
    MemoryPool_Free(mp, t);
    MemoryPool_Clear(mp);
    MemoryPool_Destroy(mp);
    return 0;
}