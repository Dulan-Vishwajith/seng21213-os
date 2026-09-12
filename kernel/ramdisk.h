#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

#define RAMDISK_SIZE        (1024 * 1024)
#define RAMDISK_BLOCK_SIZE  512
#define RAMDISK_BLOCK_COUNT (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)

/*
 * Keep the RAM disk away from the kernel and its stack.
 * QEMU is configured with 32 MB RAM.
 */
#define RAMDISK_BASE 0x00100000U

void ramdisk_init(void);
int ramdisk_read(uint32_t block, void *buffer);
int ramdisk_write(uint32_t block, const void *buffer);
uint32_t ramdisk_block_count(void);

#endif
