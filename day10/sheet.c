#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, short xsize, short ysize) {
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
	if (ctl == 0) {
		return ctl;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1;
	for (i = 0; i < MAX_SHEETS; i++) {
		/* initially, all sheets are initialized by marked as unused */
		ctl->sheets0[i].flags = 0;
	}
	return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL* ctl) {
	struct SHEET* ret;
	int i;
	for(i = 0; i < MAX_SHEETS; i++) {
		if(ctl->sheets0[i].flags == 0) {
			ret = &ctl->sheets0[i];
			ret->flags = 1; /* set as use */
			ret->height = -1; /* set as hidden, only positibe height will be stacked */
			return ret;
		}
	}
	/* ran out of available sheets */
	return 0;
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}

/* 
	move sht from its original height to the new height 
	note can move from height -1 to a positive height as well, in which case ctl->top will add one
*/
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height) {
	int h, old = sht->height;
	/* check param validity */
	/* can have up to top + 1 layers */
	if(height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if(height < -1) {
		height = -1;
	}
	sht->height = height;
	/* move it to a lower height */
	if(old > height) {
		if(height >= 0) {
			/* raise (height - old-1) one layer up */
			for(h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h-1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} 
		/* else height==-1 we need to remove it from ctl->sheets[] */
		else {
			for(h = height; h < ctl->top; h++) {
				ctl->sheets[h] = ctl->sheets[h+1];
				/* 
					note height is consecutive but not just sorted in ascending order
					so we need to assign the new consequtive height numbers 
				*/
				ctl->sheets[h]->height = h;
			}
			ctl->top--;
		}
	} 
	/* move it to a higher height */
	else if(old < height) {
		if(old >= 0) {
			for(h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h+1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		}
		/* we are adding a new layer */
		else {
			for(h = ctl->top; h > height; h--) {
				ctl->sheets[h+1] = ctl->sheets[h];;
				ctl->sheets[h+1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;
		}
	}
	sheet_refresh(ctl);
	return;
}

/* recheck the stack and paint to vram */
void sheet_refresh(struct SHTCTL *ctl) {
	int h, bx, by, vx, vy;
	unsigned char *buf, color, *vram = ctl->vram;
	struct SHEET* sht;
	for(h = 0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		for(by = 0; by < sht->bysize; by++) {
			/* convert sht relative coordinates to absolute coordinates */
			vy = sht->vy0 + by;
			for(bx = 0; bx < sht->bxsize; bx++) {
				vx = sht->vx0 + bx;
				/* 
					get the color of this pixel from buf 
					when getting it from buf, the coordinates should still be relative coordinates
				*/
				color = buf[by * sht->bxsize + bx];
				if(color != sht->col_inv) {
					/* write to vram, now should use absolute coordinates */
					vram[vy*ctl->xsize + vx] = color;
				}

			}
		}
	}
	return;
}

/* move a single sheet without chaning its height, still need to sheet_refresh */
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	/* only repaint if this sheet is being displayed */
	if(sht->height >= 0) {
		sheet_refresh(ctl);
	}
	return;
}

void sheet_free(struct SHTCTL * ctl, struct SHEET *sht) {
	if(sht->height >= 0) {
		/* first need to remove it from stack */
		sheet_updown(ctl, sht, -1);
	}
	sht->flags = 0;
	return;
}
