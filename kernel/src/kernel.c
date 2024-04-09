#include "arch/simd.h"
#include "io/interrupts.h"
#include "devices/keyboard/keyboard.h"
#include "devices/fifo/fifo.h"
#include "devices/pit/pit.h"
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

    const char * fifoa = new_fifo(100);
    driver_list();
    device_list();
    char buffera[] = "Hola Mundo\0";
    device_write(fifoa, 11, 0, (uint8_t*)buffera);
    char bufferb[11];
    device_read(fifoa, 11, 0, (uint8_t*)bufferb);

    printf("Buffer %s\n", bufferb);

    printf("Hey Mundo\n");

    char * buffer = (char*)malloc(1000);
    sprintf(buffer, "Hola Mundo %d\n", 5);
    printf("String %s\n", buffer);
    free(buffer);

    run_shell();
    while(1);
}