#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "nexus_sha256.h"

#define ROTR32(value, bits) (((value) >> (bits)) | ((value) << (32U - (bits))))

static const uint32_t round_constants[64] = {
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,
    0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,
    0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,
    0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,
    0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,
    0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,
    0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,
    0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,
    0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};

static uint32_t read_be32(const unsigned char *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

static void write_be32(unsigned char *data, uint32_t value)
{
    data[0] = (unsigned char)(value >> 24);
    data[1] = (unsigned char)(value >> 16);
    data[2] = (unsigned char)(value >> 8);
    data[3] = (unsigned char)value;
}

static void transform(uint32_t state[8], const unsigned char block[64])
{
    uint32_t words[64];
    uint32_t a,b,c,d,e,f,g,h;
    unsigned int i;

    for (i = 0; i < 16U; ++i)
        words[i] = read_be32(block + (i * 4U));

    for (i = 16U; i < 64U; ++i) {
        uint32_t s0 = ROTR32(words[i-15U],7) ^
                      ROTR32(words[i-15U],18) ^ (words[i-15U] >> 3);
        uint32_t s1 = ROTR32(words[i-2U],17) ^
                      ROTR32(words[i-2U],19) ^ (words[i-2U] >> 10);
        words[i] = words[i-16U] + s0 + words[i-7U] + s1;
    }

    a=state[0]; b=state[1]; c=state[2]; d=state[3];
    e=state[4]; f=state[5]; g=state[6]; h=state[7];

    for (i = 0; i < 64U; ++i) {
        uint32_t sum1 = ROTR32(e,6) ^ ROTR32(e,11) ^ ROTR32(e,25);
        uint32_t choose = (e & f) ^ ((~e) & g);
        uint32_t t1 = h + sum1 + choose + round_constants[i] + words[i];
        uint32_t sum0 = ROTR32(a,2) ^ ROTR32(a,13) ^ ROTR32(a,22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = sum0 + majority;

        h=g; g=f; f=e; e=d+t1;
        d=c; c=b; b=a; a=t1+t2;
    }

    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

int nexus_sha256(const void *input, size_t length, unsigned char digest[32])
{
    static const uint32_t initial_state[8] = {
        0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
        0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U
    };
    const unsigned char *data = (const unsigned char *)input;
    uint32_t state[8];
    unsigned char final_blocks[128];
    size_t full_blocks = length / 64U;
    size_t remainder = length % 64U;
    size_t i;
    size_t final_size;
    uint64_t bit_length;

    if (digest == 0 || (input == 0 && length != 0U))
        return -1;

    memcpy(state, initial_state, sizeof(state));
    for (i = 0; i < full_blocks; ++i)
        transform(state, data + (i * 64U));

    memset(final_blocks, 0, sizeof(final_blocks));
    if (remainder != 0U)
        memcpy(final_blocks, data + (full_blocks * 64U), remainder);
    final_blocks[remainder] = 0x80U;
    final_size = (remainder < 56U) ? 64U : 128U;
    bit_length = (uint64_t)length * 8U;
    for (i = 0; i < 8U; ++i)
        final_blocks[final_size - 1U - i] =
            (unsigned char)(bit_length >> (i * 8U));

    transform(state, final_blocks);
    if (final_size == 128U)
        transform(state, final_blocks + 64U);

    for (i = 0; i < 8U; ++i)
        write_be32(digest + (i * 4U), state[i]);
    return 0;
}
