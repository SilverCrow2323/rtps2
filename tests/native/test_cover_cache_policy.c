#include <assert.h>
#include <stdint.h>
#include "nexus_cover_cache_policy.h"

int main(void)
{
    unsigned char valid[NEXUS_COVER_CACHE_SLOT_COUNT] = {0,0,0,0};
    uint64_t stamp[NEXUS_COVER_CACHE_SLOT_COUNT] = {0,0,0,0};

    assert(nexus_cover_cache_choose_slot(valid, stamp) == 0);

    valid[0] = 1;
    valid[2] = 1;
    stamp[0] = 10;
    stamp[2] = 20;
    assert(nexus_cover_cache_choose_slot(valid, stamp) == 1);

    valid[1] = 1;
    valid[3] = 1;
    stamp[1] = 40;
    stamp[3] = 5;
    assert(nexus_cover_cache_choose_slot(valid, stamp) == 3);

    stamp[0] = 2;
    stamp[1] = 2;
    stamp[2] = 2;
    stamp[3] = 2;
    assert(nexus_cover_cache_choose_slot(valid, stamp) == 0);

    assert(nexus_cover_cache_choose_slot(0, stamp) == -1);
    assert(nexus_cover_cache_choose_slot(valid, 0) == -1);
    return 0;
}
