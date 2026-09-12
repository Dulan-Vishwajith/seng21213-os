#include "fs.h"
#include "ramdisk.h"




static file_t file_table[MAX_FDS];

static int clear_block(uint32_t block);




/*
 * Read the filesystem superblock.
 */
static int read_superblock(superblock_t *sb)
{
    uint8_t buffer[BLOCK_SIZE];
    uint8_t *source;
    uint8_t *destination;
    uint32_t i;

    if (sb == NULL) {
        return -1;
    }

    if (ramdisk_read(FS_SUPERBLOCK_BLOCK, buffer) != 0) {
        return -1;
    }

    source = buffer;
    destination = (uint8_t *)sb;

    for (i = 0; i < sizeof(superblock_t); i++) {
        destination[i] = source[i];
    }

    return 0;
}

/*
 * Write the filesystem superblock.
 */
static int update_superblock(const superblock_t *sb)
{
    uint8_t buffer[BLOCK_SIZE];
    uint8_t *source;
    uint8_t *destination;
    uint32_t i;

    if (sb == NULL) {
        return -1;
    }

    for (i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }

    source = (uint8_t *)sb;
    destination = buffer;

    for (i = 0; i < sizeof(superblock_t); i++) {
        destination[i] = source[i];
    }

    return ramdisk_write(FS_SUPERBLOCK_BLOCK, buffer);
}

/*
 * Test a data-block bitmap bit.
 */
static int data_bitmap_test(const uint8_t *bitmap, uint32_t block)
{
    uint32_t byte;
    uint32_t bit;

    byte = block / 8;
    bit = block % 8;

    return (bitmap[byte] & (1U << bit)) != 0;
}

/*
 * Set a data-block bitmap bit.
 */
static void data_bitmap_set(uint8_t *bitmap, uint32_t block)
{
    uint32_t byte;
    uint32_t bit;

    byte = block / 8;
    bit = block % 8;

    bitmap[byte] |= (uint8_t)(1U << bit);
}

/*
 * Clear a data-block bitmap bit.
 */
static void data_bitmap_clear(uint8_t *bitmap, uint32_t block)
{
    uint32_t byte;
    uint32_t bit;

    byte = block / 8;
    bit = block % 8;

    bitmap[byte] &= (uint8_t)~(1U << bit);
}

/*
 * Allocate one data block.
 *
 * Returns the data-block index, or -1 if none is available.
 *
 * The returned index is relative to FS_DATA_BLOCK.
 */
static int data_block_alloc(void)
{
    uint8_t bitmap[BLOCK_SIZE];
    superblock_t sb;
    uint32_t i;

    if (ramdisk_read(FS_BLOCK_BITMAP_BLOCK, bitmap) != 0) {
        return -1;
    }

    if (read_superblock(&sb) != 0) {
        return -1;
    }

    for (i = 0; i < FS_DATA_BLOCK_COUNT; i++) {

        if (!data_bitmap_test(bitmap, i)) {

            data_bitmap_set(bitmap, i);

            if (ramdisk_write(FS_BLOCK_BITMAP_BLOCK, bitmap) != 0) {
                return -1;
            }

            if (sb.free_blocks > 0) {
                sb.free_blocks--;
            }

            if (update_superblock(&sb) != 0) {
                /*
                 * Roll back bitmap allocation.
                 */
                data_bitmap_clear(bitmap, i);
                ramdisk_write(FS_BLOCK_BITMAP_BLOCK, bitmap);
                return -1;
            }

            /*
             * Clear the newly allocated block.
             */
            if (clear_block(FS_DATA_BLOCK + i) != 0) {
                data_bitmap_clear(bitmap, i);
                ramdisk_write(FS_BLOCK_BITMAP_BLOCK, bitmap);

                sb.free_blocks++;
                update_superblock(&sb);

                return -1;
            }

            return (int)i;
        }
    }

    return -1;
}

