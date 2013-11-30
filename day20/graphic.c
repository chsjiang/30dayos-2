#include "bootpack.h"

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

void init_screen8(char *vram, int xsize, int ysize)
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


/* this function is used to put a string */
void putfonts8_asc(char *vram, int xsize, int x, int y, char color, unsigned char *s) 
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
	// static char cursor[16][16] = {
	// 	"**************..",
	// 	"*OOOOOOOOOOO*...",
	// 	"*OOOOOOOOOO*....",
	// 	"*OOOOOOOOO*.....",
	// 	"*OOOOOOOO*......",
	// 	"*OOOOOOO*.......",
	// 	"*OOOOOOO*.......",
	// 	"*OOOOOOOO*......",
	// 	"*OOOO**OOO*.....",
	// 	"*OOO*..*OOO*....",
	// 	"*OO*....*OOO*...",
	// 	"*O*......*OOO*..",
	// 	"**........*OOO*.",
	// 	"*..........*OOO*",
	// 	"............*OO*",
	// 	".............***"
	// };

	// static char cursor[16][16] = {
	// 	"..************..",
	// 	"...*OOOOOOOO**..",
	// 	"*...*OOOOOO*.*..",
	// 	"**...*OOOO**.*..",
	// 	"*O*...*OO*.*.*..",
	// 	"*OO*.*OO*..*.*..",
	// 	"*OOO*OOO*..*.*..",
	// 	"*OOOOOOOO*.*.*..",
	// 	"*OOOO**OOO**.*..",
	// 	"*OOO*..*OOO*.*..",
	// 	"*OO*....*OOO**..",
	// 	"*O********OOO*..",
	// 	"**........*OOO*.",
	// 	"************OOO*",
	// 	"............*OO*",
	// 	".............***"
	// };

	static char cursor[16][16] = {
		"**..............",
		"*O**............",
		".*OO**..........",
		".*OOOO**........",
		"..*OOOOO**......",
		"..*OOOOOOO**....",
		"...*OOOOOOOO**..",
		"...*OOOOOOOOOO**",
		"....*OOOO*****..",
		"....*OOO**......",
		".....*OO*.......",
		".....*OO*.......",
		"......*O*.......",
		"......*O*.......",
		".......*........",
		".......*........"
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
