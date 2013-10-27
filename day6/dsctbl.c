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