/*
 * Free one data block.
 */
static int data_block_free(uint32_t block)
{
    uint8_t bitmap[BLOCK_SIZE];
    superblock_t sb;

    if (block >= FS_DATA_BLOCK_COUNT) {
        return -1;
    }

    if (ramdisk_read(FS_BLOCK_BITMAP_BLOCK, bitmap) != 0) {
        return -1;
    }

    if (!data_bitmap_test(bitmap, block)) {
        return -1;
    }

    data_bitmap_clear(bitmap, block);

    if (ramdisk_write(FS_BLOCK_BITMAP_BLOCK, bitmap) != 0) {
        return -1;
    }

    if (read_superblock(&sb) != 0) {
        return -1;
    }

    sb.free_blocks++;

    return update_superblock(&sb);
}



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
 * Compare two strings.
 */
static int string_equal(const char *a, const char *b)
{
    uint32_t i;

    if (a == NULL || b == NULL) {
        return 0;
    }

    for (i = 0; i < 28; i++) {
        if (a[i] != b[i]) {
            return 0;
        }

        if (a[i] == '\0') {
            return 1;
        }
    }

    return 1;
}

/*
 * Find an inode by filename.
 *
 * Returns inode number or -1.
 */
static int find_inode(const char *name)
{
    uint8_t bitmap[BLOCK_SIZE];
    inode_t inode;
    uint32_t i;

    if (name == NULL) {
        return -1;
    }

    if (read_inode_bitmap(bitmap) != 0) {
        return -1;
    }

    for (i = 0; i < MAX_INODES; i++) {

        if (!inode_bitmap_test(bitmap, i)) {
            continue;
        }

        if (inode_read(i, &inode) != 0) {
            continue;
        }

        if (inode.type != INODE_FILE) {
            continue;
        }

        if (string_equal(inode.name, name)) {
            return (int)i;
        }
    }

    return -1;
}

/*
 * Find a free file descriptor.
 */
static int alloc_fd(void)
{
    uint32_t i;

    for (i = 0; i < MAX_FDS; i++) {
        if (!file_table[i].used) {
            file_table[i].used = 1;
            file_table[i].flags = 0;
            file_table[i].reserved = 0;
            file_table[i].inode_number = 0;
            file_table[i].position = 0;

            return (int)i;
        }
    }

    return -1;
}

/*
 * Release a file descriptor.
 */
static void free_fd(int fd)
{
    if (fd < 0 || fd >= MAX_FDS) {
        return;
    }

    file_table[fd].used = 0;
    file_table[fd].flags = 0;
    file_table[fd].reserved = 0;
    file_table[fd].inode_number = 0;
    file_table[fd].position = 0;
}







/*
 * Initialise the Stage 4 file system.
 */



void fs_init(void)
{
    uint32_t block;
    uint32_t i;

    if (write_superblock() != 0) {
        return;
    }

    clear_block(FS_INODE_BITMAP_BLOCK);
    clear_block(FS_BLOCK_BITMAP_BLOCK);

    for (block = FS_INODE_TABLE_BLOCK;
         block < FS_DATA_BLOCK;
         block++) {
        clear_block(block);
    }

    /*
     * Initialise the open-file table.
     */
    for (i = 0; i < MAX_FDS; i++) {
        file_table[i].used = 0;
        file_table[i].flags = 0;
        file_table[i].reserved = 0;
        file_table[i].inode_number = 0;
        file_table[i].position = 0;
    }
}



/*
 * These functions will be implemented in later steps.
 */
