#include "fifo.h"
#include "../../../memory/heap.h"
#include "../../../util/string.h"
#include "../../../util/printf.h"
#include "../../../drivers/fifo/fifo_dd.h"
#include "../../../drivers/fifo/fifo_interface.h"

struct vfs_fifo* fifo_register_device(const char * device, uint32_t mode, const char * mountpoint) {

    struct vfs_fifo* fifo = (struct vfs_fifo*)calloc(1, sizeof(struct vfs_fifo));
    if (!fifo) return 0;
    
    snprintf(fifo->name, 32, "%s", mountpoint);
    snprintf(fifo->device, 32, "%s", device);
    printf("FIFO: Mounting device %s at %s\n", device, mountpoint);

    return fifo;
}

void fifo_unregister_device(struct vfs_fifo* fifo) {
    if (!fifo) return;
    free(fifo);
}

uint8_t fifo_search(const char* name) {
    return (uint8_t)fifo_identify(name);
}

int fifo_sync(struct vfs_fifo* fifo) {
    return fifo_dev_flush(fifo->device);
}

void fifo_dump_device(struct vfs_fifo* fifo) {
    printf("FIFO: %s, dev: %s\n", fifo->name, fifo->device);
}

uint64_t vfs_fifo_get_size(struct vfs_fifo* fifo) {
    return fifo_get_size(fifo->device);
}

uint8_t vfs_fifo_read(struct vfs_fifo * fifo, uint8_t * destination_buffer, uint64_t size, uint64_t skip) {
    return fifo_read(fifo->device, destination_buffer, skip, size);
}
uint8_t vfs_fifo_write(struct vfs_fifo * fifo, uint8_t * source_buffer, uint64_t size, uint64_t skip) {
    return fifo_write(fifo->device, source_buffer, skip, size);
}