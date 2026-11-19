#include <tamtypes.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel.h>
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
    /* REF tags bypass the DMA chain's own cache sync; flush pixel source too. */
    SyncDCache((void *)(bytes + NXA_HEADER_SIZE),
               (void *)(bytes + NXA_HEADER_SIZE + NXA_PAYLOAD_SIZE));
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
