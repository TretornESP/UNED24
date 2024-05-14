#include "paging.h"
#include "../bootservices/bootservices.h"
#include "../util/panic.h"
#include "../util/printf.h"
#include "../util/string.h"

uint8_t vmmap_size = 4;
struct mapping vmmap[] = {
    {
        .vaddr = (void*)0x1000,
        .paddr = 0x0,
        .copy = 1,
        .size = 0x17fff000,
        .flags = 0x0
    },
    {
        .vaddr = (void*)0xffff800000000000,
        .paddr = 0x0,
        .copy = 1,
        .size = 0x18000000,
        .flags = 0x0
    },
    {
        .vaddr = (void*)0xffffffff80000000,
        .paddr = 0x0,
        .copy = 1,
        .size = 0x45000,
        .flags = 0x0
    },
    {
        .vaddr = (void*)0xffffffff80045000,
        .paddr = 0x0,
        .copy = 1,
        .size = 0x23c9000,
        .flags = 0x0
    }
};

uint8_t hw_buffer[65536] = {0};

void map_memory(struct page_directory* pml4, void* virtual_memory, void* physical_memory);
void map_memory_size(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint64_t size);
void mprotect(struct page_directory *pml4, void* address, uint64_t size, uint8_t permissions);
uint8_t get_page_perms(struct page_directory *pml4, void* address);
void set_page_perms(struct page_directory *pml4, void* address, uint8_t permissions);

void address_to_map(uint64_t address, struct page_map_index* map) {
    address >>= 12;
    map->P_i = address & 0x1ff;
    address >>= 9;
    map->PT_i = address & 0x1ff;
    address >>= 9;
    map->PD_i = address & 0x1ff;
    address >>= 9;    
    map->PDP_i = address & 0x1ff;
}

void print_pde(struct page_directory_entry * pde) {
    printf("p: %d w: %d u: %d wt: %d c: %d a: %d i3: %d s: %d i2: %d [pp: 0x%x] r: %d i1: %d n: %d\n",
        pde->present, pde->writeable, pde->user_access, pde->write_through, pde->cache_disabled,
        pde->accessed, pde->ignored_3, pde->size, pde->ignored_2, pde->page_ppn, pde->reserved_1,
        pde->ignored_1, pde->execution_disabled
    );
}

void print_pte(struct page_table_entry * pte) {
    printf("p: %d w: %d u: %d wt: %d c: %d a: %d d: %d s: %d g: %d i2: %d [pp: 0x%x] r: %d i1: %d n: %d\n",
        pte->present, pte->writeable, pte->user_access, pte->write_through, pte->cache_disabled,
        pte->accessed, pte->dirty, pte->size, pte->global, pte->ignored_2, pte->page_ppn, pte->reserved_1,
        pte->ignored_1, pte->execution_disabled
    );
}

uint64_t virtual_to_physical(struct page_directory *pml4, void* virtual_memory) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual_memory, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];
    
    uint64_t physical = ((uint64_t)pte.page_ppn << 12) | ((uint64_t)virtual_memory & 0xfff);
    return physical;
}

uint64_t virtual_to_physical_current(void* virtual_memory) {
    struct page_directory *pml4 = get_pml4();
    return virtual_to_physical(pml4, virtual_memory);
}

void debug_memory_map(struct page_directory *pml4, void* virtual_memory, void* physical_memory) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual_memory, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;
    printf("[MAP DEBUG] Virtual memory: 0x%llx Physical target: 0x%llx\n", (uint64_t)virtual_memory, (uint64_t)physical_memory);
    printf("MAP: PDP_i: %x PD_i: %x PT_i: %x P_i: %x\n", map.PDP_i, map.PD_i, map.PT_i, map.P_i);
    pde = pml4->entries[map.PDP_i];
    printf("PDP entry at: %p\n", &pml4->entries[map.PDP_i]);
    print_pde(&pde);
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    printf("PD entry at: %p\n", &pd->entries[map.PD_i]);
    print_pde(&pde);
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);

    pde = pd->entries[map.PT_i];
    printf("PT entry at: %p\n", &pd->entries[map.PT_i]);
    print_pde(&pde);
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];
    printf("P entry at: %p\n", &pt->entries[map.P_i]);
    print_pte(&pte);
    
    uint64_t physical = ((uint64_t)pte.page_ppn << 12) | ((uint64_t)virtual_memory & 0xfff);
    printf("Points to physical: %llx\n", physical);
    printf("I said it was:      %llx\n", physical_memory);

}

