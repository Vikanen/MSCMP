; Unity player hooks for MSCMP.

extern SetupMSCMP: PROC
extern InitFuncReturnAddress: qword

extern SetupDebugger: PROC
extern GiveChanceToAttachDebuggerHookReturnAddress: qword
extern sub_1401736F0: qword

.code

InitHookJmp PROC
	; mod code

	mov	rbp, rsp
	sub	rsp, 100h
	and     rsp, 0FFFFFFFFFFFFFFF0h
	call    SetupMSCMP
	mov	rsp, rbp


	; call the original code

	mov     [rsp+8], rbx
	mov     [rsp+10h], rsi
	mov     [rsp+18h], rdi
	push    rbp

	jmp	InitFuncReturnAddress

InitHookJmp ENDP


GiveChanceToAttachDebuggerHookReturnJmp PROC
	; call our debug setup method

	call	SetupDebugger

	; original

	; skip first 'GiveChanceToAttachDebugger'
	; call    sub_1401F8FA0

	mov     rdx, rbx
	mov     rcx, r12
	call    sub_1401736F0

	jmp	GiveChanceToAttachDebuggerHookReturnAddress
GiveChanceToAttachDebuggerHookReturnJmp ENDP

End
