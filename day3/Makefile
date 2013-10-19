TOOLPATH	= ../z_tools/
NASK		= $(TOOLPATH)nask.exe
EDIMG		= $(TOOLPATH)edimg.exe
IMGTOL		= $(TOOLPATH)imgtol.com

all : img

ipl.bin : ipl.nas Makefile
	$(NASK) ipl.nas ipl.bin ipl.lst

haribote.sys : haribote.nas Makefile
	$(NASK) haribote.nas haribote.sys haribote.lst

# Note from now on apart from ipl.bin, we will also need haribote.sys file to build the img
# meaning the system image file is complete now, it has both ipl and system file	
# sys img will be 'concatenated' to ipl
haribote.img : ipl.bin haribote.sys Makefile
	$(EDIMG) imgin:../z_tools/fdimg0at.tek wbinimg src:ipl.bin len:512 from:0 to:0 copy from:haribote.sys to:@: imgout:haribote.img

img:
	make haribote.img

# create ipl.bin, then create a image file on top of that, then start emu
run :
	make img
	cp haribote.img ../z_tools/qemu/fdimage0.bin
	../z_tools/make.exe     -C ../z_tools/qemu

# wtf is this doing
install :
	make img
	IMGTOL w a: helloos.img

clean :
	rm ipl.bin ipl.lst haribote.sys haribote.lst

src_only :
	make clean
	rm haribote.img
