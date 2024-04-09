#include "fifo_interface.h"
#include "fifo_dd.h"
#include "../../devices/devices.h"
#include "../../devices/fifo/fifo.h"

const char * new_fifo(uint64_t size) {
    struct fifo* fifo = fifo_alloc(size);
    if (!fifo) return 0;
    return device_create((void*)0, FIFO_DRIVER_MAJOR, (uint64_t)fifo);
}

void delete_fifo(const char * fifo_name) {
    (void)fifo_name;
    //Device search
    //device_destroy(fifo_name);
}