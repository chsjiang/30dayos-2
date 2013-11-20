#include "bootpack.h"

#define FLAGS_OVERRUN	0x0001;

void fifo32_init(struct FIFO32 *fifo, int size, int* buf, struct TASK *task)
{
	fifo->buf = buf;
	fifo->size = size;
	fifo->free = size;
	fifo->flag = 0;
	fifo->start = 0;
	fifo->end = 0;
	fifo->task = task;
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

	/* 
	   fifo_put() would be the entry point all signals, specifically,
		1) timer interruption
		2) keyboard interruption
		3) mouse interruption
	   multi task is built on top of fifo32, that said, fifo32 would not 
	   be impacted by mtask
	    
	   ALL TASK ARE DRIVEN BY one of the three INTERRUPTIONs!!
	   that said, if there's no interruption, we can make a task sleep.
	   that said, if a task is at sleep, the time we need to wake it up,
	   	is the time when an interruption regarding this task happens.
	   since fifo32_put() is the entry point of a interruption, we can 
	   	add logic here to wake up a task.
	   that said, now each task would require a struct FIFO32

	   i.e:
	   	the task to move cursor/print keyboard input is sleeping,
	   	the time to wake up the task, is when next mouse/keyboard came, or when
	   	 mouse/keyboard inthandler calls fifo32_put() method
	*/
	if(fifo->task != 0) {
		if(fifo->task->flags != 2) {
			task_run(fifo->task);
		}
	}
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

