#ifndef TEST_FONT_H
#define TEST_FONT_H
#include <draw.h>
#define SINGLE_BYTE 0
#define LEFT_ALIGN 0
typedef struct {
    char name[9];
    char bold;
    char w_margin;
    char h_margin;
    int rowsize;
    int charsize;
    int offset;
    char *font;
} fontx_t;
int fontx_load(const char *path, fontx_t *fontx, int type, int wmargin, int hmargin, int bold);
void fontx_unload(fontx_t *fontx);
qword_t *fontx_print_ascii(qword_t *q, int context, const unsigned char *str, int alignment, const vertex_t *position, const color_t *color, fontx_t *fontx);
#endif
