#include "fs.h"
#include "ramdisk.h"

/*
 * Clear one RAM disk block.
 */
static int clear_block(uint32_t block)
{
    uint8_t buffer[BLOCK_SIZE];
    uint32_t i;

    for (i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }

    return ramdisk_write(block, buffer);
}

/*
 * Write the filesystem superblock.
 */
static int write_superblock(void)
{
    uint8_t buffer[BLOCK_SIZE];
    superblock_t sb;
    uint8_t *source;
    uint8_t *destination;
    uint32_t i;

    /*
     * Create superblock information.
     */
    sb.magic = FS_MAGIC;
    sb.total_blocks = FS_TOTAL_BLOCKS;
    sb.total_inodes = MAX_INODES;
    sb.free_blocks = FS_DATA_BLOCK_COUNT;
    sb.free_inodes = MAX_INODES;
    sb.inode_table_off = FS_INODE_TABLE_OFFSET;
    sb.data_off = FS_DATA_OFFSET;

    /*
     * Clear the whole block first.
     */
    for (i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }

    /*
     * Copy the superblock structure into the block buffer.
     */
    source = (uint8_t *)&sb;
    destination = buffer;

    for (i = 0; i < sizeof(superblock_t); i++) {
        destination[i] = source[i];
    }

    return ramdisk_write(FS_SUPERBLOCK_BLOCK, buffer);
}

/*
 * Initialise the Stage 4 file system.
 */
void fs_init(void)
{
    uint32_t block;

    /*
     * Create the superblock.
     */
    if (write_superblock() != 0) {
        return;
    }

    /*
     * Clear inode bitmap.
     *
     * 0 = free inode
     * 1 = allocated inode
     */
    clear_block(FS_INODE_BITMAP_BLOCK);

    /*
     * Clear block bitmap.
     *
     * 0 = free data block
     * 1 = allocated data block
     */
    clear_block(FS_BLOCK_BITMAP_BLOCK);

    /*
     * Clear the inode table.
     *
     * Inode table occupies 512 blocks.
     */
    for (block = FS_INODE_TABLE_BLOCK;
         block < FS_DATA_BLOCK;
         block++) {
        clear_block(block);
    }
}

/*
 * The remaining filesystem functions will be
 * implemented in later Stage 4 steps.
 */

int fs_open(const char *name, int flags)
{
    (void)name;
    (void)flags;
    return -1;
}

int fs_read(int fd, void *buf, int n)
{
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

int fs_write(int fd, const void *buf, int n)
{
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

void fs_close(int fd)
{
    (void)fd;
}

int fs_unlink(const char *name)
{
    (void)name;
    return -1;
}

int fs_ls(inode_t *out, int max)
{
    (void)out;
    (void)max;
    return 0;
}
