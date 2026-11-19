#ifndef TEST_DRAW_H
#define TEST_DRAW_H
#include <tamtypes.h>
#include <math3d.h>
#define DRAW_DISABLE 0
#define ZTEST_METHOD_GREATER 1
#define GRAPH_ALIGN_BLOCK 256
#define TEXTURE_COMPONENTS_RGBA 1
#define TEXTURE_FUNCTION_DECAL 1
#define CLUT_STORAGE_MODE1 0
#define CLUT_NO_LOAD 0
#define LOD_USE_K 0
#define LOD_MAG_NEAREST 0
#define LOD_MIN_NEAREST 0
typedef struct { unsigned int address,width,height,psm,mask; } framebuffer_t;
typedef struct { unsigned int enable,method,address,zsm,mask; } zbuffer_t;
typedef union { u64 rgbaq; struct { u8 r,g,b,a; float q; }; } color_t;
typedef struct { float x,y; unsigned int z; } vertex_t;
typedef union { u64 uv; struct { float u,v; }; } texel_t;
typedef struct { vertex_t v0,v1; color_t color; } rect_t;
typedef struct { unsigned char width,height,components,function; } texinfo_t;
typedef struct { unsigned int address,width,psm; texinfo_t info; } texbuffer_t;
typedef struct { unsigned int storage_mode,start,psm,load_method,address; } clutbuffer_t;
typedef struct { unsigned char calculation,max_level,mag_filter,min_filter; float l,k; } lod_t;
typedef struct { vertex_t v0; texel_t t0; vertex_t v1; texel_t t1; color_t color; } texrect_t;
qword_t *draw_setup_environment(qword_t *,int,framebuffer_t *,zbuffer_t *);
qword_t *draw_primitive_xyoffset(qword_t *,int,float,float);
qword_t *draw_rect_filled(qword_t *,int,rect_t *);
qword_t *draw_rect_textured(qword_t *,int,texrect_t *);
void draw_enable_blending(void);
void draw_disable_blending(void);
qword_t *draw_framebuffer(qword_t *,int,framebuffer_t *);
qword_t *draw_clear(qword_t *,int,float,float,float,float,int,int,int);
qword_t *draw_finish(qword_t *);
qword_t *draw_texture_transfer(qword_t *,void *,int,int,int,int,int);
qword_t *draw_texture_flush(qword_t *);
qword_t *draw_texture_sampling(qword_t *,int,lod_t *);
qword_t *draw_texturebuffer(qword_t *,int,texbuffer_t *,clutbuffer_t *);
unsigned char draw_log2(unsigned int);
void draw_wait_finish(void);
#endif
