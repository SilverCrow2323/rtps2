#ifndef TEST_LOADFILE_H
#define TEST_LOADFILE_H
#include <tamtypes.h>
int SifLoadModule(const char *path, int arg_len, const char *args);
int SifSearchModuleByName(const char *name);
int SifExecModuleBuffer(void *image, u32 size, u32 arg_len,
                        const char *args, int *module_result);
#endif
