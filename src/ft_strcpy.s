global ft_strcpy

section .text

; char *ft_strcpy(char *dst, const char *src)
;   rdi = dst, rsi = src
;   rax = dst
ft_strcpy:
	mov rdx, rdi				; keep dst
	mov rdi, rsi				; scan src to find it's len
	xor eax, eax
	mov rcx, -1
	repne scasb 				; rcx = -(len + 2)
	not rcx 					; rcx = len + 1
	mov rdi, rdx				; rdi = dst, rsi = src
	mov rax, rdx				; return value
	rep movsb
	ret

section .note.GNU-stack noalloc noexec nowrite progbits
