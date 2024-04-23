#include "scheduler.h"
#include "../devices/pit/pit.h"
#include "../arch/cpu.h"
#include "../memory/heap.h"
#include "../memory/paging.h"
#include "../arch/simd.h"
#include "../arch/gdt.h"
#include "../util/printf.h"
#include "../util/panic.h"
#include "../devices/devices.h"
#include "../util/string.h"
#include "../arch/tss.h"
#include "../arch/simd.h"
#include "../vfs/vfs_interface.h"

#define MAX_TASKS 255

atomic_int_t irq_disable_counter = 0;

char TTY[] = "default\0";

struct task *task_head = 0;
struct task * current_task = 0;
struct task * boot_task = 0;

//TODO: Implement
//extern void ctxsave(struct cpu_context *ctx);


//Returns the task that must enter cpu

struct task * schedule() {

    struct task * next_task = 0;
    struct task* current = current_task;
    struct task* original = current_task;

    //Find a task that is ready to run TASK_READY, if not found, run idle task
    while (next_task == 0) {
        if (current->next == 0) {
            current = task_head;
        } else {
            current = current->next;
        }

        if (current->state == TASK_READY || current->state == TASK_EXECUTING) {
            next_task = current;
        }

        if (current == original) {
            break;
        }
    }

    if (next_task == 0) {
        //printf("No tasks found, running idle task\n");
        next_task = task_head;
        if (next_task->state != TASK_IDLE) {
            panic("Idle task is not valid or at head\n");
        }
    }

    if (next_task == 0) {
        panic("No tasks found, not even idle!!!\n");
    }

    struct task * prev_task = current_task;  

    if (current_task->state == TASK_EXECUTING)  
        current_task->state = TASK_READY;
    
    current_task = next_task;

    if (current_task->state == TASK_READY)
        current_task->state = TASK_EXECUTING;

    //printf("Switching from task %d to %d\n", prev_task->pid, current_task->pid);

    return prev_task;
}

void block_task(long reason) {
    printf("Blocking task %d\n", current_task->pid);
    lock_scheduler();
    current_task->state = reason;
    yield();
    unlock_scheduler();
}

void unblock_task(struct task * task) {
    printf("Unblocking task %d\n", task->pid);
    lock_scheduler();
    task->state = TASK_READY;
    yield();
    unlock_scheduler();
}

void __attribute__((noinline)) yield() {
    //TODO: Implement
    sleep_ticks(50);
}

void add_task(struct task* task) {
    lock_scheduler();
    printf("Adding task %d\n", task->pid);

    if (task_head == 0) {
        task_head = task;
        unlock_scheduler();
        return;
    }

    struct task* current = task_head;
    while (current->next != 0) {
        current = current->next;
    }

    current->next = task;
    task->prev = current;
    unlock_scheduler();
}

void remove_task(struct task* task) {
    lock_scheduler();
    if (task->prev == 0) {
        task_head = task->next;
        if (task_head != 0) {
            task_head->prev = 0;
        }
        unlock_scheduler();
        return;
    }

    task->prev->next = task->next;
    if (task->next != 0) {
        task->next->prev = task->prev;
    }
    unlock_scheduler();
}

struct task* get_task(int16_t pid) {
    lock_scheduler();
    struct task* current = task_head;
    if (current == 0) {
        unlock_scheduler();
        return 0;
    }

    if (current->pid == pid) {
        unlock_scheduler();
        return current;
    }

    while (current->next != 0) {
        if (current->next->pid == pid) {
            unlock_scheduler();
            return current->next;
        }
        current = current->next;
    }

    unlock_scheduler();
    return 0;
}

struct task* get_status(long state) {
    lock_scheduler();
    struct task* current = task_head;
    if (current == 0) {
        unlock_scheduler();
        return 0;
    }

    if (current->state == state) {
        unlock_scheduler();
        return current;
    }

    while (current->next != 0) {
        if (current->next->state == state) {
            unlock_scheduler();
            return current->next;
        }
        current = current->next;
    }

    unlock_scheduler();
    return 0;
}

int16_t get_free_pid() {
    lock_scheduler();
    int16_t pid = 1;
    while (get_task(pid) != 0) {
        pid++;
    }
    printf("Free pid: %d\n", pid);
    unlock_scheduler();
    return pid;
}

