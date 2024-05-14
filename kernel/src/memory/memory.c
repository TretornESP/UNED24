#include "memory.h"
#include "memory_cb.h"

#include "../bootservices/bootservices.h"
#include "../util/panic.h"
#include "../util/string.h"
#include "../util/printf.h"

#define IS_ALIGNED(x) ((((uint64_t)(x))&(PAGESIZE-1)) == 0)
#define ALIGN_ADDR(x)  (((PAGESIZE-1)&(x)) ? ((x+PAGESIZE) & ~(PAGESIZE-1)) : (x))

#define ADDR_TO_PAGE(x) ((uint64_t)(x) >> 12)

#define SET_PAGE_BIT(x) (memory->bitfield[(x) >> 3] |= (1 << ((x) % 8)))
#define CLEAR_PAGE_BIT(x) (memory->bitfield[(x) >> 3] &= ~(1 << ((x) % 8)))
#define GET_PAGE_BIT(x) ((memory->bitfield[(x) >> 3] & (1 << ((x) % 8))) >> ((x) % 8))

struct system_memory * memory;

void set_page_bit(void * address) {
    if (ADDR_TO_PAGE(address) > memory->total_available_pages) {
        panic("set_page_bit: address out of bounds");
    }
    SET_PAGE_BIT(ADDR_TO_PAGE(address));
    if (GET_PAGE_BIT(ADDR_TO_PAGE(address)) != 1) {
        panic("set_page_bit: bit not set");
    }
}

void clear_page_bit(void * address) {
    if (ADDR_TO_PAGE(address) > memory->total_available_pages) {
        panic("clear_page_bit: address out of bounds");
    }

    CLEAR_PAGE_BIT(ADDR_TO_PAGE(address));
}

uint8_t get_page_bit(void * address) {
    if (ADDR_TO_PAGE(address) > memory->total_available_pages) {
        panic("get_page_bit: address out of bounds");
    }

    return (uint8_t)GET_PAGE_BIT(ADDR_TO_PAGE(address));
}

void reserve_page(void* address) {
    if (get_page_bit(address) == 1)
        panic("Page already reserved!");

    set_page_bit(address);

    memory->free_memory -= PAGESIZE;
    memory->reserved_memory += PAGESIZE;
}

void unreserve_page(void* address) {
    if (get_page_bit(address) == 0)
        panic("Page not reserved!");

    clear_page_bit(address);
    memory->free_memory += PAGESIZE;
    memory->reserved_memory -= PAGESIZE;
}

void lock_page(void * address) {
    if (get_page_bit(address) == 1)
        panic("Page already locked!"); //Deleteme
    
    set_page_bit(address);

    memory->free_memory -= PAGESIZE;
    memory->locked_memory += PAGESIZE;
}

void unlock_page(void * address) {
    if (get_page_bit(address) == 0)
        panic("Page not locked!"); //Deleteme
    
    clear_page_bit(address);

    memory->free_memory -= PAGESIZE;
    memory->locked_memory += PAGESIZE;
}

void reserve_pages(void* address, uint64_t pages) {
    if ((int64_t)address < 0) {
        panic("NEGATIVE");
    }
    for (uint64_t i = 0; i < pages; i++) {
        reserve_page((void*)((uint64_t)address + (i << 12)));
    }
}


void unreserve_pages(void* address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++) {
        unreserve_page((void*)((uint64_t)address + (i << 12)));
    }
}

void lock_pages(void * address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++) {
        lock_page((void*)((uint64_t)address + (i << 12)));
    }
}

void unlock_pages(void * address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++) {
        unlock_page((void*)((uint64_t)address + (i << 12)));
    }
}

void * request_page() {
    static uint64_t last_requested;
    static uint64_t statistics_requested;
    uint64_t first = last_requested-1;

    while(GET_PAGE_BIT(last_requested) == 1) {
        if (last_requested >= memory->total_available_pages)
            last_requested = 0;
        if (last_requested == first) {
            panic("No free pages available!");
        }
        last_requested++;
    }

    lock_page((void*)(last_requested << 12));

    if (GET_PAGE_BIT(last_requested) == 0)
        panic("Page not reserved!");

    statistics_requested++;

    return (void*)(memory->first_available_page_addr+(last_requested << 12));
}

void free_page(void* addr) {
    uint64_t addr_uint = (uint64_t)addr;
    uint64_t normaliced_address = (addr_uint - memory->first_available_page_addr);

    if (addr_uint < memory->first_available_page_addr)
        panic("Page below active range");

    if (GET_PAGE_BIT(normaliced_address >> 12) == 0)
        panic("page was not locked");

    unlock_page((void*)normaliced_address);

    if (GET_PAGE_BIT((normaliced_address >> 12)) == 1) {
        panic("Page not freed!");
    }
}

