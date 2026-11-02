global ft_strlen

section .text

; size_t ft_strlen(const char *s)
;   rdi = s
;   rax = len (not counting the NULL)
; rpne scasb: scans [rdi] for the byte in al (0), decrementing rcx
; each interation, until a match or rcx == 0.
; The direction flag is clear on function entry (SysV ABI), so rdi moves forward.
ft_strlen:
	xor	eax, eax 				; al = 0 (the byte to look for)
	mov	rcx, -1 				; max count = SIZE_MAX
	repne	scasb				; stops one byte past the NULL
	not rcx 					; rcx = -1 - n -> ~rcx = n = len + 1
	lea rax, [rcx - 1]			; len = n - 1
	ret

section .note.GNU-stack noalloc noexec nowrite progbits
