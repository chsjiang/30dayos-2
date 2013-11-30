/* all function declarations in bootpack.h will be swapped here*/
#include "bootpack.h"


void init_gdtidt(void)
{
	/* 
		set initial gdt address to 0x00270000, this is arbitrary as that's the address available for programming
		that said, 0x00270000-0x0027ffff will be used for GDT, as 8192 * 8 = 2^16
	*/
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) ADR_IDT;
	int i;

	/* upto 8192 selectors */
	for(i = 0; i < 8192; i++) {
		/* note gdt + 1 is not adding 1 in the address but adding the size of struct SEGMENT_DESCRIPTOR, which is 8 */
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	/* after initialization, we need to setting values of first and second segment */
	/* first segment is in charge of the entire memory 4g, therefore its limit is 0xffffffff - full 32 bits */
	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);
	/* second segment is for bootpack.hrb, which is 512 kb, inside boopack.hrb, it's ORG will be 0x00280000 */
	/* 0x280000 - 0x2fffff is for bootpack.h, it's set in asmhead.nas */
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);

	/* can't directly writie to gdt register or idt register, need to use assembly */
	/* gdtr will start from 0x0027000 */
	load_gdtr(LIMIT_GDT, ADR_GDT);

	for(i = 0; i < 256; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	
	/* ldtr will start from 0x0026f800 */
	load_idtr(LIMIT_IDT, ADR_IDT);

	/* register IDT for mouse and keyboard handler, calling callback*/
	/* 2 * 8 is segment seelctor, we are at number 2 segment therefore need to offset 16 bytes */
	set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
	/* 
	   note: since idt has 2^8 = 256 slots, but there're only 32 valid status, 
		we can use the rest to register other base system interruptions 
		executing `INT 0x40` will invoke this 'interruption' and 
		effectively call the asm_hrb_api function
		application is responsible for pushing the params to be passed to a system call
		to particular registers
	*/
	set_gatedesc(idt + 0x40, (int) asm_hrb_api, 2 * 8, AR_INTGATE32);

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
