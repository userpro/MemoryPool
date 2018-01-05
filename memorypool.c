#include "memorypool.h"

static bool merge_free_chunk(MemoryPool *mp, Chunk *c)
{
	if (!mp || !c)
		return 0;

	Chunk *p0 = *(Chunk **)((char *)c - CHUNKEND), *p1 = c;
	while ((char *)p0 >= mp->start && p0->is_free)
	{
		p1 = p0;
		p0 = *(Chunk **)((char *)p0 - CHUNKEND);
	}

	p0 = (Chunk *)((char *)p1 + p1->alloc_mem);
	while ((char *)p0 < mp->start + mp->mem_pool_size && p0->is_free)
	{
		dlinklist_delete(mp->free_list, p0);

		p1->alloc_mem += p0->alloc_mem;
		p0 = (Chunk *)((char *)p0 + p0->alloc_mem);
	}

	*(Chunk **)((char *)p1 + p1->alloc_mem - CHUNKEND) = p1;

	return 1;
}

MemoryPool *MemoryPool_Init(mem_size_t mempoolsize)
{
	if (mempoolsize > MAX_MEM_SIZE)
	{
		// printf("[MemoryPool_Init] MemPool Init ERROR! Mempoolsize is too big! \n");
		return NULL;
	}

	MemoryPool *mp = (MemoryPool *)malloc(sizeof(MemoryPool));
	if (!mp)
	{
		// printf("[MemoryPool_Init] alloc memorypool struct error!\n");
		return NULL;
	}

	mp->mem_pool_size = mempoolsize;
	mp->alloc_mem = 0;
	mp->alloc_prog_mem = 0;

	char *s = (char *)malloc(sizeof(char) * mp->mem_pool_size); // memory pool
	if (!s)
	{
		// printf("[MemoryPool_Init] No memory! \n");
		return NULL;
	}
	mp->start = s;

	mp->free_list = (Chunk *)mp->start;
	mp->free_list->is_free = 1;
	mp->free_list->alloc_mem = mp->mem_pool_size;
	mp->free_list->prev = NULL;
	mp->free_list->next = NULL;
	mp->alloc_list = NULL;

	return mp;
}

void *MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize)
{
	mem_size_t total_size = wantsize + CHUNKHEADER + CHUNKEND;
	Chunk *f = mp->free_list, *nf = NULL;

	while (f)
	{
		if (f->alloc_mem >= total_size)
		{
			#ifdef MEM_ALLOC_DEBUG
				BEFORE(mp, "MemoryPool_Alloc");
			#endif

			if (f->alloc_mem - total_size > CHUNKHEADER + CHUNKEND)
			{
				// update free CHUNKHEADER
				nf = (Chunk *)((char *)f + total_size);
				*nf = *f;
				nf->alloc_mem -= total_size;	// update alloc_mem
				*(Chunk **)((char *)nf + nf->alloc_mem - CHUNKEND) = nf;
				
				// update free_list
				if (!f->prev)
				{
					mp->free_list = nf;
					if (f->next) f->next->prev = nf;
				}
				else
				{
					f->prev->next = nf;
					if (f->next) f->next->prev = nf;
				}

				// update alloc chunk
				f->is_free = 0;
				f->alloc_mem = total_size;

				*(Chunk **)((char *)f + total_size - CHUNKEND) = f;
			}
			else
			{
				dlinklist_delete(mp->free_list, f);

				// update alloc chunk
				f->is_free = 0;
			}

			dlinklist_insert_front(mp->alloc_list, f);

			#ifdef MEM_ALLOC_DEBUG
				AFTER(mp, "MemoryPool_Alloc");
			#endif

			mp->alloc_mem += f->alloc_mem;
			mp->alloc_prog_mem += (f->alloc_mem - CHUNKHEADER - CHUNKEND);
			return (void *)((char *)f + CHUNKHEADER);
		}
		f = f->next;
	}

	// printf("[MemoryPool_Alloc] No enough memory! \n");
	return NULL;
}

bool MemoryPool_Free(MemoryPool *mp, void *p)
{
	if (!p)
	{
		// printf("[MemoryPool_Free] MemPool Free ERROR! Pointer is NULL! \n");
		return 0;
	}
	if (!mp->alloc_list)
	{
		// printf("[MemoryPool_Free] No memory can free.\n");
		return 0;
	}

	Chunk *ck = (Chunk *)((char *)p - CHUNKHEADER);

	dlinklist_delete(mp->alloc_list, ck);
	dlinklist_insert_front(mp->free_list, ck);
	
	ck->is_free = 1;

	mp->alloc_mem -= ck->alloc_mem;
	mp->alloc_prog_mem -= (ck->alloc_mem - CHUNKHEADER - CHUNKEND);

	// get memory pool manager info
	#ifdef MEM_FREE_DEBUG
		BEFORE(mp, "MemoryPool_Free");
	#endif // MEM_FREE_DEBUG

	merge_free_chunk(mp, ck);

	// get memory pool manager info
	#ifdef MEM_FREE_DEBUG
		AFTER(mp, "MemoryPool_Free"); 
	#endif // MEM_FREE_DEBUG
	return 1;
}

MemoryPool *MemoryPool_Clear(MemoryPool *mp)
{
	if (!mp)
	{
		// printf("[MemoryPool_Clear] Memory Pool is NULL! \n");
		return 0;
	}

	mp->free_list = (Chunk *)mp->start;
	mp->free_list->is_free = 1;
	mp->free_list->alloc_mem = mp->mem_pool_size;
	mp->free_list->prev = NULL;
	mp->free_list->next = NULL;
	mp->alloc_list = NULL;

	mp->alloc_mem = 0;
	mp->alloc_prog_mem = 0;

	return mp;
}

bool MemoryPool_Destroy(MemoryPool *mp)
{
	if (!mp)
	{
		// printf("[MemoryPool_Destroy] Memory Pool is NULL! \n");
		return 0;
	}

	free(mp->start);
	free(mp);

	return 1;
}

void list_count(MemoryPool *mp, mem_size_t *fl, mem_size_t *al)
{
	mem_size_t free_l = 0, alloc_l = 0;
	Chunk *p = mp->free_list;
	while (p)
	{
		free_l++;
		p = p->next;
	}

	p = mp->alloc_list;
	while (p)
	{
		alloc_l++;
		p = p->next;
	}

	*fl = free_l;
	*al = alloc_l;
}

double get_mempool_usage(MemoryPool *mp)
{
	return (double)mp->alloc_mem / mp->mem_pool_size;
}

double get_mempool_prog_usage(MemoryPool *mp)
{
	return (double)mp->alloc_prog_mem / mp->mem_pool_size;
}

