#include "cpu.h"
#include "../arch/gdt.h"
#include "../arch/tss.h"
#include "../proc/syscall.h"
#include "../memory/paging.h"
#include "../io/interrupts.h"
#include "../memory/heap.h"
#include "../util/printf.h"
#include "../util/string.h"
extern void reloadGsFs();
extern void setGsBase(uint64_t base);

struct cpu * cpu;

void init_cpus() {
    //Only bsp will initialize the cpus until the last step
    //The write to goto_address makes the other cpus startup

    cpu = kmalloc(sizeof(struct cpu));
    memset(cpu, 0, sizeof(struct cpu));

    printf("Booting CPU %d\n", BSP_CPU);

    cpu->tss = get_tss(BSP_CPU);
    cpu->ctx = kmalloc(sizeof(struct cpu_context));
    memset(cpu->ctx, 0, sizeof(struct cpu_context));

    cpu->kstack = kstackalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
    memset(cpu->kstack - KERNEL_STACK_SIZE, 0, KERNEL_STACK_SIZE);
    
    void * ist0 = kstackalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
    memset(ist0 - KERNEL_STACK_SIZE, 0, KERNEL_STACK_SIZE);
    void * ist1 = kstackalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
    memset(ist1 - KERNEL_STACK_SIZE, 0, KERNEL_STACK_SIZE);

    tss_set_stack(cpu->tss, cpu->kstack, 0);
    tss_set_ist(cpu->tss, 0, (uint64_t)ist0);
    tss_set_ist(cpu->tss, 1, (uint64_t)ist1);

    reloadGsFs();
    setGsBase((uint64_t) cpu->ctx);

    load_gdt(BSP_CPU);
    syscall_enable(GDT_KERNEL_CODE_ENTRY * sizeof(gdt_entry_t), GDT_USER_CODE_ENTRY * sizeof(gdt_entry_t));
    load_interrupts_for_local_cpu();
    cpu->ready = 1;
}

struct cpu* get_cpu(uint64_t index) {
    return cpu;
}