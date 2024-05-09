[bits 64]
global ctxswtch
global ctxcreat
global uctxcreat
extern returnoexit

; Inputs:
; RDI: struct task* old
; RSI: struct task* new
; RDX: void* fxsave_area
; RCX: void* fxrstor_area

%macro set_int 0
    sti
%endmacro

%macro clear_int 0
    cli
%endmacro

ctxswtch:
    clear_int
    ; Push registers rbx, r12, r13, r14, r15
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rsi
    push rdi
    mov rax, cs
    push rax
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax
    mov rax, ss
    push rax
    pushfq

    ; Save stack pointer in old's rsp_top
    mov qword [rdi + 8], rsp

    ;  save old fpu state
    fxsave [rdx]

    ; Save old's pd
    mov rax, cr3
    mov [rdi + 16], rax

    ; Load new's pd into CR3
    mov rax, [rsi + 16]
    mov cr3, rax

    ; Load new's fpu state into fpu 
    fxrstor [rcx]

    ; Load rsp with new's rsp_top
    mov rsp, qword [rsi + 8]

    ; Pop registers r15, r14, r13, r12, rbx
    popfq
    pop rax
    mov rax, ss
    pop rax
    mov rax, gs
    pop rax
    mov rax, fs
    pop rax
    mov rax, es
    pop rax
    mov rax, ds
    pop rax
    mov rax, cs
    pop rdi
    pop rsi
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    set_int
    ret

; RDI stack pointer
; RSI init function
; RDX fxsave_area
ctxcreat:
    ; Save registers
    push rax
    push rbx
    mov rbx, rsp

    fxsave [rdx]

    mov rsp, [rdi]
    push returnoexit
    push 0x0
    push rsi
    mov rsi, rsp
    add rsi, 0x8
    push rsi
    push 0x98
    push 0x99
    push 0x9A
    push 0x9B
    push 0x88
    push 0x89
    push 0x90
    push 0x91
    push 0x92
    push 0x93
    push 0x94
    push 0x95
    push 0x96
    push 0x97
    mov rax, cs
    push rax
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax
    mov rax, ss
    push rax
    pushfq
    mov [rdi], rsp
    mov rsp, rbx
    pop rbx
    pop rax
    ret

; RDI user entry function
userspace_trampoline: 
    pop rcx
    mov r11, 0x202
    o64 sysret

; RDI stack pointer
; RSI init function
; RDX fxsave_area
uctxcreat:
    ; Save registers
    push rax
    push rbx
    mov rbx, rsp

    fxsave [rdx]

    mov rsp, [rdi]
    push returnoexit
    push 0x0
    push rsi
    push userspace_trampoline
    mov rsi, rsp
    add rsi, 0x8
    push rsi
    push 0x98
    push 0x99
    push 0x9A
    push 0x9B
    push 0x88
    push 0x89
    push 0x90
    push 0x91
    push 0x92
    push 0x93
    push 0x94
    push 0x95
    push 0x96
    push 0x97
    mov rax, cs
    push rax
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax
    mov rax, ss
    push rax
    pushfq
    mov [rdi], rsp
    mov rsp, rbx
    pop rbx
    pop rax
    ret