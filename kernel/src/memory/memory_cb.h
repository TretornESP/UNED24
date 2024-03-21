#ifndef _MEMORY_CB_H
#define _MEMORY_CB_H
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define MEMMAP_USABLE                   0x0
#define MEMMAP_RESERVED                 0x1
#define MEMMAP_ACPI_RECLAIMABLE         0x2
#define MEMMAP_ACPI_NVS                 0x3
#define MEMMAP_BAD_MEMORY               0x4
#define MEMMAP_BOOTLOADER_RECLAIMABLE   0x5
#define MEMMAP_KERNEL_AND_MODULES       0x6
#define MEMMAP_FRAMEBUFFER              0x7

struct chunk_data {
    uint64_t addr;
    uint64_t size;
};

void get_total_memory_cb(void * global_override, uint64_t base, uint64_t length, uint64_t type) {
    *(uint64_t*)global_override += length;
}

void get_free_memory_cb(void * global_override, uint64_t base, uint64_t length, uint64_t type) {
    if (type == MEMMAP_USABLE)
        *(uint64_t*)global_override += length;
}

void init_memory_cb(void * global_override, uint64_t base, uint64_t length, uint64_t type) {
    struct chunk_data *chunk = (struct chunk_data*)global_override;

    if (type == MEMMAP_USABLE) {
        if (length > chunk->size) {
            chunk->size = length;
            chunk->addr = base;
        }
    }
}

#endif