void * request_contiguous_pages(uint64_t num) {
    static uint64_t last_requested;
    static uint64_t statistics_requested;
    uint64_t first = last_requested-1;
    uint64_t last = last_requested-1;

    while (GET_PAGE_BIT(last_requested) == 1) {
        if (last_requested >= memory->total_available_pages)
            last_requested = 0;
        if (last_requested == first) {
            panic("No free pages available!");
        }
        last_requested++;
    }

    for (uint64_t i = 0; i < num; i++) {
        if (GET_PAGE_BIT(last_requested) == 1)
            panic("Page already reserved!");
        lock_page((void*)(last_requested << 12));
        last_requested++;
    }

    statistics_requested += num;

    return (void*)(memory->first_available_page_addr+(last << 12));
}

void free_contiguous_pages(void* address, uint64_t num) {
    uint64_t addr_uint = (uint64_t) address;
    uint64_t normaliced_address = (addr_uint - memory->first_available_page_addr);

    if (addr_uint < memory->first_available_page_addr) {
        panic("Page below active range!");
    }

    for (uint64_t i = 0; i < num; i++) {
        if (GET_PAGE_BIT((normaliced_address >> 12) + i) == 0)
            panic("Page was not locked!");
        unlock_page((void*)(normaliced_address + (i << 12)));
    }

    if (GET_PAGE_BIT((normaliced_address >> 12)) == 1) {
        panic("Page not freed");
    }
}

void iterate_memory(void (*callback)(void*, uint64_t, uint64_t, uint64_t), void* global_override) {
    uint64_t memory_entries = get_memory_map_entries();

    for (uint64_t index = 0; index < memory_entries; index++) {
        uint64_t base = get_memory_map_base(index);
        uint64_t length = get_memory_map_length(index);
        uint64_t type = get_memory_map_type(index);
        //printf("[mmap entry %d] base: %llx length: %llx type: %llx\n", index, base, length, type);
        callback(global_override, base, length, type);
    }
}

uint64_t get_total_memory() {
    static uint64_t total_memory;
    if (total_memory > 0) return total_memory;
    iterate_memory(get_total_memory_cb, (void*)&total_memory);
    return total_memory;
}

uint64_t get_free_memory() {
    static uint64_t free_memory;
    if (free_memory > 0) return free_memory;
    iterate_memory(get_free_memory_cb, (void*)&free_memory);
    return free_memory;
}

struct system_memory * get_memory() {
    return memory;
}

void set_memory(void* addr) {
    memory = (struct system_memory*)addr;
}

int init_memory() {
    static struct chunk_data biggest_avail_chunk;
    if (biggest_avail_chunk.size > 0) return 0;

    uint64_t total_memory = get_total_memory();
    //uint64_t free_memory = get_free_memory();
    uint64_t total_pages = total_memory >> 12;

    iterate_memory(init_memory_cb, (void*)&biggest_avail_chunk);

    //printf("Tenemos %llx bytes de memoria\n", total_memory);
    //printf("Tenemos %llx bytes de memoria libre\n", free_memory);
    //printf("Tenemos %llx bytes de memoria disponible\n", biggest_avail_chunk.size);

    uint64_t first_available_address = ALIGN_ADDR(biggest_avail_chunk.addr);
    uint64_t last_available_adress = ALIGN_ADDR(biggest_avail_chunk.addr + biggest_avail_chunk.size);
    uint64_t available_pages = (last_available_adress - first_available_address) >> 12;
    uint64_t bitfield_entries = ((available_pages % 8) == 0) ? (available_pages >> 3) : (available_pages >> 3) + 1;

    if (biggest_avail_chunk.size < (bitfield_entries + sizeof(struct system_memory) + (3 << 12)))
        panic("init_memory: not enough memory for memory bitfield");

    memory = (struct system_memory*)first_available_address;
    memory->bitfield = (uint8_t*)ALIGN_ADDR(first_available_address + sizeof(struct system_memory*));

    //01010101
    memset(memory->bitfield, 0x55, 100);

    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 8; j++) {
            if (j % 2 == 1) {
                if (GET_PAGE_BIT(i*8 + j) == 1)
                    panic("init_memory: GET_PAGE_BIT failed");
            } else {
                if (GET_PAGE_BIT(i*8 + j) == 0)
                    panic("init_memory: GET_PAGE_BIT failed");
            }
        }
    }

    memset(memory->bitfield, 0x0, bitfield_entries);

    memory->total_available_pages = available_pages;
    memory->total_memory = total_memory;
    memory->total_pages = total_pages;
    memory->free_memory = last_available_adress - first_available_address;
    memory->locked_memory = 0;
    memory->reserved_memory = 0;

    memory->last_available_page_address = last_available_adress;
    memory->first_available_page_addr = first_available_address;

    if (!IS_ALIGNED(memory) || !IS_ALIGNED(memory->bitfield) || !IS_ALIGNED(memory->first_available_page_addr))
        panic("Memory alignment error!");

    uint64_t used_space = (bitfield_entries + sizeof(struct system_memory) + (3 << 12)) >> 12;
    reserve_pages((void*)0, used_space);

    return 1;
}