#ifndef _DEVICES_H
#define _DEVICES_H

#define DEVICE_MAX_DD 32
#define DEVICE_MAX_DEVICES 32
#define DD_NAME_MAX_SIZE 32
#define DEVICE_NAME_MAX_SIZE 32
#include <stdint.h>

//Estructuras
struct file_operations {
    uint64_t (*read)(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer);
    uint64_t (*write)(uint64_t port, uint64_t size, uint64_t skip, uint8_t* buffer);
    uint64_t (*ioctl)(uint64_t port, uint64_t op, void* buffer);
};

struct device_driver {
    struct file_operations * fops;
    uint8_t registered;
    char name[DD_NAME_MAX_SIZE];
};

struct device {
    uint8_t bc: 1; //0 = block, 1 = char
    uint8_t valid: 1;
    uint8_t major;
    uint8_t minor;
    char name[DEVICE_NAME_MAX_SIZE];
    uint64_t internal_id;
    void * device_control_structure;
    struct device * next;
};

void init_devices();

//Controladoras de dispositivos
void driver_list();
void driver_register_char(uint8_t major, const char* name, struct file_operations* fops);
void driver_register_block(uint8_t major, const char* name, struct file_operations* fops);
void driver_unregister_char(uint8_t major);
void driver_unregister_block(uint8_t major);

//Dispositivos
void device_list();
char * device_create(void * device_control_structure, uint8_t major, uint64_t id);
void device_destroy(char * device_name);

//Iterador
struct device* get_device_head();
uint32_t get_device_count();
uint32_t get_device_count_by_major(uint8_t major);
struct device* get_next_device(struct device* dev);

//Operaciones de control R|W|IOCTL
struct device* device_search(const char* name);
uint8_t device_identify(const char* name, char* driver_name);
uint64_t device_read(const char * name, uint64_t size, uint64_t offset, uint8_t * buffer);
uint64_t device_write(const char * name, uint64_t size, uint64_t offset, uint8_t * buffer);
uint64_t device_ioctl(const char * name, uint64_t op, void * buffer);

#endif