#ifndef _COMM_H
#define _COMM_H
#define SERIAL_RX_BUFFER_SIZE 4096
#define SERIAL_TX_BUFFER_SIZE 4096
#define SERIAL_MAJOR 0x8d
#define TTY_MAJOR 0xe
#define PS2_MOUSE_MAJOR 0x10
#define PS2_KEYBOARD_MAJOR 0x8f
#include <stdint.h>

void register_comm();

#endif