void pseudo_ps() {
    struct task* current = task_head;
    if (current == 0) {
        return;
    }

    while (current != 0) {
        printf("PID: %d PPID: %d STATE: %d FLAGS: %d, ENTRY: %p TTY: %s\n", current->pid, current->ppid, current->state, current->flags, current->entry, current->tty);
        current = current->next;
    }

    return;
}

struct task* get_current_task() {   
    return current_task;
}

void go(uint32_t preempt) {
    //TODO: Implement the real version
    schedule();
    printf("Switching to task %d\n", current_task->pid);
    void (*ptr)(void) = current_task->entry;
    ptr();
}

char * get_current_tty() {
    if (current_task == 0) {
        return TTY;
    }
    return current_task->tty;
}

void idle() {
    while (1) {
        __asm__ volatile("hlt");
    }
}

void exit() {
    lock_scheduler();
    printf("Exiting task %d\n", current_task->pid);
    current_task->exit_code = 0;
    current_task->state = TASK_ZOMBIE;
    unlock_scheduler();
    //Yield
}

void kill_task(int16_t pid) {
    lock_scheduler();
    struct task* task = get_task(pid);
    if (task == 0) {
        printf("No such task\n");
        unlock_scheduler();
        return;
    }

    printf("Killing task %d\n", pid);
    task->exit_code = 0;
    task->exit_signal = SIGKILL;
    task->state = TASK_ZOMBIE;
    unlock_scheduler();
}

void subscribe_signal(int signal, sighandler_t handler) {
    lock_scheduler();
    current_task->signal_handlers[signal] = handler;
    unlock_scheduler();
}

void add_signal(int16_t pid, int signal, void * data, uint64_t size) {
    lock_scheduler();
    struct task* task = get_task(pid);
    if (task == 0) {
        printf("No such task\n");
        unlock_scheduler();
        return;
    }

    //Add signal to signal_queue linked list
    struct task_signal * signal_struct = malloc(sizeof(struct task_signal));
    signal_struct->signal = signal;
    signal_struct->next = 0;
    signal_struct->signal_data = data;
    signal_struct->signal_data_size = size;

    if (task->signal_queue == 0) {
        task->signal_queue = signal_struct;
    } else {
        struct task_signal * current = task->signal_queue;
        while (current->next != 0) {
            current = current->next;
        }
        current->next = signal_struct;
    }
    
    unlock_scheduler();
}

struct task* create_task(void * init_func, const char * tty) {
    lock_scheduler();

    struct task * task = malloc(sizeof(struct task));
    task->fx = malloc(SIMD_CONTEXT_SIZE + 15);
    task->fx = (void*)(((uint64_t)task->fx + 15) & ~0xF);
    task->state = TASK_READY;
    task->flags = 0;
    task->sigpending = 0;
    task->nice = 0;
    task->mm = 0;
    task->frame = 0;
    task->processor = 0; //TODO: Change this for SMP
    task->cpu_time = 0;
    task->last_scheduled = 0;
    task->sleep_time = 0;
    task->exit_code = 0;    
    task->exit_signal = 0;
    task->pdeath_signal = 0;
    task->signal_queue = 0;
    memset(task->signal_handlers, 0, sizeof(sighandler_t) * TASK_SIGNAL_MAX);
    
    struct task * parent = get_current_task();
    if (parent == 0) {
        parent = task;
        task->ppid = 0;
    } else {
        task->ppid = parent->pid;
    }

    task->parent = parent;
    task->pid = get_free_pid();

    task->uid = 0;
    task->gid = 0;

    task->locks = 0;
    task->open_files = malloc(sizeof(int) * 32);
    memset(task->open_files, -1, sizeof(int) * 32);
    if (task->open_files == 0) {
        printf("Failed to allocate open_files");
    } else {
        //Check if tty is valid
        if (tty != 0) {
            struct device* dev = device_search(tty);
            if (dev != 0) {
                char buf[256] = {0};
                strcpy(buf, tty);
                strcat(buf, "p0/");
                task->open_files[0] = vfs_file_open(buf, 0, 0);
                if (task->open_files[0] == -1) {
                    printf("Failed to open tty %s\n", buf);
                } else {
                    memcpy(&(task->open_files[1]), &(task->open_files[0]), sizeof(int));
                    memcpy(&(task->open_files[2]), &(task->open_files[0]), sizeof(int));
                }
            }
        }
    }

