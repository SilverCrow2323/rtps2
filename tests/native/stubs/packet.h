#ifndef TEST_PACKET_H
#define TEST_PACKET_H
#include <tamtypes.h>
#define PACKET_NORMAL 0
typedef struct {
    u32 qwords;
    u16 qwc;
    u16 type;
    qword_t *data;
} packet_t;
packet_t *packet_init(int qwords, int type);
void packet_free(packet_t *packet);
#endif
