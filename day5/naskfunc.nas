; This is a file to define a c function _io_hlt() that actually calls assembly HLT to avoid cpu spinning
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
		CLI
		RET
	
_io_sti:	; void io_sti(void);
		STI
		RET
		
_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET
		
_io_in8:	; int io_in8(int port);
		MOV		EDX, [ESP+4]
		MOV		EAX, 0
		IN		AL, DX
		RET

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
		MOV		EAX,[ESP+4] ; eflags
		PUSH	EAX 		; push EAX to stack
		POPFD				; pop value from stack and assign to FLAGS
		RET
		