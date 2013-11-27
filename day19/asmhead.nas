; haribote-os boot asm
; TAB=4

[INSTRSET "i486p"]

VBEMODE EQU 	0x103			; 0x101: 640 x 480 x 8bit  mode
								; 0x103: 800 x 600 x 8bit  mode
								; 0x105: 1024 x 768 x 8bit  mode
								; 0x107: 1280 x 1024 x 8bit mode
								; 		 not supported, will be back to 320 x 200 mode

BOTPAK	EQU		0x00280000		; bootpack will be copied to this address
DSKCAC	EQU		0x00100000		; 1MB
DSKCAC0	EQU		0x00008000		; used to offset the address of second sector, 0x8200

; BOOT_INFO
CYLS	EQU		0x0ff0
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 臣its of color
SCRNX	EQU		0x0ff4			; screen X
SCRNY	EQU		0x0ff6			; screen Y
VRAM	EQU		0x0ff8			; buffer area's starting address, video RAM


		ORG		0xc200			; from haribote.img byte code, we know the sys is being added at address 0x4200 of flopy
								; since we have copied our flopy data to disk starting from 0x8000, we know the address of the sys code to be loaded on disk is 0x8000+0x4200=0xc200

; set graphics mode
; VESA: Video Electronics Standards Association, a standard to configure graphics card
; VBE: VESA BIOS Extension
; fist need to check if VBE exists
; if VBE doesn't exist, after calling INT 0x10, AX won't change to 0x004f, then we jump to 320 mode
		MOV 	AX, 0x9000
		MOV 	ES, AX
		MOV 	DI, 0
		MOV 	AX, 0x4f00
		INT 	0x10
		CMP 	AX, 0x004f
		JNE 	scrn320

; if VBE exists, we need to check its version
; version number stores in ES:DI+4, needs to be over 2.0
		MOV 	AX, [ES:DI+4]
		CMP 	AX, 0x0200
		JB 		scrn320

; get VBE mode info
		MOV 	CX, VBEMODE
		MOV 	AX, 0x4f01
		INT 	0x10
		CMP 	AX, 0x004f
		; if AX isn't 0x004f then we need to use 320 mode
		JNE 	scrn320

; check VBE mode
		; number of color needs to be 8
		CMP 	BYTE [ES:DI+0x19], 8
		JNE 	scrn320
		; needs to be pallette mode(4)
		CMP 	BYTE [ES:DI+0x1b], 4
		JNE 	scrn320
		; mode property, bit7 should be 1, otherwise we cna't add 0x4000
		MOV 	AX, [ES:DI+0x00]
		AND 	AX, 0x0080
		JZ 		scrn320

; if all above check pass then we can use higher resolution
		MOV 	BX, VBEMODE+0x4000
		MOV 	AX, 0x4f02
		INT		0x10	; An interruption to set VGA graphics card to 320*200*8 mode
		MOV		BYTE [VMODE],8
		MOV 	AX, [ES:DI+0x12]
		MOV		[SCRNX], AX
		MOV 	AX, [ES:DI+0x14]
		MOV 	[SCRNY], AX
		MOV 	EAX, [ES:DI+0x28]
		MOV 	[VRAM], EAX
		jmp 	keystatus

; use raw 320*200 mode
scrn320:
		MOV		AL,0x13
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; An interruption to set VGA graphics card to 320*200*8 mode
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000 	

; use BIOS to get status of leds on keyboard
keystatus:
		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC disable all interruptions
; PIC initialization need to be before CLI, otherwise might suspend

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; can't do OUT consequtives, let CPU stop for a timeclock unit
		OUT		0xa1,AL

		CLI						; disable CPU interruptions

; set A20GATE to enable CPU access over 1MB space

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; switch to protection mode
; in protection mode, application is not able to change get segment or use OS specific segment
[INSTRSET "i486p"]				; need to use 486 instruction set, can use LGDT, CR0 thereafter

		LGDT	[GDTR0]			; temporary GDT
		MOV		EAX,CR0			; control register, only availabe by os
		AND		EAX,0x7fffffff	; set bit31 to 0
		OR		EAX,0x00000001	; set bit0 to 1, to enable protection mode
		MOV		CR0,EAX
		JMP		pipelineflush	; after switching to protection mode, CPU will pipe instructions
pipelineflush:
		MOV		AX,1*8			;  読み書き可能セグメント32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; move bootpack
; bootpack.hrb is concatenated after asmhead, therefore we have a tag bootpack at end of asmhead.nas
; 512KB is way bigger than bootpack, but it's ok
; copy from bootpack to 0x00280000, copy 512KB
		MOV		ESI,bootpack	; src
		MOV		EDI,BOTPAK		; dest
		MOV		ECX,512*1024/4  ; size, note the reason we divide 4 is because memcpy copies a DWORD
		CALL	memcpy

; disk data will be in place

; starting from boot sector
; DSKCAC is 0x00100000, this mempy copies 512 bytes from 0x7c00 to 0x00100000 on disk, IPL(first sector, 512 bytes) starts from 0x7c00
		MOV		ESI,0x7c00		; src
		MOV		EDI,DSKCAC		; dest
		MOV		ECX,512/4       ; size
		CALL	memcpy

; rest
; coplies from 0x00008200 to 0x00100200, the second sector starts from 0x8200
		MOV		ESI,DSKCAC0+512	; src
		MOV		EDI,DSKCAC+512	; dest
		MOV		ECX,0           ; size
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; convert from number of cylinders to number of bytes/4
		SUB		ECX,512/4		; minus IPL
		CALL	memcpy

; the previous part needs to be handled in asmhead
; the reset can be handled by bootpack

; bootpack initialization

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; nothing to move
		MOV		ESI,[EBX+20]	; src
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; dest
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; initial stack value
		; 0x1b of the second segment, it's the 0x1b address of bootpack.hrb, jumping to there will start execute bootpack.hrb
		JMP		DWORD 2*8:0x0000001b

; like wait_KBC_sendready
waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; keep listening to the 'already sent' signal
		RET

; memcpy copies from [ESI] to [EDI] with size ECX, note ECX is in DWORD, therefore size would be bytes/4
memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy
		RET

		; padding DB 0 untill the address can be divided by 16, can be more effective
		ALIGNB	16

GDT0:
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; rw segment 32 bit
		DW		0xffff,0x0000,0x9a28,0x0047	; exe segment 32 bit (used by bootpack)

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16

; concatenated after this
bootpack:
