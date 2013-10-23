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
	
	/* setting base address of binfo will automatically set it's members in sequence */
	binfo = (struct BOOTINFO *) 0xff0;
	
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
	static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};
	putfont8(binfo->vram, binfo->scrnx, 10, 10, COL8_FF0000, font_A);
	

	for(;;) {
		io_hlt();
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
