#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "nexus_library.h"

static NexusLibraryEntry make_entry(uint64_t key, const char *title,
                                    uint8_t source, uint8_t platform,
                                    uint8_t backend, const char *path)
{
    NexusLibraryEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.key = key;
    entry.source = source;
    entry.platform = platform;
    entry.backend = backend;
    strncpy(entry.title, title, sizeof(entry.title) - 1U);
    strncpy(entry.path, path, sizeof(entry.path) - 1U);
    strncpy(entry.title_id, "SLUS_000.00", sizeof(entry.title_id) - 1U);
    return entry;
}

int main(void)
{
    NexusLibraryEntry a, b, c, d, update, bad;
    NexusLibraryView view;
    const NexusLibraryEntry *entry;

    nexus_library_clear();
    nexus_library_view_init(&view);
    assert(nexus_library_count() == 0U);
    assert(view.source_filter == NEXUS_LIBRARY_FILTER_ANY);
    assert(view.platform_filter == NEXUS_LIBRARY_FILTER_ANY);
    assert(view.view_mode == NEXUS_LIBRARY_VIEW_GRID);

    a = make_entry(1, "Zeta", NEXUS_LIBRARY_SOURCE_HDD,
                   NEXUS_LIBRARY_PLATFORM_PS2, NEXUS_LIBRARY_BACKEND_OPL,
                   "hdd0:ZETA.iso");
    b = make_entry(2, "alpha", NEXUS_LIBRARY_SOURCE_MMCE,
                   NEXUS_LIBRARY_PLATFORM_PS2, NEXUS_LIBRARY_BACKEND_NEUTRINO,
                   "mmce0:/games/ALPHA.iso");
    c = make_entry(3, "Beta PS1", NEXUS_LIBRARY_SOURCE_USB,
                   NEXUS_LIBRARY_PLATFORM_PS1, NEXUS_LIBRARY_BACKEND_POPS,
                   "mass:/POPS/BETA.VCD");
    d = make_entry(4, "Beta PS2", NEXUS_LIBRARY_SOURCE_HDD,
                   NEXUS_LIBRARY_PLATFORM_PS2, NEXUS_LIBRARY_BACKEND_OPL,
                   "hdd0:BETA.iso");

    assert(nexus_library_upsert(&a) == 0);
    assert(nexus_library_upsert(&b) == 0);
    assert(nexus_library_upsert(&c) == 0);
    assert(nexus_library_upsert(&d) == 0);
    assert(nexus_library_count() == 4U);

    entry = nexus_library_visible_at(&view, 0);
    assert(entry != 0 && strcmp(entry->title, "alpha") == 0);
    entry = nexus_library_visible_at(&view, 1);
    assert(entry != 0 && strcmp(entry->title, "Beta PS1") == 0);
    entry = nexus_library_visible_at(&view, 2);
    assert(entry != 0 && strcmp(entry->title, "Beta PS2") == 0);
    entry = nexus_library_visible_at(&view, 3);
    assert(entry != 0 && strcmp(entry->title, "Zeta") == 0);
    assert(nexus_library_visible_at(&view, 4) == 0);

    assert(nexus_library_set_source(&view, NEXUS_LIBRARY_SOURCE_HDD) == 0);
    assert(nexus_library_visible_count(&view) == 2U);
    entry = nexus_library_visible_at(&view, 0);
    assert(entry != 0 && strcmp(entry->title, "Beta PS2") == 0);
    assert(nexus_library_set_source(&view, NEXUS_LIBRARY_FILTER_ANY) == 0);

    assert(nexus_library_set_platform(&view, NEXUS_LIBRARY_PLATFORM_PS1) == 0);
    assert(nexus_library_visible_count(&view) == 1U);
    entry = nexus_library_visible_at(&view, 0);
    assert(entry != 0 && entry->key == 3U);
    assert(nexus_library_set_platform(&view, NEXUS_LIBRARY_FILTER_ANY) == 0);

    assert(nexus_library_set_view_mode(&view, NEXUS_LIBRARY_VIEW_COVERFLOW) == 0);
    assert(view.view_mode == NEXUS_LIBRARY_VIEW_COVERFLOW);
    assert(nexus_library_set_view_mode(&view, NEXUS_LIBRARY_VIEW_COUNT) < 0);

    assert(nexus_library_move(&view, -1) == 0);
    assert(view.selected_index == 3U);
    assert(nexus_library_selected(&view) != 0);
    assert(nexus_library_toggle_favorite(&view) == 0);
    assert(nexus_library_set_favorites_only(&view, 1) == 0);
    assert(nexus_library_visible_count(&view) == 1U);
    entry = nexus_library_selected(&view);
    assert(entry != 0 && entry->key == 4U);

    update = make_entry(4, "A Beta Remaster", NEXUS_LIBRARY_SOURCE_HDD,
                        NEXUS_LIBRARY_PLATFORM_PS2, NEXUS_LIBRARY_BACKEND_OPL,
                        "hdd0:BETAREM.iso");
    assert(nexus_library_upsert(&update) == 0);
    assert(nexus_library_count() == 4U);
    assert(nexus_library_visible_count(&view) == 1U);
    entry = nexus_library_selected(&view);
    assert(entry != 0 && entry->key == 4U && entry->favorite == 1U);
    assert(strcmp(entry->title, "A Beta Remaster") == 0);

    assert(nexus_library_set_favorites_only(&view, 0) == 0);
    assert(nexus_library_find(4U, NEXUS_LIBRARY_SOURCE_HDD) != 0);
    assert(nexus_library_find(4U, NEXUS_LIBRARY_SOURCE_USB) == 0);
    assert(nexus_library_set_source(&view, 99U) < 0);
    assert(nexus_library_set_platform(&view, 99U) < 0);
    assert(nexus_library_set_favorites_only(&view, 2) < 0);
    assert(nexus_library_move(0, 1) < 0);

    bad = a;
    bad.title[0] = '\0';
    assert(nexus_library_upsert(&bad) < 0);
    bad = a;
    bad.title[sizeof(bad.title) - 1U] = 'X';
    assert(nexus_library_upsert(&bad) < 0);
    bad = a;
    bad.key = 0;
    assert(nexus_library_upsert(&bad) < 0);

    nexus_library_clear();
    assert(nexus_library_count() == 0U);
    assert(nexus_library_visible_count(&view) == 0U);
    assert(nexus_library_move(&view, 1) == 0);
    assert(nexus_library_selected(&view) == 0);
    assert(nexus_library_toggle_favorite(&view) < 0);
    return 0;
}
