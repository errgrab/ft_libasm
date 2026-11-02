global ft_strdup
extern ft_strlen
extern malloc

section .text

ft_strdup:
	sub rsp, 24 				; locals + 16-byte alignment
	mov [rsp], rdi 				; save s
	call ft_strlen wrt ..plt
	inc rax 					; + NULL
	mov [rsp + 8], rax 			; save size
	mov rdi, rax
	call malloc wrt ..plt
	test rax, rax
	jz .done 					; NULL, errno already set by malloc
	mov rdi, rax 				; dst
	mov rdx, rax 				; keep the return value
	mov rsi, [rsp] 				; src
	mov rcx, [rsp + 8] 			; len + 1
	rep movsb
	mov rax, rdx
.done:
	add rsp, 24
	ret

section .note.GNU-stack noalloc noexec nowrite progbits
