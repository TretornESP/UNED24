#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

//This code comes from https://github.com/kot-org/Kot/blob/main/Sources/Kernel/Src/heap/heap.h
//Thank you Konect!!!

struct heap_segment_header {
    uint8_t free;
    uint64_t length;
    struct heap_segment_header* next;
    struct heap_segment_header* last;
    uint8_t isStack;
    uint32_t signature;
} __attribute__((aligned(0x10)));

struct heap {
    void* heapEnd;
    void* lastStack;
    struct heap_segment_header* lastSegment;
    struct heap_segment_header* mainSegment;
    uint64_t totalSize;
    uint64_t usedSize;
    uint64_t freeSize;
};

extern struct heap globalHeap;

void init_heap();
int heap_safeguard();
void* malloc(uint64_t size);
void* realloc(void* buffer, uint64_t size);
void free(void* address);
void expand_heap(uint64_t length);
void *calloc (uint64_t num, uint64_t size);
void* stackalloc(uint64_t length);
void debug_heap();
void walk_heap();
#endif