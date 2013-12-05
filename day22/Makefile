TOOLPATH	= ../z_tools/
INCPATH  	= ../z_tools/haribote/

CC1      	= $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK 	= $(TOOLPATH)gas2nask.exe -a
OBJ2BIM 	= $(TOOLPATH)obj2bim.exe
RULEFILE 	= $(TOOLPATH)haribote/haribote.rul
BIM2HRB  	= $(TOOLPATH)bim2hrb.exe
NASK		= $(TOOLPATH)nask.exe
EDIMG		= $(TOOLPATH)edimg.exe
IMGTOL		= $(TOOLPATH)imgtol.com
MAKEFONT	= $(TOOLPATH)makefont.exe
BIN2OBJ		= $(TOOLPATH)bin2obj.exe

OBJPACK		= bootpack.obj int.obj graphic.obj dsctbl.obj naskfunc.obj hankaku.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj mtask.obj window.obj console.obj file.obj

# the idea is first compile asmhead.nas into a binary file asmhead.bin that is executable
# then compile bootpack.c into a hrb file bootpack.hrb, hrb is executable byte code, so we can simply cancatenate asmhead.bin and bootpack.hrb in to the .sys byte code, then we can use the fdimg0at.tek tool to genereate  a .img file
# bootpack.c -> bootpack.gas (gcc is based on gas assembly and out puts .gas by default) -> bootpack.nas -> bootpack.obj -> bootpack.bim -> bootpack.hrb
# note we will skip explaining asmhead.nas at the moment... it basicly adds the ability to compile C code
# .hrb is like .exe in windows


all : img

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

# bootpack.gas : bootpack.c Makefile
# 	$(CC1) -o bootpack.gas bootpack.c
# graphic.gas : graphic.c Makefile
# 	$(CC1) -o graphic.gas graphic.c
# dsctbl.gas : dsctbl.c Makefile
# 	$(CC1) -o dsctbl.gas dsctbl.c
# can be generalized
# note: when it's generailzed, the intermediate files won't be saved UNLESS it's explicitly used in other rules
# like the obj files are used in bootpack.bim, then they will be preserved,all .gas files will be removed
%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

# bootpack.nas : bootpack.gas Makefile
# 	$(GAS2NASK) -o bootpack.gas bootpack.nas
# graphic.nas : graphic.gas Makefile
# 	$(GAS2NASK) -o graphic.gas graphic.nas
# dsctbl.nas : dsctbl.gas Makefile
# 	$(GAS2NASK) -o dsctbl.gas dsctbl.nas
%.nas : %.gas Makefile
	$(GAS2NASK) -o $*.gas $*.nas	

# bootpack.obj : bootpack.nas Makefile
# 	$(NASK) bootpack.nas bootpack.obj bootpack.lst
# graphic.obj : graphic.nas Makefile
# 	$(NASK) graphic.nas graphic.obj graphic.lst
# dsctbl.obj : dsctbl.nas Makefile
# 	$(NASK) dsctbl.nas dsctbl.obj dsctbl.lst
# add an library to implement call HTL from c
# naskfunc.obj : naskfunc.nas Makefile
# 	$(NASK) naskfunc.nas naskfunc.obj naskfunc.lst
%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

######## applications that're written in assembly, need to use $(NASK) : app.nas -> app.hrb 
######## after reading header info of .hrb file and assign data segment accordingly, 
######## we can't do .nas->.hrb, should use create a .obj and use obj2bim, 
######## this will put correct header to the hrb file
######## i.e app.nas->app.obj->app.bim->app.hrb
hello.hrb : hello.nas Makefile
	$(NASK) hello.nas hello.hrb hello.lst

hello2.hrb : hello2.nas Makefile
	$(NASK) hello2.nas hello2.hrb hello2.lst

hello5.bim : hello5.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello5.bim stack:1k map:hello5.map hello5.obj

hello5.hrb : hello5.bim Makefile
	$(BIM2HRB) hello5.bim hello5.hrb 0
########

