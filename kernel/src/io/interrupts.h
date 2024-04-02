#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H
#include "idt.h"


//Modify this to your liking or die trying (DSDT AML STUFF)
#define PIT_IRQ 0x22
#define KBD_IRQ 0x21
#define PCIA_IRQ 0x2b
#define SR1_IRQ 0x23
#define SR2_IRQ 0x24

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

#define PIC_EOI 0x20

#define IRQ_START 0x20
#define DYNAMIC_HANDLER 0x90

struct rflags{
    uint8_t CF:1;
    uint8_t Reserved0:1; //this is bit is set as default
    uint8_t PF:1;
    uint8_t Reserved1:1;
    uint8_t AF:1;

    uint8_t Reserved2:1;
    uint8_t ZF:1;
    uint8_t SF:1;
    uint8_t TF:1;
    uint8_t IF:1;

    uint8_t DF:1;
    uint8_t OF:1;
    uint8_t IOPL:2;
    uint8_t NT:1;
    uint8_t Reserved3:1;
    uint8_t RF:1;

    uint8_t VM:1;
    uint8_t AC:1;
    uint8_t VIF:1;
    uint8_t VIP:1;
    uint8_t ID:1;

    uint8_t Reserved4:1;
    uint8_t Reserved5:1;
    uint8_t Reserved6:1;
    uint8_t Reserved7:1;
    uint8_t Reserved8:1;

    uint8_t Reserved9:1;
    uint8_t Reserved10:1;
    uint8_t Reserved11:1;
    uint8_t Reserved12:1;
    uint8_t Reserved13:1;

    uint32_t Reserved14;
}__attribute__((packed));

struct interrupt_frame {   
    uint64_t rip; 
    uint64_t cs; 
    struct rflags rflags; 
    uint64_t rsp; 
    uint64_t ss;
}__attribute__((packed)); 

struct interrupt_frame_error {   
    uint64_t error_code;
    uint64_t rip; 
    uint64_t cs; 
    struct rflags rflags; 
    uint64_t rsp; 
    uint64_t ss;
}__attribute__((packed)); 

void init_interrupts(uint8_t);
void enable_interrupts();
void load_interrupts_for_local_cpu();
void hook_interrupt(uint8_t interrupt, void* handler);
void unhook_interrupt(uint8_t interrupt);
void pic_eoi(unsigned char irq);
void raise_interrupt(uint8_t interrupt);
void mask_interrupt(uint8_t irq);
void unmask_interrupt(uint8_t irq);
#endif