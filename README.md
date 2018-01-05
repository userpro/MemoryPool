# MemoryPool
一个极简内存池实现(基于First Fit算法)

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

- 预定义宏

`KB` `MB` `GB`

- 内存池

`mem_size_t` => `unsigned long long`

~~~c
MemoryPool *MemoryPool_Init(mem_size_t mempoolsize);

void *MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize);

bool MemoryPool_Free(MemoryPool *mp, void *p);

MemoryPool *MemoryPool_Clear(MemoryPool *mp);

bool MemoryPool_Destroy(MemoryPool *mp);
~~~

- 获取内存池信息

`get_mempool_usage` 获取当前内存池已使用内存比例

`get_mempool_prog_usage` 获取内存池中真实分配内存比例(除去了内存池管理结构占用的内存)

~~~c
void get_mempool_list_count(MemoryPool *mp, mem_size_t *free_list_len, mem_size_t *alloc_list_len);

double get_mempool_usage(MemoryPool *mp);

double get_mempool_prog_usage(MemoryPool *mp);
~~~
