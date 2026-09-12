#include "ramdisk.h"

static volatile uint8_t *const ramdisk =
    (volatile uint8_t *)RAMDISK_BASE;

void ramdisk_init(void)
{
    uint32_t i;

    for (i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk[i] = 0;
    }
}

int ramdisk_read(uint32_t block, void *buffer)
{
    uint32_t offset;
    uint32_t i;
    uint8_t *destination;

    if (buffer == NULL) {
        return -1;
    }

    if (block >= RAMDISK_BLOCK_COUNT) {
        return -1;
    }

    offset = block * RAMDISK_BLOCK_SIZE;
    destination = (uint8_t *)buffer;

    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        destination[i] = ramdisk[offset + i];
    }

    return 0;
}

int ramdisk_write(uint32_t block, const void *buffer)
{
    uint32_t offset;
    uint32_t i;
    const uint8_t *source;

    if (buffer == NULL) {
        return -1;
    }

    if (block >= RAMDISK_BLOCK_COUNT) {
        return -1;
    }

    offset = block * RAMDISK_BLOCK_SIZE;
    source = (const uint8_t *)buffer;

    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        ramdisk[offset + i] = source[i];
    }

    return 0;
}

uint32_t ramdisk_block_count(void)
{
    return RAMDISK_BLOCK_COUNT;
}
