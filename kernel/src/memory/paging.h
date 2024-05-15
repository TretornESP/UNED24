#ifndef _PAGING_H
#define _PAGING_H
#include <stdint.h>
#include "memory.h"

#define LIMINE_FREE_START 0x1000
#define LIMINE_FREE_PAGES 0x180000

#define HW_ADDRESS_START   ((uint64_t)0xffff800000000000)
#define PMM_BITMAP_START   ((uint64_t)0xffff900000000000)
#define IDENTITY_MAP_START ((uint64_t)0xffffA00000000000)
#define USERLAND_END       ((uint64_t)0x0000800000000000)

#define TO_KERNEL_MAP(vaddr) ((void*)(((uint64_t)(vaddr))+(uint64_t)IDENTITY_MAP_START))
#define FROM_KERNEL_MAP(vaddr) ((void*)(((uint64_t)(vaddr))-(uint64_t)IDENTITY_MAP_START))

#define PAGE_WRITE_BIT     0x1
#define PAGE_USER_BIT      0x2
#define PAGE_NX_BIT        0x4
#define PAGE_CACHE_DISABLE 0x8

struct page_directory_entry {
    uint64_t present                   :1;
    uint64_t writeable                 :1;
    uint64_t user_access               :1;
    uint64_t write_through             :1;
    uint64_t cache_disabled            :1;
    uint64_t accessed                  :1;
    uint64_t ignored_3                 :1;
    uint64_t size                      :1; // 0 means page directory mapped
    uint64_t ignored_2                 :4;
    uint64_t page_ppn                  :28;
    uint64_t reserved_1                :12; // must be 0
    uint64_t ignored_1                 :11;
    uint64_t execution_disabled        :1;
} __attribute__((packed));

struct page_table_entry{
    uint64_t present            : 1;
    uint64_t writeable          : 1;
    uint64_t user_access        : 1;
    uint64_t write_through      : 1;
    uint64_t cache_disabled     : 1;
    uint64_t accessed           : 1;
    uint64_t dirty              : 1;
    uint64_t size               : 1;
    uint64_t global             : 1;
    uint64_t ignored_2          : 3;
    uint64_t page_ppn           : 28;
    uint64_t reserved_1         : 12; // must be 0
    uint64_t ignored_1          : 11;
    uint64_t execution_disabled : 1;
} __attribute__((__packed__));

struct page_directory {
    struct page_directory_entry entries[512];
} __attribute__((aligned(PAGESIZE)));

struct page_table {
    struct page_table_entry entries[512];
} __attribute__((aligned(PAGESIZE)));


struct page_map_index{
    uint64_t PDP_i;
    uint64_t PD_i;
    uint64_t PT_i;
    uint64_t P_i;
};

void init_paging();

void* swap_pml4(void*);
struct page_directory* duplicate_current_pml4();
struct page_directory* duplicate_current_kernel_pml4();
struct page_directory* get_pml4();
void invalidate_current_pml4();
uint8_t get_page_perms(struct page_directory *pml4, void* address);
void * virtual_to_physical(struct page_directory *, void*);

void map_current_memory(void*, void*, uint8_t);
void map_memory(struct page_directory *, void*, void*, uint8_t);
void unmap_current_memory(void*);	
void unmap_memory(struct page_directory *, void*);
void * request_current_page_at(void*, uint8_t);
void * request_page_at(struct page_directory *, void*, uint8_t);
void mprotect_current(void*, uint64_t, uint8_t);
uint8_t is_present(struct page_directory* pml4, void * address);
#endif