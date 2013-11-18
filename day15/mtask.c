#include "bootpack.h"

struct TIMER *mt_timer;
int mt_tr;

/*
 for mt_timer, we don't need to assign it a fifo
 because all it doesn't is to switch by calling farjmp()
 and doesn't need to put data to any buffer
*/
void mt_init(void) {
	mt_timer = timer_alloc();
	timer_settime(mt_timer, 2);
	mt_tr = 3 * 8;
	return;
}

/* taskswitch will be directly called by inthandler20 */
void mt_taskswitch(void) {
	if(mt_tr == 3 * 8) {
		mt_tr = 4 * 8;
	}
	else if(mt_tr == 4 * 8) {
		mt_tr = 3 * 8;
	}
	timer_settime(mt_timer, 2);
	farjmp(0, mt_tr);
	return;
}
