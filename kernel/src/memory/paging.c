#include "paging.h"
#include "../bootservices/bootservices.h"
#include "../util/panic.h"
#include "../util/printf.h"
#include "../util/string.h"

#define CACHE_BIT_SET(x)((x & PAGE_CACHE_DISABLE) >> 3)
#define NX_BIT_SET(x)((x & PAGE_NX_BIT) >> 2)
#define USER_BIT_SET(x)((x & PAGE_USER_BIT) >> 1)
#define WRITE_BIT_SET(x)(x & PAGE_WRITE_BIT)


void _map_memory(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint8_t flags);
void map_memory_hh(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint8_t flags);
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

void * virtual_to_physical(struct page_directory * pml4, void* virtual) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual, &map);
    struct page_directory_entry pde;

    pde = pml4->entries[map.PDP_i];
    struct page_directory* pdp;
    if (!pde.present) {
        return NULL;
    } else {
        pdp = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pdp->entries[map.PD_i];
    struct page_directory* pd;
    if (!pde.present) {
        return NULL;
    } else {
        pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    }

    pde = pd->entries[map.PT_i];
    struct page_table* pt;
    if (!pde.present) {
        return NULL;
    } else {
        pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    }

    struct page_table_entry pte = pt->entries[map.P_i];
    if (!pte.present) {
        return NULL;
    }

    return (void*)((uint64_t)pte.page_ppn << 12);
}

struct page_directory* get_pml4() {
    struct page_directory* pml4;
    __asm__("movq %%cr3, %0" : "=r"(pml4));
    return TO_KERNEL_MAP(pml4);
}

void set_pml4(struct page_directory* pml4) {
    __asm__("movq %0, %%cr3" : : "r" (FROM_KERNEL_MAP(pml4)));
}

struct page_directory* _duplicate_pd(struct page_directory* father, uint64_t start, uint64_t end) {
    struct page_directory* pml = (struct page_directory*)request_page();
    if (pml == NULL) {
        panic("ERROR: Could not allocate page for new PML4\n");
    }
    memset(pml, 0, PAGESIZE);

