#ifndef TEST_DRAW_H
#define TEST_DRAW_H
#include <tamtypes.h>
#include <math3d.h>

#define DRAW_DISABLE 0
#define ZTEST_METHOD_GREATER 1

typedef struct {
    unsigned int address;
    unsigned int width;
    unsigned int height;
    unsigned int psm;
    unsigned int mask;
} framebuffer_t;

typedef struct {
    unsigned int enable;
    unsigned int method;
    unsigned int address;
    unsigned int zsm;
    unsigned int mask;
} zbuffer_t;

typedef union {
    u64 rgbaq;
    struct { u8 r, g, b, a; float q; };
} color_t;

typedef struct {
    float x;
    float y;
    unsigned int z;
} vertex_t;

typedef struct {
    vertex_t v0;
    vertex_t v1;
    color_t color;
} rect_t;

qword_t *draw_setup_environment(qword_t *q, int context, framebuffer_t *frame, zbuffer_t *z);
qword_t *draw_primitive_xyoffset(qword_t *q, int context, float x, float y);
qword_t *draw_rect_filled(qword_t *q, int context, rect_t *rect);
qword_t *draw_framebuffer(qword_t *q, int context, framebuffer_t *frame);
qword_t *draw_clear(qword_t *q, int context, float x, float y, float width, float height, int r, int g, int b);
qword_t *draw_finish(qword_t *q);
void draw_wait_finish(void);
#endif
