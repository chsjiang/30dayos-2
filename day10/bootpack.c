#include <stdio.h>
#include "bootpack.h"

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
	/* mouse is generating way more interruption than key, therefore we bump up the buffer to 128 */
	char buffer[40], keybuf[32], mousebuf[128];

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

	/* initialize sheets or layers */
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse;
	unsigned char *buf_back, buf_mouse[256];
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	/* buf_back will cover the entire screen, therefore we need binfo->scrnx * binfo->scrny bytes */
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	/* no invisible color */
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	/* visible color is 99, this layer for mouse is only 16*16 */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	/* we are makeing background color transparent */
	init_mouse_cursor8(buf_mouse, 99);
	/* offset the background layer to 0, 0 */
	/* will redraw the entire screen */
	sheet_slide(shtctl, sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 20 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_back, 0);
	sheet_updown(shtctl, sht_mouse, 1);
	
	/* note: now all color data can be written to buf_back, sheet_refresh() will traslate this to vram */
	/* print cursor coordinates */	
	sprintf(buffer, "(%3d, %3d)", mx, my);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, buffer);

	/* print memory */
	sprintf(buffer, "memory %dMB    free : %dKB", memtotal/(1024 * 1024), memman_total(memman)/1024);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, buffer);

	/* 
		each time we call sheet_refresh, we need to know which sheet we're redrawing and the area to be redrawn 
		this call is only fro redrawing the three lines of text
	*/

	sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, 48);


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
				boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, buffer);
				sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
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
					/* redraw which key is pressed */
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, buffer);
					sheet_refresh(shtctl, sht_back, 32, 16, 32+15*8, 32);

					/* move mouse */
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
					/* remove old coordinates */
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					/* wirte new coordinates */
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, buffer);
					/* 
						offset the mouse layer and repaint, note sheet_slide will only repaint the mouse layer-
						the coordinates won't be repaint! need to call sheet_refresh to repaint those area
					*/
					/* redraw cursor */
					sheet_slide(shtctl, sht_mouse, mx, my);
					/* redraw coordinates */
					sheet_refresh(shtctl, sht_back, 0, 0, 80, 16);
					
				}
			}
		}
	}
}
