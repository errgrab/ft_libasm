global ft_calloc
extern malloc
extern __errno_location

section .text

ft_calloc:
	sub rsp, 24 				; locals + 16-byte alignment
	mov rax, rdi
	mul rsi 					; rdx:rax = nmemb * size, CF = OF = (rdx != 0)
	jc .overflow
	mov [rsp], rax 				; save total size
	mov rdi, rax
	call malloc wrt ..plt
	test rax, rax
	jz .done 					; NULL, errno already set by malloc
	mov rdi, rax 				; zero the block
	mov rdx, rax 				; keep the return value
	mov rcx, [rsp]
	xor eax, eax
	rep stosb
	mov rax, rdx
.done:
	add rsp, 24
	ret
.overflow:
	call __errno_location wrt ..plt
	mov dword [rax], 12
	xor eax, eax
	jmp .done

section .note.GNU-stack noalloc noexec nowrite progbits
