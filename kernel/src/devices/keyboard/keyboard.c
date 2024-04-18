#include "keyboard.h"
#include "../../io/interrupts.h"
#include "../../util/string.h"
#include "../../util/printf.h"
#include "../../memory/heap.h"

volatile struct keyboard *keyboard;
void (*keyboard_callback)(uint8_t);
char asciitable[] = {
    0,   0, '1', '2',
    '3', '4', '5', '6',
    '7', '8', '9', '0',
    '-', '=',   0,   0,
    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i',
    'o', 'p', '[', ']',
    0,   0, 'a', 's',
    'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';',
    '\'','`',   0,'\\',
    'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',',
    '.', '/',   0, '*',
    0, ' '
};

char translate(uint8_t scancode, uint8_t uppercase) {
    if (scancode > 58) return 0;
    if (uppercase) return keyboard->ASCII_table[scancode] - 32;

    return keyboard->ASCII_table[scancode];
}

void init_keyboard() {

    keyboard = (struct keyboard *)malloc(sizeof(struct keyboard));

    memset((void*)keyboard, 0, sizeof(struct keyboard));

    keyboard->ASCII_table = (char*)malloc(ASCII_SIZE);

    memset(keyboard->ASCII_table, 0, ASCII_SIZE);
    memcpy(keyboard->ASCII_table, asciitable, ASCII_SIZE);

    keyboard->left_shift_pressed = 0;
    keyboard->right_shift_pressed = 0;
    keyboard->intro_buffered = 0;
    keyboard_callback = 0;

}

void handle_keyboard(uint8_t scancode) {

    switch(scancode) {
        case Spacebar:
            printf(" ");
            if (keyboard_callback != 0) keyboard_callback(' ');
            return;
        case LeftShift:
            keyboard->left_shift_pressed = 1;
            return;
        case LeftShift+0x80:
            keyboard->left_shift_pressed = 0;
            return;
        case RightShift:
            keyboard->right_shift_pressed = 1;
            return;
        case RightShift+0x80:
            keyboard->right_shift_pressed = 0;
            return;
        case Enter:
            keyboard->intro_buffered = 1;
            printf("\n");
            if (keyboard_callback != 0) keyboard_callback(Enter);
            return;
        case Backspace:
            printf("\b \b");
            if (keyboard_callback != 0) keyboard_callback(Backspace);
            return;
    }

    char ascii = translate(scancode, keyboard->left_shift_pressed || keyboard->right_shift_pressed);
    if (ascii != 0) {
        if (keyboard_callback != 0) keyboard_callback(ascii);
        printf("%c", ascii);
    }
}

void register_callback(void (*callback)(uint8_t)) {
    keyboard_callback = callback;
}

void unregister_callback() {
    keyboard_callback = 0;
}

void halt_until_enter() {
    keyboard->intro_buffered = 0;
    printf("Press enter to continue...");
    while (1) {
        if (keyboard->intro_buffered) {
            keyboard->intro_buffered = 0;
            return;
        }
    }
}