struct page_directory* get_pml4() {
    struct page_directory* pml4;
    __asm__("movq %%cr3, %0" : "=r"(pml4));
    return TO_KERNEL_MAP(pml4);
}

void set_pml4(struct page_directory* pml4) {
    __asm__("movq %0, %%cr3" : : "r" (FROM_KERNEL_MAP(pml4)));
}

struct page_directory* duplicate_current_pml4() {
    struct page_directory* father = get_pml4();
    struct page_directory* pml = (struct page_directory*)request_page();
    if (pml == NULL) {
        panic("ERROR: Could not allocate page for new PML4\n");
    }
    memset(pml, 0, PAGESIZE);
    memcpy(pml, father, PAGESIZE);

    //Copy all the page tables in 4 levels
    for (uint64_t i = 0; i < 512; i++) {
        struct page_directory_entry pde = father->entries[i];
        if (pde.present) {
            struct page_directory* pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
            struct page_directory* new_pd = (struct page_directory*)request_page();
            if (new_pd == NULL) {
                panic("ERROR: Could not allocate page for new PD\n");
            }
            memcpy(new_pd, pd, PAGESIZE);
            pde.page_ppn = (uint64_t)new_pd >> 12;
            pml->entries[i] = pde;

            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry pde = pd->entries[j];
                if (pde.present) {
                    struct page_table* pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                    struct page_table* new_pt = (struct page_table*)request_page();
                    if (new_pt == NULL) {
                        panic("ERROR: Could not allocate page for new PT\n");
                    }
                    memcpy(new_pt, pt, PAGESIZE);
                    pde.page_ppn = (uint64_t)new_pt >> 12;
                    new_pd->entries[j] = pde;

                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_table_entry pte = pt->entries[k];
                        if (pte.present) {
                            struct page_table_entry new_pte = pte;
                            new_pt->entries[k] = new_pte;
                        }
                    }
                }
            }
        }
    }
    return TO_KERNEL_MAP(pml);
}

struct page_directory* duplicate_kernel_space() {
    struct page_directory* father = get_pml4();
    struct page_directory* pml = (struct page_directory*)request_page();
    if (pml == NULL) {
        panic("ERROR: Could not allocate page for new PML4\n");
    }

    memset(pml, 0, PAGESIZE);
    memcpy(pml, father, PAGESIZE);

    //Copy the kernel space in 4 levels
    for (uint64_t i = 0; i < 256; i++) {
        struct page_directory_entry pde = father->entries[i];
        if (pde.present) {
            struct page_directory* pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
            struct page_directory* new_pd = (struct page_directory*)request_page();
            if (new_pd == NULL) {
                panic("ERROR: Could not allocate page for new PD\n");
            }
            memcpy(new_pd, pd, PAGESIZE);
            pde.page_ppn = (uint64_t)new_pd >> 12;
            pml->entries[i] = pde;

            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry pde = pd->entries[j];
                if (pde.present) {
                    struct page_table* pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                    struct page_table* new_pt = (struct page_table*)request_page();
                    if (new_pt == NULL) {
                        panic("ERROR: Could not allocate page for new PT\n");
                    }
                    memcpy(new_pt, pt, PAGESIZE);
                    pde.page_ppn = (uint64_t)new_pt >> 12;
                    new_pd->entries[j] = pde;

                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_table_entry pte = pt->entries[k];
                        if (pte.present) {
                            struct page_table_entry new_pte = pte;
                            new_pt->entries[k] = new_pte;
                        }
                    }
                }
            }
        }
    }

    return TO_KERNEL_MAP(pml);
}

