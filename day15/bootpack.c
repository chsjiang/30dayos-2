#include <stdio.h>
#include "bootpack.h"


void make_window8(unsigned char* buf, int xsize, int ysize, char *title);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char*s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void debug();
void task_b_main(void);

/* 
	TSS is task status segment, used for representing a status of a task
	a taks segment will be registered in GDT.
	When doing task switching, CPU will jump to a task segment
		by using `JMP CS:0` (far jump) where CS is code segment, jumping to i'th 
		segment will be `JMP i*8:0`
*/
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	/* the folloing two lines are register and flag values that would be buffered for this task */
	/* 
		EIP extended instruction pointer, points to which address CPU is current executing 
		JMP 0x1234 is actually changing the value of EIP
	*/

	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;

	int ldtr, iomap;
};

struct SHEET *sht_back;

void HariMain(void)
{	
	/* map 0x54 or 84 key value to a table, note this is aligned to keyboard layout */
	static char keytable[0x54] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
	};
	init_gdtidt();
	init_pic();
	/* set interruption flag to one so that it's able to accespt interruptsion, it was cleared during set_pallete() */
	io_sti();
	init_palette();
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);	

	int mx, my, i;
	unsigned int memtotal;
	struct FIFO32 fifo;
	/* mouse is generating way more interruption than key, therefore we bump up the buffer to 128 */
	char buffer[40];
	int fifobuf[128];
	struct TIMER *timer, *timer2, *timer3;

	/* initiliza pit(programmable interval timer) */
	init_pit();

	/* accept interruption from mouse and keyboard and pit */
	io_out8(PIC0_IMR, 0xf8); /*PIT, PIC1 and keyboard: 11111000 */
	io_out8(PIC1_IMR, 0xef); /*mouse:11101111*/

	/* now we only have one fifo for handling all signals */
	fifo32_init(&fifo, 128, fifobuf);

	/* initliaze timers */
	timer = timer_alloc();
	timer2 = timer_alloc();
	timer3 = timer_alloc();
	/* timers share a buffer */
	timer_init(timer, &fifo, 10);
	timer_init(timer2, &fifo, 3);
	timer_init(timer3, &fifo, 1);
	timer_settime(timer, 1000);
	timer_settime(timer2, 300);
	/* this one is for drawing a flashing cursor */
	timer_settime(timer3, 50);

	/* initilize keyboard and mouse*/
	init_keyboard(&fifo, 256);
	struct MOUSE_DEC mdec;
	enable_mouse(&fifo, 512, &mdec);

	/* initialize memory management */
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	memtotal = memtest(0x00400000, 0xbfffffff); /* byte */
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	/* initialize sheets or layers */
	struct SHTCTL *shtctl;
	struct SHEET *sht_mouse, *sht_win;
	unsigned char *buf_back, buf_mouse[256], *buf_win;
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	/* buf_back will cover the entire screen, therefore we need binfo->scrnx * binfo->scrny bytes */
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
	/* no invisible color */
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);
	make_window8(buf_win, 160, 52, "window");
	make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);

	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	/* visible color is 99, this layer for mouse is only 16*16 */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	/* we are makeing background color transparent */
	init_mouse_cursor8(buf_mouse, 99);
	/* offset the background layer to 0, 0 */
	/* will redraw the entire screen */
	sheet_slide(sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 20 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);
	
	/* note: now all color data can be written to buf_back, sheet_refresh() will traslate this to vram */
	/* print cursor coordinates */	
	sprintf(buffer, "(%3d, %3d)", mx, my);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, buffer);

	/* print memory */
	sprintf(buffer, "memory %dMB    free : %dKB", memtotal/(1024 * 1024), memman_total(memman)/1024);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, buffer);

	/* 
		each time we call sheet_refresh, we need to know which sheet we're redrawing and the area to be redrawn 
		this call is only fro redrawing the three lines of text
	*/

	sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

	/* x points to the place where the last letter is, cursor_c points to cursor color */
	int cursor_x, cursor_c;
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;

	/************
		task swtiching 
		first register 3rd and 4th segment in GDT, then load_tr to tell CPU we're in task 3 now
		after calling mt_init(), a 20 mili timer (mt_timer) is started, 
		unlike other timer, this timer won't send any data to fifo
		eachtime when mt_timer timesout, timer.inthanlder20() will directly call mt_taskswtich()
			instead of sending any data to fifo.
		Therefore the task switching happens under the hook, we don't need to setup mt_timer in bootpack.c
		effectively, HariMain() and task_b_main() are called interchangeblly!
	************/
	struct TSS32 tss_a, tss_b;
	tss_a.ldtr = 0;
	tss_a.iomap = 0x40000000;
	tss_b.ldtr = 0;
	tss_b.iomap = 0x40000000;

	/* 
		register these two tasks in GDT 
		register as 3 and 4, 1 and 2 are occupied in dsctbl.c
	*/
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	/* size is 103 bytes */
	set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);
	set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);

	/* 
		TR is task register, it contains the segment index of current running task
		note only load to TR won't switch task, we need to `JMP CS:0` to switch
	*/
	load_tr(3*8);

	int task_b_esp;
	/* we allocate a 64 deep stack to this task, stack address should point to the end of the stack */
	task_b_esp = memman_alloc_4k(memman, 64*1024) + 64 * 1024;
	/* define what task 4 values */
	/* task is merely another function in C, we point to the addr of that func */
	tss_b.eip = (int)&task_b_main;
	tss_b.eflags = 0x00000202; /* IF = 1; */
	tss_b.eax = 0;
	tss_b.ecx = 0;
	tss_b.edx = 0;
	tss_b.ebx = 0;
	tss_b.esp = task_b_esp;
	tss_b.ebp = 0;
	tss_b.esi = 0;
	tss_b.edi = 0;
	/* for segments, just randomly points to any initialized segment */
	tss_b.es = 1 * 8;
	tss_b.cs = 2 * 8;
	tss_b.ss = 1 * 8;
	tss_b.ds = 1 * 8;
	tss_b.fs = 1 * 8;
	tss_b.gs = 1 * 8;
	mt_init();

	/*
		this loop will keep looking at keybuf, if an interruption happens and keybuf is set then it prints the data
	*/
	for(;;) {

		io_cli();
		/* use unbounded buffer shared by timer/keyboard/mouse */
		if(fifo32_status(&fifo) == 0) {
			io_stihlt();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			/* keyboard signal */
			if( i >= 256 && i < 512) {
				sprintf(buffer, "%02X", i - 256);
				putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, buffer, 2);
				if( i < 256+0x54 ) {
					/* this is a valid key, need to append it */
					/* we don't want to write over the text area, fix to max size 144 */
					if(keytable[i-256] != 0 && cursor_x < 144) {
						/* need to create a c stile string, use 0 to terminate the string */
						buffer[0] = keytable[i-256];
						buffer[1] = 0;
						putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, buffer, 1);
						cursor_x += 8;
					}
				}
				/* if backspace is pressed and we have more than 1 letter typed, then we need to remove one letter */
				if( i == 256+0x0e && cursor_x > 8 ) {
					putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
					cursor_x -= 8;
				}
				/* draw cursor */
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
			/* mouse signal */
			else if ( i >= 512 && i <= 767) {

				if(mouse_decode(&mdec, i - 512) != 0) {
					sprintf(buffer, "[lcr %4d %4d]", mdec.x, mdec.y);
					/* note: unless we realase a button, the mdec.btn mask will always be set and letter will alawys be captial */
					if((mdec.btn & 0x01) != 0) {
						buffer[1] = 'L';
					}
					if((mdec.btn & 0x02) != 0) {
						buffer[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0) {
						buffer[2] = 'C';
					}
					/* redraw which key is pressed */
					putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, buffer, 15);

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
					sprintf(buffer, "(%3d, %3d)", mx, my);
					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, buffer, 10);
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
			else if(i == 3) {
				putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
			} else if (i == 10) {
				putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
			} else {
				if (i == 1) {
						timer_init(timer3, &fifo, 0);
						cursor_c = COL8_000000;
				} else if (i == 0) {
						timer_init(timer3, &fifo, 1);
						cursor_c = COL8_FFFFFF;
				}
				timer_settime(timer3, 50);
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
		}
	}
}

