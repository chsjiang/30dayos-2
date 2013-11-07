#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, short xsize, short ysize) {
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
	if (ctl == 0) {
		return ctl;
	}
	ctl->map = (unsigned char*)memman_alloc_4k(memman, xsize * ysize);
	if(ctl->map == 0) {
		memman_free_4k(memman, (int) ctl, sizeof (struct SHTCTL));
		return ctl;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1;
	for (i = 0; i < MAX_SHEETS; i++) {
		/* 
			initially, all sheets are initialized by marked as unused 
			and we need to initialize the ctl pointer upfront
		*/
		ctl->sheets0[i].flags = 0;
		ctl->sheets0[i].ctl = ctl;
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
void sheet_updown(struct SHEET *sht, int height) {
	struct SHTCTL *ctl = sht->ctl;
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
			/* this layer is still on screen, we only need to redraw the layers on top of it */
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, sht->height + 1);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, sht->height + 1, old);
		} 
		/* else height==-1 we need to remove it from ctl->sheets[] */
		else {
			if(ctl->top > old) {
				for(h = height; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h+1];
					/* 
						note height is consecutive but not just sorted in ascending order
						so we need to assign the new consequtive height numbers 
					*/
					ctl->sheets[h]->height = h;
				}
			}	
			ctl->top--;
			/* this layer is removed, we need to redraw everything */ 
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
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
			for(h = ctl->top; h >= height; h--) {
				ctl->sheets[h+1] = ctl->sheets[h];
				ctl->sheets[h+1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;
		}
		/* this layer is lifted, only draw from its height and above */
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, sht->height);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, sht->height, sht->height);
	}
	
	return;
}

/* recheck the stack and paint to vram, we need to refresh stack from the height of this layer and up */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
	struct SHTCTL *ctl = sht->ctl;
	if(sht->height >= 0) {
		/* (bx0, by0) and (bx1, by1) are the coordinates within this sheet, need to transfer it to absolute coordinates */
		sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
	}
	return;
}

/* 
	only redraw(write mem addr to) a sub area 
	and only refresh from height h0 and up
	when we want to refresh a window, we can check from the stack of the window 
	and ignore layer beneath(like background or other window that's blocked)
*/
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
	int h, bx, by, vx, vy;
	unsigned char *buf, color, *vram = ctl->vram, sid, *map = ctl->map;
	if(vx0 < 0) {
		vx0 = 0;
	}
	if(vy0 < 0) {
		vy0 = 0;
	}
	if(vx1 > ctl->xsize) {
		vx1 = ctl->xsize;
	}
	if(vy1 > ctl->ysize) {
		vy1 = ctl->ysize;
	}
	struct SHEET* sht;
	for(h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		sid = sht - ctl->sheets0;
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
				/* only redraw when the pixel falls within the sub area */
				if(vx0 <= vx && vx < vx1 && vy0 <= vy && vy < vy1) {
					if(map[vy*ctl->xsize + vx] == sid) {
						/* write to vram, now should use absolute coordinates */
						vram[vy*ctl->xsize + vx] = color;
					}
				}
			}
		}
	}
	return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
	int h, bx, by, vx, vy;
	unsigned char *buf, color, *map = ctl->map, sid;
	if(vx0 < 0) {
		vx0 = 0;
	}
	if(vy0 < 0) {
		vy0 = 0;
	}
	if(vx1 > ctl->xsize) {
		vx1 = ctl->xsize;
	}
	if(vy1 > ctl->ysize) {
		vy1 = ctl->ysize;
	}
	struct SHEET* sht;
	for(h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		sid = sht - ctl->sheets0;
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
				/* only redraw when the pixel falls within the sub area */
				if(vx0 <= vx && vx < vx1 && vy0 <= vy && vy < vy1) {
					if(color != sht->col_inv) {
						/* write to vram, now should use absolute coordinates */
						map[vy*ctl->xsize + vx] = sid;
					}
				}
			}
		}
	}
	return;
}

void sheet_refresh_all(struct SHTCTL *ctl) {
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
				/* only redraw when the pixel falls within the sub area */
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
void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
	struct SHTCTL *ctl = sht->ctl;
	int oldx = sht->vx0, oldy = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	/* only repaint if this sheet is being displayed */
	if(sht->height >= 0) {
		/* only redraw the area before and after */
		/* 
			the old area needs to be redrawn from layer 0 - because it's removed 
			new aread can be redrawn from its height
		*/
		sheet_refreshmap(ctl, oldx, oldy, oldx+sht->bxsize, oldy+sht->bysize, 0);
		sheet_refreshsub(ctl, oldx, oldy, oldx+sht->bxsize, oldy+sht->bysize, 0, sht->height-1);
		sheet_refreshmap(ctl, vx0, vy0, vx0+sht->bxsize, vy0+sht->bysize, sht->height);
		sheet_refreshsub(ctl, vx0, vy0, vx0+sht->bxsize, vy0+sht->bysize, sht->height, sht->height);
	}
	return;
}

void sheet_free(struct SHEET *sht) {
	if(sht->height >= 0) {
		/* first need to remove it from stack */
		sheet_updown(sht, -1);
	}
	sht->flags = 0;
	return;
}
