/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "generic_f32_exp.h"
#include "../../../drivers/disk/disk_interface.h"
/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS ff_disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	struct fat32_compat_device *device = get_device_at_index(pdrv);
	if (device->name[0] != 0) {
		return !disk_get_status(device->name);
	}

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS ff_disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	struct fat32_compat_device *device = get_device_at_index(pdrv);
	if (device->name[0] != 0) {
		return !disk_initialize(device->name);
	}

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT ff_disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	struct fat32_compat_device *device = get_device_at_index(pdrv);
	if (device->name[0] != 0) {
		return (disk_read(device->name, buff, sector, count) != 0) ? RES_OK : RES_ERROR;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT ff_disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	struct fat32_compat_device *device = get_device_at_index(pdrv);
	if (device->name[0] != 0) {
		return (disk_write(device->name, buff, sector, count) != 0) ? RES_OK : RES_ERROR;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT ff_disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	struct fat32_compat_device * device = get_device_at_index(pdrv);
	if (device->name[0] != 0) {
		return (disk_ioctl(device->name, cmd, buff) != 0) ? RES_OK : RES_ERROR;
	}

	return RES_PARERR;
}