#ifndef _MEMORY_H
#define _MEMORY_H
#include <stdint.h>
#define PAGESIZE 0x1000
#define STACKSIZE (0x16 * PAGESIZE)

struct system_memory {
    uint64_t total_available_pages;
    uint64_t first_available_page_addr;
    uint64_t last_available_page_address;
    uint64_t total_memory;
    uint64_t total_pages;
    uint8_t * bitfield;

    uint64_t free_memory;
    uint64_t locked_memory;
    uint64_t reserved_memory;
} __attribute__((packed));

extern uint64_t KERNEL_START;
extern uint64_t KERNEL_END;

uint64_t get_total_memory();
uint64_t get_free_memory();

int init_memory();
struct system_memory* get_memory();
void offset_memory(void*);

void reserve_page(void*);
void unreserve_page(void*);
void lock_page(void*);
void unlock_page(void*);

void reserve_pages(void*, uint64_t);
void unreserve_pages(void*, uint64_t);
void lock_pages(void*, uint64_t);
void unlock_pages(void*, uint64_t);

void * request_page();
void free_page(void*);

void * request_contiguous_pages(uint64_t);
void free_contiguous_pages(void* address, uint64_t num);

#endif