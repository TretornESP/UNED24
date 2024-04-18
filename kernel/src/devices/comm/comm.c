#include "comm.h"
#include "../../drivers/serial/serial.h"
#include "../../drivers/tty/tty.h"
#include "../devices.h"
#include "../../util/string.h"
#include "../../memory/heap.h"

void register_comm() {
    init_serial(SERIAL_RX_BUFFER_SIZE, SERIAL_TX_BUFFER_SIZE);
    int port_count = serial_count_ports();
    int *port_buffer = malloc(sizeof(int) * port_count);
    serial_get_ports(port_buffer);

    for (int i = 0; i < port_count; i++) {
        char* name = device_create((void*)0x0, SERIAL_MAJOR, port_buffer[i]);
        int index = tty_init(name, name, TTY_MODE_SERIAL, 1024, 1024);
        struct tty * tty = get_tty(index);
        if (is_valid_tty(tty)) {
            device_create((void*)tty, TTY_MAJOR, index);
        }
    }
    free(port_buffer);
}