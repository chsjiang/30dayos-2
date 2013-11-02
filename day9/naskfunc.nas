; This is a file to define a c functions that can only be achieved in assembly
; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; target file format
[INSTRSET "i486p"]				; this code is written for 486 and above processors, which is 32 bits and use EAX as extend accumulator register. Before 486 there's no EAX and it's used for a label, which will fool the compiler
[BITS 32]						; use 32 mode


; target file info

[FILE "naskfunc.nas"]			; src file name

		GLOBAL	_io_hlt, _write_mem8
								; function name to be implemented, needs to be GLOBAL, note _io_hlt is a assembly label
		GLOBAL	_io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8, _io_in16, _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
		GLOBAL	_load_cr0, _store_cr0
		EXTERN	_inthandler21, _inthandler27, _inthandler2c


; the actual function

[SECTION .text]		; obj file will first write this then write program

_io_hlt:	; void io_hlt(void);
		HLT
		RET
		
_write_mem8:		; void write_mem8(int addr, int data);
								; Note when used with c, only EAX, ECX and EDX can be used, other registers are storing programming specific values, like stack pointer
		MOV		ECX, [ESP+4]	; ESP+4 stores 'addr', int is represented by 4 bytes
		MOV		AL, [ESP+8]		; ESP+8 stores 'data'
		MOV		[ECX], AL		; move the data to the address assigned
		RET

_io_cli:	; void io_cli(void);
		io_cli ; clear IF (interruption flag), now interruption won't work
		RET
	
_io_sti:	; void io_sti(void);
		STI ; set IF (interruption flag), now interruption will be notified
		RET
		
_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET
		
_io_in8:	; int io_in8(int port);
		MOV		EDX, [ESP+4]
		MOV		EAX, 0
		IN		AL, DX
		RET ; RET will return value stored in EAX

_io_in16:	; int io_in16(int port);
		MOV		EDX, [ESP+4]
		MOV		EAX, 0
		IN		AX, DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX, [ESP+4]
		IN		EAX, DX
		RET	

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX, [ESP+4] ; port
		MOV		AL, [ESP+8]	; data
		OUT		DX, AL
		RET
		
_io_out16:	; void io_out16(int port, int data);
		MOV		EDX, [ESP+4] ; port
		MOV		EAX, [ESP+8]	; data
		OUT		DX, AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX, [ESP+4] 	; port
		MOV		EAX, [ESP+8]	; data
		OUT		DX, EAX
		RET	
		
_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; get the current FLAGS value, push it into stack
		POP		EAX ; pop the value from stack and assign to EAX
		RET			; RET will return value stored in EAX
		
_io_store_eflags:			; int io_store_eflags(it eflags);
		MOV		EAX, [ESP+4] ; eflags
		PUSH	EAX 		; push EAX to stack
		POPFD				; pop value from stack and assign to FLAGS
		RET

; note GDT is a 48 bit register, first 2 bytes is for limit, last 4 bytes is for starting address
; the function call passes limit=0x0000ffff and addr=00270000, we want GDT look like [FF FF 00 27 00 00]
; note value will be stored reversely in address, therefore when passing the param, DWORD [ESP+4] is [FF FF 00 00], DWORD [ESP+8] is [00 00 27 00] 
; so we copy the data from WORD [ESP+4] to WORD [ESP+6], then from ESP+4 it would be like [FF FF FF FF 00 27 00 00], then loading from [ESP + 6] we will get [FF FF 00 27 00 00]
_load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV 	AX, [ESP+4]	; limit
		MOV 	[ESP+6], AX
		LGDT 	[ESP+6]
		RET

_load_idtr:		; void load_gdtr(int limit, int addr);
		MOV 	AX, [ESP+4]	; limit
		MOV 	[ESP+6], AX
		LIDT 	[ESP+6]
		RET

; keyboard interruption
_asm_inthandler21:
		PUSH 	ES
		PUSH 	DS
		PUSHAD	; push all 8 general purpose registers into stack, buffer their value
				; Push EAX
				; Push ECX
				; Push EDX
				; Push EBX
				; Push ESP
				; Push EBP
				; Push ESI
				; Push EDI
		MOV 	EAX, ESP
		PUSH 	EAX
		; sync all Segment registers, DS, ES and SS
		MOV 	AX, SS
		MOV     DS, AX
		MOV 	ES, AX
		CALL 	_inthandler21
		POP 	EAX
		POPAD	; reversely pop all registers
		POP 	DS
		POP  	ES
		IRETD

; mouse interruption
_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_load_cr0:		; get the value of CR0 register, in order to disable cache we need to change value of CR0
		MOV 	EAX, CR0
		RET

_store_cr0:		; set CR0 value
		MOV 	EAX, [ESP + 4]
		MOV 	CR0, EAX
		RET