void* swap_pml4(void* pml) {
    if (pml == NULL) {
        panic("ERROR: Tried to swap to NULL PML4\n");
    }
    void* old = (void*)get_pml4();

    //Check if pml is different from current
    if (old == pml) {
        return old;
    }
    set_pml4(pml);
    return old;
}

void invalidate_current_pml4() {
    struct page_directory* pml4 = get_pml4();
    set_pml4(pml4);
}

void init_paging() {
    struct page_directory * pml4 = (struct page_directory*)request_page();
    if (pml4 == NULL) {
        panic("ERROR: Could not allocate page for PML4\n");
    }
    struct page_directory * original;
    __asm__("movq %%cr3, %0" : "=r"(original));


    memset(pml4, 0, PAGESIZE);

    //Copy the upper half of original into new pml4 in 4 levels
    for (uint64_t i = 256; i < 512; i++) {
        struct page_directory_entry pde = original->entries[i];
        if (pde.present) {
            struct page_directory* pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
            struct page_directory* new_pd = (struct page_directory*)request_page();
            if (new_pd == NULL) {
                panic("ERROR: Could not allocate page for new PD\n");
            }
            memcpy(new_pd, pd, PAGESIZE);
            pde.page_ppn = (uint64_t)new_pd >> 12;
            pml4->entries[i] = pde;

            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry pde = pd->entries[j];
                if (pde.present) {
                    struct page_table* pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                    struct page_table* new_pt = (struct page_table*)request_page();
                    if (new_pt == NULL) {
                        panic("ERROR: Could not allocate page for new PT\n");
                    }
                    memcpy(new_pt, pt, PAGESIZE);
                    pde.page_ppn = (uint64_t)new_pt >> 12;
                    new_pd->entries[j] = pde;

                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_table_entry pte = pt->entries[k];
                        if (pte.present) {
                            struct page_table_entry new_pte = pte;
                            new_pt->entries[k] = new_pte;
                        }
                    }
                }
            }
        }
    }
    
    struct system_memory * memory = get_memory();
    uint64_t first_page = 0;
    uint64_t last_page = memory->last_available_page_address; 

    for (uint64_t i = first_page; i < last_page; i += 0x1000) {
        map_memory(pml4, TO_KERNEL_MAP(i), (void*)i);
        mprotect(pml4, TO_KERNEL_MAP(i), 0x1000, PAGE_WRITE_BIT | PAGE_USER_BIT);
    }
    
    printf("Mapped from: %llx to %llx\n", first_page, last_page);
    printf("To: %llx to %llx\n", TO_KERNEL_MAP(first_page), TO_KERNEL_MAP(last_page));
    
    __asm__("movq %0, %%cr3" : : "r" (pml4));

    uint64_t virtual_start = get_kernel_address_virtual();
    uint64_t linker_kstart = (uint64_t)&KERNEL_START;

    if (linker_kstart != virtual_start) {
        printf("Crashing: KERNEL_START: %llx VIRT_ADDR: %p\n", linker_kstart, virtual_start);
        panic("init_paging: kernel virtual address does not match KERNEL_START");
    }

    offset_memory((void*)IDENTITY_MAP_START);
}

void map_current_memory(void* virtual_memory, void* physical_memory) {
    struct page_directory* pml4 = get_pml4();
    map_memory(pml4, virtual_memory, physical_memory);
}

void map_current_memory_size(void* virtual_memory, void* physical_memory, uint64_t size) {
    struct page_directory* pml4 = get_pml4();
    map_memory_size(pml4, virtual_memory, physical_memory, size);
}

void map_memory_size(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint64_t size) {
    uint64_t start = (uint64_t)virtual_memory;
    uint64_t end = start + size;
    start = start & ~0xfff;
    end = (end + 0xfff) & ~0xfff;

    for (uint64_t i = start; i < end; i += 0x1000) {
        map_memory(pml4, (void*)i, physical_memory);
        physical_memory += 0x1000;
    }
}

