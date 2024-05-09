#include "shell.h"
#include "../devices/devices.h"
#include "../util/printf.h"
#include "../util/string.h"
#include "../util/panic.h"
#include "../sched/scheduler.h"
#include "../devices/pit/pit.h"
#include "../vfs/vfs_interface.h"
#include "../vfs/vfs.h"
#include "../vfs/generic/ext2/ext2.h"
#include "../memory/heap.h"
#include "../proc/loader.h"
#include "../proc/process.h"
#include "../proc/raw.h"
#include "../drivers/tty/tty.h"
#include "../drivers/fifo/fifo_interface.h"

struct command {
    char keyword[32];
    void (*handler)(int argc, char* argv[]);
};

void tty_signal_handler(void * ttyb, uint64_t event);
//void handler(void* ttyb, uint8_t event);

const char root[] = "hdap1";
char cwd[256] = {0};
char workpath[256] = {0};
char devno[32] = {0};

void test(int argc, char *argv[]) {
    printf("%s: %d arguments\n", argv[0], argc);
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
}

void lsdsk(int argc, char* argv[]) {
    vfs_lsdisk();
}

void lsblk(int argc, char* argv[]) {
    device_list(MODE_BLOCK);
}

void lschr(int argc, char* argv[]) {
    device_list(MODE_CHAR);
}

void attach(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Sets the output device for the debugger shell\n");
        printf("Usage: attach <tty>\n");
        return;
    }

    //device_ioctl(devno, 0x2, handler); //REMOVE SUBSCRIBER
    memset(devno, 0, 32);
    strcpy(devno, argv[1]);
    device_ioctl(devno, 0x1, tty_signal_handler); //ADD SUBSCRIBER
    
    set_current_tty(argv[1]);
    printf("Attached to %s\n", argv[1]);

}

void detach(int argc, char *argv[]) {
    reset_current_tty();
    printf("Reverted to default tty\n");
}

void readelf(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Reads the ELF header of a file\n");
        printf("Usage: readelf <file>\n");
        return;
    }

    int fd = vfs_file_open(argv[1], 0, 0);
    if (fd < 0) {
        printf("Could not open file %s\n", argv[1]);
        return;
    }

    vfs_file_seek(fd, 0, 0x2); //SEEK_END
    uint64_t size = vfs_file_tell(fd);
    vfs_file_seek(fd, 0, 0x0); //SEEK_SET

    uint8_t* buf = malloc(size);
    memset(buf, 0, size);
    vfs_file_read(fd, buf, size);
    vfs_file_close(fd);

    elf_readelf(buf, size);
    free(buf);
}

void loadelf(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Loads an ELF file into memory\n");
        printf("Usage: loadelf <file>\n");
        return;
    }

    int fd = vfs_file_open(argv[1], 0, 0);
    if (fd < 0) {
        printf("Could not open file %s\n", argv[1]);
        return;
    }

    vfs_file_seek(fd, 0, 0x2); //SEEK_END
    uint64_t size = vfs_file_tell(fd);
    vfs_file_seek(fd, 0, 0x0); //SEEK_SET

    uint8_t* buf = malloc(size);
    memset(buf, 0, size);
    vfs_file_read(fd, buf, size);
    vfs_file_close(fd);

    elf_load_elf(buf, size, 0);
    free(buf);
}

void loadraw(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Loads raw shellcode file into memory at address\n");
        printf("Usage: loadraw <file> <address:hex>\n");
        return;
    }

    int fd = vfs_file_open(argv[1], 0, 0);
    if (fd < 0) {
        printf("Could not open file %s\n", argv[1]);
        return;
    }

    vfs_file_seek(fd, 0, 0x2); //SEEK_END
    uint64_t size = vfs_file_tell(fd);
    vfs_file_seek(fd, 0, 0x0); //SEEK_SET

    uint8_t* buf = malloc(size);
    memset(buf, 0, size);
    vfs_file_read(fd, buf, size);
    vfs_file_close(fd);


    uint64_t addr = atou64(argv[2]);

    load_and_execute(buf, addr, size);
    free(buf);
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

void sched(int argc, char* argv[]) {
    if (argc < 1) {
        printf("Calls the scheduler\n");
        printf("Usage: sched\n");
        return;
    }

    yield();
}

