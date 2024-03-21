#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>

#define IDT_TA_InterruptGate 0x8E
#define IDT_TA_CallGate      0x8C
#define IDT_TA_TrapGate      0x8F

struct idtdescentry {
   uint16_t offset_1;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_2;        // offset bits 16..31
   uint32_t offset_3;        // offset bits 32..63
   uint32_t zero;            // reserved
};

struct idtr {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

void set_offset(struct idtdescentry* entry, uint64_t address);
uint64_t get_offset(struct idtdescentry* entry);

#endif