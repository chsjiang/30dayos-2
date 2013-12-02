[INSTRSET "i486p"]
[BITS 32]
	MOV 	EAX, 1*8 ; access OS segment by directly modifying DS to point to OS data segment
	MOV 	DS, AX
	MOV 	BYTE [0x102600], 0
	MOV 	EDX, 4
	INT 	0x40
