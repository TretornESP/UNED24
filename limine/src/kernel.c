#include "util/string.h"
#include "bootservices/bootservices.h"

void _start(void) {
    void (*writer)(const char*, uint64_t) = get_terminal_writer();
    char string[] = "Hello World!\n";
    writer(string, strlen(string));

    while(1);
}