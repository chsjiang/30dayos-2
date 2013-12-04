[FORMAT "WCOFF"] 	; use object mode
[INSTRSET "i486p"] 	; 486 compatible
[BITS 32]	 		; 32 bit mode machine code
[FILE "a_nask.nas"] ; source file name

	GLOBAL 	_api_putchar, _api_end, _api_putstr0
; a_nask provides system api to applications
[SECTION .text]
				; wrap up the system calls to provide a c-friendly function
_api_putchar: 	; void api_putchar(int c)
	MOV 	EDX, 1  	; 1st interruption
	MOV 	AL, [ESP+4] ; c
	INT 	0x40 		; int to call system call
	RET

_api_end: 		; void api_end(void)
	MOV 	EDX, 4
	INT 	0x40

_api_putstr0: 	; 	void api_putstr0(char *s);
	PUSH 	EBX 		; buffer old ebx, this function will use EBX to pass param
	MOV 	EDX, 2 		; 2nd interruption
	MOV 	EBX, [ESP+8]; 2nd interruption will use ebx as the address of str
						; ESP+8 because we have PUSHed something, s
	INT 	0x40
	POP 	EBX
	RET
