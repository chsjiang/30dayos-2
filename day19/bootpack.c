#include <stdio.h>
#include "bootpack.h"

#define		KEYCMD_LED 		0xed
void debug();

struct SHEET *sht_back;

void HariMain(void)
{	
	/* map 0x54 or 84 key value to a table, note this is aligned to keyboard layout */
	static char keytable[0x80] = {
		0,   '`',   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',  0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', 0,   0,   '\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	/* when shift is pressed, use this table to swtich upper/lower case and puncutaions */
	static char keytable1[0x80] = {
		0,   '~',   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '|',   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', 0,   0,   '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	init_gdtidt();
	init_pic();
	/* set interruption flag to one so that it's able to accespt interruptsion, it was cleared during set_pallete() */
	io_sti();
	init_palette();
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);	

	int mx, my, i = 0;
	/* keycmd is a small buffer to switch status of Capslock, Numlock and ScrLock */
	struct FIFO32 fifo, keycmd;
	/* mouse is generating way more interruption than key, therefore we bump up the s to 128 */
	char s[40];
	int fifobuf[128], keycmd_buf[32];
	struct TIMER *timer;

	/* initiliza pit(programmable interval timer) */
	init_pit();

	/* accept interruption from mouse and keyboard and pit */
	io_out8(PIC0_IMR, 0xf8); /*PIT, PIC1 and keyboard: 11111000 */
	io_out8(PIC1_IMR, 0xef); /*mouse:11101111*/

	/* now we only have one fifo for handling all signals */
	fifo32_init(&fifo, 128, fifobuf, 0);

	/**** initialize memory management ****/
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff); /* byte */
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	/************
		task swtiching 
		task_init() will initialize task controller and register the first task to the current running thread
		then we create another task that run console_task method, then register it into task controler

		console_task will start another task that display a console and flashing cursor
	************/
	struct TASK *task_a, *task_cons;
	/* tasks_init will create first task and load task register the value of the first selector */
	task_a = task_init(memman);
	fifo.task = task_a;
	/* change task_a's priority */
	task_run(task_a, 1, 0);

	
	/* initilize keyboard and mouse*/
	/*
	 offset keyboard signal 256
	 offset mouse signal 512
	*/
	init_keyboard(&fifo, 256);
	struct MOUSE_DEC mdec;
	enable_mouse(&fifo, 512, &mdec);

	/* initialize sheets or layers */
	struct SHTCTL *shtctl;
	/* create three windows to add counter */
	struct SHEET *sht_mouse, *sht_win, *sht_cons;
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	/* 
		sht_back 
	*/
	sht_back = sheet_alloc(shtctl);
	/* buf_back will cover the entire screen, therefore we need binfo->scrnx * binfo->scrny bytes */
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	/* no invisible color */
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);

	/* 
		sht_win
	*/
	sht_win = sheet_alloc(shtctl);
	buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1);
	make_window8(buf_win, 144, 52, "task_a", 1);
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
	/* initliaze timers */
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	/* draws a flashing cursor */
	timer_settime(timer, 50);
	/* x points to the place where the last letter is, cursor_c is the cursor color */
	int cursor_x, cursor_c;
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;

	/*
		sht_cons
	*/
	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *)memman_alloc_4k(memman, 144 * 52);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
	sprintf(s, "console");
	make_window8(buf_cons, 256, 165, s, 0);

	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
	task_cons = task_alloc();
	/* give task_cons 64k stack size */
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	task_cons->tss.eip = (int)&console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	/* note since task_cons takes a parameter, we need to explicitly set esp+4 to point to the param */
	*((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;
	*((int *)(task_cons->tss.esp + 8)) = (int)memtotal;
	task_run(task_cons, 2, 2);

	/*
		sht_mouse
	*/
	sht_mouse = sheet_alloc(shtctl);
	/* visible color is 99, this layer for mouse is only 16*16 */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	/* we are makeing background color transparent */
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 20 - 16) / 2;


	/* offset the background layer to 0, 0 */
	/* will redraw the entire screen */
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_cons, 32, 4);
	sheet_slide(sht_win, 64, 56);
	sheet_slide(sht_mouse, mx, my);

	sheet_updown(sht_back, 0);
	sheet_updown(sht_cons, 1);
	sheet_updown(sht_win, 2);
	sheet_updown(sht_mouse, 3);
	
	/* note: now all color data can be written to buf_back, sheet_refresh() will traslate this to vram */
	/* print cursor coordinates */	
	// sprintf(s, "(%3d, %3d)", mx, my);
	// putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	/* print memory */
	// sprintf(s, "memory %dMB    free : %dKB", memtotal/(1024 * 1024), memman_total(memman)/1024);
	// putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	/* 
		each time we call sheet_refresh, we need to know which sheet we're redrawing and the area to be redrawn 
		this call is only fro redrawing the three lines of text
	*/
	sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

	/* 
		key_to is a switch, when 'Tab' is pressed, key_to will point to the next to task to switch to 
		note: key_to only points to which task is at front, it doesn't mean the task at back is sleep
			the task at front will be getting keyboard interruption data
	*/

	/*
		binfo->leds: it's value is captured in asmhead.nas
			4th bit: ScrollLock status
			5th bit: NumLock status
			6th bit: CapsLock status
	*/
	int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	/* buffer initial values */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	/*
		this loop will keep looking at keybuf 
		if an interruption happens and keybuf is set then it prints the data 
			or send the data to another task's fifo
		note: keyboard int will always handled by task_a, but the data is transferred
			to the fifo of another task

	*/
	for(;;) {
		/* 
			check if we need to alter keyboard LED status
			need to use io to send signal to keyboard port and wait for success code from keyboard
			keycmd_wait is the led status code
		 */
		if(fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		/* use unbounded s shared by timer/keyboard/mouse */
		if(fifo32_status(&fifo) == 0) {
			task_sleep(task_a);
			/* 
				when task_a is waked up, it will start from here
				first thing to to upon waking up is to enable interruption
			*/
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			/* keyboard signal */
			if( i >= 256 && i < 512) {
				// sprintf(s, "%02X", i - 256);
				// putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
				/* if this is a valid char, we transfer it according shift_key status */
				if( i < 0x80 + 256 ) {
					if( key_shift == 0 ) {
						s[0] = keytable[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}

				if('A' <= s[0] && s[0] <= 'Z') {
					/* CapsLock and SHIFT are both on or both off */
					if(((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)) {
						/* transfer uppercase to lower case */
						s[0] += 0x20;
					}
				}

				/* s[0] has already got the correct char */
				if( s[0] != 0 ) {
					/* if now task_a is at front, print it on screen using task_a's buf */
					if(key_to == 0) {
						if(cursor_x < 128) {
							/* need to create a c stile string, use 0 to terminate the string */
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					}
					/* otherwise task_cons is at front, send to task_cons's fifo and let task_cons to handle the info */
					else {
						fifo32_put(&task_cons->fifo, s[0] + 256);
					}
				}
				/* if backspace is pressed and we have more than 1 letter typed, then we need to remove one letter */
				if( i == 256+0x0e ) {
					if(key_to == 0) {
						if(cursor_x > 8) {
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;	
						}
					} else  {
						/* specific backspace(8+256) for task_cons */
						fifo32_put(&task_cons->fifo, 8+256);
					}
					
				}
				/* 
					when tab key is pressed, switch task
					need to repaint title bar and enable/disable cursor
				 */
				if( i == 256 + 0x0f ) {
					if(key_to == 0) {
						key_to = 1;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1;
						/* erase cursor on sht_win */
						boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						/* notify task_cons to start flashing its cursor */
						fifo32_put(&task_cons->fifo, 2);
					} else {
						key_to = 0;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						/* notify task_cons to stop flashing its cursor */
						fifo32_put(&task_cons->fifo, 3);
						cursor_c = COL8_000000;
					}
					sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				/* left shift on */
				if( i == 256 + 0x2a ) {
					key_shift |= 1;
				}
				/* right shift on */
				if( i == 256 + 0x36 ) {
					key_shift |= 2;
				}
				/* left shift off */
				if( i == 256 + 0xaa ) {
					key_shift &= ~1;
				}
				/* right shift off */
				if( i == 256 + 0xb6 ) {
					key_shift &= ~2;
				}
				/* CapsLock */
				if( i == 256 + 0x3a ) { 
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				/* NumLock */
				if( i == 256 + 0x45 ) { 
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				/* ScrollLock */
				if( i == 256 + 0x46 ) { 
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				/* 
				 keyboard has received correct value - will return signal 0xfa
					meaning we have succesfully changed the LED status 
					reset keycmd_wait status
				*/
				if( i == 256 + 0xfa ) {
					keycmd_wait = -1;
				}
				/* 
				 keyboard has not received correct value - will return signal 0xfe
					meaning we haven't changed LED status
					need to redo io to keyboard port
				 */
				if( i == 256 + 0xfe ) {
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				/* Enter key is pressed, send signal to task_cons to break a line */
				if( i == 256 + 0x1c ) {
					if( key_to != 0 ) {
						fifo32_put(&task_cons->fifo, 10 + 256);
					}
				}
				/* draw cursor, only draw it when this task is active */
				if(cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
			/* mouse signal */
			else if ( i >= 512 && i <= 767) {

				if(mouse_decode(&mdec, i - 512) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					/* note: unless we realase a button, the mdec.btn mask will always be set and letter will alawys be captial */
					// if((mdec.btn & 0x01) != 0) {
					// 	s[1] = 'L';
					// }
					// if((mdec.btn & 0x02) != 0) {
					// 	s[3] = 'R';
					// }
					// if((mdec.btn & 0x04) != 0) {
					// 	s[2] = 'C';
					// }
					/* redraw which key is pressed */
					// putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

					/* move mouse */
					mx += mdec.x;
					my += mdec.y;

					if(mx < 0) {
						mx = 0;
					}

					if(my < 0) {
						my = 0;
					}

					if(mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}

					if(my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					// sprintf(s, "(%3d, %3d)", mx, my);
					// putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					/* redraw cursor */
					/* 
						offset the mouse layer and repaint, note sheet_slide will only repaint the mouse layer-
						the coordinates won't be repaint! need to call sheet_refresh to repaint those area
					*/
					sheet_slide(sht_mouse, mx, my);
					/* if left button is pressed, then we need to move window*/
					if((mdec.btn & 0x01) != 0) {
						/* always move cursor to the central upper part of window */
						sheet_slide(sht_win, mx-80, my - 8);
					}
				}
			}
			/* 
				note: even if task_a is not highlightened, it's still running
				and the timer signals are still coming, we just need to ignore the timers for cursor flashing
			*/
			else if(i <= 1) {
				if (i != 0) {
						timer_init(timer, &fifo, 0);
						if( cursor_c >= 0 ) {
							cursor_c = COL8_000000;
						}
				} else {
						timer_init(timer, &fifo, 1);
						if( cursor_c >= 0 ) {
							cursor_c = COL8_FFFFFF;
						}
				}
				timer_settime(timer, 50);
				if( cursor_c >= 0 ) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}

void debug() {
	putfonts8_asc_sht(sht_back, 0, 155, COL8_FFFFFF, COL8_008484, "mlgb", 4);
}
