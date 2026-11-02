global ft_write
extern __errno_location

section .text

; ssize_t ft_write(int ft, const void *buf, size_t count)
; rdi, rsi, rdx syscall argument registers
ft_write:
	mov eax, 1 					; SYS_write
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
