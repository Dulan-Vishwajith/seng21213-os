#include "pmm.h"
#include "string.h"

#define MAX_MEMORY (32 * 1024 * 1024)
#define TOTAL_FRAMES (MAX_MEMORY / FRAME_SIZE)
#define BITMAP_SIZE (TOTAL_FRAMES / 32)

/* One bit represents one 4 KB frame */
static uint32_t bitmap[BITMAP_SIZE];

static uint32_t total_frames;
static uint32_t free_frames;

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
    /* Start by marking all frames as used */
    memset(bitmap, 0xFF, sizeof(bitmap));

    total_frames = TOTAL_FRAMES;
    free_frames = 0;

    /*
     * Temporarily make memory above 1 MB available.
     * We will improve this later using the E820 memory map.
     */

    for (uint32_t address = 0x100000;
         address < MAX_MEMORY;
         address += FRAME_SIZE)
    {
        uint32_t frame = address / FRAME_SIZE;

        bitmap_clear(frame);
        free_frames++;
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
