#include "shell.h"
#include "../memory/heap.h"
#include "../devices/devices.h"
#include "../devices/keyboard/keyboard.h"
#include "../devices/pit/pit.h"
#include "../util/printf.h"
#include "../util/string.h"

#define PROMPT "$"
#define MAX_BUFFER_SIZE 256
char buffer[MAX_BUFFER_SIZE];
int buffer_index = 0;

struct command {
    char keyword[32];
    void (*handler)(int argc, char* argv[]);
};

void help(int argc, char* argv[]);

//List of commands
void read(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: read <device> <size> [offset]\n");
        return;
    }

    uint64_t offset = (argc < 4) ? 0 : atoi(argv[3]);

    uint64_t size = atoi(argv[2]);

    uint8_t* buffer;
    if (size < 1024) 
        buffer = (uint8_t*)malloc(1024);
    else
        buffer = (uint8_t*)malloc((size + 511) & ~511);
        
    uint64_t read = device_read(argv[1], size, offset, buffer);

    if (read == 0) {
        printf("Error reading device\n");
    } else {
        printf("Read %d bytes\n", read);
    }

    for (uint64_t i = 0; i < read; i++) {
        printf("%c", buffer[i]);
    }

    printf("\nDump:");

    for (uint64_t i = 0; i < read; i++) {
        printf("%x ", buffer[i]);
        if (i % 16 == 0 && i != 0) {
            printf("\n");
        }
    }
    printf("\n");

    free(buffer);
}

void write(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: write <device> <text>\n");
        return;
    }

    uint64_t size = 0;
    uint64_t written = 0;
    for (int i = 2; i < argc; i++) {
        size += strlen(argv[i]);
    }

    uint8_t * buffer;
    if (size < 1024) 
        buffer = (uint8_t*)malloc(1024);
    else
        buffer = (uint8_t*)malloc((size + 511) & ~511);

    memset(buffer, 0, size);
    for (int i = 2; i < argc; i++) {
        memcpy(buffer + written, argv[i], strlen(argv[i]));
        written += strlen(argv[i]);
    }

    written = device_write(argv[1], size, 0, buffer);

    if (written == 0) {
        printf("Error writing device\n");
    } else {
        printf("Written %d bytes\n", written);
    }

    free(buffer);
}

void lsdev(int argc, char* argv[]) {
    device_list();
}

void lsdd(int argc, char* argv[]) {
    driver_list();
}

void hi(int argc, char* argv[]) {
    printf("Hello\n");
}

struct command cmdlist[] = {
    {
        .keyword = "hi",
        .handler = hi
    },
    {
        .keyword = "read",
        .handler = read
    },
    {
        .keyword = "write",
        .handler = write
    },
    {
        .keyword = "lsdev",
        .handler = lsdev
    },
    {
        .keyword = "lsdd",
        .handler = lsdd
    },
    {
        .keyword = "help",
        .handler = help
    }
};

void help(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Available commands:\n");
        for (long unsigned int i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
            printf("%s ", cmdlist[i].keyword);
            if (i > 1 && i % 5 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    } else {
        //Search for command, print all matches, even if partial
        for (long unsigned int i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
            if (strstr(cmdlist[i].keyword, argv[1]) != 0) {
                printf("%s\n", cmdlist[i].keyword);
                if (i > 1 && i % 5 == 0) {
                    printf("\n");
                }
            }
        }
    }
}

void process_commands() {
    printf("\n");

    char* args[32] = {0};
    int argc = 0;
    char* tok = strtok(buffer, " ");
    while (tok != 0) {
        args[argc] = tok;
        argc++;
        tok = strtok(0, " ");
    }

    for (uint32_t i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
        if (strcmp(cmdlist[i].keyword, args[0]) == 0) {
            cmdlist[i].handler(argc, args);
            break;
        }
    }

    printf(PROMPT);
}

void received_keypress(uint8_t c) {
    if (c == Enter) {
        buffer[buffer_index] = 0;
        if (buffer_index == 0) {
            printf(PROMPT);
            return;
        }
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