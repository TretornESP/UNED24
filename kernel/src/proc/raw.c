#include "raw.h"
#include "../memory/memory.h"
#include "../memory/paging.h"
#include "../util/string.h"
#include "../util/printf.h"

void load_and_execute(uint8_t* source_buffer, uint64_t vaddr, uint64_t size) {
    struct page_directory* pd = FROM_KERNEL_MAP(duplicate_current_pml4());
    struct page_directory* old_pd = swap_pml4(pd);

    uint64_t page_no = size / 0x1000;
    if (size % 0x1000) page_no++;

    uint8_t flags = PAGE_WRITE_BIT | PAGE_USER_BIT;
    for (uint64_t i = 0; i < page_no; i++) {
        request_current_page_at((void*)(vaddr + i*0x1000), flags);
    }

    memcpy((void*)vaddr, source_buffer, size);

    printf("Ready to execute at %p\n", (void*)vaddr);
    ((void (*)(void))vaddr)();
    printf("Process exited %p\n", (void*)vaddr);    

    swap_pml4(old_pd);

}
