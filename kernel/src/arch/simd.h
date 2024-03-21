#ifndef _SIMD_H
#define _SIMD_H
#define CR0_MONITOR_COPROC (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_NUMERIC_ERROR (1 << 5)
#define CR4_FXSR (1 << 9)
#define CR4_SIMD_EXCEPTION (1 << 10)
void init_simd();
#endif