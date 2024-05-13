#include "heap.h"
#include "paging.h"
#include "memory.h"

//This code comes from https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/heap/heap.cpp
//Thank you Konect!!!
#define HEAP_SIGNATURE  0xcafebabe

#include "../util/printf.h"
#include "../util/string.h"
#include "../util/panic.h"

struct heap kernelGlobalHeap;
struct heap userGlobalHeap;

void * _malloc(struct heap * cheap, uint64_t size);
void expand_heap(struct heap * cheap, uint64_t length);

void _initHeap(struct heap * cheap, void* heapAddress, void* stackAddress) {
    cheap->heapEnd = heapAddress;
    cheap->lastStack = stackAddress;

    void * newPhysicalAddress = request_page();
    if (!newPhysicalAddress) {
        panic("Failed to allocate heap\n");
    }

    uint8_t perms = PAGE_WRITE_BIT;
    if (cheap == &userGlobalHeap) perms |= PAGE_USER_BIT;

    cheap->heapEnd = (void*)((uint64_t)cheap->heapEnd - PAGESIZE);
    map_current_memory(cheap->heapEnd, newPhysicalAddress);
    mprotect_current(cheap->heapEnd, PAGESIZE, perms);

    cheap->mainSegment = (struct heap_segment_header*)((uint64_t)cheap->heapEnd + ((uint64_t)PAGESIZE -sizeof(struct heap_segment_header)));
    cheap->mainSegment->length = 0;
    cheap->mainSegment->free = 0;
    cheap->mainSegment->isStack = 0;
    cheap->mainSegment->last = 0;
    cheap->mainSegment->signature = HEAP_SIGNATURE;
    cheap->mainSegment->next = (struct heap_segment_header*)((uint64_t)cheap->mainSegment - (uint64_t)PAGESIZE + sizeof(struct heap_segment_header));

    cheap->mainSegment->next->free = 1;
    cheap->mainSegment->next->isStack = 0;
    cheap->mainSegment->next->length = (uint64_t)PAGESIZE - sizeof(struct heap_segment_header) - sizeof(struct heap_segment_header);
    cheap->mainSegment->next->last = cheap->mainSegment;
    cheap->mainSegment->next->next = 0;
    cheap->mainSegment->next->signature = HEAP_SIGNATURE;
    cheap->lastSegment = cheap->mainSegment->next;

    cheap->totalSize += PAGESIZE;
    cheap->freeSize += PAGESIZE;
    cheap->ready = 1;
}

int heap_safeguard(struct heap * cheap) {
    return cheap->ready;
}

void *_calloc (struct heap * cheap, uint64_t num, uint64_t size) {
    if (!cheap->ready) return 0;
    void *ptr = _malloc(cheap, num * size);
    memset(ptr, 0, num * size);
    return ptr;
}

struct heap_segment_header* GetHeapSegmentHeader(void* address) {
    return (struct heap_segment_header*)(void*)((uint64_t)address - sizeof(struct heap_segment_header));
}

void dump_segment(struct heap_segment_header* segment) {
    printf("Segment %p:\n", segment);
    printf("Length: %d\n", segment->length);
    printf("Free: %d\n", segment->free);
    printf("Last: %p\n", segment->last);
    printf("Next: %p\n", segment->next);
    printf("Signature at: %p value: %x\n", &(segment->signature), segment->signature);
}

void debug_heap(struct heap * cheap) {
    printf("Heap debug:\n");
    printf("Total size: %d\n", cheap->totalSize);
    printf("Free size: %d\n", cheap->freeSize);
    printf("Used size: %d\n", cheap->usedSize);
    printf("Heap end: %p\n", cheap->heapEnd);
    printf("Main segment:\n");
    dump_segment(cheap->mainSegment);
    printf("Last segment:\n");
    dump_segment(cheap->lastSegment);
}

struct heap_segment_header* splitSegment(struct heap * cheap, struct heap_segment_header* segment, uint64_t size) {
    if (segment->length <= size + sizeof(struct heap_segment_header)) return 0;
    if (segment->signature != HEAP_SIGNATURE) {cheap->ready = 0; panic("Invalid heap segment signature split");}
    struct heap_segment_header* newSegment = (struct heap_segment_header*)(void*)((uint64_t)segment + segment->length - size);
    memset(newSegment, 0, sizeof(struct heap_segment_header));
    newSegment->free = 1;
    newSegment->length = size;
    newSegment->isStack = 0;
    newSegment->next = segment;
    newSegment->signature = HEAP_SIGNATURE;
    newSegment->last = segment->last;

