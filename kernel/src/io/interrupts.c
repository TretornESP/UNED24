#include "interrupts.h"
#include "idt.h"
#include "io.h"

#include "../arch/cpu.h"
#include "../arch/gdt.h"
#include "../arch/tss.h"
#include "../arch/getcpuid.h"
#include "../memory/memory.h"
#include "../memory/heap.h"
#include "../memory/paging.h"
#include "../util/string.h"
#include "../util/printf.h"
#include "../util/panic.h"
#include "../sched/scheduler.h"
#include "../devices/keyboard/keyboard.h"
#include "../devices/pit/pit.h"

#define __UNDEFINED_HANDLER  __asm__ ("cli"); (void)frame; panic("Undefined interrupt handler");

extern void* interrupt_vector[IDT_ENTRY_COUNT];

uint8_t interrupts_ready = 0;
struct idtr idtr;
volatile int dynamic_interrupt = -1;

void (*dynamic_interrupt_handlers[256])(struct cpu_context* ctx, uint8_t cpuid) = {0};

__attribute__((interrupt)) void DoubleFault_Handler(struct cpu_context* frame) {
    (void)frame;
    panic("Double fault detected\n");
}

void pic_eoi(unsigned char irq) {
    if (irq >= 12) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_end_master() {
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_end_slave() {
    outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void remap_pic() {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);
    io_wait();
    a2 = inb(PIC2_DATA);
    io_wait();

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    io_wait();
    outb(PIC2_DATA, a2);
    io_wait();
}

void PageFault_Handler(struct cpu_context* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    uint64_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));
    printf("Page Fault Address: %x\n", faulting_address);
    panic("Page fault\n");
}

void GPFault_Handler(struct cpu_context* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    panic("General protection fault\n");
}

void KeyboardInt_Handler(struct cpu_context* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    uint8_t scancode = inb(0x60);
    handle_keyboard(scancode);
    pic_end_master();
}

void PitInt_Handler(struct cpu_context* ctx, uint8_t cpuid) {
    (void)ctx;
    (void)cpuid;
    tick();
}

static void interrupt_exception_handler(struct cpu_context* ctx, uint8_t cpu_id) {
    printf("GENERIC EXCEPTION %d ON CPU %d\n", ctx->interrupt_number, cpu_id);
    panic("Exception\n");
}

struct idtdescentry * get_idt_gate(uint8_t entry_offset) {
    return (struct idtdescentry*)(TO_KERNEL_MAP(idtr.offset + (entry_offset * sizeof(struct idtdescentry))));
}

void set_idt_gate(uint64_t handler, uint8_t entry_offset, uint8_t type_attr, uint8_t ist, uint16_t selector) {
    struct idtdescentry* interrupt = (struct idtdescentry*)(idtr.offset + (entry_offset * sizeof(struct idtdescentry)));
    set_offset(FROM_KERNEL_MAP(interrupt), handler);
    interrupt->type_attr.raw = type_attr;
    interrupt->selector = selector;
    interrupt->ist = ist;
}

void hook_interrupt(uint8_t interrupt, void* handler) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    __asm__("cli");
    dynamic_interrupt_handlers[interrupt] = handler;
    __asm__("sti");
}

void unhook_interrupt(uint8_t interrupt) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    __asm__("cli");
    dynamic_interrupt_handlers[interrupt] = (void*)interrupt_exception_handler;
    __asm__("sti");
}

void enable_interrupts() {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    __asm__("sti");
}

void load_interrupts_for_local_cpu() {
    if (interrupts_ready) {
        __asm__("cli");
        __asm__("lidt %0" : : "m"(idtr));
    } else {
        panic("Interrupts not ready\n");
    }
}

void init_interrupts() {
    __asm__("cli");
    
    idtr.limit = 256 * sizeof(struct idtdescentry) - 1;
    idtr.offset = (uint64_t)TO_KERNEL_MAP(request_page());
    memset((void*)idtr.offset, 0, 256 * sizeof(struct idtdescentry));
    mprotect_current((void*)idtr.offset, 256 * sizeof(struct idtdescentry), PAGE_USER_BIT | PAGE_WRITE_BIT);
    
    for (int i = 0; i < 256; i++) {
        set_idt_gate((uint64_t)interrupt_vector[i], i, IDT_TA_InterruptGate, 0, get_kernel_code_selector());
    }

    set_idt_gate((uint64_t)DoubleFault_Handler, 8, IDT_TA_InterruptGate, 1, get_kernel_code_selector());
    
    mprotect_current((void*)idtr.offset, 256 * sizeof(struct idtdescentry), PAGE_USER_BIT);

    for (int i = 0; i < 32; i++) {
        dynamic_interrupt_handlers[i] = interrupt_exception_handler;
    }

    dynamic_interrupt_handlers[0xD] = GPFault_Handler;
    dynamic_interrupt_handlers[0xE] = PageFault_Handler;
    dynamic_interrupt_handlers[KBD_IRQ] = KeyboardInt_Handler;
    dynamic_interrupt_handlers[PIT_IRQ] = PitInt_Handler;
    remap_pic();

    outb(PIC1_DATA, 0xe0);
    outb(PIC2_DATA, 0xef);
    interrupts_ready = 1;
    return;
}

void raise_interrupt(uint8_t interrupt) {
    if (!interrupts_ready) panic("Interrupts not ready\n");
    __asm__("cli");
    dynamic_interrupt = interrupt;
    __asm__("int %0" : : "i"(DYNAMIC_HANDLER));
}

void global_interrupt_handler(struct cpu_context* ctx, uint8_t cpu_id) {
    void (*handler)(struct cpu_context* ctx, uint8_t cpu_id) = (void*)dynamic_interrupt_handlers[ctx->interrupt_number];
    
    if (ctx->interrupt_number == DYNAMIC_HANDLER) {
        if (dynamic_interrupt != 0 && dynamic_interrupt != DYNAMIC_HANDLER) {   
            handler = (void*)dynamic_interrupt_handlers[dynamic_interrupt];
            dynamic_interrupt = 0;
        } else {
            panic("Invalid dynamic interrupt\n");
        }
    }

    //printf("Interrupt %d received on CPU %d\n", ctx->interrupt_number, cpu_id);

    if (handler == 0) {
        printf("No handler for interrupt %d\n", ctx->interrupt_number);
        panic("No handler for interrupt\n");
    }

    handler(ctx, cpu_id);

    pic_eoi(ctx->interrupt_number);

    if (requires_preemption()) {
        yield();
    } else if (requires_wakeup()) {
        wakeup();
    }

}