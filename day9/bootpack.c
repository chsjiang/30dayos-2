#include <stdio.h>
#include "bootpack.h"

#define MEMMAN_FREES 	4090	/* around 32K */
#define MEMMAN_ADDR		0x003c0000
struct FREEINFO 
{
	unsigned int addr, size;
};

struct MEMMAN
{
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void)
{	
	init_gdtidt();
	init_pic();
	/* set interruption flag to one so that it's able to accespt interruptsion, it was cleared during set_pallete() */
	io_sti();
	init_palette();
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);	

	int mx, my, i;
	unsigned int memtotal;
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 20 - 16) / 2;
	/* mouse is generating way more interruption than key, therefore we bump up the buffer to 128 */
	char mouse[256], buffer[40], keybuf[32], mousebuf[128];
	sprintf(buffer, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, buffer);
	init_mouse_cursor8(mouse, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mouse, 16);

	/* accept interruption from mouse and keyboard */
	io_out8(PIC0_IMR, 0xf9);
	io_out8(PIC1_IMR, 0xef);

	/* initialize unbounded buffer */
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	
	/* initilize keyboard and mouse*/
	init_keyboard();
	struct MOUSE_DEC mdec;
	enable_mouse(&mdec);

	/* initialize memory management */
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	memtotal = memtest(0x00400000, 0xbfffffff); /* byte */
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	sprintf(buffer, "memory %dMB    free : %dKB", memtotal/(1024 * 1024), memman_total(memman)/1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, buffer);

	/*
		this loop will keep looking at keybuf, if an interruption happens and keybuf is set then it prints the data
	*/
	for(;;) {
		io_cli();
		/* use unbounded buffer */
		/* check size first */
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if(fifo8_status(&keyfifo) != 0) {
				/* i can't be -1 as we already checked size */
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(buffer, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
			} 
			/* we handle keyboard int in higher priority */
			else if(fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				/* once we have gathered all three signals, print it on screen */
				if(mouse_decode(&mdec, i) != 0) {
					sprintf(buffer, "[lcr %04d %04d]", mdec.x, mdec.y);
					/* note: unless we realase a button, the mdec.btn mask will always be set and letter will alawys be captial */
					if((mdec.btn & 0x01) != 0) {
						buffer[1] = 'L';
					}
					if((mdec.btn & 0x02) != 0) {
						buffer[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0) {
						buffer[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, buffer);

					/* move mouse */
					/* first clear mouse */
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);

					mx += mdec.x;
					my += mdec.y;

					if(mx < 0) {
						mx = 0;
					}

					if(my < 0) {
						my = 0;
					}

					if(mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}

					if(my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(buffer, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, buffer);
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mouse, 16);
				}
			}
		}
	}
}

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

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
	/* Note: when initialized, frees should be 0, it will be increased by mamman_free */
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
}
