#include "bootpack.h"

void memman_init(struct MEMMAN *man)
{
	man->frees = 0; 		/*可用信息数目*/
	man->maxfrees = 0; 		/*用于观察可用状况：frees的最大值*/
	man->lostsize = 0; 		/*释放失败的内存的大小总和*/
	man->losts = 0; 		/*释放失败次数*/
	return;
}

unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++)
	{
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++)
	{
		if (man->free[i].size >= size)
		/*找到了足够大的内存*/
		{
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if(man->free[i].size == 0)
			/*如果free[i]变成了0，就减掉一条可用信息*/
			{
				man->frees--;
				for (; i < man->frees; i++)
				{
					/*结构体赋值，代入结构体*/
					man->free[i] = man->free[i + 1];
				}
			}
			return a;
		}
	}
	return 0; 		/*没有可用空间*/
}


int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	/*为便于归纳内存，将free[]按照addr的顺序排列，所以现决定应该放在哪里*/
	for(i = 0; i < man->frees; i++) 
	{
		if(man->free[i].addr > addr)
		{
			break;
		}
	}
	/* free[i-1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存 */
		if (man->free[i-1].addr + man->free[i-1].size == addr) {
			/* 可以与前面的内存归纳到一起 */
			man->free[i-1].size += size;
			if (i < man->frees) {
				/* 后面也有 */
				if(addr + size == man->free[i].addr)
				{
					/*也可以和后面的可用内存归纳到一起*/
					man->free[i-1].size += man->free[i].size;
					man->frees--;
					for(;i < man->frees; i++)
					{
						/*结构体赋值*/
						man->free[i] = man->free[i+1];
					}
				}
			}
			return 0;	/*成功完成*/
		}
	}
	/*不能与前面的可用空间归纳到一起*/
	if(i < man->frees)
	{
		/*后面还有*/
		if(addr + size == man->free[i].addr)
		{
			/*可以与后面的内容归纳到一起*/
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;	/*成功完成*/
		}
	}
	/*既不能与前面归纳到一起，也不能与后面归纳到一起*/
	if(man->frees < MEMMAN_FREES)
	{
		/*free[i]之后的，向后移动，腾出一点可用空间*/
		for(j = man->frees; j >i; j--)
		{
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if(man->maxfrees < man->frees)
		{
			man->maxfrees = man->frees; 		/*更新最大值*/
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; 				/*成功完成*/
	}
	/*不能向后移动*/
	man->losts++;
	man->lostsize += size;
	return -1; 			/*失败*/
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	// 向上取整
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}

