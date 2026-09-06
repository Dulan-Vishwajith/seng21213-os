#include "pmm.h"
#include "string.h"

#define MAX_MEMORY (32 * 1024 * 1024)
#define TOTAL_FRAMES (MAX_MEMORY / FRAME_SIZE)
#define BITMAP_SIZE (TOTAL_FRAMES / 32)

/* One bit represents one 4 KB frame */
static uint32_t bitmap[BITMAP_SIZE];

static uint32_t total_frames;

static uint32_t free_frames;




typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;





/* Set a frame as used */
static void bitmap_set(uint32_t frame)
{
    bitmap[frame / 32] |= (1u << (frame % 32));
}

/* Set a frame as free */
static void bitmap_clear(uint32_t frame)
{
    bitmap[frame / 32] &= ~(1u << (frame % 32));
}

/* Check whether a frame is used */
static int bitmap_test(uint32_t frame)
{
    return (bitmap[frame / 32] >> (frame % 32)) & 1;
}




void pmm_init(void)
{
    /* Start with every frame marked as used */
    memset(bitmap, 0xFF, sizeof(bitmap));

    total_frames = TOTAL_FRAMES;
    free_frames = 0;

    /* E820 information written by the bootloader */
    uint16_t count = *(uint16_t *)0x8000;
    e820_entry_t *map = (e820_entry_t *)0x8004;

    for (uint16_t i = 0; i < count; i++)
    {
        /* Type 1 means usable RAM */
        if (map[i].type != 1)
        {
            continue;
        }

        uint32_t start = (uint32_t)map[i].base;
        uint32_t length = (uint32_t)map[i].length;
        uint32_t end = start + length;

        /* Do not manage memory outside our 32 MB bitmap */
        if (start >= MAX_MEMORY)
        {
            continue;
        }

        if (end > MAX_MEMORY || end < start)
        {
            end = MAX_MEMORY;
        }

        /* Keep the first 1 MB reserved */
        if (start < 0x100000)
        {
            start = 0x100000;
        }

        /* Align start to the next 4 KB frame */
        start = (start + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1);

        for (uint32_t address = start;
             address + FRAME_SIZE <= end;
             address += FRAME_SIZE)
        {
            uint32_t frame = address / FRAME_SIZE;

            if (bitmap_test(frame))
            {
                bitmap_clear(frame);
                free_frames++;
            }
        }
    }
}






uint32_t pmm_alloc_frame(void)
{
    for (uint32_t i = 0; i < total_frames; i++)
    {
        if (!bitmap_test(i))
        {
            bitmap_set(i);
            free_frames--;

            return i * FRAME_SIZE;
        }
    }

    return 0;
}

void pmm_free_frame(uint32_t phys)
{
    uint32_t frame = phys / FRAME_SIZE;

    if (frame >= total_frames)
    {
        return;
    }

    if (bitmap_test(frame))
    {
        bitmap_clear(frame);
        free_frames++;
    }
}

uint32_t pmm_free_frames(void)
{
    return free_frames;
}

uint32_t pmm_total_frames(void)
{
    return total_frames;
}
