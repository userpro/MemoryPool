# MemoryPool
一个极简内存池实现

## Example

~~~c
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
~~~

## API

```
/*
 *	内存池API
 */

MemoryPool *MemoryPool_Init(mem_size_t mempoolsize);

void *MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize);

bool MemoryPool_Free(MemoryPool *mp, void *p);

MemoryPool *MemoryPool_Clear(MemoryPool *mp);

bool MemoryPool_Destroy(MemoryPool *mp);

/*
 *	获取内存池信息
 */

void list_count(MemoryPool *mp, mem_size_t *fl, mem_size_t *al);

double get_mempool_usage(MemoryPool *mp);

double get_mempool_prog_usage(MemoryPool *mp);

void get_mem_manager_info(MemoryPool *mp);
```
