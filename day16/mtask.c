#include "bootpack.h"
struct TASKCTL *taskctl;
struct TIMER *task_timer;

/* 
	init taskctl and allocate the first task to the thread that calls the init method
	we don't need to assign it a fifo
	because all it doesn't is to switch by calling farjmp()
	and doesn't need to put data to any buffer
*/
struct TASK *task_init(struct MEMMAN *memman) {
	int i;
	struct TASK *task;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
	for(i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].flags = 0;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
	}
	/* after initializaing all tasks in taskctl, 
		we need to alloc the first task and assign it to the thread that calls init */
	task = task_alloc();
	task->flags = 2; /* running */
	taskctl->running = 1; /* number of running tasks is one */
	taskctl->now = 0; /* current running task: 0 */
	taskctl->tasks[0] = task;
	/* set task register to current task */
	load_tr(task->sel);

	task_timer = timer_alloc();
	timer_settime(task_timer, 2);
	return task;
}

struct TASK *task_alloc(void) {
	int i;
	struct TASK *task;
	for(i = 0; i < MAX_TASKS; i++) {
		/* find the first available task */
		if(taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* allocated */

			/* arbitrary numbers */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;

			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			return task;
		}
	}
	return 0;
}

void task_run(struct TASK* task) {
	task->flags = 2;
	taskctl->tasks[taskctl->running] = task;
	taskctl->running++;
	return;
}

/* will be directly called by inthandler20 */
void task_switch(void) {
	timer_settime(task_timer, 2);
	/* only switch if there're more than 2 tasks running */
	if(taskctl->running >= 2) {
		/* looping through all running tasks */
		taskctl->now = (taskctl->now + 1) % (taskctl->running);
		farjmp(0, taskctl->tasks[taskctl->now]->sel);
	}
	return;
}
