#include "bootpack.h"

struct FIFO32 *mousefifo;
/* mouse data0 is offset, should be 512 here*/
int mousedata0;

void enable_mouse(struct FIFO32 *mousefifoArg, int data0, struct MOUSE_DEC *mdec)
{
	mousefifo = mousefifoArg;
	mousedata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0; /* waiting for the first 0xFA */
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, int dat)
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
	return -1;
}

/* IRQ12 ： mouse, INT12 */
void inthandler2c(int *esp)
{
	int data;
	/* need to do OUT io to inform this interruption is already consumed, otherwise PIC will not continue listen to further interruption */
	/* for mouse interruption, we need to inform both slave PIC and master PIC */
	io_out8(PIC1_OCW2, 0x64);
	io_out8(PIC0_OCW2, 0x62);
	/* then get key data from keyboard port， keyboard port is sharing data from mouse port */
	data = io_in8(PORT_KEYDAT);

	fifo32_put(mousefifo, data + mousedata0);
	return;
}
