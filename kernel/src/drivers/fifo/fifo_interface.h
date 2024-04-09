#ifndef _FIFO_DRIVER_INTERFACE_H
#define _FIFO_DRIVER_INTERFACE_H
#include <stdint.h>


const char * new_fifo(uint64_t size);
void delete_fifo(const char * fifo_name);
#endif
