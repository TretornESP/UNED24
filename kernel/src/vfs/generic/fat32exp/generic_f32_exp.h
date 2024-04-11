#ifndef _GENERIC_F32_EXP_H
#define _GENERIC_F32_EXP_H
#include "../vfs_compat.h"
#include "ff.h"

#define BPB_32_FSTYPE		82
#define BPB_16_FSTYPE		54
#define BPB_ROOT_ENT_CNT	17
#define BPB_SECTOR_SIZE		11
#define BPB_FAT_SIZE_16		22
#define BPB_32_FAT_SIZE		36
#define BPB_TOT_SECT_16		19
#define BPB_TOT_SECT_32		32
#define BPB_RSVD_CNT		14
#define BPB_NUM_FATS		16
#define BPB_CLUSTER_SIZE	13

#define MAX_F32_DEVICES 200

struct fat32_compat_device {
    char name[32];
    char partition;
    uint32_t lba;
};

struct fat32_compat_device *get_device_at_index(uint8_t index);

struct vfs_compatible * get_f32_exp_driver();
#endif