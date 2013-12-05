; haribote-ipl
; TAB=4
; it's called ipl10 because it's an ipl that only loads 10 cylinders
; define a variable CYLS=10 (cylinders equals to 10)
CYLS	EQU		10

		ORG		0x7c00			; load initial address, it will tell compiler to start using memory address starting from 0x7c00
								; this program can't exceed 512 bytes, if it's smaller than 512 bytes, we need to add `RESB	0x7dfe-$` at end of program to overwrite the tail

; mark standard FAT12 format flopy

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
		MOV		AX,0			; initialize registers, when MOV, the source is not cleared, therefore we are actually setting all registers to 0
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX			; Note: the reason we set DS=0 is because when `MOV CX,[1234]` is actually `MOVE CX, [DS:1234]` meaning loading the data in address DS*16+0x1234, need to clear DS in advance

; added new stuff here:

		MOV		AX,0x0820		; initialize register values, the second sector starts to lead from 0x0820
		MOV		ES,AX			; we're using ES to wider address ES:BX is 16*EX+BX, can be used to represent 1M memory
; note: CH, DH, CL, AH, AL BX and DL are all parameters to INT 0x13, it tells CPU to read data from where
		MOV		CH,0			; cylinder is 0
		MOV		DH,0			; head is 0, 0 is face, 1 is tail
		MOV		CL,2			; sector is 2
								; why sector is 2? because for the first sector of cylinder 0 (C0,H0,S1) is used to store this IPL(512 bytes)
								; the 'raw data' to be copied from flopy will start from the second sector(C0, H0, S2)
; we want to add a loop here to try 5 times before we give up
readloop:
		MOV		SI,0			; use src index to count failure times
retry:
		MOV		AH,0x02			; AH=0x02 : read disk
		MOV		AL,1			; we want to read 1 sector
		MOV		BX,0			; now we have [ES:BX] == [0x0820:0] == 0x0820*16+0=0x8200: the data in disk will be stored to 0x8200. Note 0x8000-0x81ff these 512 bytes are reserved for IPL
		MOV		DL,0x00			; A driver to read flopy
		INT		0x13			; invoke disk BIOS, note when this particular disk interruption is invoked, CPU will check value at certain registers. for 0x13 invoke, if AH=0x02, then it means 'read disk'
								; what does read disk do? It read 512 bytes(1 sector AL) starting from C0-H0-S2(set by CH, DH, CL) to memory from address [ES:BX]
								; we are having a loop to move CL(read from) and [ES:BX](write to) to read 17 sectors into memory here, 
								; another way is not having loop and setting AL=17, the program will load 17 sectors for one time and store to initial memory
		JNC		next			; if carry is not set then we are correct, should go to next and trying read more data
		ADD		SI,1			; otherwise add SI
		CMP		SI,5			; 
		JAE		error			; if SI > 5 then we jump to error, JAE jump if above or equal
		MOV		AH,0x00
		MOV 	DL,0x00			; otherwise we reset driver and jmp to retry
		INT 	0x13			; note AH, DL set as value to INT 0x13 is to 'reset system'
		JMP		retry			; 

next:
		MOV		AX,ES
		ADD		AX,0x20
		MOV		ES,AX			; we want to continue reading data starting from 0x200 (0x20 * 16 = 0x200) bytes after what we left, note can't directly add to ES, so we are moving to AX here
								; note 0x200 is 512, we can also move 512 by doing `ADD BX,0x200` since the address is [ES:BX]
		ADD		CL,1			; we are reading from sector 2 to sector 18
								; note after sector 18 we need to read the reverse face of the flopy
		CMP		CL,18			; we have read the outmost circle(cylinder 0) of head 1
		JBE		readloop
		MOV		CL,1			; need to read from reverse, reset cylinder to 1
		ADD		DH,1			; need to use the reverse head
		CMP		DH,2			;
		JB		readloop		; a weird way to judge if we need to stop: if the head is blow 2, then we still need to continue reading, 
								; if we reach here then we have read the 18 sectors of both head and tail of a cylinder
		MOV		DH,0			; then we continue using face header
		ADD		CH,1			; move the to next cylinder
		CMP		CH,CYLS			
		JB		readloop		; if we are still below cylinder, we will continue to read 18 sectors from face/tail of this cylinder
								; if we reach here, then we have read all data of first 10 cylinders, 10 * 2 * 18 * 512 = 180k
								;
		MOV		[0x0ff0],CH		; need to buffer the # of cylinders read by this ipl to disk, note we have only read 10 cylinders
		JMP		0xc200			; 0x4200+0x8000=0xc200, this is the address that actually stores the program in haribote.nas
		
		
		
; end

fin:
		HLT						; halt CPU, if we don't halt JMP fin will still do the infinite loop, but will make CPU useage at 100%, doing halt can save some power
		JMP		fin				; infinite loop

error:
		MOV		SI,msg			; moving the message address to SI
putloop:
		MOV		AL,[SI]			; get the data stored at address [DX:SI]
		ADD		SI,1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; display
		MOV		BX,15			; color
		INT		0x10			; graphics card interruption, using SI, AL as param, will search the message starting from SI and end with 0
		JMP		putloop
msg:
		DB		0x0a, 0x0a		; line break two times
		DB		"load error"
		DB		0x0a			; line break
		DB		0

		RESB	0x7dfe-$		; set 0x00 till 0x7dfe
								; the reason we set to 0x7dfe is 0x7c00-0x7dff is the address to store ipl, we need to make sure the first 512 bytes are written by the program
								; check ipl.lst to see the compiled program, should be just 0x200 * 16 = 512 bytes
		DB		0x55, 0xaa
