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

/* this function is used to put a string */
void putfont8_asc(char *vram, int xsize, int x, int y, char color, unsigned char *s) 
{
	extern char hankaku[4096];
	/* 0x00 is the end of a string */
	for(;*s != 0x00; s++)
	{
		putfont8(vram, xsize, x, y, color, hankaku + *s * 16);
		x += 8;
	}
}
/* font is a 8 * 16 matrix */
void putfont8(char *vram, int xsize, int x, int y, char color, char *font)
{
	int i;
	/* buffer pointer and data to run faster */
	char *p, d;
	for( i = 0; i < 16; i++ ) 
	{
		/*
		if( (font[i] & 0x80) != 0 ) { vram[xsize * (y + i) + x + 0] = color; } 
		if( (font[i] & 0x40) != 0 ) { vram[xsize * (y + i) + x + 1] = color; }
		if( (font[i] & 0x20) != 0 ) { vram[xsize * (y + i) + x + 2] = color; }
		if( (font[i] & 0x10) != 0 ) { vram[xsize * (y + i) + x + 3] = color; }
		if( (font[i] & 0x08) != 0 ) { vram[xsize * (y + i) + x + 4] = color; }
		if( (font[i] & 0x04) != 0 ) { vram[xsize * (y + i) + x + 5] = color; }
		if( (font[i] & 0x02) != 0 ) { vram[xsize * (y + i) + x + 6] = color; }
		if( (font[i] & 0x01) != 0 ) { vram[xsize * (y + i) + x + 7] = color; }
		*/
		
		d = font[i];
		p = vram + xsize * (y + i ) + x;
		if( (d & 0x80) != 0 ) { p[0] = color; } /* can also be written as `*(vram + xsize * (y+i) + x +0)` */
		if( (d & 0x40) != 0 ) { p[1] = color; }
		if( (d & 0x20) != 0 ) { p[2] = color; }
		if( (d & 0x10) != 0 ) { p[3] = color; }
		if( (d & 0x08) != 0 ) { p[4] = color; }
		if( (d & 0x04) != 0 ) { p[5] = color; }
		if( (d & 0x02) != 0 ) { p[6] = color; }
		if( (d & 0x01) != 0 ) { p[7] = color; }
	}
	return;
}

void init_mouse_cursor8(char *mouse, char backgroudcolor)
{
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};

	int x, y;
	/* need to write the entire matrix */
	for(y = 0; y < 16; y++) {
		for(x = 0; x < 16; x++) {
			/* edge */
			if(cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			/* cursor */
			if(cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}

			if(cursor[y][x] == '.') {
				mouse[y * 16 + x] = backgroudcolor;
			}
		}		
	}
	return;
}

/* used to copy a block of address to *vram */
void putblock8_8( char *vram, int vxisze, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize )
{
	int x, y;
	for(y = 0; y < pysize; y++) {
		for(x = 0; x < pxsize; x++) {
			vram[(py0 +y) * vxisze + px0 + x] = buf[y * bxsize + x];
		}
	}
	return;
}


void init_screen(char *vram, int xsize, int ysize)
{
	boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 29);
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28);
	boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27);
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1);

	boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24);
	boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4);
	boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
	boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5);
	boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
	boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3);

	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);
	return;
}

/*
	palette maps int and rgb color, we defined 16 colors, needs to be written to cpus
*/
void init_palette(void) 
{
	/* declaring static will make this table value bound to the file */
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/* black 		*/
		0xff, 0x00, 0x00,	/* light red 	*/
		0x00, 0xff, 0x00,	/* light green 	*/
		0xff, 0xff, 0x00,	/* light yellow */
		0x00, 0x00, 0xff,	/* light blue 	*/
		0xff, 0x00, 0xff,	/* light purple */
		0x00, 0xff, 0xff,	/* light light blue */
		0xff, 0xff, 0xff,	/* white */
		0xc6, 0xc6, 0xc6, 	/* light grey */
		0x84, 0x00, 0x00, 	/* dark red */
		0x00, 0x84, 0x00,	/* dark green */
		0x84, 0x84, 0x00, 	/* dark yellow */
		0x00, 0x00, 0x84,	/* dark green-blue */
		0x84, 0x00, 0x84, 	/* dark purple */
		0x00, 0x84, 0x84, 	/* light dark blue */
		0x84, 0x84, 0x84	/* light grey */
	};
	
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags();
	io_cli();
	/* 
		access palette:
			first need to disable all interruption( clear eflags and buffer them)
			then write palette # to 0x03c8, then write rgb color to 0x03c9, if there's more colors to add, just continue write rgb to 0x03c9
			then reset eflags
	*/
	
	io_out8(0x03c8, start);
	for(i = start; i <=end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb+=3;
	}
	io_store_eflags(eflags);
	return;
}

/*
	in current VGA mode, they're 320*200 pixels on the screen, each pixel can be represented by an address
	(0,0) is upper left, (319, 199) is lower right
	(0xa0000 + x + y * 320) will point to a pixel, assigning it a color value(0-15) can draw this pixel with the color
*/
void boxfill8(unsigned char* startingAddress, int xsize, unsigned char color, int x0, int y0, int x1, int y1) 
{
	int i, j;
	for(i = x0; i <= x1; i++) 
	{
		for(j = y0; j <= y1; j++) {
			startingAddress[i + xsize*j] = color;
		}
	}
}

void int_gtdidt(void)
{
	/* 
		set initial gdt address to 0x00270000, this is arbitrary as that's the address available for programming
		that said, 0x00270000-0x0027ffff will be used for GDT, as 8192 * 8 = 2^16
	*/
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000;
	struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) 0x0026f800;
	int i;

	/* upto 8192 selectors */
	for(i = 0; i < 8192; i++) {
		/* note gdt + 1 is not adding 1 in the address but adding the saize of struct SEGMENT_DESCRIPTOR, which is 8 */
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	/* after initialization, we need to setting values of first and second segment */
	/* first segment is in charge of the entire memory 4g, therefore its limit is 0xffffffff - full 32 bits */
	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
	/* second segment is for bootpack.hrb, which is 512 kb, inside boopack.hrb, it's ORG will be 0x00280000 */
	/* 0x280000 - 0x2fffff is for bootpack.h, it's set in asmhead.nas */
	set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);

	/* can't directly writie to gdt register or idt register, need to use assembly */
	/* gdtr will start from 0x0027000 */
	load_gdtr(0xffff, 0x00270000);

	for(i = 0; i < 256; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	
	/* ldtr will start from 0x0026f800 */
	load_idtr(0x7ff, 0x0026f800);
	return;
}

/* ar is attributes */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
	if( limit >= 0xfffff) {
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}
	sd->limit_low 	= limit & 0xffff;
	sd->base_low	= base & 0xffff; 
	sd->base_mid	= (base >> 16) & 0xff;
	sd->access_right= ar & 0xff;
	sd->limit_high	= ((limit >> 16) & 0xf) | ((ar >> 8) & 0xf0);
	sd->base_high 	= (base >> 24) & 0xff;
	return;	

}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
	gd->offset_low	= offset & 0xffff;
	gd->selector 	= selector;
	gd->dw_count	= (ar >> 8) & 0xff;
	gd->access_right= ar & 0xff;
	gd->offset_high	= (offset >> 16) & 0xffff;
	return;	
}
