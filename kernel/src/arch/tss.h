#ifndef _TSS_H
#define _TSS_H
#include <stdint.h>

#define TSS_DEFAULT_CPU 0x0
#define TSS_IST_STACK_SIZE 0x10000 // 64KB
#define CPU_MAX 256
#define TSS_RSP_SIZE 3
#define TSS_IST_SIZE 7

struct tss {
    uint32_t reserved0;
    uint64_t rsp[TSS_RSP_SIZE];
    uint64_t reserved1;
    uint64_t ist[TSS_IST_SIZE];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb;
} __attribute__((packed));

void tss_init();
uint16_t tss_install(uint8_t cpu);
struct tss * get_tss(uint8_t cpu);
uint64_t tss_get_ist(uint8_t cpu, uint8_t ist);
void tss_set_ist(uint8_t cpu, uint8_t ist, uint64_t stack);
uint64_t tss_get_stack(uint8_t cpu, uint8_t dpl);
void tss_set_stack(uint8_t cpu, void* stack, uint8_t dpl);

#endif
