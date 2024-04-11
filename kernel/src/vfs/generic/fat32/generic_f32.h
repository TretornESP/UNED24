#ifndef _GENERIC_F32_H
#define _GENERIC_F32_H
//Disable unused warnings 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../vfs_compat.h"
#include "fat32.h"
#include "../../../util/printf.h"

#define MAX_OPEN_DIRS 256
#define MAX_FILES_PER_DIR 4096
#define MAX_FAT32_PARTITIONS 32
struct f32_dir_usage {
    int in_use;
    struct dir_s dir;
};
struct f32_dir_usage f32_compat_open_dirs[MAX_OPEN_DIRS];

char f32_compat_partitions[MAX_FAT32_PARTITIONS] = {0};

int f32_compat_register_partition(const char* path, uint32_t lba, const char* mountpoint) {
    for (int index = 0; index < MAX_FAT32_PARTITIONS; index++) {
        if (f32_compat_partitions[index] == 0) {
            f32_compat_partitions[index] = register_fat32_partition(path, lba, mountpoint);
            return index;
        }
    }

    return -1;
}

uint8_t f32_compat_unregister_partition(int partition) {
    if (partition < 0 || partition >= MAX_FAT32_PARTITIONS)
        return 1;
    
    if (f32_compat_partitions[partition] == 0)
        return 1;

    unregstr_fat32_partition(f32_compat_partitions[partition]);
    f32_compat_partitions[partition] = 0;
    return 0;
}

uint8_t f32_compat_detect(const char* path, uint32_t lba) {
    return fat_search(path, lba);
}

int f32_compat_flush(int partition) {
    (void)partition;
    return 0;
}

int f32_compat_file_open(int partno, const char* path, int flags, int mode) {
    printf("VFS REQUESTED FAT32 FILE OPEN [partno: %d path: %s flags: %d mode: %d]\n", partno, path, flags, mode);
    return -1;
}
int f32_compat_file_close(int partno, int fd) {(void)partno; (void)fd; return -1;}
int f32_compat_file_creat(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
uint64_t f32_compat_file_read(int partno, int fd, void* buffer, uint64_t size) {(void)partno; (void)fd; (void)buffer; (void)size; return 1;}
uint64_t f32_compat_file_write(int partno, int fd, void* buffer, uint64_t size) {(void)partno; (void)fd; (void)buffer; (void)size; return 1;}
uint64_t f32_compat_file_seek(int partno, int fd, uint64_t offset, int whence) {(void)partno; (void)fd; (void) offset; (void)whence; return 1;}
uint64_t f32_compat_file_tell(int partno, int fd) {(void)partno; (void)fd; return 1;}
int f32_compat_stat(int partno, int fd, stat_t* st) {(void)partno; (void)fd; (void)st; return -1;}
int f32_compat_dir_open(int partno, const char* path) {(void)partno; (void)path; return -1;}
int f32_compat_dir_close(int partno, int dir) {(void)partno; (void)dir; return -1;}
int f32_compat_dir_load(int partno, int dir) {(void)partno; (void)dir; return -1;}
int f32_compat_dir_read(int partno, int dir, char* path, uint32_t * name_len, uint32_t * type) {(void)partno; (void)path; (void) dir; (void)name_len; (void)type; return -1;}
int f32_compat_dir_creat(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
int f32_compat_rename(int partno, const char* path, const char* newpath) {(void)partno; (void)path; (void)newpath; return -1;}
int f32_compat_remove(int partno, const char* path) {(void)partno; (void)path; return -1;}
int f32_compat_chmod(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
void f32_compat_debug(void) {}


struct vfs_compatible fat32_register = {
    .name = "FAT32",
    .majors = {0x8, 0x9, 0xa, 0xb, 0xc},
    .major_no = 5,
    .register_partition = f32_compat_register_partition,
    .unregister_partition = f32_compat_unregister_partition,
    .detect = f32_compat_detect,
    .flush = f32_compat_flush,
    .file_open = f32_compat_file_open,
    .dir_open = f32_compat_dir_open,
    .dir_close = f32_compat_dir_close,
    .dir_load = f32_compat_dir_load,
    .dir_read = f32_compat_dir_read,
    .dir_creat = f32_compat_dir_creat,
    .file_close = f32_compat_file_close,
    .file_creat = f32_compat_file_creat,
    .file_read = f32_compat_file_read,
    .file_write = f32_compat_file_write,
    .file_seek = f32_compat_file_seek,
    .file_tell = f32_compat_file_tell,
    .file_stat = f32_compat_stat,
    .rename = f32_compat_rename,
    .remove = f32_compat_remove,
    .chmod = f32_compat_chmod,
    .debug = f32_compat_debug
};

struct vfs_compatible * fat32_registrar = &fat32_register;

#endif