    if (segment->next == 0) cheap->lastSegment = segment;
    if (segment->last != 0) segment->last->next = newSegment;
    segment->last = newSegment;
    segment->length -= size + sizeof(struct heap_segment_header);

    return newSegment;
}

void walk_heap(struct heap * cheap) {
    struct heap_segment_header* currentSegment = (struct heap_segment_header*)cheap->mainSegment;
    while(1) {
        if (currentSegment->signature != HEAP_SIGNATURE) {printf("***************** HEAP SIGNATURE INVALID BELOW *****************\n");}
        dump_segment(currentSegment);
        if (currentSegment->next == 0) break;
        currentSegment = currentSegment->next;
    }
}

void* _malloc(struct heap * cheap, uint64_t size) {
    if (!cheap->ready) return 0;
    if (size == 0) return 0;

    if ((size % 0x10) > 0) {
        size -= (size % 0x10);
        size += 0x10;
    }

    struct heap_segment_header* currentSegment = (struct heap_segment_header*)cheap->mainSegment;
    uint64_t sizeWithHeader = size + sizeof(struct heap_segment_header);

    while(1) {
        if (currentSegment->signature != HEAP_SIGNATURE) {cheap->ready = 0; walk_heap(cheap); panic("Invalid heap segment signature malloc");}
        if (currentSegment->free) {
            if (currentSegment->length > sizeWithHeader) {
                currentSegment = splitSegment(cheap, currentSegment, size);
                currentSegment->free = 0;
                cheap->usedSize += currentSegment->length + sizeof(struct heap_segment_header);
                cheap->freeSize -= currentSegment->length + sizeof(struct heap_segment_header);
                return (void*)((uint64_t)currentSegment + sizeof(struct heap_segment_header));
            } else if (currentSegment->length == size) {
                currentSegment->free = 0;
                cheap->usedSize += currentSegment->length + sizeof(struct heap_segment_header);
                cheap->freeSize -= currentSegment->length + sizeof(struct heap_segment_header);
                return (void*)((uint64_t)currentSegment + sizeof(struct heap_segment_header));
            }
        }
        if (currentSegment->next == 0) break;
        if (currentSegment->next == currentSegment) {cheap->ready = 0; panic("Current segment cycle\n");}
        currentSegment = currentSegment->next;
    }

    expand_heap(cheap, size);
    return _malloc(cheap, size);
}

void MergeThisToNext(struct heap_segment_header* header){
    struct heap_segment_header* next = header->next;
    next->length += header->length + sizeof(struct heap_segment_header);
    next->last = header->last;
    header->last->next = next;

    memset(header, 0, sizeof(struct heap_segment_header));
}

void MergeLastToThis(struct heap_segment_header* header){
    struct heap_segment_header* last = header->last;
    header->length += last->length + sizeof(struct heap_segment_header);
    header->last = last->last;
    header->last->next = header;

    memset(last, 0, sizeof(struct heap_segment_header));
}

void MergeLastAndThisToNext(struct heap_segment_header* header){
    MergeLastToThis(header);
    MergeThisToNext(header);
}

void _free(struct heap * cheap, void* address) {
    if (!cheap->ready) return 0;
    if (address == 0) return;

    struct heap_segment_header* header = (struct heap_segment_header*)((uint64_t)address - sizeof(struct heap_segment_header));
    if (header->signature != HEAP_SIGNATURE) {cheap->ready = 0; panic("Invalid free, signature");}
    if (header->free) {cheap->ready = 0; panic("Invalid free, double free");}
    header->free = 1;
    cheap->freeSize += header->length + sizeof(struct heap_segment_header);
    cheap->usedSize -= header->length + sizeof(struct heap_segment_header);

    if (header->next != 0 && header->last != 0) {
        if (header->next->free && header->last->free) {
            MergeLastAndThisToNext(header);
            return;
        }
    }

    if (header->last != 0) {
        if (header->last->free) {
            MergeLastToThis(header);
            return;
        }
    }

    if (header->next != 0) {
        if (header->next->free) {
            MergeThisToNext(header);
            return;
        }
    }
}

void* _realloc(struct heap * cheap, void* buffer, uint64_t size) {
    if (!cheap->ready) return 0;
    void* newBuffer = _malloc(cheap, size);
    if (newBuffer == 0) return 0;
    if (buffer == 0) return newBuffer;

    uint64_t oldSize = GetHeapSegmentHeader(buffer)->length; //TODO: Check if this is correct
    if (size < oldSize) oldSize = size;

    memcpy(newBuffer, buffer, oldSize);
    _free(cheap, buffer);

    return newBuffer;
}