void spawn(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Spawns a process WARNING: NICE IS IGNORED\n");
        printf("Usage: spawn <nice> <addr of init>\n");
        return;
    }

    //convert string to uint64_t
    char* endptr;
    long nice = atoi(argv[1]);
    (void)nice;
    uint64_t addr = strtoull(argv[2], &endptr, 16);
    add_task(create_task((void*)addr, get_current_tty()));
}

void kill(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Kills a process\n");
        printf("Usage: kill <pid>\n");
        return;
    }

    int16_t pid = atoi(argv[1]);
    kill_task(pid);
}

void ps(int argc, char* argv[]) {
    if (argc < 1) {
        printf("Lists processes\n");
        printf("Usage: ps\n");
        return;
    }

    pseudo_ps();
}

void ptoggle(int argc, char* argv[]) {
    if (argc < 1) {
        printf("Toggles preemption\n");
        printf("Usage: preempt\n");
        return;
    }

    preempt_toggle();
}

void writed(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Writes to a device\n");
        printf("Usage: write device text\n");
        return;
    }

    int fd = vfs_file_open(argv[1], 0, 0);
    if (fd < 0) {
        printf("Could not open device %s\n", argv[1]);
        return;
    }

    printf("Writing to %s\n", argv[1]);
    for (int i = 2; i < argc; i++) {
        vfs_file_write(fd, argv[i], strlen(argv[i]));
        vfs_file_write(fd, " ", 1);
    }

    vfs_file_close(fd);
}

