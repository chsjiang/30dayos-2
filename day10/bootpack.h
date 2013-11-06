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
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
int load_cr0();
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);

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
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
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

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char* buf);

int fifo8_put(struct FIFO8 *fifo, unsigned char data);

int fifo8_get(struct FIFO8 *fifo);

int fifo8_status(struct FIFO8 *fifo);

/* keyboard.c */
#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
extern struct FIFO8 keyfifo;
void wait_KBC_sendready(void);
void init_keyboard(void);

/* mouse.c */
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
extern struct FIFO8 mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

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
};

/* sheet control, will be used to assign new sheets and sort the stack of used sheets */
struct SHTCTL {
	unsigned char *vram;
	int xsize, ysize; /* xsize and ysize is used to buffer binfo->scrnx and binfo->scrny */
	int top; /* top is the upper most layer */
	/* pointers to sheet, only top sheets are to be stacked */
	struct SHEET *sheets[MAX_SHEETS];
	/* Note SHTCTL has already created 256 struct SHEET when initialized */
	struct SHEET sheets0[MAX_SHEETS];
};

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht);
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, short xsize, short ysize);
struct SHEET *sheet_alloc(struct SHTCTL* ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xisze, int ysize, int col_inv);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1);
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height);
void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refresh_all(struct SHTCTL *ctl);
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHTCTL * ctl, struct SHEET *sht);