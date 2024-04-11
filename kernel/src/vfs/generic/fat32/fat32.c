//------------------------------------------------------------------------------
// MIT license
// Copyright (c) 2020 StrawberryHacker
//------------------------------------------------------------------------------

#include "fat32.h"
#include <stdint.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

#include "../../../memory/heap.h"
#include "../../../util/printf.h"
#include "../../../util/string.h"

#include "../../../drivers/disk/disk_interface.h"


/// Buffer and bitmask used for volume mounting. When a partition on the MSD 
/// contains a valid FAT32 file system, a FAT32 volume is dynamically allocated
/// and added to the linked list with base `volume_base`. The bitmask ensures 
/// a unique volume letter for each volume 
struct volume_s* volume_base;
uint32_t volume_bitmask;

/// UCS-2 offsets used in long file name (LFN) entries
	
/// Remove


/// Microsoft uses lookup tables for an uninitialized volume. This is because
/// the FAT12/16/32 type is dependent on the number of cluster, and the sectors
/// are dependent on the volume size.
///
/// This look-up table does ONLY apply when:
///		- sector size equals 512
///		- the reserved sector count equals 32
///		- number of FAT's equals 2
struct clust_size_s {
	uint32_t sector_cnt;
	uint32_t clust_size;
};

/// Private prototypes
uint8_t fat_volume_add(struct volume_s* vol);
uint8_t fat_volume_remove(char letter);
void fat_print_sector(const uint8_t* sector);
uint8_t fat_dir_lfn_cmp(const uint8_t* lfn, const char* name, uint32_t size);
uint8_t fat_dir_sfn_cmp(const char* sfn, const char* name, uint8_t size);
uint8_t fat_dir_sfn_crc(const uint8_t* sfn);
uint8_t fat_dir_set_index(struct dir_s* dir, uint32_t index);
uint8_t fat_dir_get_next(struct dir_s* dir);
uint8_t fat_dir_search(struct dir_s* dir, const char* name, uint32_t size);
uint8_t fat_table_get(struct volume_s* vol, uint32_t cluster, uint32_t* fat);
uint8_t fat_table_set(struct volume_s* vol, uint32_t cluster, uint32_t fat_entry);
uint8_t fat_read(struct volume_s* vol, uint32_t lba);
uint8_t fat_flush(struct volume_s* vol);
inline uint32_t fat_sect_to_clust(struct volume_s* vol, uint32_t sect);
inline uint32_t fat_clust_to_sect(struct volume_s* vol, uint32_t clust);
fstatus fat_follow_path(struct dir_s* dir, const char* path, uint32_t length);
fstatus fat_get_vol_label(struct volume_s* vol, char* label);
void fat_print_info(struct info_s* info);
uint8_t fat_file_addr_resolve(struct file_s* file);
fstatus fat_make_entry_chain(struct dir_s* dir, uint8_t entry_cnt);

/// Convert an absolute LBA address to the relative cluster number
inline uint32_t fat_clust_to_sect(struct volume_s* vol, uint32_t clust) {
	return ((clust - 2) * vol->cluster_size) + vol->data_lba;
}

/// Remove
void fat_print_table(struct volume_s* vol, uint32_t sector) {
	fat_read(vol, vol->fat_lba + sector);
	printf("\n");
	printf("FAT: %d\t", sector * 128);
	for (uint8_t i = 0; i < 128;) {
		uint32_t curr = load32(vol->buffer + i * 4);
		
		printf("%lx", (uint8_t)(curr >> 24));
		printf("%lx", (uint8_t)(curr >> 16));
		printf("%lx", (uint8_t)(curr >> 8));
		printf("%lx", (uint8_t)(curr));
		printf("   ");
		if ((i++ % 4) == 0) {
			printf("\n");
			printf("FAT: %d\t", sector * 128 + i);
			
		}
	}
	printf("\n");
}

/// Remove
void fat_print_sector(const uint8_t* sector) {
	for (uint32_t i = 0; i < 512;) {
		printf("%c", sector[i]);
		
		if ((i++ % 32) == 0) {
			printf("\n");
		}
	}
	printf("\n");
}

/// Add a volume to the system volumes and assign a letter to it
uint8_t fat_volume_add(struct volume_s* vol) {
	if (volume_base == NULL) {
		volume_base = vol; 
	} else {
		struct volume_s* vol_it = volume_base;
		while (vol_it->next != NULL) {
			vol_it = vol_it->next;
		}
		vol_it->next = vol;
	}
	vol->next = NULL;
	
	// Assign a letter to the volume based on the bitmask
	for (uint8_t i = 0; i < 32; i++) {
		if ((volume_bitmask & (1 << i)) == 0) {
			volume_bitmask |= (1 << i);
			vol->letter = 'C' + i;
			break;
		}
	}
	return 1;
}

/// Remove a volume from the system volumes. This functions does NOT delete the 
/// memory
uint8_t fat_volume_remove(char letter) {
	struct volume_s* curr;
	if (volume_base == NULL) {
		return 0;
	} else if (volume_base->letter == letter) {
		curr = volume_base;
		volume_base = volume_base->next;
	} else {
		struct volume_s* prev = volume_base;
		curr = volume_base->next;
		
		while (curr != NULL) {
			if (curr->letter == letter) {
				break;
			}
			prev = curr;
			curr = curr->next;
		}
		
		if (curr == NULL) return 0;
		prev->next = curr->next;
	}
	
	// Clear the bit set in the bitmask
	uint8_t bit_pos = curr->letter - 'C';
	volume_bitmask &= ~(1 << bit_pos);
	
	return 1;
}

