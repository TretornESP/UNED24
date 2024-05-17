;Create a simple asm code that loops infinitely without doing nothing else.
;The code must be as small as possible.
[bits 64]
global _start
section .text
_start:
    mov rax, 1
    mov rdi, 1
    mov rsi, msg
    mov rdx, 15
    syscall
loop:
    jmp _start

section .data
msg:
    db "Hello, world!", 13, 10, 0