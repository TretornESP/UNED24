#include "shell.h"
#include "../devices/keyboard/keyboard.h"
#include "../util/printf.h"
#include "../util/string.h"

#define PROMPT "$"
#define MAX_BUFFER_SIZE 256
char buffer[MAX_BUFFER_SIZE];
int buffer_index = 0;

void process_commands() {
    printf("\n");
    if (!strcmp(buffer, "help")) {
        printf("Available commands:\n");
        printf("help: Shows this help message\n");
        printf("hi: Says hi\n");
    } else if (!strcmp(buffer, "hi")) {
        printf("Hi!\n");
    } else {
        printf("Command not found: %s\n", buffer);
    }
    printf(PROMPT);
}

void received_keypress(uint8_t c) {
    if (c == Enter) {
        buffer[buffer_index] = 0;
        process_commands();
        memset(buffer, 0, MAX_BUFFER_SIZE);
        buffer_index = 0;
    } else if (c == Backspace) {
        if (buffer_index > 0) {
            buffer[buffer_index-1] = 0;
            buffer_index--;
        }
    } else {
        buffer[buffer_index] = c;
        buffer_index++;
    }
}

void run_shell() {
    register_callback(received_keypress);
    printf(PROMPT);
}