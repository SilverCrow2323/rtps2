#include <tamtypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <gs_psm.h>
#include <draw.h>

#include "nexus_atlas.h"
#include "nexus_sha256.h"

#define NXA_HEADER_SIZE 64U
#define NXA_WIDTH 256U
#define NXA_HEIGHT 128U
#define NXA_PAYLOAD_SIZE (NXA_WIDTH * NXA_HEIGHT * 4U)
#define NXA_TOTAL_SIZE (NXA_HEADER_SIZE + NXA_PAYLOAD_SIZE)
#define NXA_VERSION 1U
#define NXA_PSMCT32 0U
#define NXA_ICON_COUNT 6U
#define NXA_ICON_SIZE 64U
#define NXA_TRANSFER_QWORDS 8300


static texbuffer_t atlas_texture;
static int atlas_is_ready;

/* Compact SHA-256 used to authenticate the NXA1 pixel payload before DMA. */
static const uint32_t sha_k[64] = {
0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};
static uint32_t sha_rotr(uint32_t x, unsigned int n) { return (x >> n) | (x << (32U - n)); }
static void sha_transform(uint32_t h[8], const unsigned char block[64])
{
    uint32_t w[64],a,b,c,d,e,f,g,x,y,t1,t2; unsigned int i;
    for(i=0;i<16U;i++){unsigned int j=i*4U;w[i]=((uint32_t)block[j]<<24)|((uint32_t)block[j+1U]<<16)|((uint32_t)block[j+2U]<<8)|block[j+3U];}
    for(i=16U;i<64U;i++){x=w[i-15U];y=w[i-2U];w[i]=w[i-16U]+(sha_rotr(x,7)^sha_rotr(x,18)^(x>>3))+w[i-7U]+(sha_rotr(y,17)^sha_rotr(y,19)^(y>>10));}
    a=h[0];b=h[1];c=h[2];d=h[3];e=h[4];f=h[5];g=h[6];x=h[7];
    for(i=0;i<64U;i++){t1=x+(sha_rotr(e,6)^sha_rotr(e,11)^sha_rotr(e,25))+((e&f)^(~e&g))+sha_k[i]+w[i];t2=(sha_rotr(a,2)^sha_rotr(a,13)^sha_rotr(a,22))+((a&b)^(a&c)^(b&c));x=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=x;
}
static void sha256_payload(const unsigned char *data, size_t length, unsigned char digest[32])
{
    uint32_t h[8]={0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};
    unsigned char block[64]; size_t offset=0,used; unsigned int i;
    while(length-offset>=64U){sha_transform(h,data+offset);offset+=64U;}
    used=length-offset;memset(block,0,sizeof(block));if(used)memcpy(block,data+offset,used);
    block[used++]=0x80U;
    if(used>56U){sha_transform(h,block);memset(block,0,sizeof(block));}
    {uint64_t bits=(uint64_t)length*8U;for(i=0;i<8U;i++)block[63U-i]=(unsigned char)(bits>>(i*8U));}
    sha_transform(h,block);
    for(i=0;i<8U;i++){digest[i*4U]=(unsigned char)(h[i]>>24);digest[i*4U+1U]=(unsigned char)(h[i]>>16);digest[i*4U+2U]=(unsigned char)(h[i]>>8);digest[i*4U+3U]=(unsigned char)h[i];}
}

static unsigned int read_le16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int read_le32(const unsigned char *p)
{
    return (unsigned int)p[0] |
           ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) |
           ((unsigned int)p[3] << 24);
}

int nexus_atlas_validate_pack(const void *pack, size_t size)
{
    const unsigned char *data = (const unsigned char *)pack;
    if (data == 0 || size != NXA_TOTAL_SIZE)
        return -1;
    if (data[0] != 'N' || data[1] != 'X' ||
        data[2] != 'A' || data[3] != '1')
        return -2;
    if (read_le16(data + 4) != NXA_VERSION ||
        read_le16(data + 6) != NXA_HEADER_SIZE)
        return -3;
    if (read_le16(data + 8) != NXA_WIDTH ||
        read_le16(data + 10) != NXA_HEIGHT ||
        read_le16(data + 12) != NXA_PSMCT32 ||
        read_le16(data + 14) != 0U ||
        read_le32(data + 16) != NXA_PAYLOAD_SIZE)
        return -4;
    {
        unsigned int i;
        unsigned char digest[32];
        for (i = 52U; i < NXA_HEADER_SIZE; ++i) {
            if (data[i] != 0U)
                return -4;
        }
        if (nexus_sha256(data + NXA_HEADER_SIZE, NXA_PAYLOAD_SIZE, digest) != 0)
            return -8;
        for (i = 0; i < 32U; ++i) {
            if (digest[i] != data[20U + i])
                return -8;
        }
    }
    return 0;
}

