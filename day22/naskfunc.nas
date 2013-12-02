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
		GLOBAL	_asm_inthandler20, _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
		GLOBAL  _asm_inthandler0c, _asm_inthandler0d
		GLOBAL	_load_cr0, _store_cr0
		GLOBAL 	_memtest_sub, _asm_end_app
		GLOBAL 	_load_tr, _taskswitch3, _taskswitchi4, _farjmp, _farcall
		GLOBAL  _asm_hrb_api, _start_app
		EXTERN	_inthandler20, _inthandler21, _inthandler27, _inthandler2c, _inthandler0c, _inthandler0d
		EXTERN  _hrb_api


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

; timer interruption
; the reason we don't restore system ESP here is because now applciation has 0x60 set
; the stack backup/restore will be done by cpu
_asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

; keyboard interruption, similar to _inthandler20
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

; mouse interruption similar to _inthandler20
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

_asm_inthandler2c: ; similar to _inthandler20
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

_asm_inthandler0c:  ; 0c interruption happens when an stack exception occurs
					; this specified by x86 archetyecture
				   	; we need to terminate the application that causes the exception and print some error info
				   	; then return to console.c::cmd_app()
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0c
		CMP		EAX,0				; if EAX != 0, it would be the address of task to return
		JNE		_asm_end_app		; _asm_end_app is defined in _asm_hrb_api
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; required in INT 0x0d

_asm_inthandler0d: 	; 0d interruption happens when an general exception occurs
				   	; this specified by x86 archetyecture
				   	; we need to terminate the application that causes the exception and print some error info
				   	; then return to console.c::cmd_app()
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0d
		CMP		EAX,0				; if EAX != 0, it would be the address of task to return
		JNE		_asm_end_app		; _asm_end_app is defined in _asm_hrb_api
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			; required in INT 0x0d
		IRETD

_load_cr0:		; get the value of CR0 register, in order to disable cache we need to change value of CR0
		MOV 	EAX, CR0
		RET

_store_cr0:		; set CR0 value
		MOV 	EAX, [ESP + 4]
		MOV 	CR0, EAX
		RET


_memtest_sub:	;  unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH 	EDI						; buffer these values
		PUSH 	ESI
		PUSH 	EBX
		MOV 	ESI, 0xaa55aa55			; pat0 = 0xaa55aa55
		MOV 	EDI, 0x55aa55aa			; pat1 = 0x55aa55aa
		MOV 	EAX, [ESP+12+4] 		; i = start
mts_loop:
		MOV 	EBX, EAX
		ADD 	EBX, 0xffc 				; p = i + 0xffc
		MOV 	EDX, [EBX] 				; old = *p
		MOV 	[EBX], ESI				; *p = pat0
		XOR 	DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff
		CMP 	EDI, [EBX]
		JNE 	mts_fin					; if (*p != pat1) goto mts_fin
		XOR 	DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff
		CMP 	ESI, [EBX] 				;
		JNE 	mts_fin					; if (*p != pat0) goto mts_fin
		MOV 	[EBX], EDX				; *p = old
		ADD 	EAX, 0x1000 			; i += 0x1000
		CMP 	EAX, [ESP+12+8]
		JBE		mts_loop
		POP 	EBX
		POP 	ESI
		POP 	EDI
		RET 							; return end
mts_fin:
		MOV 	[EBX], EDX 				; *p = old
		POP 	EBX
		POP 	ESI
		POP 	EDI
		RET 							; return before end

_load_tr:								; void load_tr(int tr)
		LTR 	[ESP+4]
		RET

_taskswitch3:
		JMP		3*8:0
		RET

_taskswitch4:							; void taskswitch4(void);
		JMP 	4*8:0
		RET
; JMP FAR will first read 4 bytes from the address given to write to eip
; then continues read 2 bytes to write to cs, which we passed as [ESP+8]
_farjmp:								; farjmp(int eip, int cs);
		JMP 	FAR [ESP+4] 			; eip, cs
		RET

; farcall will also change eip and cs, then execute from there, apart from that, it will also push esp before farcall
; 	so that after _farcall is done, it would be able to return
_farcall:								; void farcall(int eip, int cs);
		CALL 	FAR [ESP+4]				; eip, cs
		RET


; this function relays system interruption to console.c::hrb_api()
;  by pushing all register values to the method
; when an application calls INT 0x40, it also needs to push parameters of the system
;	 call it's going to call to certain registers, then hrb_api() will take these value 
; 	 and call the corresponding system functions
;
; when this function is called, all params are passed as in the application segment
;  meaning it might be accessing OS segment 
; but since we have add 0x60 to the app in cmd_app(), an exception will be thrown if 
;  the app tries to access OS code, the application will be terminated in exception handling
;
; the sequence it's called:
; start_app() -> app.c -> calls system api -> _asm_hrb_api(switched to system segment)
; 			  -> console.c::hrb_api() (uses system segment) -> _asm_hrb_api(reset to app segment)
; 			  -> app.c
_asm_hrb_api:
		STI 
		PUSH 	DS
		PUSH 	ES
		PUSHAD 					; stores all register values, will be passed to system ESP
		PUSHAD 					; pass register values to _hrb_api
		MOV 	AX, SS 			; store OS segment into DS and ES
		MOV 	DS, AX
		MOV 	ES, AX
		CALL 	_hrb_api 		; system ESP has get the correct params, good to call _hrb_api now
		CMP 	EAX, 0 			; EAX is the return value of _hrb_api,
								; if it's not 0, then it would be the address of cmd_app() to return to
		JNE 	_asm_end_app
		ADD 	ESP, 32
		POPAD 
		POP 	ES
		POP 	DS
		IRETD 					; will execute STI AUTOMATICALLY

_asm_end_app:
		MOV 	ESP, [EAX]
		MOV 	DWORD [EAX+4], 0 ; ESP + 4 would point to TSS.ss0(first two are 32 bits int)
								; making ss0 = 0 will help to distinguish if there's app running in console
								; in bootpack.c when handing shift+f1
		POPAD
		RET 					; back to cmd_app

_start_app:				  ; void start_app(int eip, int cs, int esp, int ds);
		PUSHAD
		; after PUSHAD, ESP will decrease by 32, 
		; in order to get the params passed, we need to offset the 32
		; i.e [ESP + 4 + 32]
		MOV 	EAX, [ESP+36]	; application EIP
		MOV 	ECX, [ESP+40]	; application CS
		MOV 	EDX, [ESP+44]	; application ESP
		MOV 	EBX, [ESP+48]	; application DS/SS
		MOV 	EBP, [ESP+52]	; address of tss.esp0
		MOV 	[EBP], ESP 		; buffer os ESP, this is the address to return to cmd_app()
		MOV 	[EBP+4], SS 	; OS SS

		MOV 	ES, BX
		MOV 	DS, BX			; app DS, note DS and SS are equal here, 
								; it's the 64kb memory we assigned to the app
		MOV 	FS, BX
		MOV 	GS, BX
		; adjust stack so that we can jump to the application using RETF
		OR 		ECX, 3
		OR 		EBX, 3
		PUSH 	EBX				; app SS
		PUSH 	EDX				; app ESP
		PUSH 	ECX				; app CS
		PUSH 	EAX				; app EIP
		RETF					; NOTE RETF just pop out an address and jump to there
								; since we pushed app EIP, we are actually jump to app here
								; the side effect here, is the application we wrote can't use RETF to return
								; need to call a system interruption #4 to return
