#include "shell.h"
#include "../memory/heap.h"
#include "../devices/devices.h"
#include "../devices/keyboard/keyboard.h"
#include "../devices/pit/pit.h"
#include "../util/printf.h"
#include "../util/string.h"
#include "../scheduling/scheduler.h"
#include "../vfs/vfs.h"
#include "../vfs/vfs_interface.h"
#include "../vfs/generic/ext2/ext2.h"

#define MAX_BUFFER_SIZE 256
char buffer[MAX_BUFFER_SIZE];
int buffer_index = 0;
const char root[] = "hdap2";
char cwd[256] = {0};
char workpath[256] = {0};
char devno[32] = {0};

void handler(void* ttyb, uint8_t event);
void received_keypress(uint8_t c);

struct command {
    char keyword[32];
    void (*handler)(int argc, char* argv[]);
};

void promt() {
    printf("root@unedos:%s$ ", cwd);
}

void help(int argc, char* argv[]);

//List of commands
void readd(int argc, char* argv[]) {
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

void ext2st(int argc, char* argv[]) {
    ext2_stacktrace();
}

void writed(int argc, char* argv[]) {
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

void lsdsk(int argc, char* argv[]) {
    vfs_lsdisk();
}

//This function receives a relative path and returns an absolute path
void apply_cd(char* path) {
    memset(workpath, 0, 256);
    if (path[0] == '/') {
        strcpy(workpath, path+1);
    } else {
        strcpy(workpath, cwd);
        strcat(workpath, "/");
        strcat(workpath, path);
    }
}

void ls(int argc, char*argv[]) {
    if (argc < 2) {
        printf("Lists the contents of a directory\n");
        printf("Usage: ls <directory>\n");
        return;
    }

    apply_cd(argv[1]);
    printf("Listing directory %s\n", workpath);
    vfs_dir_list(workpath);
}

void attach(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Attaches a device\n");
        printf("Usage: attach <device>\n");
        return;
    }

    if (device_search(argv[1]) == 0) {
        printf("Could not find device %s, to go default use detach cmd\n", argv[1]);
        return;
    }

    char * current_tty = get_current_tty();

    if ((current_tty == 0) || !strcmp(current_tty, "default")) {
        unregister_callback();
    } else {
        device_ioctl(current_tty, 0x2, handler); //REMOVE SUBSCRIBER
    }

    memset(devno, 0, 32);
    strncpy(devno, argv[1], strlen(argv[1]));
    device_ioctl(argv[1], 0x1, handler); //ADD SUBSCRIBER
    set_current_tty(argv[1]);

    printf("\n");
    promt();
}

void detach(int argc, char* argv[]) {

    if (device_search(devno) != 0) {
        device_ioctl(devno, 0x2, handler); //REMOVE SUBSCRIBER
    }

    memset(devno, 0, 32);
    register_callback(received_keypress);
    set_current_tty("default");

    printf("\n");
    promt();
}

void read(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Reads n characters of a file\n");
        printf("Usage: read <file> [size] [offset]\n");
        return;
    }

    uint64_t size = 0;
    uint64_t offset = 0;

    apply_cd(argv[1]);
    int fd = vfs_file_open(workpath, 0, 0);
    if (fd < 0) {
        printf("Could not open file %s\n", workpath);
        return;
    }

    if (argc > 3) {
        offset = atou64(argv[3]);
    }

    if (argc > 2) {
        size = atou64(argv[2]);
    } else {
        vfs_file_seek(fd, 0, 0x2); //SEEK_END
        size = vfs_file_tell(fd);
        vfs_file_seek(fd, 0, 0x0); //SEEK_SET
    }

    printf("Reading %d bytes from %s offset: %d\n", size, workpath, offset);

    vfs_file_seek(fd, offset, 0x0); //SEEK_SET
    uint8_t* buf = malloc(size);
    memset(buf, 0, size);
    vfs_file_read(fd, buf, size);
    vfs_file_close(fd);

    printf("%s\n", buf);
    free(buf);
}

