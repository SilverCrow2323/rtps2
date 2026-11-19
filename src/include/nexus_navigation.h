#ifndef NEXUS_NAVIGATION_H
#define NEXUS_NAVIGATION_H

#include <stdint.h>

typedef enum {
    NEXUS_PAGE_HOME = 0,
    NEXUS_PAGE_GAMES,
    NEXUS_PAGE_PS1,
    NEXUS_PAGE_HOMEBREW,
    NEXUS_PAGE_STORE,
    NEXUS_PAGE_FILES,
    NEXUS_PAGE_SYSTEM,
    NEXUS_PAGE_COUNT
} NexusPage;

typedef enum {
    NEXUS_KEY_NONE = 0,
    NEXUS_KEY_UP,
    NEXUS_KEY_DOWN,
    NEXUS_KEY_LEFT,
    NEXUS_KEY_RIGHT,
    NEXUS_KEY_CONFIRM,
    NEXUS_KEY_BACK,
    NEXUS_KEY_HOME,
    NEXUS_KEY_PREVIOUS,
    NEXUS_KEY_NEXT
} NexusKey;

typedef struct {
    NexusPage page;
    uint8_t selected;
    uint8_t in_section;
} NexusNavState;

void nexus_nav_init(NexusNavState *state);
void nexus_nav_apply(NexusNavState *state, NexusKey key);
const char *nexus_page_name(NexusPage page);

#endif
