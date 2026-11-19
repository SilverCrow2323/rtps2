#ifndef NEXUS_COVER_CACHE_H
#define NEXUS_COVER_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <tamtypes.h>
#include <draw.h>

#include "nexus_cover_cache_policy.h"

#define NEXUS_COVER_CACHE_WIDTH 128U
#define NEXUS_COVER_CACHE_HEIGHT 128U
#define NEXUS_COVER_CACHE_BYTES (NEXUS_COVER_CACHE_WIDTH * NEXUS_COVER_CACHE_HEIGHT * 4U)
#define NEXUS_COVER_CACHE_TOTAL_BYTES (NEXUS_COVER_CACHE_BYTES * NEXUS_COVER_CACHE_SLOT_COUNT)

/* Reserves one contiguous 512x128 PSMCT32 area; call once after the atlas. */
int nexus_cover_cache_init(void);
int nexus_cover_cache_ready(void);

/* pixels must be 16-byte aligned RGBA32, 128x128, row-major. */
int nexus_cover_cache_store(uint32_t key, const void *pixels, size_t bytes);
int nexus_cover_cache_find(uint32_t key, unsigned int *slot_index);

/* x/y are GS centered coordinates, matching nexus_atlas_draw(). */
qword_t *nexus_cover_cache_draw(qword_t *q, int context, uint32_t key,
                                int x, int y, int width, int height);

#endif
