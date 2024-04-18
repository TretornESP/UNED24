#include "scheduler.h"	
#include "../devices/devices.h"
#include "../util/string.h"

char current_tty[DEVICE_NAME_MAX_SIZE];
uint8_t current_tty_set = 0;

char * get_current_tty() {
    if (!current_tty_set) {
        return 0;
    }
    return current_tty;
}

void set_current_tty(char * tty) {
    current_tty_set = 1;
    strncpy(current_tty, tty, DEVICE_NAME_MAX_SIZE);
}