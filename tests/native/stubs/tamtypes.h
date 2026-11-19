#ifndef TEST_TAMTYPES_H
#define TEST_TAMTYPES_H
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef struct __attribute__((aligned(16))) {
    u64 low;
    u64 high;
} qword_t;
#endif