void map_memory(struct page_directory* pml4, void* virtual_memory, void* physical_memory) {

    if ((uint64_t)virtual_memory & 0xfff) {
        printf("Crashing: virtual_memory: %p\n", virtual_memory);
        panic("map_memory: virtual_memory must be aligned to 0x1000");
    }

    if ((uint64_t)physical_memory & 0xfff) {
        printf("Crashing: physical_memory: %p\n", physical_memory);
        panic("map_memory: physical_memory must be aligned to 0x1000");
    }

    struct page_map_index map;
    address_to_map((uint64_t)virtual_memory, &map);
    struct page_directory_entry pde;

    pde = pml4->entries[map.PDP_i];
    struct page_directory* pdp;
    if (!pde.present) {
        pdp = (struct page_directory*)request_page();
        if (pdp == NULL) {
            panic("ERROR: Could not allocate page for new PDP\n");
        }
        memset(pdp, 0, PAGESIZE);
        pde.page_ppn = (uint64_t) pdp >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pde.user_access = 1;
        pml4->entries[map.PDP_i] = pde;
    } else {
        pdp = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pdp->entries[map.PD_i];
    struct page_directory* pd;
    if (!pde.present) {
        pd = (struct page_directory*)request_page();
        if (pd == NULL) {
            panic("ERROR: Could not allocate page for new PD\n");
        }
        memset(pd, 0, PAGESIZE);
        pde.page_ppn = (uint64_t) pd >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pde.user_access = 1;
        pdp->entries[map.PD_i] = pde;
    } else {
        pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pd->entries[map.PT_i];
    struct page_table* pt;
    if (!pde.present) {
        pt = (struct page_table*)request_page();
        if (pt == NULL) {
            panic("ERROR: Could not allocate page for new PT\n");
        }
        memset(pt, 0, PAGESIZE);
        pde.page_ppn = (uint64_t) pt >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pde.user_access = 1;
        pd->entries[map.PT_i] = pde;
    } else {
        pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    }

    struct page_table_entry pte = pt->entries[map.P_i];
    pte.page_ppn = (uint64_t)physical_memory >> 12;
    pte.present = 1;
    pte.writeable = 1;
    pte.user_access = 1;
    pt->entries[map.P_i] = pte;
}

void unmap_memory(struct page_directory* pml4, void* virtual_memory) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual_memory, &map);
    struct page_directory_entry pde;

    pde = pml4->entries[map.PDP_i];
    struct page_directory* pdp;
    if (!pde.present) {
        return;
    } else {
        pdp = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pdp->entries[map.PD_i];
    struct page_directory* pd;
    if (!pde.present) {
        return;
    } else {
        pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pd->entries[map.PT_i];
    struct page_table* pt;
    if (!pde.present) {
        return;
    } else {
        pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    }

    struct page_table_entry pte = pt->entries[map.P_i];
    pte.present = 0;
    pt->entries[map.P_i] = pte;
}

void * get_hw_page() {
    static uint32_t index = 0;
    uint32_t start = index;
    while (hw_buffer[index] != 0) {
        index++;
        if (index >= 65536) {
            index = 0;
        }
        if (index == start) {
            panic("ERROR: No more hardware pages available\n");
        }
    }

    hw_buffer[index] = 1;
    return (void*)(HW_ADDRESS_START + (index * PAGESIZE));
}

void unmap_current_memory(void* virtual_address) {
    struct page_directory* pml4 = get_pml4();
    unmap_memory(pml4, virtual_address);
}

void set_page_perms(struct page_directory *pml4, void* address, uint8_t permissions) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry *pte = &pt->entries[map.P_i];

    pte->writeable = (permissions & 1);
    pte->user_access = ((permissions & 2) >> 1);
    pte->execution_disabled = ((permissions & 4) >> 2);
    pte->cache_disabled = ((permissions & 8) >> 3);
}

uint8_t get_page_perms(struct page_directory *pml4, void* address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];

    uint8_t result = pte.writeable;
    result |= (pte.user_access << 1);
    result |= (pte.execution_disabled << 2);
    result |= (pte.cache_disabled << 3);

    return result;
}

