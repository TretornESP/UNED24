[bits 64]
;Get the cr0 register
getCr0:
    mov rax, cr0

;Get the cr2 register
getCr2:
    mov rax, cr2

;Get the cr3 register
getCr3:
    mov rax, cr3

;Get the cr4 register
getCr4:
    mov rax, cr4

;Get the cr8 register
getCr8:
    mov rax, cr8

getEfer:
    mov rax, 0xC0000080
    rdmsr

;Get the fs base msr
getFsBase:
    mov rax, 0xC0000100
    rdmsr

;Get the gs base msr
getGsBase:
    mov rax, 0xC0000101
    rdmsr

;Get the kernel gs base msr
getKernelGsBase:
    mov rax, 0xC0000102
    rdmsr

;Get the rflags register
getRflags:
    pushfq
    pop rax

GLOBAL getCr0
GLOBAL getCr2
GLOBAL getCr3
GLOBAL getCr4
GLOBAL getCr8
GLOBAL getEfer
GLOBAL getFsBase
GLOBAL getGsBase
GLOBAL getKernelGsBase
GLOBAL getRflags