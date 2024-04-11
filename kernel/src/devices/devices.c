#include "devices.h"
#include "acpi/acpi.h"
#include "pci/pci.h"
#include "../memory/heap.h"
#include "../memory/memory.h"
#include "../util/string.h"
#include "../util/printf.h"

struct device_driver char_device_drivers[DEVICE_MAX_DD] = {0};
struct device_driver block_device_drivers[DEVICE_MAX_DD] = {0};
struct device *device_header = 0;

//Dispositivos de bloque tienen majors 0x0 a 0x7F
//Dispositivos de caracter tienen majors 0x80 a 0xFF

const char* device_identifiers[] = {
    "none", // 0
    "none", // 1
    "none", // 2
    "none", // 3
    "none", // 4
    "none", // 5
    "none", // 6
    "none", // 7
    "hd",   // 8
    "cd",   // 9
    "semb", // a
    "pm",   // b
    "sd",   // c
    "none", // d
    "tty", // e
    "none", // f
    "mouse", // 10
    "net", // 11
    "fb", // 12
    "none", // 13 
    [0x8d] = "magic", // 80
    [0x8e] = "fifo",
    [0x8f] = "kbd", // 81
};


void driver_list() {
    printf("### CHAR DEVICES ###\n");
    for (int i = 0; i < DEVICE_MAX_DD; i++) {
        if (char_device_drivers[i].registered) {
            printf("MAJ: %x, NAME: %s\n", i | 0x80, char_device_drivers[i].name);
        }
    }
    printf("### BLOCK DEVICES ###\n");
    for (int i = 0; i < DEVICE_MAX_DD; i++) {
        if (block_device_drivers[i].registered) {
            printf("MAJ: %x, NAME: %s\n", i, block_device_drivers[i].name);
        }
    }
}

void driver_register_char(uint8_t major, const char* name, struct file_operations* fops) {
    uint8_t major_id = major & 0x7F;
    memset(char_device_drivers[major_id].name, 0, DEVICE_NAME_MAX_SIZE);
    strncpy(char_device_drivers[major_id].name, name, strlen(name));
    char_device_drivers[major_id].fops = fops;
    char_device_drivers[major_id].registered = 1;

    printf("Registered char device: %s [MAJ: %x]\n", name, major);

}

void driver_register_block(uint8_t major, const char* name, struct file_operations* fops) {
    memset(block_device_drivers[major].name, 0, DEVICE_NAME_MAX_SIZE);
    strncpy(block_device_drivers[major].name, name, strlen(name));
    block_device_drivers[major].fops = fops;
    block_device_drivers[major].registered = 1;

    printf("Registered block device: %s [MAJ: %x]\n", name, major);

}

void driver_unregister_char(uint8_t major) {
    uint8_t major_id = major & 0x7F;
    char_device_drivers[major_id].registered = 0;
    printf("Unregistered char device: %s [MAJ: %x]\n", char_device_drivers[major_id].name, major);

}
void driver_unregister_block(uint8_t major) {
    block_device_drivers[major].registered = 0;
    printf("Unregistered block device: %s [MAJ: %x]\n", block_device_drivers[major].name, major);
}

//Dispositivos
void device_list() {
    struct device* device = device_header;
    while (device->next != 0) {
        printf("Device: %s [MAJ: %x MIN: %x ID: %x CTRL: %x]\n", device->name, device->bc << 7 | device->major, device->minor, device->internal_id, device->device_control_structure);
        device = device->next;
    }
}

struct device* get_device_head() {
    if (device_header->valid)
        return device_header;
    return (struct device*)0;
}

uint32_t get_device_count() {
    uint32_t count = 0;
    struct device* dev = device_header;
    while (dev->valid) {
        count++;
        dev = dev->next;
    }
    return count;
}

uint32_t get_device_count_by_major(uint8_t major) {
    uint32_t count = 0;
    struct device* dev = device_header;
    while (dev->valid) {
        if ((uint8_t)(dev->bc << 7 | dev->major) == major)
            count++;
        dev = dev->next;
    }
    return count;
}

struct device* get_next_device(struct device* dev) {
    if (dev->next->valid)
        return dev->next;
    return (struct device*)0;
}