void nexus_atlas_release(void)
{
    if (atlas_is_ready) {
        graph_vram_free((int)atlas_texture.address);
        memset(&atlas_texture, 0, sizeof(atlas_texture));
        atlas_is_ready = 0;
    }
}

int nexus_atlas_upload(const void *pack, size_t pack_size)
{
    const unsigned char *bytes = (const unsigned char *)pack;
    packet_t *packet;
    qword_t *q;
    int result;
    int dma_result;

    result = nexus_atlas_validate_pack(bytes, pack_size);
    if (result != 0)
        return result;

    /* DMA source must be suitably aligned. The NXA1 payload starts at +64. */
    if (((uintptr_t)(bytes + NXA_HEADER_SIZE) & 15U) != 0U)
        return -5;

    nexus_atlas_release();
    memset(&atlas_texture, 0, sizeof(atlas_texture));
    atlas_texture.width = NXA_WIDTH;
    atlas_texture.psm = GS_PSM_32;
    atlas_texture.address = (unsigned int)graph_vram_allocate(
        NXA_WIDTH, NXA_HEIGHT, GS_PSM_32, GRAPH_ALIGN_BLOCK);
    if (atlas_texture.address == (unsigned int)-1)
        return -6;

    packet = packet_init(NXA_TRANSFER_QWORDS, PACKET_NORMAL);
    if (packet == 0 || packet->data == 0) {
        graph_vram_free((int)atlas_texture.address);
        memset(&atlas_texture, 0, sizeof(atlas_texture));
        return -7;
    }

    q = draw_texture_transfer(packet->data,
                              (void *)(bytes + NXA_HEADER_SIZE),
                              NXA_WIDTH, NXA_HEIGHT, GS_PSM_32,
                              (int)atlas_texture.address, NXA_WIDTH);
    q = draw_texture_flush(q);
    dma_wait_fast();
    dma_result = dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data,
                                        (int)(q - packet->data), 0, 0);
    if (dma_result < 0) {
        packet_free(packet);
        graph_vram_free((int)atlas_texture.address);
        memset(&atlas_texture, 0, sizeof(atlas_texture));
        return -9;
    }
    dma_wait_fast();
    packet_free(packet);

    atlas_texture.info.width = draw_log2(NXA_WIDTH);
    atlas_texture.info.height = draw_log2(NXA_HEIGHT);
    atlas_texture.info.components = TEXTURE_COMPONENTS_RGBA;
    atlas_texture.info.function = TEXTURE_FUNCTION_DECAL;
    atlas_is_ready = 1;
    return 0;
}

int nexus_atlas_ready(void)
{
    return atlas_is_ready;
}

qword_t *nexus_atlas_bind(qword_t *q, int context)
{
    clutbuffer_t clut;
    lod_t lod;

    if (!atlas_is_ready)
        return q;

    memset(&clut, 0, sizeof(clut));
    clut.storage_mode = CLUT_STORAGE_MODE1;
    clut.load_method = CLUT_NO_LOAD;

    memset(&lod, 0, sizeof(lod));
    lod.calculation = LOD_USE_K;
    lod.max_level = 0;
    lod.mag_filter = LOD_MAG_NEAREST;
    lod.min_filter = LOD_MIN_NEAREST;

    q = draw_texture_sampling(q, context, &lod);
    q = draw_texturebuffer(q, context, &atlas_texture, &clut);
    return q;
}

qword_t *nexus_atlas_draw(qword_t *q, int context, unsigned int icon,
                          int x, int y, int size)
{
    texrect_t rect;
    unsigned int cell_x, cell_y;

    if (!atlas_is_ready || icon >= NXA_ICON_COUNT || size <= 0)
        return q;

    cell_x = (icon % 4U) * NXA_ICON_SIZE;
    cell_y = (icon / 4U) * NXA_ICON_SIZE;

    rect.v0.x = (float)x;
    rect.v0.y = (float)y;
    rect.v0.z = 0;
    rect.v1.x = (float)(x + size);
    rect.v1.y = (float)(y + size);
    rect.v1.z = 0;

    /* draw_rect_textured converts pixel-space UVs to 12.4; inset half a texel. */
    rect.t0.u = (float)cell_x + 0.5f;
    rect.t0.v = (float)cell_y + 0.5f;
    rect.t1.u = (float)(cell_x + NXA_ICON_SIZE) - 0.5f;
    rect.t1.v = (float)(cell_y + NXA_ICON_SIZE) - 0.5f;

    rect.color.r = 0x80;
    rect.color.g = 0x80;
    rect.color.b = 0x80;
    rect.color.a = 0x80;
    rect.color.q = 1.0f;
    draw_enable_blending();
    q = draw_rect_textured(q, context, &rect);
    draw_disable_blending();
    return q;
}
