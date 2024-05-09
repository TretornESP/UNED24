#ifndef _RAW_H
#define _RAW_H
#include <stdint.h>

void load_and_execute(uint8_t* source_buffer, uint64_t vaddr, uint64_t size);
#endif