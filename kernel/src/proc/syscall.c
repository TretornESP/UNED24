#include "syscall.h"
#include "../arch/msr.h"
#include "../arch/gdt.h"
#include "../util/printf.h"

#define SYSRET(ctx, val) ctx->rax = val; return;
#define SYSCALL_ARG0(ctx) ctx->rdi
#define SYSCALL_ARG1(ctx) ctx->rsi
#define SYSCALL_ARG2(ctx) ctx->rdx
#define SYSCALL_ARG3(ctx) ctx->rcx

extern void syscall_entry();
void undefined_syscall_handler(struct cpu_context* ctx) {
    printf("Undefined syscall %d\n", ctx->rax);
    SYSRET(ctx, SYSCALL_UNDEFINED);
}

void read_syscall_handler(struct cpu_context* ctx) {
    printf("Read syscall\n");
    SYSRET(ctx, SYSCALL_SUCCESS);
}

void write_syscall_hanlder(struct cpu_context* ctx) {
    for (uint64_t i = 0; i < SYSCALL_ARG2(ctx); i++) {
        printf("%c", ((char*)SYSCALL_ARG1(ctx))[i]);
    }
    SYSRET(ctx, SYSCALL_SUCCESS);
}

syscall_handler syscall_handlers[SYSCALL_HANDLER_COUNT] = {
    [0] = read_syscall_handler,
    [1] = write_syscall_hanlder,
    [2 ... SYSCALL_HANDLER_COUNT-1] = undefined_syscall_handler
};

void global_syscall_handler(struct cpu_context* ctx) {
    if (ctx->rax < SYSCALL_HANDLER_COUNT) {
        syscall_handlers[ctx->rax](ctx);
    } else {
        printf("Syscall number overflow %d\n", ctx->rax);
        SYSRET(ctx, SYSCALL_ERROR);
    }
}

void syscall_set_user_gs(uintptr_t addr)
{
    cpu_set_msr_u64(MSR_GS_BASE, addr);
}

void syscall_set_kernel_gs(uintptr_t addr)
{
    cpu_set_msr_u64(MSR_KERN_GS_BASE, addr);
}