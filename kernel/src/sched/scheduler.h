#ifndef _SCHEDULER_H
#define _SCHEDULER_H
#include <stdint.h>
#include "../memory/paging.h"
#include "../proc/process.h"
#include "../io/interrupts.h"
#include "concurrency.h"

#define KERNEL_TASK 0
#define USER_TASK 1

#define TASK_EXECUTING       0
#define TASK_READY           1
#define TASK_STOPPED         2
#define TASK_UNINTERRUPTIBLE 3
#define TASK_INTERRUPTIBLE   4
#define TASK_ZOMBIE          5
#define TASK_CREATED         6
#define TASK_FREE            7
#define TASK_IDLE            8

#define SIGKILL 9

extern atomic_int_t irq_disable_counter;

//x86_64 system v abi calling convention stack frame
struct stack_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12; 
    uint64_t r11; 
    uint64_t r10; 
    uint64_t r9; 
    uint64_t r8; 
    uint64_t rbp;
    uint64_t rdi; 
    uint64_t rsi; 
    uint64_t rdx; 
    uint64_t rcx; 
    uint64_t rbx; 
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

extern void ctxswtch(struct task * old_task, struct task* new_task, void* fxsave, void* fxrstor);
extern void ctxcreat(void* rsp, void* intro, void* fxsave);
extern void uctxcreat(void* rsp, void* intro, void* fxsave);

extern void newctxswtch(struct cpu_context * old_task, struct cpu_context* new_task, void* fxsave, void* fxrstor);
extern void newctxcreat(void* rsp, void* intro);
extern void newuctxcreat(void* rsp, void* intro);

void dump_processes();
struct task* get_current_task();
char * get_current_tty();
void set_current_tty(char *);
void reset_current_tty();
void add_task(struct task* task);
struct task* create_task(void * init_func, const char* tty, uint8_t privilege);
void kill_task(int16_t pid);
void pause_task(struct task* task);
void resume_task(struct task* task);
void init_scheduler();
void pseudo_ps();
void returnoexit();
void exit();
void yield();
void task_test();
void go(uint32_t preempt);
void add_signal(int16_t pid, int signal, void * data, uint64_t size);
void subscribe_signal(int signal, sighandler_t handler);
void process_loop();

static inline void lock_scheduler(void) {
#ifndef SMP
    __asm__("cli");
    atomic_increment(&irq_disable_counter);
#endif
}
 
static inline void unlock_scheduler(void) {
#ifndef SMP
    atomic_decrement(&irq_disable_counter);
    if (irq_disable_counter == 0) {
        __asm__("sti");
    }
#endif
}

#endif