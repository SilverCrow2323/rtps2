#include "nexus_cover_cache_policy.h"

int nexus_cover_cache_choose_slot(
    const unsigned char valid[NEXUS_COVER_CACHE_SLOT_COUNT],
    const uint64_t stamp[NEXUS_COVER_CACHE_SLOT_COUNT])
{
    unsigned int i;
    unsigned int oldest = 0;

    if (valid == 0 || stamp == 0)
        return -1;

    for (i = 0; i < NEXUS_COVER_CACHE_SLOT_COUNT; ++i) {
        if (valid[i] == 0U)
            return (int)i;
    }

    for (i = 1; i < NEXUS_COVER_CACHE_SLOT_COUNT; ++i) {
        if (stamp[i] < stamp[oldest])
            oldest = i;
    }

    return (int)oldest;
}