######## applications that're written in c, need to use $(OBJ2BIM) and $(BIM2HRB) : 
# 	     app.c -> app.gas -> app.nas -> app.obj -> app.bim -> app.hrb
# 		 note for .c files we can't use $(NASK) to do `app.nas -> app.hrb` directly
# since we defined a function in assembly(a_nask.nas) to be called in c(a.c)
# we need to get the obj files first then combine them into an hrb file
a.bim : a.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:a.bim map:a.map a.obj a_nask.obj

a.hrb : a.bim Makefile
	$(BIM2HRB) a.bim a.hrb 0 

bug1.bim : bug1.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug1.bim map:bug1.map bug1.obj a_nask.obj

bug1.hrb : bug1.bim Makefile
	$(BIM2HRB) bug1.bim bug1.hrb 0 

bug2.bim : bug2.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug2.bim map:bug2.map bug2.obj a_nask.obj

bug2.hrb : bug2.bim Makefile
	$(BIM2HRB) bug2.bim bug2.hrb 0 

bug3.bim : bug3.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug3.bim map:bug3.map bug3.obj a_nask.obj

bug3.hrb : bug3.bim Makefile
	$(BIM2HRB) bug3.bim bug3.hrb 0 

# hello3 is similar
hello3.bim : hello3.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello3.bim map:hello3.map hello3.obj a_nask.obj

hello3.hrb : hello3.bim Makefile
	$(BIM2HRB) hello3.bim hello3.hrb 0 

hello4.bim : hello4.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello4.bim map:hello4.map hello4.obj a_nask.obj

hello4.hrb : hello4.bim Makefile
	$(BIM2HRB) hello4.bim hello4.hrb 0 

winhelo.bim : winhelo.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo.bim map:winhelo.map winhelo.obj a_nask.obj

winhelo.hrb : winhelo.bim Makefile
	$(BIM2HRB) winhelo.bim winhelo.hrb 0 

winhelo2.bim : winhelo2.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo2.bim map:winhelo2.map winhelo2.obj a_nask.obj

winhelo2.hrb : winhelo2.bim Makefile
	$(BIM2HRB) winhelo2.bim winhelo2.hrb 0 
########

ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

bootpack.bim : $(OBJPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map $(OBJPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0


haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

# hankaku.bin is created by makefont.exe, which reads the hankaku.txt and generate a 16*256 = 4096 bytes file.
# hankaku.txt contains 256 characters, each character will be represented by 16 bytes ( a 16*8 matrix ) after generated to a bin file
hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

# like the following assembly:
#	_hankaku:
#		DB 0x00 ... (all 4096 bytes)
#
hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku


# Note from now on apart from ipl10.bin, we will also need haribote.sys file to build the img
# meaning the system image file is complete now, it has both ipl10 and system file	
# sys img will be 'concatenated' to ipl10
haribote.img : ipl10.bin haribote.sys hello.hrb hello2.hrb a.hrb bug1.hrb bug2.hrb bug3.hrb \
				hello3.hrb hello4.hrb hello5.hrb winhelo.hrb winhelo2.hrb Makefile
	$(EDIMG) imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		copy from:ipl10.nas to:@: \
		copy from:Makefile to:@: \
		copy from:hello.hrb to:@: \
		copy from:hello2.hrb to:@: \
		copy from:a.hrb to:@: \
		copy from:bug1.hrb to:@: \
		copy from:bug2.hrb to:@: \
		copy from:bug3.hrb to:@: \
		copy from:hello3.hrb to:@: \
		copy from:hello4.hrb to:@: \
		copy from:hello5.hrb to:@: \
		copy from:winhelo.hrb to:@: \
		copy from:winhelo2.hrb to:@: \
		imgout:haribote.img

img:
	make haribote.img

# create ipl10.bin, then create a image file on top of that, then start emu
run :
	make img
	cp haribote.img ../z_tools/qemu/fdimage0.bin
	../z_tools/make.exe     -C ../z_tools/qemu

	

# wtf is this doing
install :
	make img
	IMGTOL w a: helloos.img

clean :
	rm *.bin *.lst *.obj *.hrb *.map *.bim bootpack.hrb haribote.sys

src_only :
	make clean
	rm haribote.img

crun :
	make clean run