void readd(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Reads n characters of a device\n");
        printf("Usage: read device [size] [offset]\n");
        return;
    }

    uint64_t size = 0;
    uint64_t offset = 0;

    int fd = vfs_file_open(argv[1], 0, 0);
    if (fd < 0) {
        printf("Could not open device %s\n", argv[1]);
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

    printf("Reading %d bytes from %s offset: %d\n", size, argv[1], offset);

    vfs_file_seek(fd, offset, 0x0); //SEEK_SET
    uint8_t* buf = malloc(size);
    memset(buf, 0, size);
    vfs_file_read(fd, buf, size);
    vfs_file_close(fd);
    buf[size] = '\0';    
    printf("str: %s\n", buf);
    printf("hex: ");
    for (uint64_t i = 0; i < size; i++) {
        printf("%02x ", buf[i]);
        if (i > 0 && i % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    free(buf);
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

void ioctl(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Creates a buffer of size <bsize> and calls <device>'s\n");
        printf("ioctl code <code> with the buffer as argument\n");
        printf("Then prints the buffer. If content is specified\n");
        printf("it will be copied to the buffer\n");
        printf("Usage: ioctl <device> <code> <bsize> <content...>\n");
        return;
    }

    uint64_t bsize = atou64(argv[3]);
    uint8_t* buffer = malloc(bsize);
    memset(buffer, 0, bsize);
    if (argc > 4) {
        for (int i = 4; i < argc; i++) {
            buffer[i-4] = atou8(argv[i]);
        }
        printf("Sending: %s\n", argv[4]);
        printf("Sending hex: ");
        for (int i = 0; i < argc - 4; i++) {
            printf("%02x ", buffer[i]);
            if (i > 0 && i % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }

    uint64_t result = device_ioctl(argv[1], atou64(argv[2]), buffer);
    printf("Ioctl complete, Result: %d\n", result);
    printf("Buffer (str): %s\n", buffer);
    printf("Buffer hex: ");
    for (uint64_t i = 0; i < bsize; i++) {
        printf("%02x ", buffer[i]);
        if (i > 0 && i % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    free(buffer);
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

void ticks(int argc, char* argv[]) {
    if (argc < 1) {
        printf("Prints the number of ticks since boot\n");
        printf("Usage: ticks\n");
        return;
    }

    printf("Ticks: %ld\n", get_ticks_since_boot());
}

void dc(int argc, char* argv[]);
void help(int argc, char* argv[]);

struct command cmdlist[] = {
    {
        .keyword = "test",
        .handler = test
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
        .keyword = "readelf",
        .handler = readelf
    },
    {
        .keyword = "ls",
        .handler = ls
    },
    {
        .keyword = "lsblk",
        .handler = lsblk
    },
    {
        .keyword = "lschr",
        .handler = lschr
    },
    {
        .keyword = "readd",
        .handler = readd
    },
    {
        .keyword = "writed",
        .handler = writed
    },
    {
        .keyword = "lsdsk",
        .handler = lsdsk
    },
    {
        .keyword = "loadelf",
        .handler = loadelf
    },
    {
        .keyword = "loadraw",
        .handler = loadraw
    },
    {
        .keyword = "ps",
        .handler = ps
    },
    {
        .keyword = "d",
        .handler = dc
    },
    {
        .keyword = "spawn",
        .handler = spawn
    },
    {
        .keyword = "sched",
        .handler = sched
    },
    {
        .keyword = "kill",
        .handler = kill
    },
    {
        .keyword = "ioctl",
        .handler = ioctl
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
    },
    {
        .keyword = "cd",
        .handler = cd
    },
    {
        .keyword = "preempt",
        .handler = ptoggle
    },
    {
        .keyword = "ticks",
        .handler = ticks
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

//TODO: Remove this, it's utterly stupid
void dc(int argc, char* argv[]) {
    char *av1[] = {"attach", "ttya"};
    int ac1 = 2;
    attach(ac1, av1);
    char *av2[] = {"loadelf", "hdap1/export/main.elf"};
    int ac2 = 2;
    loadelf(ac2, av2);
}

void promt() {
    printf("rotero@bloodmon:%s$ ", cwd);
}

void tty_signal_handler(void * ttyb, uint64_t event) {
    switch (event) {
        case 0x1: { //TTY_INB
            char cmd[1024] = {0};
            //Make sure cmd is aligned
            if ((uint64_t)cmd % 16 != 0) {
                panic("cmd not aligned");
            }

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

void print_prompt() {
    struct task * curr = get_current_task();
    int fd = curr->open_files[1];
    if (fd == -1) {
        return;
    }

    vfs_file_flush(fd);
    vfs_file_write(fd, "    ____  __    ____  ____  ____  __  _______  ____  _   __\n", 60);
    vfs_file_write(fd, "   / __ )/ /   / __ \\/ __ \\/ __ \\/  |/  / __ \\/ __ \\/ | / /\n", 60);
    vfs_file_write(fd, "  / __  / /   / / / / / / / / / / /|_/ / / / / / / /  |/ / \n", 60);
    vfs_file_write(fd, " / /_/ / /___/ /_/ / /_/ / /_/ / /  / / /_/ / /_/ / /|  /  \n", 60);
    vfs_file_write(fd, "/_____/_____/\\____/\\____/_____/_/  /_/\\____/\\____/_/ |_/   \n", 60);
    vfs_file_write(fd, "                                                           \n", 60);
    vfs_file_flush(fd);

    vfs_file_close(fd);
}

void kill_dbgshell() {
    device_ioctl(devno, 0x2, tty_signal_handler); //REMOVE SUBSCRIBER
    memset(devno, 0, 32);
}

//void process_commands() {
//    printf("\n");
//
//    char* args[32] = {0};
//    int argc = 0;
//    char* tok = strtok(buffer, " ");
//    while (tok != 0) {
//        args[argc] = tok;
//        argc++;
//        tok = strtok(0, " ");
//    }
//
//    for (uint32_t i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
//        if (strcmp(cmdlist[i].keyword, args[0]) == 0) {
//            cmdlist[i].handler(argc, args);
//            break;
//        }
//    }
//
//    promt();
//}
//
//void received_keypress(uint8_t c) {
//    if (c == Enter) {
//        buffer[buffer_index] = 0;
//        if (buffer_index == 0) {
//            promt();
//            return;
//        }
//        process_commands();
//        memset(buffer, 0, MAX_BUFFER_SIZE);
//        buffer_index = 0;
//    } else if (c == Backspace) {
//        if (buffer_index > 0) {
//            buffer[buffer_index-1] = 0;
//            buffer_index--;
//        }
//    } else {
//        buffer[buffer_index] = c;
//        buffer_index++;
//    }
//}

void run_shell() {
    const char * tty = get_current_tty();
    if (tty == 0) {
        return;
    }

    if (device_search(tty) == 0) {
        printf("Could not find device %s\n", tty);
        return;
    }

    strncpy(devno, tty, strlen(tty));
    device_ioctl(tty, 0x1, tty_signal_handler); //ADD SUBSCRIBER
    printf("\n");
    strcpy(cwd, root);

    //register_callback(received_keypress);
    //subscribe_signal(SIGKILL, kill_handler);
    promt();
    process_loop();
}