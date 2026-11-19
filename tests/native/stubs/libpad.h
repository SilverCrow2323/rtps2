#ifndef TEST_LIBPAD_H
#define TEST_LIBPAD_H

#define PAD_STATE_DISCONN 0x00
#define PAD_STATE_STABLE 0x06

#define PAD_LEFT      0x0080
#define PAD_DOWN      0x0040
#define PAD_RIGHT     0x0020
#define PAD_UP        0x0010
#define PAD_START     0x0008
#define PAD_SELECT    0x0001
#define PAD_CROSS     0x4000
#define PAD_SQUARE    0x8000
#define PAD_L2        0x0100
#define PAD_R2        0x0200
#define PAD_CIRCLE    0x2000
#define PAD_TRIANGLE  0x1000
#define PAD_L1        0x0400
#define PAD_R1        0x0800

struct padButtonStatus {
    unsigned char ok;
    unsigned char mode;
    unsigned short btns;
    unsigned char rjoy_h;
    unsigned char rjoy_v;
    unsigned char ljoy_h;
    unsigned char ljoy_v;
    unsigned char right_p;
    unsigned char left_p;
    unsigned char up_p;
    unsigned char down_p;
    unsigned char triangle_p;
    unsigned char circle_p;
    unsigned char cross_p;
    unsigned char square_p;
    unsigned char l1_p;
    unsigned char r1_p;
    unsigned char l2_p;
    unsigned char r2_p;
    unsigned char unkn16[12];
} __attribute__((packed));

int padInit(int mode);
int padPortOpen(int port, int slot, void *pad_area);
int padGetState(int port, int slot);
unsigned char padRead(int port, int slot, struct padButtonStatus *data);

#endif
