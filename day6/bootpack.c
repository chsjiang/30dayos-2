#include <stdio.h>
/* declare there's another function defined in other place (naskfunc.obj) */

void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

/* declare a function defined in this file */
void init_palette(void);
void set_palette(int, int, unsigned char*);
void boxfill8(unsigned char*, int, unsigned char, int x0, int y0, int x1, int y1);
void init_screen(char *vram, int xsize, int ysize);
void putfont8(char *vram, int xsize, int x, int y, char color, char *font);
void putfont8_asc(char *vram, int xsize, int x, int y, char color, unsigned char *s);
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

/* struct definition needs to be followed by comma */
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

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

void int_gtdidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

void HariMain(void)
{	
	/*
	char* vram;
	int xsize, ysize;
	
	short *binfo_scrnx, *binfo_scrny;
	int *binfo_vram;
	*/
	init_palette();
	
	struct BOOTINFO *binfo;
	
	/* setting base address of binfo will automatically set it's members in sequence, note binfo_scrnx = (short *) 0xff4; */
	binfo = (struct BOOTINFO *) 0xff0;

	/* as we will compile hankaku with bootpack, the fonts will be define in hankaku.bin, just need to declar it here to use it */
	/* extern: when used, the variable is supposed to be used somehwere else and the resolving is deferred to the linker */
	/*
	extern char hankaku[4096];
	*/
	/* these address are from asmhead, it returns the width/heigh of current vga mode and the base vga ram address */
	/*
	binfo_scrnx = (short *) 0xff4;
	binfo_scrny = (short *) 0xff6;
	binfo_vram = (int *) 0xff8;
	*/
	
	/*
	xsize = (*binfo).scrnx;
	ysize = (*binfo).scrny;
	*/
	/* we're doing the cast here to tell compiler this is a BYTE pointer */
	/* note vram is a char pointer, or an ADDRESS, all pointers/address are 4 bytes, which is the same size as an int, therefore we use a int* to point to the ADDRESS that stores the address value of vram */
	/*
	vram = (*binfo).vram;
	*/
	/* vram = (char*)0xa0000; */
	/* in 32 bit mode, all pointer is an address with 4 bytes */
	/*
		char: 1 byte
		short: 2 bytes
		int: 4 bytes
		all pointers are an address represented by 4 bytes
	*/
	
	/*
	boxfill8(p, 320, COL8_FF0000, 20, 20, 120, 120);
	boxfill8(p, 320, COL8_00FF00, 70, 50, 170, 150);
	boxfill8(p, 320, COL8_0000FF, 120, 80, 220, 180);
	
	xsize = 320;
	ysize = 200;
	*/
	
	
	/* draw bunch of boxes that looks like an OS UI */
	
	/* Note (*binfo).vram can be shortened as binfo->vram */
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);	
	
	/* write a letter A ! */
	/*
	static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};
	*/
	/* 
		since hankaku is a 256 * 16 byte memory block and each letter (16 byte) is stored as the ascii sequence,
		to get the address of letter A, we just need to offset 0x41 ('A') bytes
	*/

	/*
	putfont8(binfo->vram, binfo->scrnx, 8, 8, COL8_FF0000, hankaku + 'A' * 16);
	putfont8(binfo->vram, binfo->scrnx, 16, 8, COL8_FF0000, hankaku + 'B' * 16);
	putfont8(binfo->vram, binfo->scrnx, 24, 8, COL8_FF0000, hankaku + 'C' * 16);
	putfont8(binfo->vram, binfo->scrnx, 32, 8, COL8_FF0000, hankaku + '1' * 16);
	putfont8(binfo->vram, binfo->scrnx, 40, 8, COL8_FF0000, hankaku + '2' * 16);
	putfont8(binfo->vram, binfo->scrnx, 48, 8, COL8_FF0000, hankaku + '3' * 16);
	*/
	/*
	putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FF0000, "MLGBmlgb");

	putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "MLGB OS.");
	putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "MLGB OS.");
	*/
	/* 
		unlike printf(), which can't be used in MLGB OS, sprintf() is used for writing to a particular address, therefore we can use it to initialize a string, need to use stdio.h
		so we can print a variable's value on screen!
	*/
	/*
	char* s;
	sprintf(s, "scrnx=%d", binfo->scrnx);
	putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);
	*/

	int mx, my;
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 20 - 16) / 2;
	char mouse[256], cor[40];
	sprintf(cor, "(%d, %d)", mx, my);
	putfont8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, cor);
	init_mouse_cursor8(mouse, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mouse, 16);
	for(;;) {
		io_hlt();
	}

}
