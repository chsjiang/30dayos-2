#include "bootpack.h"
struct FIFO32 *keyfifo;
/*keydata0 is offset, should be 256 here*/
int keydata0;

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

void init_keyboard(struct FIFO32 *argKeyfifo, int data0)
{
	keyfifo = argKeyfifo;
	keydata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

/* IRQ1 : keyboard, INT21 */
/* note we're just buffering the key value here to process faster, there's a loop in bootpack.c to check the buffer and print it */
void inthandler21(int *esp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61);
	data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, data + keydata0);
	return;
}
