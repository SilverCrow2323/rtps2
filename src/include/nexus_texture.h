#ifndef NEXUS_TEXTURE_H
#define NEXUS_TEXTURE_H

#include <stddef.h>
#include <tamtypes.h>
#include <draw.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NEXUS_TEXTURE_MAX_PIXELS 32768U
#define NEXUS_TEXTURE_RGBA32_BYTES_PER_PIXEL 4U

typedef struct {
    texbuffer_t gs;
    u32 payload_bytes;
    u16 width;
    u16 height;
    u8 allocated;
    u8 uploaded;
} NexusTexture;

typedef enum {
    NEXUS_TEXTURE_OK = 0,
    NEXUS_TEXTURE_ERR_ARGUMENT = -1,
    NEXUS_TEXTURE_ERR_DIMENSIONS = -2,
    NEXUS_TEXTURE_ERR_VRAM = -3,
    NEXUS_TEXTURE_ERR_MEMORY = -4,
    NEXUS_TEXTURE_ERR_PACKET = -5,
    NEXUS_TEXTURE_ERR_DMA = -6,
    NEXUS_TEXTURE_ERR_STATE = -7
} NexusTextureResult;

/* Allocate a persistent GS slot. Slots are reused by re-uploading, not freed. */
int nexus_texture_init(NexusTexture *texture, unsigned int width,
                       unsigned int height);

/* Upload row-major RGBA32 bytes; source alpha must use the GS 0..128 range. */
int nexus_texture_upload_rgba32(NexusTexture *texture, const void *pixels,
                                size_t pixel_bytes);

/* Bind an uploaded texture to a GS context before drawing it. */
qword_t *nexus_texture_bind(qword_t *q, int context,
                            const NexusTexture *texture);

/* Draw a source rectangle (pixel UVs) into a destination rectangle.
   x/y coordinates use the renderer's local coordinate system. */
qword_t *nexus_texture_draw_region(qword_t *q, int context,
                                   const NexusTexture *texture,
                                   int x, int y, int width, int height,
                                   int source_x, int source_y,
                                   int source_width, int source_height);

#ifdef __cplusplus
}
#endif

#endif
