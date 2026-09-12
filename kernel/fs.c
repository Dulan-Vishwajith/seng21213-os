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

    sb.magic = FS_MAGIC;
    sb.total_blocks = FS_TOTAL_BLOCKS;
    sb.total_inodes = MAX_INODES;
    sb.free_blocks = FS_DATA_BLOCK_COUNT;
    sb.free_inodes = MAX_INODES;
    sb.inode_table_off = FS_INODE_TABLE_OFFSET;
    sb.data_off = FS_DATA_OFFSET;

    for (i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }

    source = (uint8_t *)&sb;
    destination = buffer;

    for (i = 0; i < sizeof(superblock_t); i++) {
        destination[i] = source[i];
    }

    return ramdisk_write(FS_SUPERBLOCK_BLOCK, buffer);
}

/*
 * Read the inode bitmap.
 */
static int read_inode_bitmap(uint8_t *bitmap)
{
    return ramdisk_read(FS_INODE_BITMAP_BLOCK, bitmap);
}

/*
 * Write the inode bitmap.
 */
static int write_inode_bitmap(const uint8_t *bitmap)
{
    return ramdisk_write(FS_INODE_BITMAP_BLOCK, bitmap);
}

/*
 * Check whether an inode is allocated.
 *
 * 0 = free
 * 1 = allocated
 */
static int inode_bitmap_test(const uint8_t *bitmap, uint32_t inode)
{
    uint32_t byte;
    uint32_t bit;

    byte = inode / 8;
    bit = inode % 8;

    return (bitmap[byte] & (1U << bit)) != 0;
}

/*
 * Mark an inode as allocated.
 */
static void inode_bitmap_set(uint8_t *bitmap, uint32_t inode)
{
    uint32_t byte;
    uint32_t bit;

    byte = inode / 8;
    bit = inode % 8;

    bitmap[byte] |= (uint8_t)(1U << bit);
}

/*
 * Mark an inode as free.
 */
static void inode_bitmap_clear(uint8_t *bitmap, uint32_t inode)
{
    uint32_t byte;
    uint32_t bit;

    byte = inode / 8;
    bit = inode % 8;

    bitmap[byte] &= (uint8_t)~(1U << bit);
}

/*
 * Read one inode from the inode table.
 */
static int inode_read(uint32_t inode_number, inode_t *inode)
{
    uint8_t buffer[BLOCK_SIZE];
    uint32_t byte_offset;
    uint32_t block;
    uint32_t offset;
    uint8_t *source;
    uint8_t *destination;
    uint32_t i;

    if (inode == NULL || inode_number >= MAX_INODES) {
        return -1;
    }

    /*
     * Each inode occupies 256 bytes.
     */
    byte_offset = inode_number * sizeof(inode_t);

    block = FS_INODE_TABLE_BLOCK +
            (byte_offset / BLOCK_SIZE);

    offset = byte_offset % BLOCK_SIZE;

    if (ramdisk_read(block, buffer) != 0) {
        return -1;
    }

    source = buffer + offset;
    destination = (uint8_t *)inode;

    for (i = 0; i < sizeof(inode_t); i++) {
        destination[i] = source[i];
    }

    return 0;
}

/*
 * Write one inode to the inode table.
 */
static int inode_write(uint32_t inode_number, const inode_t *inode)
{
    uint8_t buffer[BLOCK_SIZE];
    uint32_t byte_offset;
    uint32_t block;
    uint32_t offset;
    uint8_t *source;
    uint8_t *destination;
    uint32_t i;

    if (inode == NULL || inode_number >= MAX_INODES) {
        return -1;
    }

    byte_offset = inode_number * sizeof(inode_t);

    block = FS_INODE_TABLE_BLOCK +
            (byte_offset / BLOCK_SIZE);

    offset = byte_offset % BLOCK_SIZE;

    /*
     * Inodes are 256 bytes and blocks are 512 bytes,
     * so an inode always fits completely inside one block.
     */
    if (ramdisk_read(block, buffer) != 0) {
        return -1;
    }

    source = (uint8_t *)inode;
    destination = buffer + offset;

    for (i = 0; i < sizeof(inode_t); i++) {
        destination[i] = source[i];
    }

    return ramdisk_write(block, buffer);
}

/*
 * Allocate one free inode.
 *
 * Returns inode number, or -1 if no inode is available.
 */
static int inode_alloc(void)
{
    uint8_t bitmap[BLOCK_SIZE];
    inode_t inode;
    uint32_t i;

    if (read_inode_bitmap(bitmap) != 0) {
        return -1;
    }

    for (i = 0; i < MAX_INODES; i++) {

        if (!inode_bitmap_test(bitmap, i)) {

            /*
             * Mark inode as allocated.
             */
            inode_bitmap_set(bitmap, i);

            if (write_inode_bitmap(bitmap) != 0) {
                return -1;
            }

            /*
             * Create a clean inode.
             */
            {
                uint8_t *p = (uint8_t *)&inode;

                uint32_t j;

                for (j = 0; j < sizeof(inode_t); j++) {
                    p[j] = 0;
                }
            }

            inode.type = INODE_FILE;

            if (inode_write(i, &inode) != 0) {
                /*
                 * Roll back bitmap allocation if inode write fails.
                 */
                inode_bitmap_clear(bitmap, i);
                write_inode_bitmap(bitmap);
                return -1;
            }

            return (int)i;
        }
    }

    return -1;
}

/*
 * Free an inode.
 */
static int inode_free(uint32_t inode_number)
{
    uint8_t bitmap[BLOCK_SIZE];
    inode_t inode;
    uint8_t *p;
    uint32_t i;

    if (inode_number >= MAX_INODES) {
        return -1;
    }

    if (read_inode_bitmap(bitmap) != 0) {
        return -1;
    }

    if (!inode_bitmap_test(bitmap, inode_number)) {
        return -1;
    }

    /*
     * Clear the inode.
     */
    p = (uint8_t *)&inode;

    for (i = 0; i < sizeof(inode_t); i++) {
        p[i] = 0;
    }

    if (inode_write(inode_number, &inode) != 0) {
        return -1;
    }

    /*
     * Mark inode as free.
     */
    inode_bitmap_clear(bitmap, inode_number);

    if (write_inode_bitmap(bitmap) != 0) {
        return -1;
    }

    return 0;
}

/*
 * Initialise the Stage 4 file system.
 */
void fs_init(void)
{
    uint32_t block;

    if (write_superblock() != 0) {
        return;
    }

    /*
     * Clear inode bitmap.
     */
    clear_block(FS_INODE_BITMAP_BLOCK);

    /*
     * Clear block bitmap.
     */
    clear_block(FS_BLOCK_BITMAP_BLOCK);

    /*
     * Clear inode table.
     */
    for (block = FS_INODE_TABLE_BLOCK;
         block < FS_DATA_BLOCK;
         block++) {
        clear_block(block);
    }
}

/*
 * These functions will be implemented in later steps.
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
