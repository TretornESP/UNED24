#include "idt.h"

void set_offset(struct idtdescentry* entry, uint64_t offset) {
    entry->offset0 = (uint16_t) (offset & 0x000000000000ffff);
    entry->offset1 = (uint16_t) ((offset & 0x00000000ffff0000) >> 16);
    entry->offset2 = (uint32_t) ((offset & 0xffffffff00000000) >> 32);
}

uint64_t get_offset(struct idtdescentry* entry) {
    uint64_t offset = 0;
    offset |= (uint64_t) entry->offset0;
    offset |= (uint64_t) entry->offset1 << 16;
    offset |= (uint64_t) entry->offset2 << 32;
    return offset;
}