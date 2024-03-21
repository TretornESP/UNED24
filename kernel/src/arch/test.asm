[bits 64]

global set_to_val

set_to_val:
    ; Arguments:
    ; rdi -> struct data* source
    ; rsi -> struct data* destination
    
    ; Copy the values of 'a' and 'b'
    mov eax, [rdi]      ; Load source->a
    mov [rsi], eax      ; Store it in destination->a
    mov eax, [rdi + 4]  ; Load source->b
    mov [rsi + 4], eax  ; Store it in destination->b
    
    ret