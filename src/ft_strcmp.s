global ft_strcmp

section .text

; int ft_strcmp(const char *s1, const char *s2)
;   rdi = s1, rsi = s2
;   rax = result
ft_strcmp:
.loop:
	movzx eax, byte [rdi]
	movzx ecx, byte [rsi]
	cmp eax, ecx
	jne .diff
	test eax, eax 				; equal means that strings are identical
	je .diff
	inc rdi
	inc rsi
	jmp .loop
.diff:
	sub eax, ecx
	ret

section .note.GNU-stack noalloc noexec nowrite progbits
