#include "fifo_dd.h"
#include "../../devices/devices.h"
#include "../../devices/fifo/fifo.h"

uint64_t fifo_dd_read(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    struct fifo* fifo = (struct fifo*)port;
    uint64_t i = 0;
    while ((i < size) && (fifo_size(fifo) > 0)) {
        if (skip > 0) {
            fifo_read(fifo);
            skip--;
            continue;
        }
        buffer[i] = fifo_read(fifo);
        i++;
    }

    return i;
}

uint64_t fifo_dd_write(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer) {
    (void)skip;
    struct fifo* fifo = (struct fifo*)port;
    uint64_t i = 0;
    while ((i < size) && (fifo_size(fifo) < fifo->max_size)) {
        fifo_write(fifo, buffer[i]);
        i++;
    }

    return i;
}

uint64_t fifo_dd_ioctl(uint64_t port, uint64_t op, void* buffer) {
    (void)buffer;
    struct fifo * fifo = (struct fifo*)port;

    switch (op) {            
        case IOCTL_FIFO_FLUSH:
            //fifo_flush(fifo);
            return 1;
        case IOCTL_FIFO_VALIDATE:
            return 1;
        case IOCTL_FIFO_GET_SIZE:
            return fifo_size(fifo);
        default:
            return 0;
    }

    return 0;
}

struct file_operations fifo_fops = {
    .read = fifo_dd_read,
    .write = fifo_dd_write,
    .ioctl = fifo_dd_ioctl
};

void init_fifo_dd() {
    driver_register_char(FIFO_DRIVER_MAJOR, FIFO_DRIVER_NAME, &fifo_fops);
}