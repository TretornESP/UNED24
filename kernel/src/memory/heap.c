#include "heap.h"
#include "paging.h"
#include "memory.h"

//This code comes from https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/heap/heap.cpp
//Thank you Konect!!!
#define HEAP_SIGNATURE  0xcafebabe

#include "../util/printf.h"
#include "../util/string.h"
#include "../util/panic.h"

struct heap globalHeap;
int ready = 0;

void initHeap(void* heapAddress, void* stackAddress, uint64_t pageCount) {
    globalHeap.heapEnd = heapAddress;
    globalHeap.lastStack = stackAddress;

    void * newPhysicalAddress = request_page();
    if (!newPhysicalAddress) {
        panic("Failed to allocate heap\n");
    }
    globalHeap.heapEnd = (void*)((uint64_t)globalHeap.heapEnd - PAGESIZE);
    map_current_memory(globalHeap.heapEnd, newPhysicalAddress);
    mprotect_current(globalHeap.heapEnd, PAGESIZE, PAGE_WRITE_BIT);

    globalHeap.mainSegment = (struct heap_segment_header*)((uint64_t)globalHeap.heapEnd + ((uint64_t)PAGESIZE -sizeof(struct heap_segment_header)));
    globalHeap.mainSegment->length = 0;
    globalHeap.mainSegment->free = 0;
    globalHeap.mainSegment->isStack = 0;
    globalHeap.mainSegment->last = 0;
    globalHeap.mainSegment->signature = HEAP_SIGNATURE;
    globalHeap.mainSegment->next = (struct heap_segment_header*)((uint64_t)globalHeap.mainSegment - (uint64_t)PAGESIZE + sizeof(struct heap_segment_header));

    globalHeap.mainSegment->next->free = 1;
    globalHeap.mainSegment->next->isStack = 0;
    globalHeap.mainSegment->next->length = (uint64_t)PAGESIZE - sizeof(struct heap_segment_header) - sizeof(struct heap_segment_header);
    globalHeap.mainSegment->next->last = globalHeap.mainSegment;
    globalHeap.mainSegment->next->next = 0;
    globalHeap.mainSegment->next->signature = HEAP_SIGNATURE;
    globalHeap.lastSegment = globalHeap.mainSegment->next;

    globalHeap.totalSize += PAGESIZE;
    globalHeap.freeSize += PAGESIZE;
}

int heap_safeguard() {
    return ready;
}

