#include <stdio.h>
#include "bootpack.h"

/* keybuf is defined in int.c */
/* extern struct KEYBUF keybuf; */
extern struct FIFO8 keyfifo;

void HariMain(void)
{	
	/*
	char* vram;
	int xsize, ysize;
	
	short *binfo_scrnx, *binfo_scrny;
	int *binfo_vram;
	*/
	init_palette();


	init_gdtidt();
	init_pic();
	/* set interruption flag to one so that it's able to accespt interruptsion, it was cleared during set_pallete() */
	io_sti();
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	
	/* as we will compile hankaku with bootpack, the fonts will be define in hankaku.bin, just need to declar it here to use it */
	/* extern: when used, the variable is supposed to be used somehwere else and the resolving is deferred to the linker */
	/*
	extern char hankaku[4096];
	*/
	/* these address are from asmhead, it returns the width/heigh of current vga mode and the base vga ram address */
	/*
	binfo_scrnx = (short *) 0xff4;
	binfo_scrny = (short *) 0xff6;
	binfo_vram = (int *) 0xff8;
	*/
	
	/*
	xsize = (*binfo).scrnx;
	ysize = (*binfo).scrny;
	*/
	/* we're doing the cast here to tell compiler this is a BYTE pointer */
	/* note vram is a char pointer, or an ADDRESS, all pointers/address are 4 bytes, which is the same size as an int, therefore we use a int* to point to the ADDRESS that stores the address value of vram */
	/*
	vram = (*binfo).vram;
	*/
	/* vram = (char*)0xa0000; */
	/* in 32 bit mode, all pointer is an address with 4 bytes */
	/*
		char: 1 byte
		short: 2 bytes
		int: 4 bytes
		all pointers are an address represented by 4 bytes
	*/
	
	/*
	boxfill8(p, 320, COL8_FF0000, 20, 20, 120, 120);
	boxfill8(p, 320, COL8_00FF00, 70, 50, 170, 150);
	boxfill8(p, 320, COL8_0000FF, 120, 80, 220, 180);
	
	xsize = 320;
	ysize = 200;
	*/
	
	
	/* draw bunch of boxes that looks like an OS UI */
	
	/* Note (*binfo).vram can be shortened as binfo->vram */
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);	
	
	/* write a letter A ! */
	/*
	static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};
	*/
	/* 
		since hankaku is a 256 * 16 byte memory block and each letter (16 byte) is stored as the ascii sequence,
		to get the address of letter A, we just need to offset 0x41 ('A') bytes
	*/

	/*
	putfont8(binfo->vram, binfo->scrnx, 8, 8, COL8_FF0000, hankaku + 'A' * 16);
	putfont8(binfo->vram, binfo->scrnx, 16, 8, COL8_FF0000, hankaku + 'B' * 16);
	putfont8(binfo->vram, binfo->scrnx, 24, 8, COL8_FF0000, hankaku + 'C' * 16);
	putfont8(binfo->vram, binfo->scrnx, 32, 8, COL8_FF0000, hankaku + '1' * 16);
	putfont8(binfo->vram, binfo->scrnx, 40, 8, COL8_FF0000, hankaku + '2' * 16);
	putfont8(binfo->vram, binfo->scrnx, 48, 8, COL8_FF0000, hankaku + '3' * 16);
	*/
	/*
	putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FF0000, "MLGBmlgb");

	putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "MLGB OS.");
	putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "MLGB OS.");
	*/
	/* 
		unlike printf(), which can't be used in MLGB OS, sprintf() is used for writing to a particular address, therefore we can use it to initialize a string, need to use stdio.h
		so we can print a variable's value on screen!
	*/
	/*
	char* s;
	sprintf(s, "scrnx=%d", binfo->scrnx);
	putfonts8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);
	*/

	int mx, my, i;
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 20 - 16) / 2;
	char mouse[256], buffer[40], keybuf[32];
	sprintf(buffer, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, buffer);
	init_mouse_cursor8(mouse, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mouse, 16);

	/* accept interruption from mouse and keyboard */
	io_out8(PIC0_IMR, 0xf9);
	io_out8(PIC1_IMR, 0xef);


	/* initialize unbounded buffer */
	fifo8_init(&keyfifo, 32, keybuf);
	/*
		this loop will keep looking at keybuf, if an interruption happens and keybuf is set then it prints the data
	*/
	for(;;) {
		io_cli();
		/* use unbounded buffer */
		/* check size first */
		if(fifo8_status(&keyfifo) == 0) {
			io_stihlt();
		} else {
			/* i can't be -1 as we already checked size */
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf(buffer, "%02x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
		}
		/* reading one byte from the ring buffer */
		/* need to re-enable interruption io_sti(); */
		/*
		if(keybuf.start < keybuf.end) {
			io_sti();
			struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
			sprintf(buffer, "%02x", keybuf.data[keybuf.start++]);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
		} else if(keybuf.start > keybuf.end) {
			io_sti();
			struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
			sprintf(buffer, "%02x", keybuf.data[keybuf.start]);
			keybuf.start = (keybuf.start + 1) % 32;
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
		} 
		// else keybuf.start == keybuf.end, need to check if it's full
		else if(keybuf.full){
			io_sti();
			struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
			sprintf(buffer, "%02x", keybuf.data[keybuf.start]);
			keybuf.start = (keybuf.start + 1) % 32;
			keybuf.full = 0;
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
		} 
		else {
			io_stihlt();
		}
		/*
		/*
		if(keybuf.flag == 1) {
			keybuf.flag = 0;
			io_sti();
			struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
			sprintf(buffer, "%02x", keybuf.data);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
		} else {
			io_stihlt();
		}
		*/
	}

}