int fs_open(const char *name, int flags)
{
    int inode_number;
    int fd;
    inode_t inode;
    uint8_t *p;
    uint32_t i;

    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    /*
     * Find existing file.
     */
    inode_number = find_inode(name);

    /*
     * File does not exist.
     */
    if (inode_number < 0) {

        /*
         * Create only when O_CREAT is specified.
         */
        if (!(flags & O_CREAT)) {
            return -1;
        }

        inode_number = inode_alloc();

        if (inode_number < 0) {
            return -1;
        }

        /*
         * Read the newly allocated inode.
         */
        if (inode_read((uint32_t)inode_number, &inode) != 0) {
            inode_free((uint32_t)inode_number);
            return -1;
        }

        /*
         * Clear inode before assigning its name.
         */
        p = (uint8_t *)&inode;

        for (i = 0; i < sizeof(inode_t); i++) {
            p[i] = 0;
        }

        inode.type = INODE_FILE;

        /*
         * Copy filename.
         */
        for (i = 0; i < 27 && name[i] != '\0'; i++) {
            inode.name[i] = name[i];
        }

        inode.name[i] = '\0';

        if (inode_write((uint32_t)inode_number, &inode) != 0) {
            inode_free((uint32_t)inode_number);
            return -1;
        }
    }

    /*
     * Existing file.
     */
    else {

        if (inode_read((uint32_t)inode_number, &inode) != 0) {
            return -1;
        }

        /*
         * O_TRUNC resets the file size.
         *
         * Data block freeing will be implemented
         * in a later step.
         */




if (flags & O_TRUNC) {

    /*
     * Return all allocated data blocks.
     */
    for (i = 0; i < inode.block_count; i++) {
        data_block_free(inode.blocks[i]);
        inode.blocks[i] = 0;
    }

    inode.size = 0;
    inode.block_count = 0;

    if (inode_write((uint32_t)inode_number, &inode) != 0) {
        return -1;
    }
}
 



    }

    /*
     * Allocate a file descriptor.
     */
    fd = alloc_fd();

    if (fd < 0) {
        return -1;
    }

    file_table[fd].flags = (uint8_t)flags;
    file_table[fd].inode_number = (uint32_t)inode_number;
    file_table[fd].position = 0;

    return fd;
}








int fs_read(int fd, void *buf, int n)
{
    inode_t inode;
    uint32_t position;
    uint32_t available;
    uint32_t to_read;
    uint32_t block_index;
    uint32_t block_offset;
    uint32_t remaining;
    uint32_t destination_offset;
    uint32_t chunk;
    uint32_t i;
    uint8_t block_buffer[BLOCK_SIZE];
    uint8_t *destination;

    /*
     * Validate file descriptor.
     */
    if (fd < 0 || fd >= MAX_FDS) {
        return -1;
    }

    if (!file_table[fd].used) {
        return -1;
    }

    /*
     * File must be opened for reading.
     */
    if (!(file_table[fd].flags & O_RDONLY)) {
        return -1;
    }

    if (buf == NULL || n <= 0) {
        return -1;
    }

    /*
     * Read the inode.
     */
    if (inode_read(file_table[fd].inode_number, &inode) != 0) {
        return -1;
    }

    /*
     * Current position is stored in the file descriptor.
     */
    position = file_table[fd].position;

    /*
     * If we are already at or beyond EOF,
     * there is nothing to read.
     */
    if (position >= inode.size) {
        return 0;
    }

    /*
     * Calculate how many bytes are actually available.
     */
    available = inode.size - position;

    to_read = (uint32_t)n;

    if (to_read > available) {
        to_read = available;
    }

    destination = (uint8_t *)buf;

    remaining = to_read;
    destination_offset = 0;

    /*
     * Read one data block at a time.
     */
    while (remaining > 0) {

        block_index = position / BLOCK_SIZE;
        block_offset = position % BLOCK_SIZE;

        /*
         * Make sure the inode contains the required
         * direct block.
         */
        if (block_index >= inode.block_count ||
            block_index >= INODE_DIRECT) {
            return -1;
        }

        /*
         * Read the complete RAM-disk block.
         */
        if (ramdisk_read(
                FS_DATA_BLOCK + inode.blocks[block_index],
                block_buffer) != 0) {
            return -1;
        }

        /*
         * Determine how much data to copy from
         * this block.
         */
        chunk = BLOCK_SIZE - block_offset;

        if (chunk > remaining) {
            chunk = remaining;
        }

        /*
         * Copy data to the user's buffer.
         */
        for (i = 0; i < chunk; i++) {
            destination[destination_offset + i] =
                block_buffer[block_offset + i];
        }

        position += chunk;
        destination_offset += chunk;
        remaining -= chunk;
    }

    /*
     * Update the file position.
     */
    file_table[fd].position = position;

    return (int)to_read;
}