/// Checks for a valid FAT32 file system on the given partition. The `bpb`
/// should point to a buffer containing the first sector in this parition. 
uint8_t fat_search(const char* name, uint32_t lba) {
	uint8_t bpb[512];
    if (!disk_read(name, bpb, lba, 1)) return 0;
	
	// Check the BPB boot signature
	if (load16(bpb + 510) != 0xAA55) return 0;
	
	// A valid FAT file system will have the "FAT" string in either the FAT16
	// boot sector, or in the FAT32 boot sector. This does NOT indicate the
	// FAT file system fype
	if (!memcmp(bpb + BPB_32_FSTYPE, "FAT", 3)) {
		if (!memcmp(bpb + BPB_16_FSTYPE, "FAT", 3)) {
			return 0;
		}
	}
	
	// A FAT12, FAT16 or FAT32 file system is present. The type is determined 
	// by the count of data cluster.
	uint32_t root_sectors = ((load16(bpb + BPB_ROOT_ENT_CNT) * 32) + 
		(load16(bpb + BPB_SECTOR_SIZE) - 1)) / 
		(load16(bpb + BPB_SECTOR_SIZE) - 1);
	
	uint32_t fat_size = (load16(bpb + BPB_FAT_SIZE_16)) ? 
		(uint32_t)load16(bpb + BPB_FAT_SIZE_16) : 
		load32(bpb + BPB_32_FAT_SIZE);
		
	uint32_t tot_sect = (load16(bpb + BPB_TOT_SECT_16)) ? 
		(uint32_t)load16(bpb + BPB_TOT_SECT_16) :
		load32(bpb + BPB_TOT_SECT_32);
		
	uint32_t data_sectors = tot_sect - (load16(bpb + BPB_RSVD_CNT) + 
		(bpb[BPB_NUM_FATS] * fat_size) + root_sectors);
		
	uint32_t data_clusters = data_sectors / bpb[BPB_CLUSTER_SIZE];
	
	// Only FAT32 is supported
	if (data_clusters < 65525) {
		return 0;
	}
	return 1;
}

/// Calculates the SFN checksum based on the 8.3 short file name
uint8_t fat_dir_sfn_crc(const uint8_t* sfn) {
	uint8_t crc = 0;
	uint8_t count = 11;
	do {
		crc = ((crc & 1) << 7) + (crc >> 1) + *sfn++;
	} while (--count);
	
	return crc;
}

/// Move the directory pointer to entry `index` relative to the directory base. 
uint8_t fat_dir_set_index(struct dir_s* dir, uint32_t index) {
	return 1;
}

/// Move the `dir` pointer to the next 32-byte directory entry
uint8_t fat_dir_get_next(struct dir_s* dir) {
	// Update the rw offset to point to the next 32-byte entry
	dir->rw_offset += 32;
	
	// Check for sector overflow
	if (dir->rw_offset >= dir->vol->sector_size) {
		dir->rw_offset -= dir->vol->sector_size;
		dir->sector++;
		
		// Check for cluster overflow
		if (dir->sector >= (fat_clust_to_sect(dir->vol, dir->cluster) +
				dir->vol->cluster_size)) {
			
			// Get the next cluster from the FAT table	
			uint32_t new_cluster;
			if (!fat_table_get(dir->vol, dir->cluster, &new_cluster)) {
				return 0;
			}
			
			// Check if the FAT table entry is the EOC. The FAT table entry
			// will in these cases be either EOC or date clusters. No need 
			// to check for bad clusters. 
			uint32_t eoc_value = new_cluster & 0xFFFFFFF;
			if ((eoc_value >= 0xFFFFFF8) && (eoc_value <= 0xFFFFFFF)) {
				return 0;
			}
			
			// Update the sector LBA from the new cluster number
			dir->cluster = new_cluster;
			dir->sector = fat_clust_to_sect(dir->vol, dir->cluster);
		}
	}
	return 1;
}

/// Resolves any overflow on rw_offset, sector and cluster on the given `file`
/// descriptor
uint8_t fat_file_addr_resolve(struct file_s* file) {	
	// Check for sector overflow
	if (file->rw_offset >= file->vol->sector_size) {
		file->rw_offset -= file->vol->sector_size;
		file->sector++;
		
		// Check for cluster overflow
		if (file->sector >= (fat_clust_to_sect(file->vol, file->cluster) +
			file->vol->cluster_size)) {
			
			// Get the next cluster from the FAT table
			uint32_t new_cluster;
			if (!fat_table_get(file->vol, file->cluster, &new_cluster)) {
				return 0;
			}
			
			// Check if the FAT table entry is the EOC
			uint32_t eoc_value = new_cluster & 0xFFFFFFF;
			if ((eoc_value >= 0xFFFFFF8) && (eoc_value <= 0xFFFFFFF)) {
				printf("EOC value: %d\n", eoc_value);
				return 0;
			}
			
			// Update the sector LBA from the cluster number
			file->cluster = new_cluster;
			file->sector = fat_clust_to_sect(file->vol, file->cluster);	 
		}
	}
	return 1;
}

