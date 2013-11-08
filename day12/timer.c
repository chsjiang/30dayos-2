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
	int i;
	for(i = 0; i < MAX_TIMER; i++) {
		timerctl.timers[i].flags = 0;
	}
	return;
}

struct TIMER *timer_alloc(void) {
	int i;
	for(i = 0; i < MAX_TIMER; i++) {
		if(timerctl.timers[i].flags == 0) {
			timerctl.timers[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers[i];
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
void timer_init(struct TIMER *timer, struct FIFO8 *timerfifo, unsigned char data) {
	timer->fifo = timerfifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
	timer->timeout = timeout;
	timer->flags = TIMER_FLAGS_USING;
	return;
}

void inthandler20(int *esp) {
	io_out8(PIC0_OCW2, 0x60); /* notify PIC this signal is consumed */
	timerctl.count++;
	int i;
	/* each time an interruption arrives, we decrement timeout of each allocated timer */
	for(i = 0; i < MAX_TIMER; i++) {
		if(timerctl.timers[i].flags == TIMER_FLAGS_USING) {
			timerctl.timers[i].timeout--;
			if(timerctl.timers[i].timeout == 0) {
				timerctl.timers[i].flags = TIMER_FLAGS_ALLOC;
				fifo8_put(timerctl.timers[i].fifo, timerctl.timers[i].data);
			}
		}
	}
	return;
}