    //Copy all the page tables in 4 levels
    for (uint64_t i = start; i < end; i++) {
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

struct page_directory* duplicate_current_pml4() {
    return _duplicate_pd(get_pml4(), 0, 512);
}

struct page_directory* duplicate_current_kernel_pml4() {
    return _duplicate_pd(get_pml4(), 256, 512);
}

void * request_current_page_at(void* vaddr, uint8_t flags) {
    struct page_directory *pml4 = get_pml4();
    void * result = request_page();
    if (result == NULL) {
        panic("ERROR: Could not allocate page for mapping\n");
    }
    map_memory_hh(pml4, vaddr, result, flags);
    return vaddr;
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

    //Copy all the page tables in 4 levels
    for (uint64_t i = 0; i < 512; i++) {
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
        _map_memory(pml4, TO_KERNEL_MAP(i), (void*)i, PAGE_WRITE_BIT);
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

void map_memory(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint8_t flags) {
    map_memory_hh(pml4, virtual_memory, physical_memory, flags);
}

void map_current_memory(void* virtual_memory, void* physical_memory, uint8_t flags) {
    struct page_directory* pml4 = get_pml4();
    map_memory_hh(pml4, virtual_memory, physical_memory, flags);
}

void map_memory_hh(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint8_t flags) {
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
        pdp = (struct page_directory*)TO_KERNEL_MAP(request_page());
        if (pdp == NULL) {
            panic("ERROR: Could not allocate page for new PDP\n");
        }
        memset(pdp, 0, PAGESIZE);
        pde.page_ppn = (uint64_t) FROM_KERNEL_MAP((uint64_t)pdp) >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pde.user_access = 1;
        pml4->entries[map.PDP_i] = pde;
    } else {
        pdp = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    }

    pde = ((struct page_directory*)pdp)->entries[map.PD_i];
    struct page_directory* pd;
    if (!pde.present) {
        pd = (struct page_directory*)TO_KERNEL_MAP(request_page());
        if (pd == NULL) {
            panic("ERROR: Could not allocate page for new PD\n");
        }
        memset(pd, 0, PAGESIZE);
        pde.page_ppn = (uint64_t) FROM_KERNEL_MAP((uint64_t)pd) >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pde.user_access = 1;
        ((struct page_directory*)pdp)->entries[map.PD_i] = pde;
    } else {
        pd = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    }

    pde = ((struct page_directory*)pd)->entries[map.PT_i];
    struct page_table* pt;
    if (!pde.present) {
        pt = (struct page_table*)TO_KERNEL_MAP(request_page());
        if (pt == NULL) {
            panic("ERROR: Could not allocate page for new PT\n");
        }
        memset(pt, 0, PAGESIZE);
        pde.page_ppn = (uint64_t) FROM_KERNEL_MAP((uint64_t)pt) >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pde.user_access = 1;
        ((struct page_directory*)pd)->entries[map.PT_i] = pde;
    } else {
        pt = (struct page_table*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    }

    struct page_table_entry pte = ((struct page_table*)pt)->entries[map.P_i];
    pte.page_ppn = (uint64_t)physical_memory >> 12;
    pte.present = 1;
    pte.writeable = WRITE_BIT_SET(flags);
    pte.user_access = USER_BIT_SET(flags);
    pte.execution_disabled = NX_BIT_SET(flags);
    pte.cache_disabled = CACHE_BIT_SET(flags);
    ((struct page_table*)pt)->entries[map.P_i] = pte;
}

void _map_memory(struct page_directory* pml4, void* virtual_memory, void* physical_memory, uint8_t flags) {

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
    pte.writeable = WRITE_BIT_SET(flags);
    pte.user_access = USER_BIT_SET(flags);
    pte.execution_disabled = NX_BIT_SET(flags);
    pte.cache_disabled = CACHE_BIT_SET(flags);
    pt->entries[map.P_i] = pte;
}

void _unmap_memory(struct page_directory* pml4, void* virtual_address) {
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);
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

void unmap_current_memory(void* virtual_address) {
    struct page_directory* pml4 = get_pml4();
    struct page_map_index map;
    address_to_map((uint64_t)virtual_address, &map);
    struct page_directory_entry pde;

    pde = pml4->entries[map.PDP_i];
    struct page_directory* pdp;
    if (!pde.present) {
        return;
    } else {
        pdp = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    }

    pde = pdp->entries[map.PD_i];
    struct page_directory* pd;
    if (!pde.present) {
        return;
    } else {
        pd = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    }

    pde = pd->entries[map.PT_i];
    struct page_table* pt;
    if (!pde.present) {
        return;
    } else {
        pt = (struct page_table*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    }

    struct page_table_entry pte = pt->entries[map.P_i];
    pte.present = 0;
    pt->entries[map.P_i] = pte;
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

void set_page_perms_hh(struct page_directory *pml4, void* address, uint8_t permissions) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
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

uint8_t get_page_perms_hh(struct page_directory *pml4, void* address) {
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    pd = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    pde = pd->entries[map.PD_i];
    pd = (struct page_directory*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    pde = pd->entries[map.PT_i];
    
    struct page_table *pt = (struct page_table*)((uint64_t)TO_KERNEL_MAP(((uint64_t)pde.page_ppn) << 12));
    struct page_table_entry pte = pt->entries[map.P_i];

    uint8_t result = pte.writeable;
    result |= (pte.user_access << 1);
    result |= (pte.execution_disabled << 2);
    result |= (pte.cache_disabled << 3);

    return result;
}

void * request_page_at(struct page_directory *pml4, void* vaddr, uint8_t flags) {
    void * result = request_page();
    if (result == NULL) {
        panic("ERROR: Could not allocate page for mapping\n");
    }
    map_memory_hh(pml4, vaddr, result, flags);
    return vaddr;
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

void mprotect_hh(struct page_directory *pml4, void* address, uint64_t size, uint8_t permissions) {
    uint64_t start = (uint64_t)address;
    uint64_t end = start + size;
    start = start & ~0xfff;
    end = (end + 0xfff) & ~0xfff;

    for (uint64_t i = start; i < end; i += 0x1000) {
        set_page_perms_hh(pml4, (void*)i, permissions);
    }
}

void mprotect_current(void* address, uint64_t size, uint8_t permissions) {
    struct page_directory *pml4 = get_pml4();
    mprotect_hh(pml4, address, size, permissions);
}

uint8_t is_present(struct page_directory* pml4, void * address) {
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
    printf("pte.present: %d\n", pte.present);
    return pte.present;
}

uint8_t is_user_access(struct page_directory* pml4, void * address) {
//Check the user bit for each level of directory also
    struct page_map_index map;
    address_to_map((uint64_t)address, &map);

    struct page_directory_entry pde;
    struct page_directory *pd;

    pde = pml4->entries[map.PDP_i];
    if (!pde.user_access) {
        printf("PDP not user accessible\n");
        return 0;
    }
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PD_i];
    if (!pde.user_access) {
        printf("PD not user accessible\n");
        return 0;
    }
    pd = (struct page_directory*)((uint64_t)pde.page_ppn << 12);
    pde = pd->entries[map.PT_i];
    if (!pde.user_access) {
        printf("PT not user accessible\n");
        return 0;
    }

    struct page_table *pt = (struct page_table*)((uint64_t)pde.page_ppn << 12);
    struct page_table_entry pte = pt->entries[map.P_i];
    printf("pte.user_access: %d\n", pte.user_access);
    return pte.user_access;
}

uint8_t is_executable(struct page_directory* pml4, void * address) {
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
    printf("pte.execution_disabled: %d\n", pte.execution_disabled);
    return !pte.execution_disabled;
}