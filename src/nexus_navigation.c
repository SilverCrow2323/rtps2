#include "nexus_navigation.h"

static const NexusPage section_pages[] = {
    NEXUS_PAGE_GAMES,
    NEXUS_PAGE_PS1,
    NEXUS_PAGE_HOMEBREW,
    NEXUS_PAGE_STORE,
    NEXUS_PAGE_FILES,
    NEXUS_PAGE_SYSTEM
};

#define NEXUS_SECTION_COUNT ((uint8_t)(sizeof(section_pages) / sizeof(section_pages[0])))

void nexus_nav_init(NexusNavState *state)
{
    if (state == 0) {
        return;
    }
    state->page = NEXUS_PAGE_HOME;
    state->selected = 0;
    state->in_section = 0;
}

static void select_relative(NexusNavState *state, int direction)
{
    int next = (int)state->selected + direction;

    if (next < 0) {
        next = (int)NEXUS_SECTION_COUNT - 1;
    } else if (next >= (int)NEXUS_SECTION_COUNT) {
        next = 0;
    }
    state->selected = (uint8_t)next;
}

static void change_section(NexusNavState *state, int direction)
{
    int current = 0;
    uint8_t i;

    for (i = 0; i < NEXUS_SECTION_COUNT; ++i) {
        if (section_pages[i] == state->page) {
            current = (int)i;
            break;
        }
    }

    current += direction;
    if (current < 0) {
        current = (int)NEXUS_SECTION_COUNT - 1;
    } else if (current >= (int)NEXUS_SECTION_COUNT) {
        current = 0;
    }
    state->selected = (uint8_t)current;
    state->page = section_pages[current];
}

void nexus_nav_apply(NexusNavState *state, NexusKey key)
{
    if (state == 0 || state->page < NEXUS_PAGE_HOME || state->page >= NEXUS_PAGE_COUNT) {
        return;
    }

    if (key == NEXUS_KEY_HOME) {
        state->page = NEXUS_PAGE_HOME;
        state->selected = 0;
        state->in_section = 0;
        return;
    }

    if (!state->in_section) {
        switch (key) {
            case NEXUS_KEY_UP:
            case NEXUS_KEY_LEFT:
            case NEXUS_KEY_PREVIOUS:
                select_relative(state, -1);
                break;
            case NEXUS_KEY_DOWN:
            case NEXUS_KEY_RIGHT:
            case NEXUS_KEY_NEXT:
                select_relative(state, 1);
                break;
            case NEXUS_KEY_CONFIRM:
                state->page = section_pages[state->selected];
                state->in_section = 1;
                break;
            default:
                break;
        }
        return;
    }

    switch (key) {
        case NEXUS_KEY_BACK:
            state->page = NEXUS_PAGE_HOME;
            state->in_section = 0;
            break;
        case NEXUS_KEY_PREVIOUS:
        case NEXUS_KEY_LEFT:
            change_section(state, -1);
            break;
        case NEXUS_KEY_NEXT:
        case NEXUS_KEY_RIGHT:
            change_section(state, 1);
            break;
        default:
            break;
    }
}

const char *nexus_page_name(NexusPage page)
{
    switch (page) {
        case NEXUS_PAGE_HOME: return "HOME";
        case NEXUS_PAGE_GAMES: return "GAMES";
        case NEXUS_PAGE_PS1: return "PS1";
        case NEXUS_PAGE_HOMEBREW: return "HOMEBREW";
        case NEXUS_PAGE_STORE: return "STORE";
        case NEXUS_PAGE_FILES: return "FILES";
        case NEXUS_PAGE_SYSTEM: return "SYSTEM";
        default: return "UNKNOWN";
    }
}
