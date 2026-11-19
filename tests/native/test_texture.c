#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <tamtypes.h>
#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <gs_psm.h>
#include <draw.h>
#include "nexus_texture.h"

static qword_t packet_words[128] __attribute__((aligned(128)));
static packet_t test_packet;
static int alloc_result=0x1234;
static int packet_fail;
static int dma_result;
static int alloc_calls;
static int transfer_calls;
static int flush_calls;
static int bind_calls;
static int textured_draw_calls;
static int blend_state;
static int blend_on_calls;
static int blend_off_calls;
static unsigned int last_alloc_width,last_alloc_height;
static unsigned int last_transfer_width,last_transfer_height;
static unsigned int last_transfer_psm,last_transfer_dest,last_transfer_stride;

int graph_vram_allocate(int w,int h,int psm,int alignment) {
    assert(psm==GS_PSM_32); assert(alignment==GRAPH_ALIGN_BLOCK);
    ++alloc_calls; last_alloc_width=(unsigned int)w; last_alloc_height=(unsigned int)h;
    return alloc_result;
}
packet_t *packet_init(int qwords,int type) {
    assert(qwords==128); assert(type==PACKET_NORMAL);
    if(packet_fail) return 0;
    memset(packet_words,0,sizeof(packet_words));
    test_packet.data=packet_words; test_packet.qwords=(u32)qwords; return &test_packet;
}
void packet_free(packet_t *packet) { assert(packet==&test_packet); }
int dma_channel_send_chain(int channel,void *data,int qwc,int flags,int spr) {
    assert(channel==DMA_CHANNEL_GIF); assert(data==packet_words);
    assert(qwc>0 && qwc<128); assert(flags==0 && spr==0); return dma_result;
}
void dma_wait_fast(void) { }
qword_t *draw_texture_transfer(qword_t *q,void *src,int w,int h,int psm,int dest,int stride) {
    assert(((unsigned long)src & 127UL)==0UL);
    ++transfer_calls; last_transfer_width=(unsigned int)w; last_transfer_height=(unsigned int)h;
    last_transfer_psm=(unsigned int)psm; last_transfer_dest=(unsigned int)dest; last_transfer_stride=(unsigned int)stride;
    return q+8;
}
qword_t *draw_texture_flush(qword_t *q) { ++flush_calls; return q+2; }
qword_t *draw_texture_sampling(qword_t *q,int context,lod_t *lod) {
    assert(context==0); assert(lod->mag_filter==LOD_MAG_LINEAR); ++bind_calls; return q+1;
}
qword_t *draw_texturebuffer(qword_t *q,int context,texbuffer_t *tex,clutbuffer_t *clut) {
    assert(context==0); assert(tex->info.components==TEXTURE_COMPONENTS_RGBA);
    assert(clut->load_method==CLUT_NO_LOAD); ++bind_calls; return q+1;
}
unsigned char draw_log2(unsigned int x) {
    unsigned char r=0; while(x>1U){x>>=1;++r;} return r;
}
qword_t *draw_rect_textured(qword_t *q,int context,texrect_t *rect) {
    assert(context==0); assert(rect->color.a==0x80); assert(rect->t1.u>rect->t0.u);
    ++textured_draw_calls; assert(blend_state==1); return q+1;
}
void draw_enable_blending(void) { blend_state=1; ++blend_on_calls; }
void draw_disable_blending(void) { blend_state=0; ++blend_off_calls; }

int main(void) {
    NexusTexture texture;
    unsigned char pixels[256U*128U*4U];
    qword_t *q=packet_words;
    memset(&texture,0,sizeof(texture));
    memset(pixels,0x80,sizeof(pixels));

    assert(nexus_texture_init(0,256,128)==NEXUS_TEXTURE_ERR_ARGUMENT);
    assert(nexus_texture_init(&texture,250,128)==NEXUS_TEXTURE_ERR_DIMENSIONS);
    assert(nexus_texture_init(&texture,256,256)==NEXUS_TEXTURE_ERR_DIMENSIONS);
    alloc_result=-1;
    assert(nexus_texture_init(&texture,256,128)==NEXUS_TEXTURE_ERR_VRAM);
    alloc_result=0x1234;
    assert(nexus_texture_init(&texture,256,128)==NEXUS_TEXTURE_OK);
    assert(texture.allocated==1 && texture.uploaded==0);
    assert(texture.payload_bytes==sizeof(pixels));
    assert(texture.gs.address==0x1234 && texture.gs.info.width==8 && texture.gs.info.height==7);
    assert(nexus_texture_init(&texture,256,128)==NEXUS_TEXTURE_ERR_STATE);
    assert(nexus_texture_upload_rgba32(&texture,0,sizeof(pixels))==NEXUS_TEXTURE_ERR_ARGUMENT);
    assert(nexus_texture_upload_rgba32(&texture,pixels,sizeof(pixels)-1U)==NEXUS_TEXTURE_ERR_ARGUMENT);
    assert(nexus_texture_bind(q,0,&texture)==q);
    assert(nexus_texture_draw_region(q,0,&texture,0,0,64,64,0,0,64,64)==q);

    packet_fail=1;
    assert(nexus_texture_upload_rgba32(&texture,pixels,sizeof(pixels))==NEXUS_TEXTURE_ERR_PACKET);
    packet_fail=0;
    assert(nexus_texture_upload_rgba32(&texture,pixels,sizeof(pixels))==NEXUS_TEXTURE_OK);
    assert(texture.uploaded==1 && transfer_calls==1 && flush_calls==1);
    assert(last_transfer_width==256 && last_transfer_height==128);
    assert(last_transfer_psm==GS_PSM_32 && last_transfer_dest==0x1234 && last_transfer_stride==256);

    q=nexus_texture_bind(q,0,&texture);
    assert(q==packet_words+2 && bind_calls==2);
    q=nexus_texture_draw_region(q,0,&texture,10,20,64,48,64,0,64,64);
    assert(q==packet_words+5 && bind_calls==4 && textured_draw_calls==1);
    assert(blend_on_calls==1 && blend_off_calls==1 && blend_state==0);
    assert(nexus_texture_draw_region(q,0,&texture,0,0,64,64,240,0,64,64)==q);
    assert(textured_draw_calls==1);

    dma_result=-1;
    assert(nexus_texture_upload_rgba32(&texture,pixels,sizeof(pixels))==NEXUS_TEXTURE_ERR_DMA);
    assert(texture.uploaded==1);
    return 0;
}
