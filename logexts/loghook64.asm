LogProcessHook PROTO r_rbp:QWORD, retAddr:QWORD, index:DWORD

.code

_LogHook proc
	var_28= qword ptr -28h
	var_20= qword ptr -20h
	var_18= qword ptr -18h
	var_10= qword ptr -10h
	arg_0= qword ptr  8
	arg_8= qword ptr  10h
	arg_10= qword ptr  18h
	arg_18= qword ptr  20h

	mov     [rsp+arg_0], rcx
	mov     [rsp+arg_8], rdx
	mov     [rsp+arg_10], r8
	mov     [rsp+arg_18], r9
	sub     rsp, 48h
	movq    [rsp+48h+var_28], xmm0
	movq    [rsp+48h+var_20], xmm1
	movq    [rsp+48h+var_18], xmm2
	movq    [rsp+48h+var_10], xmm3
	mov     r8, r11
	mov     rdx, [rsp+48h]
	mov     rcx, rsp
	call    LogProcessHook
	add     rsp, 48h
	ret
_LogHook endp

_LogHookCallFunction proc ;r_rbp:QWORD , real_func:QWORD, p_num_of_param:QWORD

	var_s0= qword ptr  0
	var_s8= qword ptr  8
	var_s10= qword ptr  10h
	var_s18= qword ptr  18h
	var_s20= qword ptr  20h
	var_s28= qword ptr  28h
	var_s30= qword ptr  30h

	sub     rsp, 38h
	mov     [rsp+var_s20], rbp
	mov     [rsp+var_s28], rsi
	mov     [rsp+var_s30], rdi
	mov     rbp, rsp
	mov     rax, [r8]
	mov     r11, rcx
	mov     r9, rax
	add     rax, 10h
	inc     rax
	and     rax, 0FFFFFFFFFFFFFFFEh
	shl     rax, 3
	sub     rsp, rax
	lea     rsi, [r11+50h]
	mov     rdi, rsp
	mov     rcx, r9
	add     rcx, 10h
	rep movsq
	mov     rcx, [r11+50h]
	movq    xmm0, qword ptr [r11+20h]
	mov     r10, [r11+58h]
	movq    xmm1, qword ptr [r11+28h]
	mov     r8, [r11+60h]
	movq    xmm2, qword ptr [r11+30h]
	xchg    rdx, r10
	mov     r9, [r11+68h]
	movq    xmm3, qword ptr [r11+38h]
	mov     [rsp+var_s0], rcx
	mov     [rsp+var_s8], rdx
	mov     [rsp+var_s10], r8
	mov     [rsp+var_s18], r9
	call    r10
	mov     rsi, [rbp+var_s28]
	mov     rdi, [rbp+var_s30]
	mov     rsp, rbp
	mov     rbp, [rbp+var_s20]
	add     rsp, 38h
	ret
_LogHookCallFunction endp

END