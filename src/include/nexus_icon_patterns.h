#ifndef NEXUS_ICON_PATTERNS_H
#define NEXUS_ICON_PATTERNS_H

#include <tamtypes.h>

/* 5x5 one-bit glyphs, top row first; bit 4 is the leftmost pixel. */
static const u8 nexus_icon_patterns[6][5] = {
    /* GAMES: controller */
    { 0x0e, 0x1f, 0x1b, 0x0e, 0x0a },
    /* PS1: disc / optical media */
    { 0x0e, 0x11, 0x15, 0x11, 0x0e },
    /* HOMEBREW: crossed tools */
    { 0x11, 0x0a, 0x04, 0x0a, 0x11 },
    /* STORE: storefront */
    { 0x0e, 0x1f, 0x15, 0x1f, 0x1b },
    /* FILES: folder */
    { 0x0c, 0x1f, 0x11, 0x1f, 0x11 },
    /* SYSTEM: gear */
    { 0x04, 0x15, 0x0e, 0x15, 0x04 }
};

#endif