/// Returns the 32-bit FAT entry corresponding with the cluster number
uint8_t fat_table_get(struct volume_s* vol, uint32_t cluster, uint32_t* fat_entry) {
	// Calculate the sector LBA from the FAT table base address
	uint32_t start_sect = vol->fat_lba + cluster / 128;
	uint32_t start_off = cluster % 128;
	
	if (!fat_read(vol, start_sect)) {
		return 0;
	}
	*fat_entry = load32(vol->buffer + start_off * 4);
	return 1;
}

/// Set the FAT table entry corresponding to `cluster` to a specified value
uint8_t fat_table_set(struct volume_s* vol, uint32_t cluster, uint32_t fat_entry) {
	// Calculate the sector LBA from the FAT table base address
	uint32_t start_sect = vol->fat_lba + cluster / 128;
	uint32_t start_offset = cluster % 128;
	
	if (!fat_read(vol, start_sect)) {
		return 0;
	}
	store32(vol->buffer + 4 * start_offset, fat_entry);
	
	// Mark the buffer as dirty
	vol->buffer_dirty = 1;
	
	// Remove?
	fat_flush(vol);
	return 1;
}

/// Get the next free cluster from the FAT table and update the FSinfo to
/// point to the next free cluster
uint8_t fat_get_cluster(struct volume_s* vol, uint32_t* cluster) {
	// Load the FSinfo sector
	if (!fat_read(vol, vol->fsinfo_lba)) {
		return 0;
	}
	uint32_t next_free = load32(vol->buffer + INFO_NEXT_FREE);
	uint32_t tot_free = load32(vol->buffer + INFO_CLUST_CNT);
	
	// The `next_free` pointer does necessary point to a free cluster. However
	// it specifies where to start looking for a free block. The `sector` and 
	// `rw` combined point to this position
	uint32_t sector = vol->fat_lba + (next_free / 128);
	uint32_t rw = (next_free % 128) * 4;
	
	uint32_t entry = 0;
	uint8_t match_found = 0;
	while (1) {
		// Load the current sector
		if (!fat_read(vol, sector)) {
			return 0;
		}
		
		// Check if the entry is available
		entry = load32(vol->buffer + rw);
		if ((entry & 0x7f) == 0) {
			if (match_found) {
				break;
			} else {
				match_found = 1;
				*cluster = (128 * (sector - vol->fat_lba)) + (rw / 4);
				store32(vol->buffer + rw, 0xFFFFFFF);
				vol->buffer_dirty = 1;
			}
		}
		// Make `rw` and `sector` point to the next 4-byte table entry
		rw += 4;
		if (rw >= vol->sector_size) {
			sector++;
			rw = 0;
		}
	}
	// `cluster` points to the first free cluster which can now be used, while
	// `sector` holdes the next free cluster value which should be written back
	// to the FSinfo sector
	if (!fat_read(vol, vol->fsinfo_lba)) {
		return 0;
	}	
	store32(vol->buffer + INFO_NEXT_FREE, (128 * (sector - 
		vol->fat_lba)) + (rw / 4));
	store32(vol->buffer + INFO_CLUST_CNT, tot_free - 1);
	vol->buffer_dirty = 1;
	
	// Remove?
	fat_flush(vol);
	return 1;
}

/// Caches the `lba` sector in the volume buffer. If this buffer is already
/// present, the function returns `1`. Any dirty buffer will be written back
/// before the next sector is fetched. Return `0` in case of hardware fault
uint8_t fat_read(struct volume_s* vol, uint32_t lba) {
	
	// Check if the sector is already cached
	if (vol->buffer_lba != lba) {
		// Flush any dirty buffer back to the storage device
		if (!fat_flush(vol)) {
			return 0;
		}
		
		// Cache the next sector
		if (!disk_read(vol->disk, vol->buffer, lba, 1)) {
			return 0;
		}
		vol->buffer_lba = lba;
	}
	return 1;
}

/// Clean a volume buffer
uint8_t fat_flush(struct volume_s* vol) {
	if (vol->buffer_dirty) {
		if (!disk_write(vol->disk, vol->buffer, vol->buffer_lba, 1)) {
			return 0;
		}
		vol->buffer_dirty = 0;
	}
	return 1;
}

/// Convert a relative cluster number to the absolute LBA address
inline uint32_t fat_sect_to_clust(struct volume_s* vol, uint32_t sect) {
	return ((sect - vol->data_lba) / vol->cluster_size) + 2;
}

/// Compares `size` characters from two strings without case sensitivity
uint8_t fat_dir_sfn_cmp(const char* sfn, const char* name, uint8_t size) {
	if (size > 8) {
		size = 8;
	}
	do {
		char tmp_char = *name;
		
		// A lowercase characters is converted to an uppercase
		if (tmp_char >= 'a' && tmp_char <= 'z') {
			tmp_char -= 32;
		}
		if (tmp_char != *sfn) {
			return 0;
		}
		sfn++;
		name++;
	} while (--size);
	return 1;
}

