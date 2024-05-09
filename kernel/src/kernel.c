#include "arch/simd.h"
#include "io/interrupts.h"
#include "devices/keyboard/keyboard.h"
#include "devices/fifo/fifo.h"
#include "devices/pit/pit.h"
#include "devices/comm/comm.h"
#include "devices/devices.h"
#include "util/string.h"
#include "bootservices/bootservices.h"
#include "util/printf.h"
#include "memory/memory.h"
#include "memory/heap.h"
#include "memory/paging.h"
#include "arch/cpu.h"
#include "arch/gdt.h"
#include "debug/shell.h"

#include "drivers/fifo/fifo_dd.h"
#include "drivers/fifo/fifo_interface.h"
#include "drivers/disk/disk_dd.h"
#include "drivers/disk/disk_interface.h"

#include "vfs/vfs.h"
#include "vfs/vfs_interface.h"
#include "vfs/generic/fat32/generic_f32.h"
#include "vfs/generic/ext2/generic_ext2.h"
#include "vfs/generic/fifo/generic_fifo.h"
#include "vfs/generic/ext2/ext2.h"

#include "drivers/serial/serial_dd.h"
#include "drivers/serial/serial_interface.h"
#include "drivers/tty/tty_dd.h"
#include "drivers/tty/tty_interface.h"

#include "sched/scheduler.h"
#include "proc/process.h"

//Opcion a: todo igual
//Opcion b: un bloque de as y un bloque de bs
//Opcion c: se bloquea en a o b
//Opcion d: segfault

void kwrite(int fd, char * buf, int size) {
    printf("%s", buf);
    yield();
}

void a() {
    while (1) kwrite(1, "a", 1);
}

void b() {
    while (1) kwrite(1, "b", 1);
}

void _start(void) {
    init_simd();
    init_memory();
    init_paging();
    init_heap();
    create_gdt();
    init_pit(50);
    init_interrupts();
    init_keyboard();
    init_cpus();
    enable_interrupts();
    init_devices();
    init_fifo_dd();
    init_drive_dd();
    init_serial_dd();
    init_tty_dd();
    ext2_set_debug_base("ext2/");
    register_filesystem(fat32_registrar);
    register_filesystem(ext2_registrar);
    register_filesystem(fifo_registrar);
    register_comm();
    probe_fs();
    init_scheduler();
    set_current_tty("ttya");
    struct task * task = create_task((void*)run_shell, "ttya");
    add_task(task);

    go(5);
    while(1);
}