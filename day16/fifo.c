#include "bootpack.h"

#define FLAGS_OVERRUN	0x0001;

void fifo32_init(struct FIFO32 *fifo, int size, int* buf)
{
	fifo->buf = buf;
	fifo->size = size;
	fifo->free = size;
	fifo->flag = 0;
	fifo->start = 0;
	fifo->end = 0;
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
{
	if(fifo->free == 0) {
		fifo->flag |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->end] = data;
	fifo->end = (fifo->end + 1) % fifo->size;
	fifo->free--;
	return 0;
}

int fifo32_get(struct FIFO32 *fifo)
{	
	int data;
	/* fifo is empty, nothing to return */
	if(fifo->free == fifo->size) {
		return -1;
	}
	data = fifo->buf[fifo->start];
	fifo->start = (fifo->start + 1) % fifo->size;
	fifo->free++;
	return data;
}

/* check number of valid entry of this fifo, if it's >0 then return the size */ 
int fifo32_status(struct FIFO32 *fifo)
{
	return fifo->size - fifo->free;
}

