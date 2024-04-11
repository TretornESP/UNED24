#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include "ext2_sb.h"

#include "../../../drivers/disk/disk_interface.h"
#include "../../../util/printf.h"

uint8_t ext2_flush_sb(struct ext2_partition* partition, struct ext2_block_group_descriptor* bg, uint32_t bgid) {
    (void)bg;
    if (bgid >= partition->backup_bgs_count || partition->backup_bgs[bgid] == -1) return 1;

    uint32_t block_size = 1024 << ((struct ext2_superblock*)(partition->sb))->s_log_block_size;
    uint32_t sectors_per_group = ((struct ext2_superblock*)(partition->sb))->s_blocks_per_group * (block_size / partition->sector_size);

    if (!write_disk(partition->disk, (uint8_t*)partition->sb, partition->lba+(sectors_per_group*bgid)+partition->sb_block, 2)) {
        return 1;
    }
    
    return 0;
}

void ext2_dump_sb(struct ext2_partition* partition) {
    struct ext2_superblock* sb = (struct ext2_superblock*)partition->sb;
    printf("ext2 superblock:\n");
    printf("s_inodes_count: %u\n", sb->s_inodes_count);
    printf("s_blocks_count: %u\n", sb->s_blocks_count);
    printf("s_r_blocks_count: %u\n", sb->s_r_blocks_count);
    printf("s_free_blocks_count: %u\n", sb->s_free_blocks_count);
    printf("s_free_inodes_count: %u\n", sb->s_free_inodes_count);
    printf("s_first_sb_block: %u\n", sb->s_first_sb_block);
    printf("s_log_block_size: %u\n", sb->s_log_block_size);
    printf("s_log_frag_size: %u\n", sb->s_log_frag_size);
    printf("s_blocks_per_group: %u\n", sb->s_blocks_per_group);
    printf("s_frags_per_group: %u\n", sb->s_frags_per_group);
    printf("s_inodes_per_group: %u\n", sb->s_inodes_per_group);
    printf("s_mtime: %u\n", sb->s_mtime);
    printf("s_wtime: %u\n", sb->s_wtime);
    printf("s_mnt_count: %u\n", sb->s_mnt_count);
    printf("s_max_mnt_count: %u\n", sb->s_max_mnt_count);
    printf("s_magic: %u\n", sb->s_magic);
    printf("s_state: %u\n", sb->s_state);
    printf("s_errors: %u\n", sb->s_errors);
    printf("s_minor_rev_level: %u\n", sb->s_minor_rev_level);
    printf("s_lastcheck: %u\n", sb->s_lastcheck);
    printf("s_checkinterval: %u\n", sb->s_checkinterval);
    printf("s_creator_os: %u\n", sb->s_creator_os);
    printf("s_rev_level: %u\n", sb->s_rev_level);
    printf("s_def_resuid: %u\n", sb->s_def_resuid);
    printf("s_def_resgid: %u\n", sb->s_def_resgid);
}
