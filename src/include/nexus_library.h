#ifndef NEXUS_LIBRARY_H
#define NEXUS_LIBRARY_H

#include <stdint.h>

#define NEXUS_LIBRARY_CAPACITY 96U
#define NEXUS_LIBRARY_TITLE_CAPACITY 48U
#define NEXUS_LIBRARY_TITLE_ID_CAPACITY 16U
#define NEXUS_LIBRARY_PATH_CAPACITY 256U
#define NEXUS_LIBRARY_FILTER_ANY 0xffU

typedef enum {
    NEXUS_LIBRARY_SOURCE_HDD = 0,
    NEXUS_LIBRARY_SOURCE_MMCE,
    NEXUS_LIBRARY_SOURCE_USB,
    NEXUS_LIBRARY_SOURCE_COUNT
} NexusLibrarySource;

typedef enum {
    NEXUS_LIBRARY_PLATFORM_PS2 = 0,
    NEXUS_LIBRARY_PLATFORM_PS1,
    NEXUS_LIBRARY_PLATFORM_COUNT
} NexusLibraryPlatform;

typedef enum {
    NEXUS_LIBRARY_BACKEND_AUTO = 0,
    NEXUS_LIBRARY_BACKEND_OPL,
    NEXUS_LIBRARY_BACKEND_NEUTRINO,
    NEXUS_LIBRARY_BACKEND_POPS,
    NEXUS_LIBRARY_BACKEND_COUNT
} NexusLibraryBackend;

typedef enum {
    NEXUS_LIBRARY_VIEW_GRID = 0,
    NEXUS_LIBRARY_VIEW_CAROUSEL,
    NEXUS_LIBRARY_VIEW_COVERFLOW,
    NEXUS_LIBRARY_VIEW_LIST,
    NEXUS_LIBRARY_VIEW_COUNT
} NexusLibraryViewMode;

typedef struct {
    uint64_t key;
    uint64_t size_bytes;
    uint8_t source;
    uint8_t platform;
    uint8_t backend;
    uint8_t favorite;
    char title[NEXUS_LIBRARY_TITLE_CAPACITY];
    char title_id[NEXUS_LIBRARY_TITLE_ID_CAPACITY];
    char path[NEXUS_LIBRARY_PATH_CAPACITY];
} NexusLibraryEntry;

typedef struct {
    uint16_t selected_index;
    uint8_t source_filter;
    uint8_t platform_filter;
    uint8_t favorites_only;
    uint8_t view_mode;
} NexusLibraryView;

void nexus_library_clear(void);
unsigned int nexus_library_count(void);

/* Stable key must be unique within a source. Returns 0 on insert/update. */
int nexus_library_upsert(const NexusLibraryEntry *entry);
const NexusLibraryEntry *nexus_library_find(uint64_t key, uint8_t source);

void nexus_library_view_init(NexusLibraryView *view);
int nexus_library_set_source(NexusLibraryView *view, uint8_t source_or_any);
int nexus_library_set_platform(NexusLibraryView *view, uint8_t platform_or_any);
int nexus_library_set_favorites_only(NexusLibraryView *view, int enabled);
int nexus_library_set_view_mode(NexusLibraryView *view, uint8_t mode);
unsigned int nexus_library_visible_count(const NexusLibraryView *view);
const NexusLibraryEntry *nexus_library_visible_at(
    const NexusLibraryView *view, unsigned int visible_index);
const NexusLibraryEntry *nexus_library_selected(const NexusLibraryView *view);
int nexus_library_move(NexusLibraryView *view, int delta);
int nexus_library_toggle_favorite(NexusLibraryView *view);

#endif
