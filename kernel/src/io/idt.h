#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>
#include "../memory/paging.h"

#define IDT_ENTRY_COUNT (PAGESIZE / sizeof(struct idtdescentry))

#define IDT_TA_InterruptGate        0x8E
#define IDT_TA_InterruptGateUser    0xEE
#define IDT_TA_CallGate             0x8C
#define IDT_TA_TrapGate             0x8F

#define IST_Null                    0x0
#define IST_Interrupts              0x1

struct idt_type {
    uint8_t gate_type : 4;
    uint8_t storage_segment : 1;
    uint8_t descriptor_privilege_level : 2;
    uint8_t present : 1;
} __attribute__((packed));

struct idtdescentry {
    uint16_t offset0;
    uint16_t selector;
    uint8_t ist;
    union {
        struct idt_type bits;
        uint8_t raw;
    } type_attr;
    uint16_t offset1;
    uint32_t offset2;
    uint32_t ignore;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

void set_offset(struct idtdescentry *, uint64_t);
uint64_t get_offset(struct idtdescentry *);
#endif