void *calloc (uint64_t num, uint64_t size) {
    if (!ready) return 0;
    void *ptr = malloc(num * size);
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

void debug_heap() {
    printf("Heap debug:\n");
    printf("Total size: %d\n", globalHeap.totalSize);
    printf("Free size: %d\n", globalHeap.freeSize);
    printf("Used size: %d\n", globalHeap.usedSize);
    printf("Heap end: %p\n", globalHeap.heapEnd);
    printf("Main segment:\n");
    dump_segment(globalHeap.mainSegment);
    printf("Last segment:\n");
    dump_segment(globalHeap.lastSegment);
}

struct heap_segment_header* splitSegment(struct heap_segment_header* segment, uint64_t size) {
    if (segment->length <= size + sizeof(struct heap_segment_header)) return 0;
    if (segment->signature != HEAP_SIGNATURE) {ready = 0; panic("Invalid heap segment signature split");}
    struct heap_segment_header* newSegment = (struct heap_segment_header*)(void*)((uint64_t)segment + segment->length - size);
    memset(newSegment, 0, sizeof(struct heap_segment_header));
    newSegment->free = 1;
    newSegment->length = size;
    newSegment->isStack = 0;
    newSegment->next = segment;
    newSegment->signature = HEAP_SIGNATURE;
    newSegment->last = segment->last;

    if (segment->next == 0) globalHeap.lastSegment = segment;
    if (segment->last != 0) segment->last->next = newSegment;
    segment->last = newSegment;
    segment->length -= size + sizeof(struct heap_segment_header);

    return newSegment;
}

void walk_heap() {
    struct heap_segment_header* currentSegment = (struct heap_segment_header*)globalHeap.mainSegment;
    while(1) {
        if (currentSegment->signature != HEAP_SIGNATURE) {printf("***************** HEAP SIGNATURE INVALID BELOW *****************\n");}
        dump_segment(currentSegment);
        if (currentSegment->next == 0) break;
        currentSegment = currentSegment->next;
    }
}

void* malloc(uint64_t size) {
    if (!ready) return 0;
    if (size == 0) return 0;

    if ((size % 0x10) > 0) {
        size -= (size % 0x10);
        size += 0x10;
    }

    struct heap_segment_header* currentSegment = (struct heap_segment_header*)globalHeap.mainSegment;
    uint64_t sizeWithHeader = size + sizeof(struct heap_segment_header);

    while(1) {
        if (currentSegment->signature != HEAP_SIGNATURE) {ready = 0; walk_heap(); panic("Invalid heap segment signature malloc");}
        if (currentSegment->free) {
            if (currentSegment->length > sizeWithHeader) {
                currentSegment = splitSegment(currentSegment, size);
                currentSegment->free = 0;
                globalHeap.usedSize += currentSegment->length + sizeof(struct heap_segment_header);
                globalHeap.freeSize -= currentSegment->length + sizeof(struct heap_segment_header);
                return (void*)((uint64_t)currentSegment + sizeof(struct heap_segment_header));
            } else if (currentSegment->length == size) {
                currentSegment->free = 0;
                globalHeap.usedSize += currentSegment->length + sizeof(struct heap_segment_header);
                globalHeap.freeSize -= currentSegment->length + sizeof(struct heap_segment_header);
                return (void*)((uint64_t)currentSegment + sizeof(struct heap_segment_header));
            }
        }
        if (currentSegment->next == 0) break;
        if (currentSegment->next == currentSegment) {ready = 0; panic("Current segment cycle\n");}
        currentSegment = currentSegment->next;
    }

    expand_heap(size);
    return malloc(size);
}

void MergeThisToNext(struct heap_segment_header* header){
    if (header->signature != HEAP_SIGNATURE) {ready = 0; panic("Invalid MergeThisToNext");}
    struct heap_segment_header* next = header->next;
    next->length += header->length + sizeof(struct heap_segment_header);
    next->last = header->last;
    header->last->next = next;

    memset(header, 0, sizeof(struct heap_segment_header));
}

void MergeLastToThis(struct heap_segment_header* header){
    if (header->signature != HEAP_SIGNATURE) {ready = 0; panic("Invalid MergeLastToThis");}
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

void free(void* address) {
    if (!ready) return 0;
    if (address == 0) return;

    struct heap_segment_header* header = (struct heap_segment_header*)((uint64_t)address - sizeof(struct heap_segment_header));
    if (header->signature != HEAP_SIGNATURE) {ready = 0; panic("Invalid free, signature");}
    if (header->free) {ready = 0; panic("Invalid free, double free");}
    header->free = 1;
    globalHeap.freeSize += header->length + sizeof(struct heap_segment_header);
    globalHeap.usedSize -= header->length + sizeof(struct heap_segment_header);

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

void* realloc(void* buffer, uint64_t size) {
    if (!ready) return 0;
    void* newBuffer = malloc(size);
    if (newBuffer == 0) return 0;
    if (buffer == 0) return newBuffer;

    uint64_t oldSize = GetHeapSegmentHeader(buffer)->length; //TODO: Check if this is correct
    if (size < oldSize) oldSize = size;

    memcpy(newBuffer, buffer, oldSize);
    free(buffer);

    return newBuffer;
}

void expand_heap(uint64_t length) {
    length += sizeof(struct heap_segment_header);
    if (length % PAGESIZE) {
        length -= length % PAGESIZE;
        length += PAGESIZE;
    }

    //Divide rounded up length by page size
    uint64_t pages = length / PAGESIZE;
    if (length % PAGESIZE) pages++;

    for (uint64_t i = 0; i < pages; i++) {
        void * newPage = request_page();
        if (newPage == 0) {ready = 0; panic("Failed to expand heap");}
        globalHeap.heapEnd = (void*)((uint64_t)globalHeap.heapEnd - (uint64_t)PAGESIZE);
        map_current_memory(globalHeap.heapEnd, newPage);
    }

    struct heap_segment_header* newSegment = (struct heap_segment_header*)globalHeap.heapEnd;

    if (globalHeap.lastSegment != 0 && globalHeap.lastSegment->free && globalHeap.lastSegment->last != 0) {
        uint64_t size = globalHeap.lastSegment->length + length;
        newSegment->length = size - sizeof(struct heap_segment_header);
        newSegment->free = 1;
        newSegment->isStack = 0;
        newSegment->signature = HEAP_SIGNATURE;
        newSegment->last = globalHeap.lastSegment->last;
        newSegment->last->next = newSegment;
        newSegment->next = 0;
        globalHeap.lastSegment = newSegment;
    } else {
        newSegment->length = length - sizeof(struct heap_segment_header);
        newSegment->free = 1;
        newSegment->isStack = 0;
        newSegment->signature = HEAP_SIGNATURE;
        newSegment->last = globalHeap.lastSegment;
        newSegment->next = 0;
        if (globalHeap.lastSegment != 0) globalHeap.lastSegment->next = newSegment;
        globalHeap.lastSegment = newSegment;
    }

    globalHeap.totalSize += (length + sizeof(struct heap_segment_header));
    globalHeap.freeSize += (length + sizeof(struct heap_segment_header));
}

void* stackalloc(uint64_t length) {
    uint64_t pages = length / PAGESIZE;
    if (length % PAGESIZE) pages++;

    globalHeap.lastStack = (void*)((uint64_t)globalHeap.lastStack - (uint64_t)PAGESIZE);
    
    for (uint64_t i = 0; i < pages; i++) {
        void * newPage = request_page();
        if (newPage == 0) panic("Out of memory");
        globalHeap.lastStack = (void*)((uint64_t)globalHeap.lastStack - (uint64_t)PAGESIZE);
        map_current_memory(globalHeap.lastStack, newPage);
        mprotect_current(globalHeap.lastStack, PAGESIZE, PAGE_WRITE_BIT);
    }

    void * address = globalHeap.lastStack;	

    globalHeap.lastStack = (void*)((uint64_t)globalHeap.lastStack - (uint64_t)PAGESIZE);

    return address;
}

void init_heap() {
    void * heapAddress = (void*)0xffffffff60000000;
    void * stackAddress = (void*)0xffffffff70000000;
    uint64_t pageCount = 0x0; //Unused
 
    initHeap(heapAddress, stackAddress, pageCount);
    ready = 1;
}