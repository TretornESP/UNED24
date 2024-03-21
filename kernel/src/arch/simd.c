#include "simd.h"
#include <stdint.h>

void init_simd(){
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~((uint64_t)CR0_EM);
    cr0 |= CR0_MONITOR_COPROC;
    cr0 |= CR0_NUMERIC_ERROR;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_FXSR;
    cr4 |= CR4_SIMD_EXCEPTION;
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    __asm__ volatile("finit");
}