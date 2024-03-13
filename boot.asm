[ORG 0x7c00]
[BITS 16]

mov bp, 0x1000
mov sp, bp

mov bx, STRING
call print_string

jmp $

print_string:
    pusha
    xor si, si
.loop:
    mov al, byte [bx+si]
    inc si
    cmp al, 0
    je .end
    call print_char
    jmp .loop
.end:
    popa
    ret

print_char:
    push ax
    mov ah, 0x0e
    int 0x10
    pop ax
    ret

STRING: db "Hola Mundo!", 0
times 510-($-$$) db 0
dw 0xaa55