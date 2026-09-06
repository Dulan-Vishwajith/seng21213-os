#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define FRAME_SIZE 4096

void pmm_init(void);

uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys);

uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);

#endif