int fs_write(int fd, const void *buf, int n)
{
    inode_t inode;
    uint32_t position;
    uint32_t end_position;
    uint32_t first_block;
    uint32_t last_block;
    uint32_t required_blocks;
    uint32_t block_index;
    uint32_t block_offset;
    uint32_t remaining;
    uint32_t chunk;
    uint32_t source_offset;
    uint32_t i;
    uint8_t block_buffer[BLOCK_SIZE];
    const uint8_t *source;

    if (fd < 0 || fd >= MAX_FDS) {
        return -1;
    }

    if (!file_table[fd].used) {
        return -1;
    }

    if (!(file_table[fd].flags & O_WRONLY)) {
        return -1;
    }

    if (buf == NULL || n <= 0) {
        return -1;
    }

    /*
     * Maximum file size is:
     * 8 direct blocks × 512 bytes = 4096 bytes.
     */
    position = file_table[fd].position;
    end_position = position + (uint32_t)n;

    if (end_position > INODE_DIRECT * BLOCK_SIZE) {
        return -1;
    }

    if (inode_read(file_table[fd].inode_number, &inode) != 0) {
        return -1;
    }

    /*
     * Calculate the required number of blocks.
     */
    if (end_position == 0) {
        required_blocks = 0;
    } else {
        required_blocks =
            (end_position + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    /*
     * Allocate any additional blocks needed.
     */
    while (inode.block_count < required_blocks) {

        int new_block;

        new_block = data_block_alloc();

        if (new_block < 0) {
            return -1;
        }

        inode.blocks[inode.block_count] = (uint32_t)new_block;
        inode.block_count++;
    }

    /*
     * Write the data block by block.
     */
    source = (const uint8_t *)buf;
    source_offset = 0;
    remaining = (uint32_t)n;

    first_block = position / BLOCK_SIZE;
    last_block = (end_position - 1) / BLOCK_SIZE;

    for (block_index = first_block;
         block_index <= last_block;
         block_index++) {

        block_offset = 0;

        if (block_index == first_block) {
            block_offset = position % BLOCK_SIZE;
        }

        /*
         * Read the existing block first so that writing
         * part of a block does not destroy existing data.
         */
        if (ramdisk_read(
                FS_DATA_BLOCK + inode.blocks[block_index],
                block_buffer) != 0) {
            return -1;
        }

        chunk = BLOCK_SIZE - block_offset;

        if (chunk > remaining) {
            chunk = remaining;
        }

        for (i = 0; i < chunk; i++) {
            block_buffer[block_offset + i] =
                source[source_offset + i];
        }

        if (ramdisk_write(
                FS_DATA_BLOCK + inode.blocks[block_index],
                block_buffer) != 0) {
            return -1;
        }

        source_offset += chunk;
        remaining -= chunk;
    }

    /*
     * Update file size.
     */
    if (end_position > inode.size) {
        inode.size = end_position;
    }

    /*
     * Move file position forward.
     */
    file_table[fd].position = end_position;

    /*
     * Save inode.
     */
    if (inode_write(file_table[fd].inode_number, &inode) != 0) {
        return -1;
    }

    return n;
}







void fs_close(int fd)
{
    if (fd < 0 || fd >= MAX_FDS) {
        return;
    }

    if (!file_table[fd].used) {
        return;
    }

    free_fd(fd);
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
