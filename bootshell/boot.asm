[ORG 0x7c00]
[BITS 16]

mov [MAIN_DISK], dl

; Codigo de arranque
mov bp, 0x1000
mov sp, bp

mov bx, STRING
call print_string

;Configurar la lectura del disco
mov dl, [MAIN_DISK]
mov ah, 0x02 ; Operacion de lectura
mov al, 0x01 ; Nº de sectores a leer
mov ch, 0x00 ; Cilindro
mov dh, 0x00 ; Cabezal
mov cl, 0x02 ; Sector
mov bx, 0x8000 ; Dirección a la que escribimos
int 0x13 ; Llamada a la bios

mov ax, handler_kbd
call install_keyboard

call second_stage

jmp $

;bx direccion de la string
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
    mov ah, 0x0E
    int 0x10
    pop ax
    ret

;ax = handler address
install_keyboard:
    push word 0
    pop ds
    cli
    ; Instalar el ISR del teclado
    mov [4 * KEYBOARD_INTERRUPT], word keyboardHandler
    mov [4 * KEYBOARD_INTERRUPT + 2], cs
    mov word [HANDLER], ax
    sti
    ret

handler_kbd:
    mov al, [bx]
    cmp al, 'h'
    je .hola
    cmp al, 'a'
    je .adios
    mov bx, INVALID
    call print_string
    ret
.hola:
    mov bx, STRING
    call print_string
    ret
.adios:
    mov bx, STRONG
    call print_string
    ret

;bx = direccion de la string, cl = tamanho string (max 64c)
keyboardHandler:
    pusha
    in al, 0x60
    test al, 0x80
    jnz .end
    mov bl, al
    xor bh, bh
    mov al, [cs:bx + keymap]
    cmp al, 13
    je .enter
    mov bl, [WORD_SIZE]
    mov [WRD+bx], al
    inc bx
    mov [WORD_SIZE], bl
    mov ah, 0x0e
    int 0x10
.end:
    mov al, 0x61
    out 0x20, al
    popa
    iret
.enter:
    mov bx, WRD
    mov cl, [WORD_SIZE]
    mov dx, [HANDLER]
    call dx
    mov byte [WORD_SIZE], 0
    jmp .end

WORD_SIZE: db 0
WRD: times 64 db 0
STRING: db "Hola Mundo!", 0
STRONG: db "Adios Mundo!", 0
INVALID: db "Comando invalido", 0
MAIN_DISK: db 0
KEYBOARD_INTERRUPT EQU 9
HANDLER: dw 0

keymap:
%include "keymap.inc"

times 510-($-$$) db 0
dw 0xaa55

second_stage:
    jmp $

times 1024-($-$$) db 0

;https://github.com/TretornESP