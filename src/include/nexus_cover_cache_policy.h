#ifndef NEXUS_COVER_CACHE_POLICY_H
#define NEXUS_COVER_CACHE_POLICY_H

#include <stdint.h>

#define NEXUS_COVER_CACHE_SLOT_COUNT 4U

/* Returns the first invalid slot, otherwise the least-recently-used slot. */
int nexus_cover_cache_choose_slot(
    const unsigned char valid[NEXUS_COVER_CACHE_SLOT_COUNT],
    const uint64_t stamp[NEXUS_COVER_CACHE_SLOT_COUNT]);

#endif
