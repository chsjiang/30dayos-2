all : helloos.img

ipl.bin : ipl.nas Makefile
	../z_tools/nask.exe ipl.nas ipl.bin ipl.lst

helloos.img : ipl.bin Makefile
	../z_tools/edimg.exe imgin:../z_tools/fdimg0at.tek wbinimg src:ipl.bin len:512 from:0 to:0 imgout:helloos.img

# create the ipl.bin by using the assembly file(ipl.nas) we wrote
asm :
	make ipl.bin

# create ipl.bin, then create a image file on top of that, then start emu
run :
	make
	cp helloos.img ../z_tools/qemu/fdimage0.bin
	../z_tools/make.exe     -C ../z_tools/qemu

# wtf is this doing
install :
	make
	../z_tools/imgtol.com w a: helloos.img

clean:
	rm ipl.bin helloos.img ipl.lst 
