#ifndef _IO_H
#define _IO_H
#include <stdint.h>

void outb(uint16_t, uint8_t);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);
uint8_t inb(uint16_t);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void insw(uint16_t port, uint8_t* buffer, int count);
void outsw(uint16_t port, uint8_t *buffer, int count);
void io_wait();
#endif