#include "bootpack.h"
struct TASKCTL *taskctl;
struct TIMER *task_timer;

/* searching for the current running task from current running level */
struct TASK *task_now(void) {
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

/* add a task to a level */
void task_add(struct TASK *task) {
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2;
	return;
}

/* updating the correct running level in taskctl->lv_change*/
void task_switchsub(void) {
	int i;
	/* from highest to lowest, find the first level that has running tasks */
	for(i = 0; i < MAX_TASKLEVELS; i++) {
		if(taskctl->level[i].running > 0) {
			break;
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

/* remove the task from it's level */
void task_remove(struct TASK *task) {
		int i;
		struct TASKLEVEL *tl = &taskctl->level[task->level];

		
		/* looking for the task to sleep */
		for(i = 0; i < tl->running; i++) {
			if(tl->tasks[i] == task) {
				break;
			}
		}
		tl->running--;
		/* we removed a task before now, need to move now one block back to point to the task it was pointing to */
		if(i < tl->now) {
			tl->now--;
		}

		/* if value screwed, set it back to 0 */
		if(tl->now > tl->running) {
			tl->now = 0;
		}
		/* sleeping */
		task->flags = 1;
		/* offset the rest */
		for(; i < tl->running; i++) {
			tl->tasks[i] = tl->tasks[i+1];
		}
	return;
}

/* 
	init taskctl and allocate the first task to the thread that calls the init method
	we don't need to assign it a fifo
	because all it doesn't is to switch by calling farjmp()
	and doesn't need to put data to any buffer
*/
struct TASK *task_init(struct MEMMAN *memman) {
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
	for(i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].flags = 0;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
	}

	/* initialize all levels */
	for(i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	/* after initializaing all tasks in taskctl, 
		we need to alloc the first task and assign it to the thread that calls init */
	task = task_alloc();
	task->flags = 2; /* running */
	task->priority = 2; 
	task->level = 0; /* the first task(for mouse/keyboard/timer interruption) should be in highest level */
	task_add(task);
	task_switchsub(); /* update taskctl->now_lv */
	/* set task register to current task */
	load_tr(task->sel);

	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	idle->tss.eip = (int)&task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	/* 
		the idle task will always be running at lowest level 
		in case there's no when there's no active task to run and OS doesn't know where to jump
	*/
	task_run(idle, MAX_TASKLEVELS - 1, 1);

	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);
	return task;
}

struct TASK *task_alloc(void) {
	int i;
	struct TASK *task;
	for(i = 0; i < MAX_TASKS; i++) {
		/* find the first available task */
		if(taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* allocated, might be sleeping! */
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
			task->tss.ss0 = 0;
			return task;
		}
	}
	return 0;
}

/* add a task to be running at particular level, with priority */
void task_run(struct TASK* task, int level, int priority) {
	/* don't change level */
	if( level < 0 ) {
		level = task->level;
	}

	/* need to change its priority */
	if(priority > 0) {
		task->priority = priority;
	}

	/* need to change the level of a running task */
	if(task->flags == 2 && task->level != level) {
		/* remove task from current level, will change task->flags to 1 */
		task_remove(task);
	}

	/* waking up a task */
	if(task->flags != 2) {
		task->level = level;
		/* when adding will put the task to correct level */
		task_add(task);
	}

	/* mark level change, there might be task in a higher level then taskctl->now_lv, 
		need to look for correct level next time */
	taskctl->lv_change = 1;
	return;
}

/* remove task from its level */
void task_sleep(struct TASK *task) {
	struct TASK *current_task;
	/* if the task is running then we remove it from running buffer and mark it as not running */
	if(task->flags == 2) {
		current_task = task_now();
		/*task->flags would become 1 */
		task_remove(task);

		/* if a task is termination it self, need to explicitly jump to the new running task */
		if(task == current_task) {
			task_switchsub();
			current_task = task_now();
			farjmp(0, current_task->sel);
		}
	}
	return;
}

/* will be directly called by inthandler20 */
void task_switch(void) {
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];

	/* 
		why can't we use this??
		tl->now = (tl->now + 1) % (tl->running);
	*/
		
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}

	/* need to make sure we are runnig the curret task, update correct running level */
	if(taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}

	new_task = tl->tasks[tl->now];
	/* align time slice to new task now */
	timer_settime(task_timer, new_task->priority);

	/* if it's not the same task jump to the new task */
	if(new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}

/* 
	idle task is a task at the lowest level, when there's no other task executing
	the idle task will be executed and do io_hlt()
*/
void task_idle(void) {
	for(;;) {
		io_hlt();
	}
}