char * device_create(void * device_control_structure, uint8_t major, uint64_t id) {
    
    char name[32];
    struct device* device = device_header;
    uint8_t minor = 0;
    while (device->next != 0) {
        if ((uint8_t)(device->bc << 7 | device->major) == major)
            minor++;
        device = device->next;
    }

    sprintf(name, "%s%x", device_identifiers[major], minor+0xa);

    printf("Registering device: %s [MAJ: %x MIN: %x ID: %x CTRL: %x]\n", name, major, minor, id, device_control_structure);

    device->next = (struct device*)request_page();
    memset(device->next, 0, sizeof(struct device));
    device->bc = ((major & 0x80) >> 7);
    device->major = major & 0x7F;
    device->minor = minor;
    strncpy(device->name, name, strlen(name));
    device->device_control_structure = device_control_structure;
    device->internal_id = id;
    device->valid = 1;
    
    return device->name;
}
void device_destroy(char * device_name) {
    struct device* device = device_header;
    struct device* prev = 0;
    while (device->valid) {
        if (memcmp((void*)device->name, (void*)device, strlen(device_name)) == 0) {
            device->valid = 0;
            if (prev != 0) {
                prev->next = device->next;
                free_page(device);
            } else {
                free_page(device);
            }
            return;
        }
        prev = device;
        device = device->next;
    }
}

char* insert_device_cb(void* device_control_structure, uint8_t major, uint64_t id) {
    return device_create(device_control_structure, major, id);
}

void init_devices() {
    device_header = (struct device*)request_page();
    memset(device_header, 0, sizeof(struct device));

    struct mcfg_header* mcfg = get_acpi_mcfg();
    if (mcfg != 0) {
        register_pci(mcfg, insert_device_cb);
    }
}

//Operaciones de control R|W|IOCTL
struct device* device_search(const char* device) {
    struct device* dev = device_header;
    while (dev->valid) {
        if (memcmp((void*)dev->name, (void*)device, strlen(device)) == 0) {
            return dev;
        }
        dev = dev->next;
    }
    
    return (struct device*)0;
}
uint8_t device_identify(const char* device, char* driver_name) {
    struct device* dev = device_header;
    while (dev->valid) {
        if (memcmp((void*)dev->name, (void*)device, strlen(device)) == 0) {
            if (dev->bc == 0) {
                if (!block_device_drivers[dev->major].registered)
                    return 0;
                return !strcmp(block_device_drivers[dev->major].name, driver_name);
            } else {
                if (!char_device_drivers[dev->major].registered)
                    return 0;
                return !strcmp(char_device_drivers[dev->major].name, driver_name);
            }
        }
        dev = dev->next;
    }

    return 0;
}

uint64_t device_write(const char * name, uint64_t size, uint64_t offset, uint8_t * buffer) {
    struct device* dev = device_header;
    while (dev->valid) {
        if (memcmp((void*)dev->name, (void*)name, strlen(name)) == 0) {
            struct device_driver driver = (dev->bc == 0) ? block_device_drivers[dev->major] : char_device_drivers[dev->major];
            if (!driver.registered || !driver.fops || !driver.fops->write) {
                printf("Device %s has no driver or it is invalid\n", dev->name);
                return 0;
            }
            return driver.fops->write(dev->internal_id, size, offset, buffer);
        }
        dev = dev->next;
    }

    printf("Device %s not found\n", name);
    return 0;

}

uint64_t device_read(const char * device, uint64_t size, uint64_t offset, uint8_t * buffer) {
    struct device* dev = device_header;
    while (dev->valid) {
        if (memcmp((void*)dev->name, (void*)device, strlen(device)) == 0) {
            struct device_driver driver = (dev->bc == 0) ? block_device_drivers[dev->major] : char_device_drivers[dev->major];
            if (!driver.registered || !driver.fops || !driver.fops->read) {
                printf("Device %s has no driver or it is invalid\n", dev->name);
                return 0;
            }
            return driver.fops->read(dev->internal_id, size, offset, buffer);
        }
        dev = dev->next;
    }

    printf("Device %s not found\n", device);
    return 0;
}

uint64_t device_ioctl(const char * name, uint64_t op, void * buffer) {
    struct device* dev = device_header;
    while (dev->valid) {
        if (memcmp((void*)dev->name, (void*)name, strlen(name)) == 0) {
            struct device_driver driver = (dev->bc == 0) ? block_device_drivers[dev->major] : char_device_drivers[dev->major];
            if (!driver.registered || !driver.fops || !driver.fops->ioctl) {
                printf("Device %s has no driver or it is invalid\n", dev->name);
                return 0;
            }
            return driver.fops->ioctl(dev->internal_id, op, buffer);
        }
        dev = dev->next;
    }

    printf("Device %s not found\n", name);
    return 0;
}
