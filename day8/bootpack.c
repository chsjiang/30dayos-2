#include <stdio.h>
#include "bootpack.h"

/* keybuf is defined in int.c */
/* extern struct KEYBUF keybuf; */
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

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

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

/* once this returns we can send mode setting instruction data to keyboard */
void wait_KBC_sendready(void)
{
	while(1) {
		/* if succeed, the last but two bit should be 0 */
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(void)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0; /* waiting for the first 0xFA */
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if( mdec->phase == 0 ) {
		if(dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	} else if( mdec->phase == 1 ) {
		/* 
			if mouse is broken, the first byte might be missing, and the mouse buffer will screw up
			therefore we need to start phase change only after getting the correct first byte
		*/
		if((dat & 0xc8) == 0x08) {
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	} else if( mdec->phase == 2 ) {
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	} else if( mdec->phase == 3 ) {
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07; /* last 3 bits is storing button state */
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];

		/* mask for direction - also stored in the first byte, the corordinates values are stored in buf[1] and buf[2] */
		if((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}

		if((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;	
		}

		mdec->y = -mdec->y; /* reverse to screen coordinates */
		return 1;
	} 
}
