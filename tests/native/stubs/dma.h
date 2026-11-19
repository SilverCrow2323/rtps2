#ifndef TEST_DMA_H
#define TEST_DMA_H
#define DMA_CHANNEL_GIF 2
int dma_channel_initialize(int channel, void *handler, int flags);
void dma_channel_fast_waits(int channel);
void dma_wait_fast(void);
int dma_channel_send_normal(int channel, void *data, int qwc, int flags, int spr);
#endif
