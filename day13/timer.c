#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040
#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2

struct TIMERCTL timerctl;

/* pit: programmable Interval timer, it's used to generate timer interruptions */
void init_pit(void) {
	io_out8(PIT_CTRL, 0x34);
	/* 
		set the period of interruption to be generated
		when period is 11932, the frequency of a  timer interruption is 100Hz
		meaning 100 interruptions will be generated in 1 second, or 1 int every 10ms
		11932=0x2e9c
	*/
	io_out8(PIT_CNT0, 0x9c); /* least significant 8 bits */
	io_out8(PIT_CNT0, 0x2e); /* most significant 8 bits */
	timerctl.count = 0;
	timerctl.next = 0xffffffff;
	timerctl.using = 0;
	int i;
	for(i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0;
	}
	return;
}

struct TIMER *timer_alloc(void) {
	int i;
	for(i = 0; i < MAX_TIMER; i++) {
		if(timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	/* no more available timer */
	return 0;
}

void timer_free(struct TIMER *timer) {
	timer->flags = 0;
	return;
}

/* by interchangably setting data to 0 and 1, and checking data as 0 or 1, 
	we can set an infinite loop that monitors a flashing cursor */
void timer_init(struct TIMER *timer, struct FIFO32 *timerfifo, int data) {
	timer->fifo = timerfifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
	int flags, i, j;
	
	/* need to disable interruption during registration */
	flags = io_load_eflags();

	timer->timeout = timerctl.count + timeout;
	timer->flags = TIMER_FLAGS_USING;

	io_cli();

	/* insert the timer pointer to the correct place */
	for(i = 0; i < timerctl.using; i++) {
		if(timerctl.timers[i]->timeout >= timer->timeout) {
			break;
		}
	}
	
	/* offset a bucket for new timer */
	for(j = timerctl.using; j > i; j--) {
		timerctl.timers[j] = timerctl.timers[j-1];
	}
	timerctl.using++;
	timerctl.timers[j] = timer;

	timerctl.next = timerctl.timers[0]->timeout;
	io_store_eflags(flags);
	return;
}


void inthandler20(int *esp) {
	io_out8(PIC0_OCW2, 0x60); /* notify PIC this signal is consumed */
	timerctl.count++;
	int i, j;
	/* we know the closet timeout is next, if current epoch is smaller than next than we don't need to check */
	if(timerctl.count < timerctl.next) {
		return;
	}

	/* we only search for up to using timers */
	for(i = 0; i < timerctl.using; i++) {
		if(timerctl.timers[i]->timeout > timerctl.count) {
			break;	
		}
		timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
		fifo32_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
	}
	timerctl.using -= i;
	/* remove the first i time out timers, offset the rest to top(if at all) */
	for(j = 0; j < timerctl.using; j++) {
		timerctl.timers[j] = timerctl.timers[j+i];
	}

	/* update next, since timers[] are always sorted, timerctl.next only needs to point to its head */
	if(timerctl.using > 0) {
		timerctl.next = timerctl.timers[0]->timeout;
	} else {
		timerctl.next = 0xffffffff;
	}
	return;
}
