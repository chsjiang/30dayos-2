/* declare there's another function defined in other place (naskfunc.obj) */

void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

/* declare a function defined in this file */
void init_palette(void);
void set_palette(int, int, unsigned char*);


void HariMain(void)
{
	int i;
	char* p;
	init_palette();
	/* we're doing the cast here to tell compiler this is a BYTE pointer */
	p = (char*)0xa0000;
	/* in 32 bit mode, all pointer is an address with 4 bytes */
	/*
		char: 1 byte
		short: 2 bytes
		int: 4 bytes
		all pointers are an address represented by 4 bytes
	*/
	for(i = 0x0000; i < 0xffff; i++) {
		/* use pointer to directly write to address */
		/* *(p + i) = i & 0x0F;i */
		/* can also use the following, note p[i] is just a shortcut of *(p+i) */
		/* p[i] = i & 0x0F; */
		/* can also use the following, note i[p] is just a shortcut of *(i+p) */
		i[p] = i & 0x0F;
		

		/* equals MOV BYTE [p+i], i & 0x0F */
		/* 
			if we want to do 
				MOV WORD [i], i & 0x0F
			can write 
			q = (short*) i;
			*q = i & 0x0F
			can also use int* to do DWORD
		*/
	}

	for(;;) {
		io_hlt();
	}

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
