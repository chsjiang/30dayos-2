/***********
	naskfunc.nas
***********/
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_inthandler0d(void);
void asm_hrb_api(void);
void load_tr(int tr);
int load_cr0();
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void taskswitch3(void);
void taskswitch4(void);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);

/***********
	graphic.c
***********/
/* declare a function defined in this file */
void init_palette(void);
void set_palette(int, int, unsigned char*);
void boxfill8(unsigned char*, int, unsigned char, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int xsize, int ysize);
void putfont8(char *vram, int xsize, int x, int y, char color, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char color, unsigned char *s);
void init_mouse_cursor8(char *mouse, char backgroudcolor);
void putblock8_8( char *vram, int vxisze, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

/***********
	dsctbl.c
***********/
/* 
	a segment descriptor contains 8 bytes , global segment descriptor table(GDT) contains upto 8191 descriptors( because segment register has 13 valid bits)
	GDTR(GDT register) stores the starting address and number of valid descriptors
*/
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e
#define AR_TSS32		0x0089

/***********
	asmhead.nas
***********/
/* struct definition needs to be followed by comma */
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};
/* setting base address of binfo will automatically set it's members in sequence, note binfo_scrnx = (short *) 0xff4; */
#define ADR_BOOTINFO	0x00000ff0
/* disk img is copied to memory from 0x00100000 */
#define ADR_DISKIMG		0x00100000

/* int.c */
/* KEYBUF is used to buffer a key value when interruption happens, 
	we should print the value in other places rather than in the interruption handling function,
	because interruption handling should be fast and short
*/
struct KEYBUF {
	/* unsigned char data, flag; */

	/* instead of defining just one char, we need a FIFO buffer to handle multiple chars, because one keystroke will generate 2 key code */
	unsigned char data[32];
	int start, end;
	/* can't define default value here! */
	int full;
};

void init_pic(void);
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
#define PORT_KEYDAT		0x0060

/* fifo.c */
/* defining an unbounded resizable buffer */
struct FIFO8 {
	unsigned char *data;
	int start, end, size, full, flag;
};

/* 
	we want to use a fifo for all types of interruptions
	by using differernt range for differernt int
		0-255: timer signal
		255-511: keyboard signal
		512-up: mouse signal
	so we need a longer *data
*/
struct FIFO32 {
	int *buf;
	int start, end, size, free, flag;
	struct TASK *task;
};

void fifo32_init(struct FIFO32 *fifo, int size, int* buf, struct TASK *task);

int fifo32_put(struct FIFO32 *fifo, int data);

int fifo32_get(struct FIFO32 *fifo);

int fifo32_status(struct FIFO32 *fifo);

/* keyboard.c */
#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *argKeyfifo, int data0);
void inthandler21(int *esp);

/* mouse.c */
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
void enable_mouse(struct FIFO32 *mousefifoArg, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, int dat);
void inthandler2c(int *esp);

/* memory.c */
#define MEMMAN_FREES 		4090	/* around 32K */
#define MEMMAN_ADDR			0x003c0000
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000
struct FREEINFO 
{
	unsigned int addr, size;
};

struct MEMMAN
{
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned size);

/*********
	 sheet.c
*********/
#define MAX_SHEETS	256
#define SHEET_USE 	1
/* a layer to be drawn, the screen will be represented by a stack of layers */
struct SHEET {
	unsigned char *buf; /* buffers the vram address to start writing color bytes to */
	int bxsize, bysize; /* size of this sheet */
	int vx0, vy0; /* coordinates of upper left conor */
	int col_inv; /* invisible(transparent) color */
	int height; /* only the heighest will be drawn */
	int flags; /* mark if used */
	struct SHTCTL *ctl; /* eliminating the usage of struct SHTCTL */
};

/* sheet control, will be used to assign new sheets and sort the stack of used sheets */
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize; /* xsize and ysize is used to buffer binfo->scrnx and binfo->scrny */
	int top; /* top is the upper most layer */
	/* pointers to sheet, only top sheets are to be stacked */
	struct SHEET *sheets[MAX_SHEETS];
	/* Note SHTCTL has already created 256 struct SHEET when initialized */
	struct SHEET sheets0[MAX_SHEETS];
};

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL* ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xisze, int ysize, int col_inv);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refresh_all(struct SHTCTL *ctl);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/*********
	 timer.c
*********/
#define MAX_TIMER	500
struct TIMER {
	/* link it to make timer int handling faster */
	struct TIMER *next;
	unsigned int timeout, flags;
	struct FIFO32 *fifo;
	int data;
};

struct TIMERCTL {
	/*next is the next epoch that would */
	unsigned int count, next;
	struct TIMER timers0[MAX_TIMER];
	struct TIMER *head;
};

extern struct TIMERCTL timerctl;
void init_pit(void);
void inthandler20(int* esp);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *timerfifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);

/*********
	 mtask.c
*********/
/*
	Some tasks have higher priority that when they are running(music, mousehandling etc.),
		all other tasks need to wait.
	Therefore we implement levels, the higher the level, the more important the task.
	OS will only exeute lower tasks when all upper level are empty
*/
#define		MAX_TASKS	1000 /* max tasks number */
#define 	TASK_GDT0 	3 /* selector starts from 3, the first two are already occupied */
#define     MAX_TASKS_LV	100 /* maximum 100 tasks in one level */
#define 	MAX_TASKLEVELS 	10  /* maximum 10 levels */
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

struct TASK {
	int sel; /* sel is the index of the task in taskctl */
	int flags;
	int level, priority; /* each task will get a time slice task->priority * 10 ms to run */
	struct FIFO32 fifo; /* each task has a fifo to store interruption signals */
	struct TSS32 tss;
};

struct TASKLEVEL {
	int running; /* number of running tasks at this level */
	int now; /* current running task */
	struct TASK *tasks[MAX_TASKS_LV];
};

struct TASKCTL  {
	int now_lv; /* current active level */
	char lv_change; /* is current active level changed? */
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};

extern struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK* task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
struct TASK *task_now(void);
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_switchsub(void);
void task_idle(void);

/*********
	window.c
*********/
void make_window8(unsigned char* buf, int xsize, int ysize, char *title, char act);
void make_wtitle8(unsigned char* buf, int xsize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char*s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

/*********
	console.c
*********/	
struct CONSOLE {
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
};
void console_task(struct SHEET *sheet, unsigned int memtotal);
void cons_newline(struct CONSOLE *cons);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0d(int *esp);

/*********
	file.c
*********/
/*
	microsoft defined file format, when we burn the image, 
	the files crunched into the img(haribote.sys, ipl10.nas, Makefile)
	are stored in the img file with the following format
	the struct is 32bytes
*/
struct FILEINFO {
	/*
		name and extention
		name[0]: 0x00 empty
				 0xef deleted
		type: 0x01: readonly
			  0x02: hidden file
			  0x04: sys file
			  0x08: non-file info( disk name etc. )
			  0x10: dir
	*/
	unsigned char name[8], ext[3], type;
	char reserve[10];
	/* 
		clustno is clustor number
		the actual content of file is stored at a start of a clustor, a clustor is 512 bytes,
		and clustor 0 has address 0x003e00, so the (floppy)address of the file is
			0x003e00 + 512 * clustno
	*/
	unsigned short time, date, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);

/***********
	bootpack.c
***********/
void debug(struct SHEET *sheet, char *str);
