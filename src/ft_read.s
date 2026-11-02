global ft_read
extern __errno_location

section .text

ft_read:
	xor eax, eax 				; SYS_read
	syscall
	cmp rax, -4095 				; rax in [-4095, -1] means -errno
	jae .error
	ret
.error:
	neg eax 					; eax = errno
	push rax					; save it (aligns the stack)
	call __errno_location wrt ..plt
	pop rdx
	mov [rax], edx 				; errno = value
	mov rax, -1
	ret

section .note.GNU-stack noalloc noexec nowrite progbits
