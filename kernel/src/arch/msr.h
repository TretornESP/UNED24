#ifndef _MSR_H
#define _MSR_H
#include <stdint.h>
void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi);
void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi);
#endif