void expand_heap(struct heap * cheap, uint64_t length) {
    length += sizeof(struct heap_segment_header);
    if (length % PAGESIZE) {
        length -= length % PAGESIZE;
        length += PAGESIZE;
    }

    //Divide rounded up length by page size
    uint64_t pages = length / PAGESIZE;
    if (length % PAGESIZE) pages++;

    uint8_t perms = PAGE_WRITE_BIT;
    if (cheap == &userGlobalHeap) perms |= PAGE_USER_BIT;

    for (uint64_t i = 0; i < pages; i++) {
        void * newPage = request_page();
        if (newPage == 0) {cheap->ready = 0; panic("Failed to expand heap");}
        cheap->heapEnd = (void*)((uint64_t)cheap->heapEnd - (uint64_t)PAGESIZE);
        map_current_memory(cheap->heapEnd, newPage);
        mprotect_current(cheap->heapEnd, PAGESIZE, perms);
    }

    struct heap_segment_header* newSegment = (struct heap_segment_header*)cheap->heapEnd;

    if (cheap->lastSegment != 0 && cheap->lastSegment->free && cheap->lastSegment->last != 0) {
        uint64_t size = cheap->lastSegment->length + length;
        newSegment->length = size - sizeof(struct heap_segment_header);
        newSegment->free = 1;
        newSegment->isStack = 0;
        newSegment->signature = HEAP_SIGNATURE;
        newSegment->last = cheap->lastSegment->last;
        newSegment->last->next = newSegment;
        newSegment->next = 0;
        cheap->lastSegment = newSegment;
    } else {
        newSegment->length = length - sizeof(struct heap_segment_header);
        newSegment->free = 1;
        newSegment->isStack = 0;
        newSegment->signature = HEAP_SIGNATURE;
        newSegment->last = cheap->lastSegment;
        newSegment->next = 0;
        if (cheap->lastSegment != 0) cheap->lastSegment->next = newSegment;
        cheap->lastSegment = newSegment;
    }

    cheap->totalSize += (length + sizeof(struct heap_segment_header));
    cheap->freeSize += (length + sizeof(struct heap_segment_header));
}

void* _stackalloc(struct heap * cheap, uint64_t length) {
    uint64_t pages = length / PAGESIZE;
    if (length % PAGESIZE) pages++;

    cheap->lastStack = (void*)((uint64_t)cheap->lastStack - (uint64_t)PAGESIZE);
    uint8_t perms = PAGE_WRITE_BIT;
    if (cheap == &userGlobalHeap) perms |= PAGE_USER_BIT;

    for (uint64_t i = 0; i < pages; i++) {
        void * newPage = request_page();
        if (newPage == 0) panic("Out of memory");
        cheap->lastStack = (void*)((uint64_t)cheap->lastStack - (uint64_t)PAGESIZE);
        map_current_memory(cheap->lastStack, newPage);
        mprotect_current(cheap->lastStack, PAGESIZE, perms);
    }

    void * address = cheap->lastStack;	

    cheap->lastStack = (void*)((uint64_t)cheap->lastStack - (uint64_t)PAGESIZE);

    return address;
}

void init_heap() {
    void * kernelHeapAddress =  (void*)0xffffffff60000000;
    void * kernelStackAddress = (void*)0xffffffff70000000;
    void * userHeapAddress =    (void*)0x00007fff60000000; //Unused
    void * userStackAddress =   (void*)0x00007fff70000000; //Unused
 
    _initHeap(&kernelGlobalHeap, kernelHeapAddress, kernelStackAddress);
    _initHeap(&userGlobalHeap, userHeapAddress, userStackAddress);
}

void * kmalloc(uint64_t size) {
    return _malloc(&kernelGlobalHeap, size);
}

void kfree(void* address) {
    _free(&kernelGlobalHeap, address);
}

void * kcalloc(uint64_t num, uint64_t size) {
    return _calloc(&kernelGlobalHeap, num, size);
}

void * krealloc(void* buffer, uint64_t size) {
    return _realloc(&kernelGlobalHeap, buffer, size);
}

void * kstackalloc(uint64_t length) {
    return _stackalloc(&kernelGlobalHeap, length);
}

void * umalloc(uint64_t size) {
    return _malloc(&userGlobalHeap, size);
}

void ufree(void* address) {
    _free(&userGlobalHeap, address);
}

void * ucalloc(uint64_t num, uint64_t size) {
    return _calloc(&userGlobalHeap, num, size);
}

void * urealloc(void* buffer, uint64_t size) {
    return _realloc(&userGlobalHeap, buffer, size);
}

void * ustackalloc(uint64_t length) {
    return _stackalloc(&userGlobalHeap, length);
}