void page_set(struct page_directory *pml4, void* address, uint8_t field) {
    uint8_t perms = get_page_perms(pml4, address);
    perms |= field;
    set_page_perms(pml4, address, perms);
}

void page_clear(struct page_directory *pml4, void* address, uint8_t field) {
    uint8_t perms = get_page_perms(pml4, address);
    perms &= ~field;
    set_page_perms(pml4, address, perms);
}

void * request_page_identity() {
    void * result = request_page();
    if (result == NULL) {
        panic("ERROR: Could not allocate page for identity mapping\n");
    }
    return TO_KERNEL_MAP(result);
}

void * request_current_page_identity() {
    return request_page_identity();
}

void * request_current_pages_identity(uint64_t count) {
    struct page_directory *pml4 = get_pml4();
    void * result = request_contiguous_pages(count);
    if (result == NULL) {
        panic("ERROR: Could not allocate pages for identity mapping\n");
    }
    map_memory_size(pml4, result, result, count * PAGESIZE);
    return result;
}

void * request_page_at(struct page_directory *pml4, void* vaddr) {
    void * result = request_page();
    if (result == NULL) {
        panic("ERROR: Could not allocate page for mapping\n");
    }
    map_memory(pml4, vaddr, result);
    return vaddr;
}

void * request_current_page_at(void* vaddr) {
    struct page_directory *pml4 = get_pml4();
    return request_page_at(pml4, vaddr);
}

void * request_accessible_page_at(struct page_directory* pml4, void* vaddr, void * access_pointer) {
    void * result = request_page();
    if (result == NULL) {
        panic("ERROR: Could not allocate page for mapping\n");
    }
    map_memory(pml4, vaddr, result);
    map_current_memory(access_pointer, result);

    return access_pointer;
}

void * request_current_accessible_page_at(void* vaddr, void * access_pointer) {
    struct page_directory *pml4 = get_pml4();
    return request_accessible_page_at(pml4, vaddr, access_pointer);
}

void mprotect(struct page_directory *pml4, void* address, uint64_t size, uint8_t permissions) {
    uint64_t start = (uint64_t)address;
    uint64_t end = start + size;
    start = start & ~0xfff;
    end = (end + 0xfff) & ~0xfff;

    for (uint64_t i = start; i < end; i += 0x1000) {
        set_page_perms(pml4, (void*)i, permissions);
    }
}

void mprotect_current(void* address, uint64_t size, uint8_t permissions) {
    struct page_directory *pml4 = get_pml4();
    mprotect(pml4, address, size, permissions);
}

void dump_mappings() {
    struct page_directory *pml4 = get_pml4();
    //Four levels of page tables
    for (uint64_t i = 0; i < 512; i++) {
        struct page_directory_entry pde = pml4->entries[i];
        if (pde.present) {
            struct page_directory *pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
            for (uint64_t j = 0; j < 512; j++) {
                struct page_directory_entry pde = pd->entries[j];
                if (pde.present) {
                    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
                    for (uint64_t k = 0; k < 512; k++) {
                        struct page_table_entry pte = pt->entries[k];
                        if (pte.present) {
                            printf("PML4: %d PD: %d PT: %d P: %d -> 0x%llx", i, j, k, pte.page_ppn, (uint64_t)pte.page_ppn << 12);
                            printf(" p: %d w: %d u: %d wt: %d c: %d a: %d d: %d s: %d g: %d i2: %d r: %d i1: %d n: %d\n",
                                pte.present, pte.writeable, pte.user_access, pte.write_through, pte.cache_disabled,
                                pte.accessed, pte.dirty, pte.size, pte.global, pte.ignored_2, pte.reserved_1,
                                pte.ignored_1, pte.execution_disabled
                            );
                        }
                    }
                }
            }
        }
    }
}