#ifndef _FIFO_H
#define _FIFO_H 

#include <stdint.h>

struct fifo {
    uint8_t * buffer;
    uint64_t p;
    uint64_t q;
    uint64_t max_size;
};

struct fifo * fifo_alloc(uint64_t size);
void fifo_free(struct fifo* fifo);

void fifo_write(struct fifo* fifo, uint8_t data);
uint8_t fifo_read(struct fifo* fifo);
uint64_t fifo_size(struct fifo* fifo);

#endif