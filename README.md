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

## Tips

- 可通过注释`main.c`里的`#include "memorypool.h"`来切换对比系统`malloc` `free`和内存池

- 暂不支持多线程

- 多食用`MemoryPool_Clear`

- 在**2GB**数据量下比系统`malloc` `free`平均快 **30%-50%** (食用`MemoryPool_Clear`效果更明显)

- `mem_size_t`使用`unsigned long long`以支持4GB以上内存管理(头文件中`MAX_MEM_SIZE`宏定义了最大可管理内存)

- 大量小块内存分配会有 **20%-30%** 内存空间损失(用于存储管理结构体 emmmmm)