/* similar to init_screen8, create a window with X and title */
void make_window8(unsigned char* buf, int xsize, int ysize, char *title) {
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};

	int x, y;
	char c;
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}

/* repaint a box area with a string, length l */
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int color, int background, char*s, int l) {
	boxfill8(sht->buf, sht->bxsize, background, x, y, x + l*8 -1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, color, s);
	sheet_refresh(sht, x, y, x + l*8, y+16);
	return;
}

void debug() {
	putfonts8_asc_sht(sht_back, 0, 155, COL8_FFFFFF, COL8_008484, "mlgb", 4);
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}

void task_b_main(void) {
	/* task b takes over cpu for 5 seconds and switch back to task a, there would be 5 secs halt */
	struct FIFO32 fifo;
	struct TIMER *timer_put, *timer_1s;
	int i, fifobuf[128], count = 0, count0 = 0;
	char buffer[12];

	fifo32_init(&fifo, 128, fifobuf);
	timer_put = timer_alloc();
	timer_init(timer_put, &fifo, 1);
	timer_settime(timer_put, 1);
	timer_1s = timer_alloc();
	timer_init(timer_1s, &fifo, 100);
	timer_settime(timer_1s, 100);


	for(;;) {
		count++;
		io_cli();
		if(fifo32_status(&fifo) == 0) {
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if( i == 1 ) {
				/* we only paint counter on screen every 10 mili - the count won't be consequtive */
				/* but it's still too fast to be captured by eye */
				sprintf(buffer, "%10d", count);
				putfonts8_asc_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, buffer, 11);
				timer_settime(timer_put, 1);
			}
			else if(i == 100) {
				sprintf(buffer, "%10d", count - count0);
				putfonts8_asc_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, buffer, 11);
				count0 = count;
				timer_settime(timer_1s, 100);
			}
		}
	}
	/*note: this function is called through context switch, so we can't return! same as harimain */
}
