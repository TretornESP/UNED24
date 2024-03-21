#include "printf.h"
void panic(const char * str) {
    printf("PANIC: %s\n", str);
    while (1);
}