#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <kernel.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <gs_psm.h>
#include <draw.h>

#include "nexus_cover_cache.h"

#define COVER_PACKET_QWORDS 8300
#define COVER_SLOT_WORDS (NEXUS_COVER_CACHE_WIDTH * NEXUS_COVER_CACHE_HEIGHT)
#define COVER_BLOCK_WIDTH (NEXUS_COVER_CACHE_WIDTH * NEXUS_COVER_CACHE_SLOT_COUNT)

typedef struct {
    texbuffer_t texture;
    uint32_t key;
    uint64_t stamp;
    unsigned char valid;
} NexusCoverSlot;

static NexusCoverSlot slots[NEXUS_COVER_CACHE_SLOT_COUNT];
static uint64_t use_clock;
static int cache_is_ready;

static void touch_slot(unsigned int index)
{
    unsigned int i;

    if (use_clock == UINT64_MAX) {
        for (i = 0; i < NEXUS_COVER_CACHE_SLOT_COUNT; ++i)
            slots[i].stamp = 0;
        use_clock = 0;
    }

    slots[index].stamp = ++use_clock;
}

int nexus_cover_cache_init(void)
{
    int base;
    unsigned int i;

    if (cache_is_ready)
        return 0;

    /*
     * graph_vram_allocate() returns addresses in 32-bit GS words. Four
     * 128x128 RGBA32 images occupy four adjacent 16,384-word ranges.
     * One contiguous reservation respects PS2SDK's FIFO free semantics.
     */
    base = graph_vram_allocate((int)COVER_BLOCK_WIDTH,
                               (int)NEXUS_COVER_CACHE_HEIGHT,
                               GS_PSM_32, GRAPH_ALIGN_BLOCK);
    if (base < 0)
        return -1;

    memset(slots, 0, sizeof(slots));
    use_clock = 0;
    for (i = 0; i < NEXUS_COVER_CACHE_SLOT_COUNT; ++i) {
        slots[i].texture.width = NEXUS_COVER_CACHE_WIDTH;
        slots[i].texture.psm = GS_PSM_32;
        slots[i].texture.address =
            (unsigned int)base + (i * COVER_SLOT_WORDS);
        slots[i].texture.info.width = draw_log2(NEXUS_COVER_CACHE_WIDTH);
        slots[i].texture.info.height = draw_log2(NEXUS_COVER_CACHE_HEIGHT);
        slots[i].texture.info.components = TEXTURE_COMPONENTS_RGBA;
        slots[i].texture.info.function = TEXTURE_FUNCTION_DECAL;
    }

    cache_is_ready = 1;
    return 0;
}

int nexus_cover_cache_ready(void)
{
    return cache_is_ready;
}

int nexus_cover_cache_find(uint32_t key, unsigned int *slot_index)
{
    unsigned int i;

    if (!cache_is_ready || slot_index == 0)
        return -1;

    for (i = 0; i < NEXUS_COVER_CACHE_SLOT_COUNT; ++i) {
        if (slots[i].valid && slots[i].key == key) {
            touch_slot(i);
            *slot_index = i;
            return 0;
        }
    }

    return -1;
}

int nexus_cover_cache_store(uint32_t key, const void *pixels, size_t bytes)
{
    unsigned char valid[NEXUS_COVER_CACHE_SLOT_COUNT];
    uint64_t stamps[NEXUS_COVER_CACHE_SLOT_COUNT];
    packet_t *packet;
    qword_t *q;
    unsigned int i, slot;
    int send_result;

    if (!cache_is_ready || pixels == 0 ||
        bytes != NEXUS_COVER_CACHE_BYTES)
        return -1;
    if (((uintptr_t)pixels & 15U) != 0U)
        return -2;

    if (nexus_cover_cache_find(key, &slot) != 0) {
        for (i = 0; i < NEXUS_COVER_CACHE_SLOT_COUNT; ++i) {
            valid[i] = slots[i].valid;
            stamps[i] = slots[i].stamp;
        }
        i = (unsigned int)nexus_cover_cache_choose_slot(valid, stamps);
        if (i >= NEXUS_COVER_CACHE_SLOT_COUNT)
            return -3;
        slot = i;
    }

    packet = packet_init(COVER_PACKET_QWORDS, PACKET_NORMAL);
    if (packet == 0 || packet->data == 0) {
        if (packet != 0)
            packet_free(packet);
        return -4;
    }

    /* A failed/partial transfer must never leave stale pixels addressable. */
    slots[slot].valid = 0;
    q = draw_texture_transfer(packet->data, (void *)pixels,
                              NEXUS_COVER_CACHE_WIDTH,
                              NEXUS_COVER_CACHE_HEIGHT, GS_PSM_32,
                              (int)slots[slot].texture.address,
                              NEXUS_COVER_CACHE_WIDTH);
    q = draw_texture_flush(q);

    /* REF tags bypass the DMA chain's own cache sync; flush pixels too. */
    SyncDCache((void *)pixels,
               (void *)((const unsigned char *)pixels + bytes));
    dma_wait_fast();
    send_result = dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data,
                                         (int)(q - packet->data), 0, 0);
    if (send_result < 0) {
        packet_free(packet);
        return -5;
    }
    dma_wait_fast();
    packet_free(packet);

    slots[slot].key = key;
    slots[slot].valid = 1;
    touch_slot(slot);
    return (int)slot;
}

qword_t *nexus_cover_cache_draw(qword_t *q, int context, uint32_t key,
                                int x, int y, int width, int height)
{
    clutbuffer_t clut;
    lod_t lod;
    texrect_t rect;
    unsigned int slot;

    if (width <= 0 || height <= 0 ||
        nexus_cover_cache_find(key, &slot) != 0)
        return q;

    memset(&clut, 0, sizeof(clut));
    clut.storage_mode = CLUT_STORAGE_MODE1;
    clut.load_method = CLUT_NO_LOAD;

    memset(&lod, 0, sizeof(lod));
    lod.calculation = LOD_USE_K;
    lod.max_level = 0;
    lod.mag_filter = LOD_MAG_LINEAR;
    lod.min_filter = LOD_MIN_LINEAR;

    q = draw_texture_sampling(q, context, &lod);
    q = draw_texturebuffer(q, context, &slots[slot].texture, &clut);

    rect.v0.x = (float)x;
    rect.v0.y = (float)y;
    rect.v0.z = 0;
    rect.v1.x = (float)(x + width);
    rect.v1.y = (float)(y + height);
    rect.v1.z = 0;
    rect.t0.u = 0.5f;
    rect.t0.v = 0.5f;
    rect.t1.u = (float)NEXUS_COVER_CACHE_WIDTH - 0.5f;
    rect.t1.v = (float)NEXUS_COVER_CACHE_HEIGHT - 0.5f;
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
