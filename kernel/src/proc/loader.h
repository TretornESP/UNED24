#ifndef _LOADER_H
#define _LOADER_H

#include <stdint.h>

void elf_readelf(uint8_t * buffer, uint64_t size);
uint8_t elf_load_elf(uint8_t * buffer, uint64_t size, void* env);

#endif