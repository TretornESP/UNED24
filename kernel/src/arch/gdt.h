#ifndef _GDT_H
#define _GDT_H

//https://wiki.osdev.org/Global_Descriptor_Table
//https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/arch/x86-64/gdt/gdt.h

#include <stdint.h>

#define GDT_DPL_KERNEL                  0x0
#define GDT_DPL_USER                    0x3

#define GDT_SYSTEM_TYPE_LDT             0x2
#define GDT_SYSTEM_TYPE_TSS_AVAILABLE   0x9
#define GDT_SYSTEM_TYPE_TSS_BUSY        0xB

#define MAX_GDT_ENTRIES ((65535) / sizeof(struct gdt_entry))

struct gdt_access_byte {
    uint8_t a : 1;
    uint8_t rw : 1;
    uint8_t dc : 1;
    uint8_t ex : 1;
    uint8_t s : 1;
    uint8_t dpl : 2;
    uint8_t p : 1;
} __attribute__((packed));

struct gdt_flags {
    uint8_t limit : 4;
    uint8_t reserved : 1;
    uint8_t l : 1;
    uint8_t db : 1;
    uint8_t g : 1;
} __attribute__((packed));

struct gdt_system_access_byte {
    uint8_t type: 4;
    uint8_t s : 1;
    uint8_t dpl : 2;
    uint8_t p : 1;
} __attribute__((packed));

struct gdt_descriptor {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

struct gdt_priv_info {
    uint16_t data;
    uint16_t code;
};

struct gdt_entry {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    struct gdt_access_byte access;
    struct gdt_flags flags_and_limit1;
    uint8_t base2;
} __attribute__((packed));

struct gdt_tss_entry {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    struct gdt_system_access_byte access;
    uint8_t limit1;
    uint8_t base2;
    uint32_t base3;
    uint32_t reserved;
} __attribute__((packed));

void init_gdt();
extern void load_gdt(struct gdt_descriptor *gdt);
void debug_gdt();
uint16_t create_tss_descriptor(uint64_t base, uint64_t limit);
uint16_t get_kernel_code_selector();
uint16_t get_kernel_data_selector();
uint16_t get_user_code_selector();
uint16_t get_user_data_selector();
#endif