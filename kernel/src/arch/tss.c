#include "tss.h"
#include "../util/string.h"
#include "../memory/heap.h"
#include "gdt.h"

void tss_set_stack(struct tss* tss, void* stack, uint8_t dpl) {
    tss->rsp[dpl] = (uint64_t) stack;
}

uint64_t tss_get_stack(struct tss* tss, uint8_t dpl) {
    return tss->rsp[dpl];
}

void tss_set_ist(struct tss* tss, uint8_t ist, uint64_t stack) {
    tss->ist[ist - 1] = stack;
}

uint64_t tss_get_ist(struct tss* tss, uint8_t ist) {
    return tss->ist[ist - 1];
}