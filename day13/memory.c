#include "bootpack.h"

/*
	memtest is used to calculate available memory, basicly it does the following:
	we first need to check if a memory unit (a byte) is valid. It's valid if 
		1) we read the value from the memroy unit
		2) we change the value to value' and write back to memroy unit
		3) we read the value from memory again, if it's value', then the memory is valid 
	Now we move to the next memory unit to check if it's valid, we keep increasing the pointer until the check fails
	then the pointer will point to the LAST availe memory, which is the size of memory available

*/
unsigned int memtest(unsigned int start, unsigned int end)
{	
	char flg486 = 0;
	unsigned int eflg, cr0, i;
	/* 
		to do the write read check, we need to disable cache, 
		because with cache in place writing to a memory might be cached 
			and reading from the same address unit might return differernt value
		Note only 486+ CPU has cache, therefore we need first check if it's 486 or lower
		if it's lower then we don't need to disable cache
	*/
	eflg = io_load_eflags();
	/* only 486+ has AC bit, that said in 386 and lower, even if we set AC to 1, it will be cleared */
	eflg |= EFLAGS_AC_BIT; 
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if((eflg & EFLAGS_AC_BIT) != 0) {
		/* if AC is NOT cleared, then it's 486 and we need to disable cache */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflg);

	/* disable cache by setting a bit in CR0 register */
	if(flg486) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	/* re-enable cache */
	if(flg486) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	return i;
}

/* 
	we read a memory unit and write pat0 to it
	then we change its value and write to memory (reversing all bits)
	then we read from memory, if the value is actually reversed, then we know this memory is valid
		otherwise we break, the current point will be pointing to the correct max memory, will return that
	if memroy is valid then we advance pointer to probe the next memory unit, each time we advance 4kb

	0xffc is 1111 1111 1100, we are only checking that last 4 bytes in each loop
	each loop we advance the pointer for 4kb
*/

/*
	Note the compiler will check the condition and optimize code before it's actually compiled,
	Compiler isn't aware of cache or memory check
	since here *p is bound to qual pat1/pat0 after flipping, compiler will ignore the if condition,
	then we left old = *p; *p = old; which doesn't do anything, this will also be ignored.
	then all unused vars are ignored, left this function only a empty for loop to be compiled
	therefore this method won't return the actualy memory but always return 3072MB as the for loop never breaks
	can see the assembly code in bootpack.nas make -r bootpack.nas
	Therefore in order to get the correct memory value, we need to implement memtest_sub in assembly in naskfun.nas
*/		
/*
unsigned int memtest_sub(unsigned int start, unsigned int end)
{
	unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
	for(i = start; i <= end; i += 0x1000) {
		
		p = (unsigned int *)(i + 0xffc); 
		old = *p;
		*p = pat0;
		*p ^= 0xffffffff;
		if(*p != pat1) {
			*p = old;
			break;
		}
		*p ^= 0xffffffff;
		if(*p != pat0) {
			*p = old;
			break;
		}
		*p = old;
	}
	return i;
}
*/

void memman_init(struct MEMMAN *man)
{
	/* Note: when initialized, frees should be 0, it will be increased by memman_free */
	man->frees = 0;
	/* maxfrees is used for statistics */
	man->maxfrees = 0;
	man->losts = 0;
	man->lostsize = 0;
	return;
}

/* calculate availe free memory */
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int size = 0, i;
	for(i = 0; i < man->frees; i++) {
		size += man->free[i].size;
	}
	return size;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a = 0;
	for(i = 0; i < man->frees; i++) {
		if(man->free[i].size >= size)
		{
			a = man->free[i].addr; 
			man->free[i].addr += size;
			man->free[i].size -= size;
		}
		/* if this block is drained, remote it from man->free[] */
		if(man->free[i].size == 0) {
			man->frees--;
			for(; i < man->frees; i++)
			{
				man->free[i] = man->free[i+1];
			}
		}
		return a;
	}
	/* failed to find an available free block */
	return 0;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	/* we need to manage memory sequencially, find the correct place to insert first */
	for(i = 0; i < man->frees; i++) {
		if(man->free[i].addr > addr) {
			break;
		}
	}

	/* there's free block before */
	if( i > 0 )
	{
		/* if the previous block is consequtive, then we need to merge this with previous */
		if(man->free[i-1].addr + man->free[i-1].size == addr)
		{
			man->free[i-1].size += size;
			/* need to check if we can merge with next block */
			if( i < man->frees )
			{
				if(addr + size == man->free[i].addr)
				{
					man->free[i-1].size += man->free[i].size;
					man->frees--;
					for(; i < man->frees; i++)
					{
						man->free[i] = man->free[i+1];
					}
				}
			}
			return 0;
		}
	}

	/* there's no free block before and there's free block after */
	if( i < man->frees )
	{
		if(addr + size == man->free[i].addr)
		{
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}

	/* there's no free block before and there's no free block after */
	/* do this check to avoid we overrun MEMMAN_FREES */
	if( i < MEMMAN_FREES )
	{
		/* note for here j is overflowed by one intentionally */
		for(j = man->frees; j > i; j--)
		{
			man->free[j] = man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees < man->frees)
		{
			man->maxfrees = man->frees;
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}

	/* 
		if we reach here then it means we need to create a new block 
		but there's already MEMMAN_FREES blocks allocated
	*/
		man->losts++;
		man->lostsize += size;
		return -1;
}

/* 
	to avoid too small memory pieces, we need to make sure each time memory allocated is the mutiplier of a proper 'chunck' size
	so for here we're having chunck size 0x1000, each time we need to roundup size to the smallest muplitler of 0x1000 that's bigger than size
*/
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	size = (size + 0xfff) & 0xfffff000;
	return memman_alloc(man, size);
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned size)
{
	size = (size + 0xfff) & 0xfffff000;
	return memman_free(man, addr, size);
}
