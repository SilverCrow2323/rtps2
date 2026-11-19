#ifndef TEST_FILEXIO_RPC_H
#define TEST_FILEXIO_RPC_H
#include <stdint.h>

#define FIO_S_IFMT  0xF000U
#define FIO_S_IFREG 0x2000U
#define FIO_S_IFDIR 0x1000U
#define FIO_S_ISREG(mode) (((mode) & FIO_S_IFMT) == FIO_S_IFREG)
#define FIO_S_ISDIR(mode) (((mode) & FIO_S_IFMT) == FIO_S_IFDIR)

typedef struct {
    unsigned int mode;
    unsigned int attr;
    unsigned int size;
    unsigned char ctime[8];
    unsigned char atime[8];
    unsigned char mtime[8];
    unsigned int hisize;
    unsigned int private_0, private_1, private_2, private_3, private_4, private_5;
} iox_stat_t __attribute__((aligned(64)));

typedef struct {
    iox_stat_t stat;
    char name[256];
    void *privdata;
} iox_dirent_t __attribute__((aligned(64)));

int fileXioInit(void);
int fileXioDopen(const char *path);
int fileXioDread(int fd, iox_dirent_t *entry);
int fileXioDclose(int fd);

#endif
