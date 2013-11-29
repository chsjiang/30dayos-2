#include "bootpack.h"
#include <string.h>
#include <stdio.h>


/* 
	start a new line, append/scroll when necessary 
	eventually each command is also appending line by line 
	use this method whenever a line break is required
*/
int cons_newline(int cursor_y, struct SHEET *sheet) {
	int x, y;
	/* if we're note at bottom line, add a new line */
	if( cursor_y < 28 + 112 ) {
		cursor_y += 16;
	} 
	/* otherwise offset line (2:n) to line (1:n-1) and draw new line at bottom */
	else {
		for(y = 28; y < 28 + 112; y++) {
			for(x = 8; x < 248; x++) {
				sheet->buf[x + y*sheet->bxsize] = sheet->buf[x + (y+16) * sheet->bxsize];
			}
		}
		/* clear the last line */
		for(y = 28 + 112; y < 28 + 128; y++) {
			for(x = 8; x < 248; x++) {
				sheet->buf[x + y*sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, 8+240, 28+128);
	}
	return cursor_y;
}

/* 
	start a task that runs a console 
	a task needs a timer for flashing cursor
	a fifo for transferring int data
	a sheet to display info
*/
void console_task(struct SHEET *sheet, unsigned int memtotal) {
	/* need to sleep the task when it's not active, so we need to get the reference of the task */
	struct TASK *task = task_now();
	struct TIMER *timer;

	int i, fifobuf[128], cursor_x = 56, cursor_c = -1, cursor_y = 28;
	/* fifo belongs to a task, this allows task_a to put data to task_cons's fifo */
	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	/* cmdline is used to buffer the info we input in command */
	char s[30], cmdline[30], *p;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	/* from 0x002600 of the img addr, file info are stored, when copied to disk, we need to offset disk starting address */
	struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);

	/* up to 2880 clusters in a floppy */
	int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);

	/* we need to use gdt to start an application */
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	int x, y;

	/* PS1 */
	putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "MLGB:>", 6);

	/* fat stores at 0x000200 of floppy, read its info to have fat[] in place */
	file_readfat(fat, (unsigned char*)(ADR_DISKIMG + 0x000200));

	for(;;) {
		io_cli();
		if(fifo32_status(&task->fifo) == 0) {
			/* sleep as much as possible */
			task_sleep(task);
			io_sti();
		} else  {
			i = fifo32_get(&task->fifo);
			io_sti();
			if(i <= 1) {
				/* siwtching data/color */
				if(i != 0) {
					timer_init(timer, &task->fifo, 0);
					if(cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				} else {
					timer_init(timer, &task->fifo, 1);
					if(cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				}
				timer_settime(timer, 50);
			}
			/* tab is pressed, now console_task is on focus, should begin flashing its cursor */
			if (i == 2) {
				cursor_c = COL8_FFFFFF;
			}
			/* console_task lost focus, erase cursor and stop flashing */
			if (i == 3) {
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
				cursor_c = -1;
			}
			/* keyboard data sent from task_a */
			if(256<=i && i <= 511) {
				/* note i is already transferred to char, (char)i-256 will be the char */
				/* backspace */
				if(i == 256 + 8) {
					/* keep console head (6+1)*8=56 */
					if(cursor_x > 56) {
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
						cursor_x -= 8;
					}
				} 
				/* enter key, do line break */
				else if( i == 256 + 10 )  {
					/* clear current cursor */
					putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
					/* 
						finish collecting a command input, add 0 to terminate c style string 
						we add this in case string like 'memmlgb' is also parsed as valid command
					*/
					cmdline[cursor_x/8-7] = 0;
					/* do line break */
					cursor_y = cons_newline(cursor_y, sheet);
					/* if input is 'mem' then execute the command */
					if(strcmp(cmdline, "mem") == 0) {
						/* memtotal is in byte */
						sprintf(s, "total %dMB", memtotal/1024/1024);
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						sprintf(s, "free %dKB", memman_total(memman)/1024);
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						/* add an additional line break*/
						cursor_y = cons_newline(cursor_y, sheet);
					}
					/* clean entire screen! */
					else if(strcmp(cmdline, "cls") == 0) {
						for(y = 28; y < 28 + 128; y++) {
							for(x = 8; x < 240; x++) {
								sheet->buf[x + y*sheet->bxsize] = COL8_000000;
							}
						}
						sheet_refresh(sheet, 8, 28, 8+240, 28+128);
						cursor_y = 28;
					}
					/* display info of file that crunched with the img */
					else if(strcmp(cmdline, "dir") == 0) {
						/* can crunch up to 224 files with img */
						for(x = 0; x < 224; x++) {
							/* 0x00 is empty file */
							if(finfo[x].name[0] == 0x00) {
								break;
							}
							/* 
								if it's not deleted, then it's a valid file/dir 
								note if it's not empty then it will be parsed as a valid character
							*/
							if(finfo[x].name[0] != 0xe5) {
								/* if it's not dir or non-file info( if it's a file) */
								if((finfo[x].type & 0x18) == 0) {
									sprintf(s, "filename.ext  %7d", finfo[x].size);
									/* since file name and ext length are fixed, we can replace them in s afterwards */
									for(y = 0; y < 8; y++) {
										s[y] = finfo[x].name[y];
									}
									for(y = 9; y < 12; y++) {
										s[y] = finfo[x].ext[y-9];
									}
									putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
									cursor_y = cons_newline(cursor_y, sheet);
								}
							}
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}
					/* 
						`type blah` == `cat blah` 
						strncmp(ml, gb, num): only compare first number letters of ml and gb
					*/
					else if(strncmp(cmdline, "type ", 5) == 0) {
						/* get the file name */
						for(y = 0; y < 11; y++) {
							s[y] = ' ';
						}
						y = 0;
						for(x = 5; (y < 11) && (cmdline[x] != 0); x++) {
							/* 
								note if file name is shorter than 8, we need to padding it with ' '
								mlgb.txt will be stored as 'mlgb    txt'
							*/
							if(cmdline[x] == '.' && y <= 8) {
								y = 8;
							} else {
								s[y] = cmdline[x];
								/* convert to upper case */
								if('a' <= s[y] && s[y] <= 'z') {
									s[y] -= 0x20;
								}
								y++;
							}
						}
						/* search for the file that matches the name */
						for(x = 0; x < 224;) {
							/* 0x00 is empty file */
							if(finfo[x].name[0] == 0x00) {
								break;
							}

							if((finfo[x].type & 0x18) == 0) {
								for(y = 0; y < 11; y++) {
									if(finfo[x].name[y] != s[y]) {
										goto next_file;
									}
								}
								/* found it! */
								break;
							}

						next_file:
							x++;
						}
						/* found the file, use the fomula to gets where it's actually stored, then print it char by char */
						if(x < 224 && finfo[x].name[0] != 0x00) {
							/* 
								since file is not consequtively sotred, we first alloc a chunk of memory, copy the file content there
								then read from that chunk of memory byte by byte
								finally we free that chunk of data
							*/							
							p = (char *)memman_alloc_4k(memman, finfo[x].size);
							/* add ADR_DISKIMG to convert the address of flopy to the address of disk */
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)0x003e00 + ADR_DISKIMG);

							cursor_x = 8;
							/* now p points to the memory that holds the file content, just read finfo[x].size bytes from there */
							for(y = 0; y < finfo[x].size; y++) {
								s[0] = p[y];
								s[1] = 0;
								if(s[0] == 0x09) /* tab, will pad spaces until it's multiple of 4 */
								{
									/* 31 pixels is 4 letters */
									do {
										putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
										cursor_x += 8;
										if(cursor_x == 248) {
											cursor_x = 8;
											cursor_y = cons_newline(cursor_y, sheet);
										}
									} while(((cursor_x-8) % 32) != 0);
									/* 
										note we can also do the following:
										while(((cursor_x-8) & 31) != 0);
										because all 32's multiplier would have the trailing bits as 0,
										we just apply a trailing number of all 1 can do the trick
									*/
								}
								else if(s[0] == 0x0a) /* line break, we only break a line for 0x0a, like linux */
								{
									cursor_x = 8;
									cursor_y = cons_newline(cursor_y, sheet);
								} else if(s[0] == 0x0d) /* carrirage return */
								{
									/* nothing */
								} 
								/* regular chars, just print it */
								else {
									putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
									cursor_x += 8;
									/* linebreak */
									if(cursor_x == 248) {
										cursor_x = 8;
										cursor_y = cons_newline(cursor_y, sheet);
									}
								}
							}
							/* finish reading the file, free the memory */
							memman_free(memman, (int) p, finfo[x].size);
						}
						/* file not found */
						else  {
							putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
							cursor_y = cons_newline(cursor_y, sheet);
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}
					/* execute an application - execute another method, need to use fatjmp */
					else if(strcmp(cmdline, "hlt") == 0) {
						for(y = 0; y < 11; y++) {
							s[y] = ' ';
						}
						/* 
							we need to find the file nameed hlt.hrb, 
							loads it into memory and farjump to there to execute 
						*/
						s[0] = 'H';
						s[1] = 'L';
						s[2] = 'T';
						s[8] = 'H';
						s[9] = 'R';
						s[10] = 'B';
						/* search for the file that matches the name */
						for(x = 0; x < 224;) {
							/* 0x00 is empty file */
							if(finfo[x].name[0] == 0x00) {
								break;
							}

							if((finfo[x].type & 0x18) == 0) {
								for(y = 0; y < 11; y++) {
									if(finfo[x].name[y] != s[y]) {
										goto hlt_next_file;
									}
								}
								/* found it! */
								break;
							}

						hlt_next_file:
							x++;
						}

						/* found the file */
						if(x < 224 && finfo[x].name[0] != 0x00) {
							p = (char *)memman_alloc_4k(memman, finfo[x].size);
							/* load it into memory */
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)0x003e00 + ADR_DISKIMG);

							/* 
								note: the new application shouldn't occupy any task(1-1000), 
								we assign it 1003 to bypass taskmgr
								this application still belongs to console_task,
								pressing 'tab' will switch to the other window task
							*/
							set_segmdesc(gdt + 1003, finfo[x].size - 1, (int)p, AR_CODE32_ER);
							farjmp(0, 1003 * 8);
							memman_free(memman, (int) p, finfo[x].size);
						}
						/* file not found */
						else  {
							putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
							cursor_y = cons_newline(cursor_y, sheet);
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}
					/* otherwise if it's not a blank line, shout*/
					else if(cmdline[0] != 0) {
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "MLGB! Invalid Command!", 22);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}

					/* PS1 */
					putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "MLGB:>", 6);
					cursor_x = 56;
				} 
				else {
					/* regular chars, append it */
					if(cursor_x < 240) {
						s[0] = i-256;
						s[1] = 0;
						/* buffer this char to cmdline, when enter is pressed, check buffered command */
						cmdline[cursor_x/8-7] = i-256;
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
						cursor_x += 8;
					}
				}
			}
			/* redisplay the cursor */
			if(cursor_c >= 0) {
				boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x+7, cursor_y + 15);
			}
			sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
		}
	}
}

