#include "io.h"

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void insw(uint16_t port, uint8_t* buffer, int count) {
    __asm__ volatile("rep insw" :: "c"(count), "d"(port), "D"(buffer));
}

void outsw(uint16_t port, uint8_t *buffer, int count) {
    __asm__ volatile("rep outsw " : : "c"(count), "d"(port), "S"(buffer));
}

void io_wait() {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}