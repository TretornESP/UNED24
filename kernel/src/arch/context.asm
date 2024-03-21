[bits 64]
global ctxswtch

; Inputs:
; RDI: struct task* old
; RSI: struct task* new

ctxswtch:
    cli

    ; Push registers rbx, r12, r13, r14, r15
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save stack pointer in old's rsp_top
    mov qword [rdi + 8], rsp
    
    ; Load new's pd into CR3
    mov rax, [rsi + 16]
    mov cr3, rax

    ; Load rsp with new's rsp_top
    mov rsp, qword [rsi + 8]

    ; Pop registers r15, r14, r13, r12, rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    sti
    ret
