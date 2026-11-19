#ifndef TEST_GRAPH_H
#define TEST_GRAPH_H
#define GRAPH_MODE_PAL 2
#define GRAPH_ALIGN_PAGE 2048
#define GRAPH_ALIGN_BLOCK 64
int graph_get_region(void);
void graph_vram_clear(void);
int graph_vram_allocate(int width,int height,int psm,int alignment);
void graph_vram_free(int address);
int graph_initialize(int fbp,int width,int height,int psm,int x,int y);
void graph_set_framebuffer_filtered(int fbp,int width,int psm,int x,int y);
void graph_wait_vsync(void);
#endif
