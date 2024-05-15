#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <stdint.h>
#include "../arch/cpu.h"

#define SYSCALL_INITIAL_FLAGS 0x200

#define SYSCALL_UNDEFINED (-1)
#define SYSCALL_ERROR (-2)

#define SYSCALL_HANDLER_COUNT 256

typedef void (*syscall_handler)(struct cpu_context* ctx);

void global_syscall_handler(struct cpu_context* ctx);
void syscall_enable();
void syscall_set_gs(uint64_t addr);
#endif