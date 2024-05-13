#ifndef _PAGING_H
#define _PAGING_H
#include <stdint.h>
#include "memory.h"

#define PAGE_WRITE_BIT     0x1
#define PAGE_USER_BIT      0x2
#define PAGE_NX_BIT        0x4
#define PAGE_CACHE_DISABLE 0x8

#define PAGE_ALLOW_WRITE(x)     ((page_set  ((x), PAGE_WRITE_BIT)))
#define PAGE_RESTRICT_WRITE(x)  ((page_clear((x), PAGE_WRITE_BIT)))
#define PAGE_ALLOW_USER(x)      ((page_set  ((x), PAGE_USER_BIT )))
#define PAGE_RESTRICT_USER(x)   ((page_clear((x), PAGE_USER_BIT )))
#define PAGE_ALLOW_NX(x)        ((page_set  ((x), PAGE_NX_BIT   )))
#define PAGE_RESTRICT_NX(x)     ((page_clear((x), PAGE_NX_BIT   )))
#define PAGE_DISABLE_CACHE(x)   ((page_set  ((x), PAGE_CACHE_DISABLE)))
#define PAGE_ENABLE_CACHE(x)    ((page_clear((x), PAGE_CACHE_DISABLE)))

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
//void debug_memory_map(void*, void*);
uint64_t virtual_to_physical(struct page_directory *, void*);
uint64_t virtual_to_physical_current(void*);

struct page_directory* allocate_pml4();
struct page_directory* duplicate_current_pml4();
struct page_directory* get_pml4();

void* swap_pml4(void*);
void invalidate_current_pml4();
void map_current_memory(void*, void*);
void map_memory(struct page_directory*, void*, void*);

void map_current_memory_size(void*, void*, uint64_t);
void map_memory_size(struct page_directory*, void*, void*, uint64_t);

void * request_page_identity(struct page_directory *);
void * request_current_page_identity();

void * request_page_at(struct page_directory *, void*);
void * request_current_page_at(void*);
void * request_current_pages_identity(uint64_t count);

void * request_accessible_page_at(struct page_directory*, void*, void *);
void * request_current_accessible_page_at(void*, void *);

void mprotect(struct page_directory *, void*, uint64_t, uint8_t);
void mprotect_current(void*, uint64_t, uint8_t);
#endif