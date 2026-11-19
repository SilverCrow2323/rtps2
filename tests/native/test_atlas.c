#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "nexus_atlas.h"

#define PACK_SIZE (64U + 256U * 128U * 4U)

static void put16(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

int main(void)
{
    unsigned char *pack = (unsigned char *)calloc(1, PACK_SIZE);
    assert(pack != 0);
    memcpy(pack, "NXA1", 4);
    put16(pack + 4, 1);
    put16(pack + 6, 64);
    put16(pack + 8, 256);
    put16(pack + 10, 128);
    put16(pack + 12, 0);
    put16(pack + 14, 0);
    put32(pack + 16, 256U * 128U * 4U);

    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == 0);
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE - 1U) == -1);
    assert(nexus_atlas_validate_pack(0, PACK_SIZE) == -1);

    pack[0] = 'X';
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -2);
    pack[0] = 'N';

    put16(pack + 4, 2);
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -3);
    put16(pack + 4, 1);

    put16(pack + 8, 128);
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -4);
    put16(pack + 8, 256);

    put16(pack + 14, 1);
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -4);
    put16(pack + 14, 0);

    put32(pack + 16, 1);
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -4);

    free(pack);
    return 0;
}
