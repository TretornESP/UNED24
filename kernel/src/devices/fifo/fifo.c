#include "fifo.h"
#include "../../memory/heap.h"

struct fifo * fifo_alloc(uint64_t size) {
    struct fifo* fifo = (struct fifo*)malloc(sizeof(struct fifo));
    fifo->buffer = (uint8_t*)malloc(size);
    fifo->max_size = size;
    fifo->p = 0;
    fifo->q = 0;
    return fifo;
}
void fifo_free(struct fifo* fifo) {
    free(fifo->buffer);
    free(fifo);
}

uint64_t fifo_size(struct fifo* fifo) {
    return (fifo->p - fifo->q + fifo->max_size) % fifo->max_size;
}

void fifo_write(struct fifo* fifo, uint8_t data) {
    fifo->buffer[fifo->p] = data;
    fifo->p = (fifo->p + 1) % fifo->max_size;
}

uint8_t fifo_read(struct fifo* fifo) {
    uint8_t data = fifo->buffer[fifo->q];
    fifo->q = (fifo->q + 1) % fifo->max_size;
    return data;
}