#ifndef FS_H
#define FS_H

#include "../include/types.h"

/*
 * Stage 4 File System Constants
 */

#define FS_MAGIC        0x21213F5
#define BLOCK_SIZE      512
#define MAX_INODES      1024
#define INODE_DIRECT    8

/*
 * File types
 */
#define INODE_FREE      0
#define INODE_FILE      1
#define INODE_DIR       2

/*
 * fs_open() flags
 */
#define O_RDONLY        0x01
#define O_WRONLY        0x02
#define O_CREAT         0x04
#define O_TRUNC         0x08

/*
 * Superblock
 *
 * Stored at RAM disk offset 0x000.
 */
typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t inode_table_off;
    uint32_t data_off;
} superblock_t;

/*
 * Inode
 *
 * Each inode is 256 bytes.
 */
typedef struct {
    uint32_t size;
    uint32_t blocks[INODE_DIRECT];
    uint32_t block_count;
    uint8_t  type;
    char     name[28];
} inode_t;

/*
 * File System API
 */

void fs_init(void);

int fs_open(const char *name, int flags);

int fs_read(int fd, void *buf, int n);

int fs_write(int fd, const void *buf, int n);

void fs_close(int fd);

int fs_unlink(const char *name);

int fs_ls(inode_t *out, int max);

#endif
