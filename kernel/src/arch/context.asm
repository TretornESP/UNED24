[bits 64]
global ctxswtch
global ctxcreat
global uctxcreat
global newctxswtch
global newctxcreat
global newuctxcreat
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

%define save(offset, register) mov [rdi + (8 * offset)], register
%define load(offset, register) mov register, [rsi + (8 * offset)]

; RDI old's context
; RSI new's context
; RDX old's fxsave_area
; RCX new's fxrstor_area
newctxswtch:
    clear_int
;first of all save rax
    save(2,rax)
    mov rax, cr3
    save(0,rax)
    save(3,rbx)
    save(4,rcx)
    save(5,rdx)
    save(6,rsi)
    save(7,rdi)
    save(8,rbp)
    save(9,r8)
    save(10,r9)
    save(11,r10)
    save(12,r11)
    save(13,r12)
    save(14,r13)
    save(15,r14)
    save(16,r15)
    xor rax, rax
    save(17,rax)
    save(18,rax)
    pop rax
    save(19,rax) ; Return address
    push rax
    mov rax, cs
    save(20,rax) ;cs
    pushfq
    pop rax
    save(21,rax) ; rflags
    mov rax, rsp
    save(22,rax) ; rsp
    mov rax, ss
    save(23,rax) ; ss

    fxsave [rdx]

    load(0,rax)
    mov cr3, rax

    fxrstor [rcx]

    load(3, rbx)
    load(4, rcx)
    load(5, rdx)
    load(8, rbp)
    load(9, r8)
    load(10, r9)
    load(11, r10)
    load(12, r11)
    load(13, r12)
    load(14, r13)
    load(15, r14)
    load(16, r15)
    load(19, rax) ; rip
    push rax 
    load(20, rax) ; cs
    load(21, rax) ; rflags
    push rax
    popfq
    load(22, rsp) ; rsp
    load(23, rax) ; ss
    load(2, rax)
    load(7, rdi)
    load(6, rsi)
    set_int
    ret

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
    pop rax ; Pop ss (ignore)
    pop rax ; Pop gs (ignore)
    pop rax ; Pop fs (ignore)
    pop rax ; Pop es (ignore)
    pop rax ; Pop ds (ignore)
    pop rax ; Pop cs (ignore)
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

; RSI stack pointer
; RDI init function
userspace_trampoline: 
    pop rsi ; Pop stack pointer
    mov rsi, [rsi]
    pop rdi ; Pop init function

    mov rax, (4 * 8) | 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push (4 * 8) | 3
    push rsi
    push 0x200
    push (5 * 8) | 3
    push rdi
    iretq

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
    push rsi ; Init function
    push rdi ; Stack pointer
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

; RDI stack pointer
; RSI init function
newctxcreat:
    push rbx
    mov rbx, rsp
    mov rsp, QWORD [rdi]
    
    push returnoexit
    push 0x0
    push rsi

    mov QWORD [rdi], rsp
    mov rsp, rbx
    pop rbx
    ret

; RDI stack pointer
; RSI init function
newuctxcreat:
    push rbx
    mov rbx, rsp
    mov rsp, [rdi]

    push returnoexit
    push 0x0
    push rsi ; Init function
    push rdi ; Stack pointer
    push userspace_trampoline

    mov [rdi], rsp
    mov rsp, rbx
    pop rbx
    ret