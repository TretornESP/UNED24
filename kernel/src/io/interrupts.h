#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H
#include "idt.h"

struct interrupt_frame;
__attribute__((interrupt)) void GenericException_Handler(struct interrupt_frame* frame);

void init_interrupts();
#endif