    task->entry = init_func;
    task->rsp = (uint64_t)stackalloc(STACK_SIZE);
    task->rsp_top = task->rsp + STACK_SIZE;

    //Create a stack frame (do not use structs)
    //Set r12-r15 to 0, rbx to 0
    //Set rip to init_func

    uint64_t * saved_rsp;
    //Save current rsp
    __asm__ volatile("movq %%rsp, %0" : "=r"(saved_rsp));

    //Set rsp to the top of the stack
    __asm__ volatile("movq %0, %%rsp" : : "r"(task->rsp_top));

    //Push the return address
    __asm__ volatile("pushq %0" : : "r"(init_func));

    //Push the rsp_top
    __asm__ volatile("pushq %0" : : "r"(task->rsp_top));

    __asm__ volatile("pushq $0x98"); //RAX
    __asm__ volatile("pushq $0x99"); //RBX
    __asm__ volatile("pushq $0x9A"); //RCX
    __asm__ volatile("pushq $0x9B"); //RDX
    __asm__ volatile("pushq $0x88"); //R8
    __asm__ volatile("pushq $0x89"); //R9
    __asm__ volatile("pushq $0x90"); //R10
    __asm__ volatile("pushq $0x91"); //R11
    __asm__ volatile("pushq $0x92"); //R12
    __asm__ volatile("pushq $0x93"); //R13
    __asm__ volatile("pushq $0x94"); //R14
    __asm__ volatile("pushq $0x95"); //R15
    __asm__ volatile("pushq $0x96"); //RSI
    __asm__ volatile("pushq $0x97"); //RDI

    //Save rsp to the task struct
    __asm__ volatile("movq %%rsp, %0" : "=r"(task->rsp_top));
    //Set rsp to the saved rsp
    __asm__ volatile("movq %0, %%rsp" : : "r"(saved_rsp));

    task->pd = duplicate_current_pml4();
    strncpy(task->tty, tty, strlen(tty));
    task->descriptors = 0;
    task->next = 0;
    task->prev = 0;

    printf("Created task with pid %d for address %p\n", task->pid, init_func);
    unlock_scheduler();
    return task;
}

void pause_task(struct task* task) {
    lock_scheduler();
    //printf("Pausing task %d\n", task->pid);
    task->state = TASK_UNINTERRUPTIBLE;
    unlock_scheduler();
}
void resume_task(struct task* task) {
    lock_scheduler();
    //printf("Resuming task %d\n", task->pid);
    task->state = TASK_READY;
    unlock_scheduler();
}

void init_scheduler() {
    printf("### SCHEDULER STARTUP ###\n");
    lock_scheduler();
    struct task * idle_task = create_task(idle, "default"); //Spawn idle task
    idle_task->pid = 0;
    idle_task->state = TASK_IDLE;

    add_task(idle_task);

    current_task = task_head;

    printf("### SCHEDULER STARTUP DONE ###\n");
    unlock_scheduler();
}

void set_current_tty(char * tty) {
    lock_scheduler();
    if (current_task == 0) {
        unlock_scheduler();
        return;
    }
    memset(current_task->tty, 0, 32);
    strncpy(current_task->tty, tty, strlen(tty));
    //Make sure it's null terminated
    current_task->tty[strlen(tty)] = '\0';
    unlock_scheduler();
}

void reset_current_tty() {
    lock_scheduler();
    if (current_task == 0) {
        unlock_scheduler();
        return;
    }
    memset(current_task->tty, 0, 32);
    strncpy(current_task->tty, "default\0", 8);
    unlock_scheduler();
}

void process_loop() {
    while (1) {
        //Process all signals in the queueR
        if (current_task->signal_queue != 0) {
            struct task_signal * current = current_task->signal_queue;
            while (current != 0) {
                if (current_task->signal_handlers[current->signal] != 0) {
                    current_task->signal_handlers[current->signal](current->signal, current->signal_data, current->signal_data_size);
                }
                struct task_signal * next = current->next;
                free(current);
                current = next;
            }
            current_task->signal_queue = 0;
        }
        yield();
        //Yield to the next task
    }
}