#include "tss.h"
#include "../util/string.h"
#include "gdt.h"

static struct tss tss[CPU_MAX];

void tss_init() {
    memset(tss, 0, sizeof(struct tss) * CPU_MAX);
}

uint16_t tss_install(uint8_t cpu) {
    uint64_t base = (uint64_t) &tss[cpu];
    memset((void*)base, 0, sizeof(struct tss));

    uint16_t location = create_tss_descriptor(base, sizeof(struct tss));

    tss[cpu].iopb = 0;

    return location;
}

void tss_set_stack(uint8_t cpu, void* stack, uint8_t dpl) {
    tss[cpu].rsp[dpl] = (uint64_t) stack;
}

uint64_t tss_get_stack(uint8_t cpu, uint8_t dpl) {
    return tss[cpu].rsp[dpl];
}

void tss_set_ist(uint8_t cpu, uint8_t ist, uint64_t stack) {
    tss[cpu].ist[ist - 1] = stack;
}

uint64_t tss_get_ist(uint8_t cpu, uint8_t ist) {
    return tss[cpu].ist[ist - 1];
}

struct tss * get_tss(uint8_t cpu) {
    return &tss[cpu];
}