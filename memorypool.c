#include "memorypool.h"

void get_memory_list_count(MemoryPool *mp, mem_size_t *free_list_len, mem_size_t *alloc_list_len)
{
	mem_size_t free_l = 0, alloc_l = 0;
	Chunk *p = NULL;
	Memory *mm = mp->mlist;
	while (mm)
	{	
		p = mm->free_list;
		while (p)
		{
			free_l++;
			p = p->next;
		}

		p = mm->alloc_list;
		while (p)
		{
			alloc_l++;
			p = p->next;
		}

		mm = mm->next;
	}

	*free_list_len = free_l;
	*alloc_list_len = alloc_l;
}

static Memory *extend_memory_list(MemoryPool *mp)
{
	Memory *mm = (Memory *)malloc(sizeof(Memory));
	if (!mm)
		return NULL;

	mm->start = (char *)malloc(mp->mem_pool_size * sizeof(char));
	if (!mm->start)
		return NULL;

	init_Memory(mm);

	mm->next = mp->mlist;
	mp->mlist = mm;
	return mm;
}

static Memory *find_memory_list(MemoryPool *mp, void *p)
{
	Memory *tmp = mp->mlist;
	while (tmp)
	{
		if (tmp->start < (char *)p && tmp->start + mp->mem_pool_size > (char *)p)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

static bool merge_free_chunk(MemoryPool *mp, Memory *mm, Chunk *c)
{
	if (!mp || !mm || !c)
		return 0;

	Chunk *p0 = *(Chunk **)((char *)c - CHUNKEND), *p1 = c;
	while ((char *)p0 >= mm->start && p0->is_free)
	{
		p1 = p0;
		p0 = *(Chunk **)((char *)p0 - CHUNKEND);
	}

	p0 = (Chunk *)((char *)p1 + p1->alloc_mem);
	while ((char *)p0 < mm->start + mp->mem_pool_size && p0->is_free)
	{
		dlinklist_delete(mm->free_list, p0);

		p1->alloc_mem += p0->alloc_mem;
		p0 = (Chunk *)((char *)p0 + p0->alloc_mem);
	}

	*(Chunk **)((char *)p1 + p1->alloc_mem - CHUNKEND) = p1;

	return 1;
}

MemoryPool *MemoryPool_Init(mem_size_t mempoolsize, bool auto_extend)
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

	mp->total_mem_pool_size = mp->mem_pool_size = mempoolsize;
	mp->auto_extend = auto_extend;

	mp->mlist = (Memory*)malloc(sizeof(Memory));
	if (!mp->mlist)
		return NULL;

	char *s = (char *)malloc(sizeof(char) * mp->mem_pool_size); // memory pool
	if (!s)
	{
		// printf("[MemoryPool_Init] No memory! \n");
		return NULL;
	}
	mp->mlist->start = s;
	init_Memory(mp->mlist);
	mp->mlist->next = NULL;

	return mp;
}

void *MemoryPool_Alloc(MemoryPool *mp, mem_size_t wantsize)
{
	mem_size_t total_size = wantsize + CHUNKHEADER + CHUNKEND;
	Memory *mm = NULL, *mm1 = NULL;
	Chunk *f = NULL, *nf = NULL;

FIND_FREE_CHUNK:
	mm = mp->mlist;
	while (mm)
	{
		if (mp->mem_pool_size - mm->alloc_mem < total_size)
		{
			mm = mm->next;
			continue;
		}

		f = mm->free_list;
		nf = NULL;

		while (f)
		{
			if (f->alloc_mem >= total_size)
			{
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
						mm->free_list = nf;
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
					dlinklist_delete(mm->free_list, f);

					// update alloc chunk
					f->is_free = 0;
				}

				dlinklist_insert_front(mm->alloc_list, f);

				mm->alloc_mem += f->alloc_mem;
				mm->alloc_prog_mem += (f->alloc_mem - CHUNKHEADER - CHUNKEND);
				return (void *)((char *)f + CHUNKHEADER);
			}
			f = f->next;
		}

		mm = mm->next;
	}

	if (mp->auto_extend)
	{
		mm1 = extend_memory_list(mp);
		if (!mm1)
			return NULL;
		mp->total_mem_pool_size += mp->mem_pool_size;
		goto FIND_FREE_CHUNK;
	}

	// printf("[MemoryPool_Alloc] No enough memory! \n");
	return NULL;
}

bool MemoryPool_Free(MemoryPool *mp, void *p)
{
	if (!p || !mp)
		return 0;

	Memory *mm = mp->mlist;
	if (mp->auto_extend)
		mm = find_memory_list(mp, p);

	Chunk *ck = (Chunk *)((char *)p - CHUNKHEADER);
	dlinklist_delete(mm->alloc_list, ck);
	dlinklist_insert_front(mm->free_list, ck);
	ck->is_free = 1;

	mm->alloc_mem -= ck->alloc_mem;
	mm->alloc_prog_mem -= (ck->alloc_mem - CHUNKHEADER - CHUNKEND);

	merge_free_chunk(mp, mm, ck);

	return 1;
}

MemoryPool *MemoryPool_Clear(MemoryPool *mp)
{
	if (!mp)
	{
		// printf("[MemoryPool_Clear] Memory Pool is NULL! \n");
		return 0;
	}

	Memory *mm = mp->mlist;
	while (mm)
	{
		init_Memory(mm);
		mm = mm->next;
	}

	return mp;
}

bool MemoryPool_Destroy(MemoryPool *mp)
{
	if (!mp)
	{
		// printf("[MemoryPool_Destroy] Memory Pool is NULL! \n");
		return 0;
	}

	Memory *mm = mp->mlist, *mm1 = NULL;
	while (mm)
	{
		free(mm->start);
		mm1 = mm;
		mm = mm->next;
		free(mm1);
	}
	free(mp);

	return 1;
}

double get_mempool_usage(MemoryPool *mp)
{
	mem_size_t total_alloc = 0;
	Memory *mm = mp->mlist;
	while (mm)
	{
		total_alloc += mm->alloc_mem;
		mm = mm->next;
	}
	return (double)total_alloc / mp->total_mem_pool_size;
}

double get_mempool_prog_usage(MemoryPool *mp)
{
	mem_size_t total_alloc_prog = 0;
	Memory *mm = mp->mlist;
	while (mm)
	{
		total_alloc_prog += mm->alloc_prog_mem;
		mm = mm->next;
	}
	return (double)total_alloc_prog / mp->total_mem_pool_size;
}

