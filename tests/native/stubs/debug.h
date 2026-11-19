#ifndef TEST_DEBUG_H
#define TEST_DEBUG_H
#include <tamtypes.h>
void init_scr(void);
void scr_printf(const char *format, ...);
void scr_setbgcolor(u32 color);
void scr_setfontcolor(u32 color);
void scr_clear(void);
#endif
