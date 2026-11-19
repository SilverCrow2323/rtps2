#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "nexus_atlas.h"

#define PACK_SIZE (64U + 256U * 128U * 4U)

static const unsigned char zero_payload_sha256[32] = {
    0xfa,0x43,0x23,0x9b,0xce,0xe7,0xb9,0x7c,
    0xa6,0x2f,0x00,0x7c,0xc6,0x84,0x87,0x56,
    0x0a,0x39,0xe1,0x9f,0x74,0xf3,0xdd,0xe7,
    0x48,0x6d,0xb3,0xf9,0x8d,0xf8,0xe4,0x71
};

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
    {
        static const unsigned char zero_payload_sha256[32] = {
            0xfa,0x43,0x23,0x9b,0xce,0xe7,0xb9,0x7c,
            0xa6,0x2f,0x00,0x7c,0xc6,0x84,0x87,0x56,
            0x0a,0x39,0xe1,0x9f,0x74,0xf3,0xdd,0xe7,
            0x48,0x6d,0xb3,0xf9,0x8d,0xf8,0xe4,0x71
        };
        memcpy(pack + 20, zero_payload_sha256, sizeof(zero_payload_sha256));
    }

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
    put32(pack + 16, 256U * 128U * 4U);

    pack[64U + 123U] = 1U;
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -8);
    pack[64U + 123U] = 0U;
    pack[20] ^= 1U;
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -8);
    pack[20] ^= 1U;

    pack[52] = 1;
    assert(nexus_atlas_validate_pack(pack, PACK_SIZE) == -4);

    free(pack);
    return 0;
}
