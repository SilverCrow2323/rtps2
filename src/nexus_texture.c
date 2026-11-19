#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <gs_psm.h>
#include <draw.h>

#include "nexus_texture.h"

#define NEXUS_TEXTURE_PACKET_QWORDS 128
#define NEXUS_TEXTURE_DMA_ALIGNMENT 128

static int is_power_of_two(unsigned int value)
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

int nexus_texture_init(NexusTexture *texture, unsigned int width,
                       unsigned int height)
{
    int address;

    if (texture == 0) {
        return NEXUS_TEXTURE_ERR_ARGUMENT;
    }
    if (texture->allocated) {
        return NEXUS_TEXTURE_ERR_STATE;
    }
    if (!is_power_of_two(width) || !is_power_of_two(height) ||
        width > 256U || height > 256U ||
        width * height > NEXUS_TEXTURE_MAX_PIXELS) {
        return NEXUS_TEXTURE_ERR_DIMENSIONS;
    }

    memset(texture, 0, sizeof(*texture));
    address = graph_vram_allocate((int)width, (int)height,
                                  GS_PSM_32, GRAPH_ALIGN_BLOCK);
    if (address < 0) {
        return NEXUS_TEXTURE_ERR_VRAM;
    }

    texture->width = (u16)width;
    texture->height = (u16)height;
    texture->payload_bytes = width * height *
                             NEXUS_TEXTURE_RGBA32_BYTES_PER_PIXEL;
    texture->gs.address = (unsigned int)address;
    texture->gs.width = width;
    texture->gs.psm = GS_PSM_32;
    texture->gs.info.width = draw_log2(width);
    texture->gs.info.height = draw_log2(height);
    texture->gs.info.components = TEXTURE_COMPONENTS_RGBA;
    texture->gs.info.function = TEXTURE_FUNCTION_DECAL;
    texture->allocated = 1;
    texture->uploaded = 0;
    return NEXUS_TEXTURE_OK;
}

int nexus_texture_upload_rgba32(NexusTexture *texture, const void *pixels,
                                size_t pixel_bytes)
{
    packet_t *packet;
    qword_t *q;
    void *aligned_pixels;
    int send_result;

    if (texture == 0 || pixels == 0) {
        return NEXUS_TEXTURE_ERR_ARGUMENT;
    }
    if (!texture->allocated || texture->payload_bytes == 0U) {
        return NEXUS_TEXTURE_ERR_STATE;
    }
    if (pixel_bytes != (size_t)texture->payload_bytes) {
        return NEXUS_TEXTURE_ERR_ARGUMENT;
    }

    aligned_pixels = memalign(NEXUS_TEXTURE_DMA_ALIGNMENT, pixel_bytes);
    if (aligned_pixels == 0) {
        return NEXUS_TEXTURE_ERR_MEMORY;
    }
    memcpy(aligned_pixels, pixels, pixel_bytes);

    packet = packet_init(NEXUS_TEXTURE_PACKET_QWORDS, PACKET_NORMAL);
    if (packet == 0 || packet->data == 0) {
        if (packet != 0) {
            packet_free(packet);
        }
        free(aligned_pixels);
        return NEXUS_TEXTURE_ERR_PACKET;
    }

    q = packet->data;
    q = draw_texture_transfer(q, aligned_pixels,
                             (int)texture->width, (int)texture->height,
                             GS_PSM_32, (int)texture->gs.address,
                             (int)texture->gs.width);
    q = draw_texture_flush(q);
    send_result = dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data,
                                         (int)(q - packet->data), 0, 0);
    if (send_result < 0) {
        packet_free(packet);
        free(aligned_pixels);
        return NEXUS_TEXTURE_ERR_DMA;
    }

    /* The REF tags still point at aligned_pixels until this wait completes. */
    dma_wait_fast();
    packet_free(packet);
    free(aligned_pixels);
    texture->uploaded = 1;
    return NEXUS_TEXTURE_OK;
}

qword_t *nexus_texture_bind(qword_t *q, int context,
                            const NexusTexture *texture)
{
    clutbuffer_t clut;
    lod_t lod;

    if (q == 0 || texture == 0 || !texture->allocated || !texture->uploaded) {
        return q;
    }

    memset(&clut, 0, sizeof(clut));
    clut.storage_mode = CLUT_STORAGE_MODE1;
    clut.start = 0;
    clut.psm = 0;
    clut.load_method = CLUT_NO_LOAD;
    clut.address = 0;

    memset(&lod, 0, sizeof(lod));
    lod.calculation = LOD_USE_K;
    lod.max_level = 0;
    lod.mag_filter = LOD_MAG_LINEAR;
    lod.min_filter = LOD_MIN_LINEAR;
    lod.mipmap_select = LOD_MIPMAP_REGISTER;
    lod.l = 0;
    lod.k = 0.0f;

    q = draw_texture_sampling(q, context, &lod);
    return draw_texturebuffer(q, context, (texbuffer_t *)&texture->gs, &clut);
}

qword_t *nexus_texture_draw_region(qword_t *q, int context,
                                   const NexusTexture *texture,
                                   int x, int y, int width, int height,
                                   int source_x, int source_y,
                                   int source_width, int source_height)
{
    texrect_t rect;

    if (q == 0 || texture == 0 || !texture->allocated || !texture->uploaded ||
        width <= 0 || height <= 0 ||
        source_x < 0 || source_y < 0 ||
        source_width <= 0 || source_height <= 0 ||
        source_x + source_width > (int)texture->width ||
        source_y + source_height > (int)texture->height) {
        return q;
    }

    q = nexus_texture_bind(q, context, texture);

    rect.v0.x = (float)x;
    rect.v0.y = (float)y;
    rect.v0.z = 0;
    rect.v1.x = (float)(x + width);
    rect.v1.y = (float)(y + height);
    rect.v1.z = 0;
    rect.t0.u = (float)source_x;
    rect.t0.v = (float)source_y;
    rect.t1.u = (float)(source_x + source_width);
    rect.t1.v = (float)(source_y + source_height);
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
