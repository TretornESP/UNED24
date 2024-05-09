#include "syscall.h"
#include "../arch/msr.h"
#include "../arch/gdt.h"
#include "../util/printf.h"

#define SYSRET(ctx, val) ctx->rax = val; return;
extern void syscall_entry();
void undefined_syscall_handler(struct cpu_context* ctx) {
    printf("Undefined syscall %d\n", ctx->rax);
    SYSRET(ctx, SYSCALL_UNDEFINED);
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0 ... SYSCALL_HANDLER_COUNT-1] = undefined_syscall_handler
};
void global_syscall_handler(struct cpu_context* ctx) {
    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        syscall_handlers[ctx->rax](ctx);
    } else {
        printf("Syscall number overflow %d\n", ctx->rax);
        SYSRET(ctx, SYSCALL_ERROR);
    }
}

void syscall_enable() {
    cpu_set_msr_u64(MSR_EFER, cpu_get_msr_u64(MSR_EFER) | 1);

    // when starting the syscall:
    // CS = kcode
    // SS = kcode + 8
    // when returning:
    // cs=  ucode + 16
    // ss = ucode + 8

    // so we need to have:
    // kcode : kernel code
    // kcode + 8: kernel data
    // ucode + 8 : user data
    // ucode + 16 : user code

    cpu_set_msr_u64(MSR_STAR, ((uint64_t)(0x20) << 32) | ((uint64_t)(0x18) << 48));
    cpu_set_msr_u64(MSR_LSTAR, (uint64_t)syscall_entry);
    cpu_set_msr_u64(MSR_SYSCALL_FLAG_MASK, 0xfffffffe);

    uint64_t efer, star, lstar, syscall_flag_mask;
    efer = cpu_get_msr_u64(MSR_EFER);
    star = cpu_get_msr_u64(MSR_STAR);
    lstar = cpu_get_msr_u64(MSR_LSTAR);
    syscall_flag_mask = cpu_get_msr_u64(MSR_SYSCALL_FLAG_MASK);

    printf("Syscall enabled\n");
    printf("EFER: %llx STAR: %llx LSTAR: %llx SYSCALL_FLAG_MASK: %llx\n", efer, star, lstar, syscall_flag_mask);
    printf("STAR[63:48]: %llx\n", star >> 48);
    //CS is loaded from STAR[63:48]+16 and or'd with 3
    //SS is loaded from STAR[63:48]+8 and or'd with 3
    printf("STAR RESULING CS: %llx\n", ((star >> 48) + 16) | 3);
    printf("STAR RESULING SS: %llx\n", ((star >> 48) + 8) | 3);
}

void syscall_set_gs(uintptr_t addr)
{
    cpu_set_msr_u64(MSR_GS_BASE, addr);
    cpu_set_msr_u64(MSR_KERN_GS_BASE, addr);
}
