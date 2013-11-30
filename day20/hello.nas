[INSTRSET "i486p"]
[BITS 32]
	MOV ECX, msg
	MOV EDX, 1
putloop:
	MOV AL, [CS:ECX]
	CMP AL, 0
	JE 	fin
	; since we have registered asm_cons_putchar as interruption 0x40 in idt in dsctbl.c, 
	;	we can directly call it as an interruption here
	; after calling INT, ECX might change, therefore in _asm_cons_putchar, 
	; 	we need to buffer register values
	INT 0x40
	ADD ECX, 1
	JMP putloop
fin:
	; this application is started by a farcall in nasfuncnas, so we can return after it's done by RETF
	RETF
msg:
	DB "hello", 0
