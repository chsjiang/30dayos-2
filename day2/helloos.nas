; This is a assembly program to create a simple os image file that boot a system and pint hello world
; hello-os
; TAB=4

		ORG		0x7c00			; load initial address

; mark standard FAT12 format floopy

		JMP		entry			; goto tag entry, actually we can write as JMP 0x7c50 since that's the address of entry, note this line occupies two bytes!
		DB		0x90			; write 0x90, one byte
		DB		"HELLOIPL"		; IPL: initial program loader, can be arbitrary string, 8 bytes
		DW		512				; each sector is 512 bytes
		DB		1				; each cluster is one sector
		DW		1				; Starting point of FAT, needs to be from the first sector
		DB		2				; # of FATS, needs to be 2
		DW		224				; size of root dir, usually 224
		DW		2880			; size of the disk ,needs to be 2880
		DB		0xf0			; type of disc, needs to be 0xf0
		DW		9				; length of FAT, needs to be 9 sectors
		DW		18				; # of sectors in a track, needs to be 18
		DW		2				; # of magnetic heads, needs to be 2
		DD		0				; don't use partition, needs to be 0
		DD		2880			; size when rewriting disc
		DB		0,0,0x29		; wtf
		DD		0xffffffff		; might be name 
		DB		"HELLO-OS   "	; name of disk(11 bytes)
		DB		"FAT12   "		; format of disk(8 bytes)
		RESB	18				; reserved 18 bytes



entry:
		MOV		AX,0			; initialize registers, when 'moving' the source are not cleared, therefore we are actually setting all registers to 0
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX
		MOV		ES,AX

		MOV		SI,msg			; SI = msg, copy the address of msg to SI
putloop:
		MOV		AL,[SI]			; short for MOV AL, BYTE[SI], because the value stored at [SI] can only be 8 bits, so we can skip the BYTE
		ADD		SI,1			; SI = SI+1, we are adding address value, meaning SI now points to "hello, world"
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; display a text, this can be searched online, when certain register has certain value, and call INT 0x10, we will display the text and colour to screen
		MOV		BX,15			; color
		INT		0x10			; call graphics card BIOS
		JMP		putloop
fin:
		HLT						; halt CPU, if we don't halt JMP fin will still do the infinite loop, but will make CPU useage at 100%, doing halt can save some power
		JMP		fin				; infinite loop

msg:
		DB		0x0a, 0x0a		; two line breaks
		DB		"hello, world"
		DB		0x0a			; line break
		DB		0

		RESB	0x7dfe-$		; set 0x00 till 0x7dfe

		DB		0x55, 0xaa

; outside booting area

		DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		RESB	4600
		DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		RESB	1469432