/// Compartes a LFN entry against a given file name. `name` is the full string 
/// to be comared and `lfn` is only one LFN entry. The code will just compare
/// the affected fragment of the `name` string.
uint8_t fat_dir_lfn_cmp(const uint8_t* lfn, const char* name, uint32_t size) {
	const uint8_t lfn_lut[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

	// Compute the `name` offset of a fragment which should match the LFN name
	uint8_t name_off = 13 * ((lfn[LFN_SEQ] & LFN_SEQ_MSK) - 1);
	
	for (uint8_t i = 0; i < 13; i++) {
		// The first empty UCS-2 character will contain 0x0000 and the rest will
		// contain 0xFFFF. 
		if (lfn[lfn_lut[i]] == 0x00 || lfn[lfn_lut[i]] == 0xff) {
			break;
		}
		// Compare the first charater in the UCS-2. This will typically be a
		// ordinary ASCII character
		if (lfn[lfn_lut[i]] != name[name_off + i]) {
			return 0;
		}
	}
	return 1;
}

/// Takes in a pointer to a directory (does not need to be the leading entry)
/// and tries to find a directory entry matching `name`
uint8_t fat_dir_search(struct dir_s* dir, const char* name, uint32_t size) {
	
	// A search start from the leading entry
	if (dir->start_sect != dir->sector) {
		dir->sector = dir->start_sect;
		dir->cluster = fat_sect_to_clust(dir->vol, dir->sector);
		dir->rw_offset = 0;
	}
	
	uint8_t lfn_crc = 0;
	uint8_t lfn_match = 1;
	uint8_t match = 0;
	
	while (1) {
		// Update the buffer if needed
		if (!fat_read(dir->vol, dir->sector)) {
			return 0;
		}
		uint8_t* buffer = dir->vol->buffer;
		uint32_t rw_offset = dir->rw_offset;
		
		uint8_t sfn_tmp = buffer[rw_offset];
		// Check for the EOD marker
		if (sfn_tmp == 0x00) {
			break;
		}
		// Only allow used folders to be compared
		if (!((sfn_tmp == 0x00) || (sfn_tmp == 0x05) || (sfn_tmp == 0xE5))) {
			
			// Check if the entry pointed to by `dir` is a LFN or a SFN
			if ((buffer[rw_offset + SFN_ATTR] & ATTR_LFN) == ATTR_LFN) {
				
				// If the LFN name does not match the input
				if (!fat_dir_lfn_cmp(buffer + rw_offset, name, size)) {
					// TODO:
					// LFN contains the sequence number so we could jump to
					// the next entry right away. This will speed up directory
					// search around x3 times
					lfn_match = 0;
				}
				lfn_crc = buffer[rw_offset + LFN_CRC];
			} else {
				
				// The current entry is a SFN
				if (lfn_crc && lfn_match) {
					// The current SFN entry is the last in a sequence of LFN's
					if (lfn_crc == fat_dir_sfn_crc(buffer + rw_offset)) {
						match = 1;
					}
				} else {
					// Compare `name` with the SFN 8.3 file name
					if (fat_dir_sfn_cmp((char *)buffer + rw_offset, name, size)) {
						match = 1;
					}
				}
				if (match) {
					// Update the `dir` pointer
					dir->cluster = (load16(buffer + rw_offset +
						SFN_CLUSTH) << 16) | load16(buffer +
						rw_offset + SFN_CLUSTL);
					dir->sector = fat_clust_to_sect(dir->vol, dir->cluster);
					dir->start_sect = dir->sector;
					dir->size = load32(buffer + rw_offset + SFN_FILE_SIZE);
					dir->rw_offset = 0;

					return 1;
				}
				lfn_match = 1;
				lfn_crc = 0;
			}
		}
		// Get the next 32-byte directory entry
		if (!fat_dir_get_next(dir)) {
			return 0;
		}
	}
	return 0;
}

/// Follows the `path` and returns the `dir` object pointing to the last found 
/// folder. If not found the functions returns `0`, but the `dir` object may
/// still be changed
/// 
/// Path should be on the form: C:/home/usr/bin/chrome.exe
fstatus fat_follow_path(struct dir_s* dir, const char* path, uint32_t length) {
	
	// Volume object is determined from the first character
	struct volume_s* vol = volume_get(*path++);
	if (vol == NULL) {
		return FSTATUS_NO_VOLUME;
	}
	// Rewind the `dir` object to the root directory
	dir->vol = vol;
	dir->start_sect = dir->sector = vol->root_lba;
	dir->cluster = fat_clust_to_sect(vol, vol->root_lba);
	dir->rw_offset = 0;
	
	// Check for the colon
	if (*path++ != ':') {
		return FSTATUS_PATH_ERR;
	}
	if (*path != '/') {
		return FSTATUS_PATH_ERR;
	}
	
	// `frag_ptr` and `frag_size` will contain one fragment of the path name
	const char* frag_ptr;
	uint8_t frag_size;
	while (1) {
		
		// Search for the first `/`
		while (*path && (*path != '/')) {
			path++;
		}
		// TODO: This is not right is it?
		// Check if the next fragment exist
		if (*path++ == '\0') break;
		if (*path == '\0') break;
		
		// `path` points to the first character in the current name fragment
		const char* tmp_ptr = path;
		frag_ptr = path;
		frag_size = 0;
		
		while ((*tmp_ptr != '\0') && (*tmp_ptr != '/')) {
						
			// The current fragment describes is a file, However, the name 
			// fragment before it has been found
			if (*tmp_ptr == '.') {
				return FSTATUS_OK;
			}
			frag_size++;
			tmp_ptr++;
		}
		
		// Now `frag_ptr` will point to the first character in the name
		// fragment, and `frag_size` will contain the size

		printf("Searching for directory: %s\n", frag_ptr);
		printf("\n");

		// Search for a matching directory name in the current directory. If
		// matched, the `fat_dir_search` will update the `dir` pointer as well
		if (!fat_dir_search(dir, frag_ptr, frag_size)) {
			printf("Directory not fount\n");
			return 0;
		}
	}	
	return FSTATUS_OK;
}

/// Get the volume label stored in the root directory. This is the one used by
/// Microsoft, not the BPB volume ID
fstatus fat_get_vol_label(struct volume_s* vol, char* label) {	
	// Make a directory object pointing to the root directory
	struct dir_s dir;
	dir.sector = vol->root_lba;
	dir.rw_offset = 0;
	dir.cluster = fat_sect_to_clust(vol, dir.sector);
	
	// The volume label is a SFN entry in the root directory with bit 3 set in
	// the attribute field. Volume label is limited to 13 uppercase characters
	while (1) {
		if (!fat_read(vol, dir.sector)) {
			return FSTATUS_ERROR;
		}
		
		// Check if the attribute is volume label
		uint8_t attribute = vol->buffer[dir.rw_offset + SFN_ATTR];
		if (attribute & ATTR_VOL_LABEL) {
			
			// LFN file name entries are also marked with volume label
			if ((attribute & ATTR_LFN) != ATTR_LFN) {
				const char* src = (const char *)(vol->buffer + dir.rw_offset);
				
				for (uint8_t i = 0; i < 11; i++) {
					*label++ = *src++;
				}
				return 1;
			}
		}
		// Get the next directory
		if (!fat_dir_get_next(&dir)) {
			return FSTATUS_ERROR;
		}
	}
}

/// Remove
/// Print directory information
void fat_print_info(struct info_s* info) {
	const char file_size_ext[] = {'k', 'M', 'G'};

	uint32_t size = info->size;
	char ext = 0;	
	uint8_t ext_cnt = 0;
	while (size >= 1000) {
		size /= 1000;
		ext = file_size_ext[ext_cnt++];
	}
	printf("%d", size);
	if (ext) {
		printf("%c", ext);
	}
	printf("B\t");
	uint16_t time = info->w_time;
	uint16_t date = info->w_date;
	
	printf("%d/%d/%d %d:%d\t", 
		date & 0x1f,
		(date >> 5) & 0xf,
		((date >> 9) & 0x7f) + 1980,
		(time >> 11) & 0x1f,
		(time >> 5) & 0x3f);
		
	if (info->attribute & ATTR_DIR) {
		printf("DIR\t");
	} else {
		printf("\t");
	}
	
	printf("%s\n", info->name);
}


//------------------------------------------------------------------------------
// FAT23 file system API
// This section will implement the file system API exposed to the user
//------------------------------------------------------------------------------

/// This is the `main` functions that is used to tast the file system

void fat32_debug(const char* disk) {
				
	struct volume_s* tmp = volume_get('D');
	printf("Volume: %c:\n", tmp->letter);

	uint32_t cluster;
	
	fat_get_cluster(tmp, &cluster);
	fat_table_set(tmp, 33, 0);
	//fat_print_table(tmp, 0);

	// Print all the volumes on the system
	printf("Displaying system volumes:\n");
	struct volume_s* vol = volume_get_first();
	while (vol) {
		for (uint8_t i = 0; i < 11; i++) {
			if (vol->label[i]) {
				printf("%c", vol->label[i]);
			}
		}
		printf(" (%c:)\n", vol->letter);
		vol = vol->next;
	}
	printf("\n");
	
	
	// List all directories
	struct dir_s dir;
	fat_dir_open(&dir, "C:/test/", 0);
	
	struct info_s* info = (struct info_s *)malloc(sizeof(struct info_s));
	memset(info, 0, sizeof(struct info_s));
	fstatus status;
	printf("\nListing directories in: C:/test/\n");
	do {
		status = fat_dir_read(&dir, info);
				
		// Print the information
		if (status == FSTATUS_OK) {
			fat_print_info(info);
		}
	} while (status != FSTATUS_EOF);
	printf("- EOD -\n");

}

/// Mounts a physical disk. It checks for a valid FAT32 file system in all
/// available disk partitions. All valid file system is dynamically allocated
/// and added to the system volumes
/// 
/// Note that this is the only functions referencing the `disk` parameter. All
/// further interactions happend will via the volume letter e.g. D: drive.

char register_fat32_partition(const char* disk, uint32_t lba, const char* mountpoint) {
	(void)mountpoint; //TODO: Implement this
	uint8_t mount_buffer[512];

	if (!disk_read(disk, mount_buffer, lba, 1)) {
		return 0;
	}

	struct volume_s* vol = (struct volume_s *)malloc(sizeof(struct volume_s));
	memset(vol, 0, sizeof(struct volume_s));

	vol->sector_size = load16(mount_buffer + BPB_SECTOR_SIZE);
	vol->cluster_size = mount_buffer[BPB_CLUSTER_SIZE];
	vol->total_size = load32(mount_buffer + BPB_TOT_SECT_32);

	vol->fsinfo_lba = lba + load16(mount_buffer + BPB_32_FSINFO);
	vol->fat_lba = lba + load16(mount_buffer + BPB_RSVD_CNT);
	vol->data_lba = vol->fat_lba + (load32(mount_buffer + BPB_32_FAT_SIZE) * mount_buffer[BPB_NUM_FATS]);

	vol->root_lba = fat_clust_to_sect(vol, load32(mount_buffer + BPB_32_ROOT_CLUST));
	memcpy(vol->disk, disk, strlen(disk));

	vol->buffer_lba = 0;
	fat_get_vol_label(vol, vol->label);

	fat_volume_add(vol);

	return vol->letter;
}

/*
uint8_t disk_mount(const char* disk) {
	
	// Verify that the storage device are present
	if (!disk_get_status(disk)) return 0;
	
	// Initialize the hardware and protocols
	if (!disk_initialize(disk)) return 0;

	// Retrieve the partition info from all four partitions, thus avoiding 
	// multiple accesses to the MBR sector
	struct partition_s * partitions = 0x0;	
	uint32_t valid_partitions = read_gpt(disk, (struct partition*)partitions);
	uint8_t mount_buffer[512];
	
	// Search for a valid FAT32 file systems on all valid paritions
	uint8_t fat_partitions = 0;
	for (uint8_t i = 0; i < valid_partitions; i++) {
		if (partitions[i].lba) {
			if (!disk_read(disk, mount_buffer, partitions[i].lba, 1)) {
				return 0;
			}
			
			// Check if the current partition contains a FAT32 file system
			if (fat_search(mount_buffer)) {
				printf("FAT32 found at LBA %x\n", partitions[i].lba);
				// Allocate the file system structure
				struct volume_s* vol = (struct volume_s *)malloc(sizeof(struct volume_s));
				memset(vol, 0, sizeof(struct volume_s));

				// Update FAT32 information
				vol->sector_size = load16(mount_buffer + BPB_SECTOR_SIZE);
				vol->cluster_size = mount_buffer[BPB_CLUSTER_SIZE];
				vol->total_size = load32(mount_buffer + BPB_TOT_SECT_32);
				
				// Update FAT32 offsets that will be used by the driver
				vol->fsinfo_lba = partitions[i].lba + load16(mount_buffer +
					BPB_32_FSINFO);
				vol->fat_lba = partitions[i].lba + load16(mount_buffer + 
					BPB_RSVD_CNT);
				vol->data_lba = vol->fat_lba + (load32(mount_buffer + 
					BPB_32_FAT_SIZE) * mount_buffer[BPB_NUM_FATS]);
				
				vol->root_lba = fat_clust_to_sect(vol, load32(mount_buffer +
					BPB_32_ROOT_CLUST));
				memcpy(vol->disk, disk, strlen(disk));
				
				// Sector zero will not exist in any file system. This forces 
				// the code to read the first block from the storage device
				vol->buffer_lba = 0;
				// Get the volume label
				fat_get_vol_label(vol, vol->label);

				// Add the newly made volume to the list of system volumes
				fat_volume_add(vol);
				printf("Added volume: %s, %c\n", vol->label, vol->letter);
				fat_partitions++;
			} else {
				printf("No FAT32 found at LBA %x\n", partitions[i].lba);
			}
		}
	}

	//Salloc_super(disk, 512, FAT32_FS, 0);
	return fat_partitions;
}*/

/// Remove the volumes corresponding with a physical disk and delete the memory.
/// This function must be called before a storage device is unplugged, if not,
/// cached data may be lost. 

uint8_t unregstr_fat32_partition(char letter) {
	struct volume_s* vol = volume_get(letter);
	if (vol) {
		if (!fat_volume_remove(letter)) {
			return 0;
		}
		free(vol);
	}
	return 1;
}

uint8_t disk_eject(const char* disk) {
	struct volume_s* vol = volume_get_first();
	
	while (vol != NULL) {
		
		// Remove all volumes which matches the `disk` number
		if (!memcmp(vol->disk, disk, strlen(disk))) {
			if (!fat_volume_remove(vol->letter)) {
				return 0;
			}
			free(vol);
		}
		vol = vol->next;
	}
	return 1;
}

/// Get the first volume in the system. If no volumes are present it return
/// NULL
struct volume_s* volume_get_first(void) {
	return volume_base;
}

/// Get a volume based on its letter
struct volume_s* volume_get(char letter) {
	
	struct volume_s* vol = volume_base;
	while (vol != NULL) {
		if (vol->letter == letter) {
			return vol;
		}
		vol = vol->next;
	}
	return NULL;
}

/// Set the volume label in the BPB SFN entry
fstatus volume_set_label(struct volume_s* vol, const char* name, uint8_t length) {
	// Make a directory object pointing to the root directory
	struct dir_s dir;
	dir.sector = vol->root_lba;
	dir.rw_offset = 0;
	dir.cluster = fat_sect_to_clust(vol, dir.sector);
	
	while (1) {
		if (!fat_read(vol, dir.sector)) {
			return FSTATUS_ERROR;
		}
		
		// Check if the attribute is volume label
		uint8_t attribute = vol->buffer[dir.rw_offset + SFN_ATTR];
		if (attribute & ATTR_VOL_LABEL) {
			
			// Check that it is not a LFN entry
			if ((attribute & ATTR_LFN) != ATTR_LFN) {
				char* src = (char *)(vol->buffer + dir.rw_offset);
				for (uint8_t i = 0; i < 11; i++) {
					// The volume label is padded with spaces
					if (i >= length) {
						*src++ = ' ';
					} else {
						*src++ = *name++;
					}
					vol->buffer_dirty = 1;
				}
				// Writes the buffer back to the storage device
				// TODO: Do I need this?
				fat_flush(vol);
				return 1;
			}
		}
		// Get the next directory
		if (!fat_dir_get_next(&dir)) {
			return 0;
		}
	}
}

/// Get the volume label
fstatus volume_get_label(struct volume_s* vol, char* name) {
	// TODO: Hmm, this label is stored in the vol->label. Why fetch it two times
	return fat_get_vol_label(vol, name);
}

/// Formats the volume to a blank FAT32 volume
fstatus volume_format(struct volume_s* vol, struct fat_fmt_s* fmt) {
	return FSTATUS_OK;
}

/// Open a directory specified by `path`. The `dir` object will point to this
/// directory
fstatus fat_dir_open(struct dir_s* dir, const char* path, uint16_t length) {
	return fat_follow_path(dir, path, length);
}

/// Close an open directory
fstatus fat_dir_close(struct dir_s* dir) {
	// Check if the volume is clean
	if (!fat_flush(dir->vol)) {
		return FSTATUS_ERROR;
	}
	return FSTATUS_OK;
}

/// Read one entry pointed to by `dir` and move the directory pointer to the
/// next directory entry
fstatus fat_dir_read(struct dir_s* dir, struct info_s* info) {
	const uint8_t lfn_lut[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

	uint8_t lfn_crc = 0;
	uint8_t name_length = 0;
	
	while (1) {
		if (!fat_read(dir->vol, dir->sector)) {
			return FSTATUS_ERROR;
		}
		uint8_t* entry_ptr = dir->vol->buffer + dir->rw_offset;
		
		// Check if the entry is in use
		uint8_t sfn_check = entry_ptr[0];
		
		// Check for the end marker
		if (sfn_check == 0x00) {
			return FSTATUS_EOF;
		}
		if ((sfn_check != 0xE5) && (sfn_check != 0x05)) {
			uint8_t sfn_attr = entry_ptr[SFN_ATTR];
			
			// Check if the directory entry is a LFN or a SFN
			if ((sfn_attr & ATTR_LFN) == ATTR_LFN) {
				
				// LFN case
				uint8_t name_offset = 13 * ((entry_ptr[0] & LFN_SEQ_MSK) - 1);
				for (uint8_t i = 0; i < 13; i++) {
					uint8_t tmp_char = entry_ptr[lfn_lut[i]]; 
					if (!((tmp_char == 0x00) || (tmp_char == 0xFF))) {
						info->name[name_offset + i] = tmp_char;
						name_length++;
					}
				}
				lfn_crc = entry_ptr[LFN_CRC];
			} else {
				if (lfn_crc) {
					// This SFN entry is the last entry in a chain of
					// LFN entries. Return CRC error if the checksum is wrong
					if (lfn_crc != fat_dir_sfn_crc(entry_ptr)) {
						return FSTATUS_ERROR;
					}
					
				} else {
					// The directory contains only one SFN entry
					for (uint8_t i = 0; i < 11; i++) {
						info->name[i] = entry_ptr[i];
						name_length++;
					}
				}
				
				// In any case the last SFN entry will contain the information
				// about the directory
				info->attribute = entry_ptr[SFN_ATTR];
				info->c_time_tenth = entry_ptr[SFN_CTIME_TH];
				info->c_time = load16(entry_ptr + SFN_CTIME);
				info->c_date = load16(entry_ptr + SFN_CDATE);
				info->w_time = load16(entry_ptr + SFN_WTIME);
				info->w_date = load16(entry_ptr + SFN_WDATE);
				info->a_date = load16(entry_ptr + SFN_ADATE);
				info->size = load32(entry_ptr + SFN_FILE_SIZE);
				info->name_length = name_length;
				
				// Make `dir` point to the next entry
				fat_dir_get_next(dir);
				
				return FSTATUS_OK;
			}
		}
		// Get the next entry
		fat_dir_get_next(dir);
	}
}

/// Make a new directory in the specified `path`
fstatus fat_dir_make(const char* path) {
	return FSTATUS_OK;
}

/// Rename a directory item
fstatus fat_dir_rename(struct dir_s* dir, const char* name, uint8_t length) {
	
	// Get the lenght of the name
	uint8_t name_length = 0;
	for (uint8_t i = 0; i < length; i++) {
		if ((name[i] == '.') && (i != 0)) {
			break;
		}
		name_length++;
	}
	
	printf("Name length: %d\n", name_length);
	uint8_t entries_req = 1;
	if (name_length > 8) {
		entries_req = (length / 13) - 1;
	}
	
	// Check the number of entries pointed to by dir
	if (!fat_read(dir->vol, dir->sector)) {
		return FSTATUS_ERROR;
	}
	uint8_t entries_pres = 1;
	if ((dir->vol->buffer[dir->rw_offset + SFN_ATTR] & ATTR_LFN) == ATTR_LFN) {
		entries_pres = (dir->vol->buffer[dir->rw_offset] & LFN_SEQ_MSK) + 1;
	}
	
	if (entries_req > entries_pres) {
		// Allocate a chain of entries
		
	} else {
		// Enough entry are present
	}
	printf("Ent req: %d\nEnt pres: %d\n", entries_req, entries_pres);
	return FSTATUS_OK;
}

/// Open a file and return the file object. It takes in a global path.
fstatus fat_file_open(struct file_s* file, const char* path, uint16_t length) {
	// Make a pointer to the directory where the file is stored
	struct dir_s dir;
	fstatus status = fat_follow_path(&dir, path, length);
	if (status != FSTATUS_OK) {
		return status;
	}
	
	// Grab the last name fragment which will be the file name
	const char* char_ptr = path + length - 1;
	if (*char_ptr == 0x00) return FSTATUS_PATH_ERR;
	if (*char_ptr == '/') {
		char_ptr--;
	}
	uint8_t frag_size = 0;
	
	while ((*char_ptr != '/') && (*char_ptr != 0x00)) {
		frag_size++;
		char_ptr--;
	}
	if (*char_ptr == 0x00) {
		return FSTATUS_PATH_ERR;
	}
	
	// Try to find the file in the directory pointed to by `dir`
	if (!fat_dir_search(&dir, char_ptr + 1, frag_size)) {
		return FSTATUS_PATH_ERR;
	}
	
	// Update whe address of the file
	file->sector = dir.sector;
	file->start_sect = dir.sector;
	file->cluster = dir.cluster;
	file->rw_offset = 0;
	file->vol = dir.vol;
	file->size = dir.size;
	
	printf("File size: %d\n", file->size);
	return FSTATUS_OK;
}

/// Closes a currently open file object
fstatus fat_file_close(struct file_s* file) {
	if(!fat_flush(file->vol)) {
		return FSTATUS_ERROR;
	}
	return FSTATUS_OK;
}

/// Reads `count` number of bytes from the file (at whatever position `file` is
/// pointing to). It returns the `status` field which contains the number of
/// bytes written. If the `count` and `status` does not match the EOF marker
/// has been hit.
fstatus fat_file_read(struct file_s* file, uint8_t* buffer, uint32_t count, uint32_t* status) {
	*status = 0;
	uint16_t sector_size = file->vol->sector_size;
	uint32_t file_size = file->size;
	
	if (!fat_read(file->vol, file->sector)) {
		return FSTATUS_ERROR;
	}
	while (count--) {
		
		// Resolve the address
		if (file->rw_offset >= sector_size) {
			fat_file_addr_resolve(file);
			
			// Update the file system buffer
			if (!fat_read(file->vol, file->sector)) {
				return FSTATUS_ERROR;
			}
		}
		*buffer++ = file->vol->buffer[file->rw_offset++];
		
		// Update the offsets
		file->glob_offset++;
		(*status)++;
		
		if (file->glob_offset >= file_size) {
			break;
		}
	}
	return FSTATUS_OK;
}

/// Write a number of characters to the location pointed to be file
fstatus fat_file_write(struct file_s* file, const uint8_t* buffer, uint32_t count) {
	return FSTATUS_OK;
}

/// Move the read / write file pointer. The offset is cumputed with respect 
/// to the file start address
fstatus fat_file_jump(struct file_s* file, uint32_t offset) {
	
	// Get the file start address
	file->cluster = fat_sect_to_clust(file->vol, file->start_sect);
	
	// Get the relative offsets
	uint32_t sector_offset = offset / file->vol->sector_size;
	uint32_t cluster_offset = sector_offset / file->vol->cluster_size;
	sector_offset = sector_offset % file->vol->cluster_size;
	
	while (cluster_offset) {
		uint32_t new_cluster;
		if (!fat_table_get(file->vol, file->cluster, &new_cluster)) {
			return FSTATUS_ERROR;
		}
		// Check if the FAT table entry is EOC
		uint32_t eoc_value = new_cluster & 0xFFFFFFF;
		if ((eoc_value >= 0xFFFFFF8) && (eoc_value <= 0xFFFFFFF)) {
			return 0;
		}
		 
		file->cluster = new_cluster;		
		cluster_offset--;
	}
	
	// The base cluster address is determined. Update the sector and rw offset
	// from the relative offsets calulated above. 
	file->sector = fat_clust_to_sect(file->vol, file->cluster) + sector_offset;
	file->rw_offset = offset % file->vol->sector_size;
	file->glob_offset = offset;
	
	return FSTATUS_OK;
}

fstatus fat_dir_delete(struct dir_s* dir);
fstatus fat_dir_chmod(struct dir_s* dir, const char* mod);
fstatus fat_file_flush(struct file_s* file);