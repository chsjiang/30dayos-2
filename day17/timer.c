#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040
#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2
/* define NULL !*/
#define NULL ((void*)0)

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
	timerctl.head = NULL;
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
	timer->next = NULL;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
	int flags;
	
	/* need to disable interruption during registration */
	flags = io_load_eflags();

	timer->timeout = timerctl.count + timeout;
	timer->flags = TIMER_FLAGS_USING;

	io_cli();

	/* insert the timer pointer to the correct place */
	struct TIMER *prob = timerctl.head;
	/* the linked list is empty, add head */
	if(prob == NULL) {
		timerctl.head = timer;
		timer->next = NULL;
	}
	/* not empty, we might add it at head/tail/between */
	else {
		struct TIMER *prev = timer;
		while(prob != NULL && prob->timeout < timer->timeout ) {
			prev = prob;
			prob = prob->next;
		}
		/* prob is at head or somewhere in between */
		if(prob) {
			/* prev has not been updated, prob is pointing to head, we need to add timer to head */
			if(prev == timer) {
				timer->next = prob;
				timerctl.head = timer;
			} 
			/* prev is updated, prob is somewhere in between */
			else {
				prev->next = timer;
				timer->next = prob;
			}
		} 
		/* prob is at tail, add timer to tail */
		else {
			prev->next = timer;
			timer->next = NULL;
		}
	}
	
	timerctl.next = timerctl.head->timeout;
	io_store_eflags(flags);
	return;
}


void inthandler20(int *esp) {
	char ts = 0;	
	io_out8(PIC0_OCW2, 0x60); /* notify PIC this signal is consumed */
	struct TIMER *timer;
	timerctl.count++;
	/* we know the closet timeout is next, if current epoch is smaller than next than we don't need to check */
	if(timerctl.count < timerctl.next) {
		return;
	}

	timer = timerctl.head;
	while(timer) {
		if(timer->timeout > timerctl.count) {
			break;
		}
		/* we set flags to TIMER_FLAGS_ALLOC in case it's used again (like the cursor .5 sec timer) */
		timer->flags = TIMER_FLAGS_ALLOC;
		if(timer != task_timer) {
			fifo32_put(timer->fifo, timer->data);
		} else {
			/* we need to switch task after all interruptions are handled */
			ts = 1;
		}
		timer = timer->next;
	}

	/* if there's still valid timer then we just point head to it */
	if(timer) {
		timerctl.head = timer;
		timerctl.next = timer->timeout;
	} 
	/* otherwise there's no valid timer, we clear the linked list in timerctl */
	else {
		timerctl.head = NULL;	
		timerctl.next = 0xffffffff;
	}

	if(ts!=0) {
		task_switch();
	}

	return;
}