void write(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Writes n characters to a file\n");
        printf("Usage: write <file>[:offset] <text...>\n");
        return;
    }

    uint64_t offset = 0;
    //Do not use strchr
    for (uint64_t i = 0; i < strlen(argv[1]); i++) {
        if (argv[1][i] == ':') {
            argv[1][i] = '\0';
            offset = atou64(argv[1] + i + 1);
            break;
        }
    }

    apply_cd(argv[1]);
    
    int fd = vfs_file_open(workpath, 0, 0);
    if (fd < 0) {
        printf("Could not open file %s\n", workpath);
        return;
    }

    printf("Writing to %s offset: %d\n", workpath, offset);
    printf("Text: ");
    for (int i = 2; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    vfs_file_seek(fd, offset, 0x0); //SEEK_SET
    for (int i = 2; i < argc; i++) {
        vfs_file_write(fd, argv[i], strlen(argv[i]));
        vfs_file_write(fd, " ", 1);
    }

    vfs_file_close(fd);
}

void create(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Creates a file\n");
        printf("Usage: create <file>\n");
        return;
    }

    apply_cd(argv[1]);
    int result = vfs_file_creat(workpath, 0);
    if (result < 0) {
        printf("Could not create file %s\n", workpath);
    }
}

void delete(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Deletes a file\n");
        printf("Usage: delete <file> [-force]\n");
        return;
    }

    uint8_t force = 0;
    if (argc > 2) {
        if (strcmp(argv[2], "-force") == 0)
            force = 1;
    }

    apply_cd(argv[1]);
    int result = vfs_remove(workpath, force);
    if (result < 0) {
        printf("Could not delete file %s\n", workpath);
    }
}

void mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Creates a directory\n");
        printf("Usage: mkdir <dir>\n");
        return;
    }

    apply_cd(argv[1]);
    int result = vfs_mkdir(workpath, 0);
    if (result < 0) {
        printf("Could not create directory %s\n", workpath);
    }
}

void normalizePath(char *path) {
    if (path == NULL || strlen(path) == 0) {
        return;
    }

    char normalizedPath[strlen(path) + 1];
    normalizedPath[0] = '\0';  // Initialize the normalizedPath

    char *token = strtok(path, "/");
    int depth = 0;

    while (token != NULL) {
        if (strcmp(token, "..") == 0) {
            if (depth > 0) {
                char *lastSlash = normalizedPath + strlen(normalizedPath) - 1;
                while (lastSlash > normalizedPath && *lastSlash != '/') {
                    lastSlash--;
                }
                if (lastSlash != normalizedPath) {
                    *lastSlash = '\0';
                    depth--;
                }
            }
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            if (depth > 0) {
                strcat(normalizedPath, "/");
            }
            strcat(normalizedPath, token);
            depth++;
        }

        token = strtok(NULL, "/");
    }

    if (strlen(normalizedPath) > 0 && normalizedPath[strlen(normalizedPath) - 1] == '/') {
        normalizedPath[strlen(normalizedPath) - 1] = '\0';
    }

    strcpy(path, normalizedPath);
}

void cd(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Changes the current directory\n");
        printf("Usage: cd <dir>\n");
        return;
    }

    apply_cd(argv[1]);
    memset(cwd, 0, 256);
    strcpy(cwd, workpath);
    //Make sure cwd ends in \0
    cwd[strlen(cwd)] = '\0';
    normalizePath(cwd);
}

struct command cmdlist[] = {
    {
        .keyword = "hi",
        .handler = hi
    },
    {
        .keyword = "readd",
        .handler = readd
    },
    {
        .keyword = "ext2st",
        .handler = ext2st
    },
    {
        .keyword = "writed",
        .handler = writed
    },
    {
        .keyword = "cd",
        .handler = cd
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
        .keyword = "attach",
        .handler = attach
    },
    {
        .keyword = "detach",
        .handler = detach
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
    },
    {
        .keyword = "lsdsk",
        .handler = lsdsk
    },
    {
        .keyword = "ls",
        .handler = ls
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
        .keyword = "create",
        .handler = create
    },
    {
        .keyword = "delete",
        .handler = delete
    },
    {
        .keyword = "mkdir",
        .handler = mkdir
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

    promt();
}

void handler(void* ttyb, uint8_t event) {
    (void)ttyb;
    switch (event) {
        case 0x1: { //TTY_INB
            char cmd[1024] = {0};
            int read = device_read(devno, 1024, 0, (uint8_t*)cmd);
            if (read > 0) {
                //Convert string to array of words
                char* args[32] = {0};
                int argc = 0;
                char* tok = strtok(cmd, " ");
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
            }

            promt();
        }
        default:
            break;
    }
}

void received_keypress(uint8_t c) {
    if (c == Enter) {
        buffer[buffer_index] = 0;
        if (buffer_index == 0) {
            promt();
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
    promt();
}