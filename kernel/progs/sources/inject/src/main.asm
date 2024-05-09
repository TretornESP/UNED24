;Create a simple asm code that loops infinitely without doing nothing else.
;The code must be as small as possible.
[bits 64]
global _start
section .text
_start:
    cli
    mov rax, 1
    mov rdi, 1
    mov rsi, msg
    mov rdx, 13
    syscall
    sti
loop:
    jmp loop

section .data
msg:
    db "Hello, world!", 10