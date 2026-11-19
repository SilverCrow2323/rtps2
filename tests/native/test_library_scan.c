#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "nexus_library_scan.h"

typedef struct {
    const char *path;
    NexusScanDirEntry entries[8];
    unsigned int count;
    int read_error;
} MockDirectory;

typedef struct {
    MockDirectory directories[4];
    unsigned int open_calls;
    unsigned int close_calls;
} MockFs;

static void set_entry(NexusScanDirEntry *entry, const char *name,
                      uint8_t type, uint64_t size)
{
    memset(entry, 0, sizeof(*entry));
    strncpy(entry->name, name, sizeof(entry->name) - 1U);
    entry->type = type;
    entry->size_bytes = size;
}

static int mock_open(void *opaque, const char *path, int *handle)
{
    MockFs *fs = (MockFs *)opaque;
    unsigned int i;
    ++fs->open_calls;
    for (i = 0; i < 4U; ++i) {
        if (fs->directories[i].path != 0 &&
            strcmp(fs->directories[i].path, path) == 0) {
            *handle = (int)i;
            return 0;
        }
    }
    return NEXUS_SCAN_IO_NOT_FOUND;
}

static int mock_read(void *opaque, int handle, NexusScanDirEntry *entry)
{
    MockFs *fs = (MockFs *)opaque;
    MockDirectory *dir;
    static unsigned int positions[4];

    if (handle < 0 || handle >= 4)
        return -1;
    dir = &fs->directories[handle];
    if (dir->read_error != 0 && positions[handle] >= dir->count)
        return dir->read_error;
    if (positions[handle] >= dir->count) {
        positions[handle] = 0;
        return 0;
    }
    *entry = dir->entries[positions[handle]++];
    return 1;
}

static int mock_close(void *opaque, int handle)
{
    MockFs *fs = (MockFs *)opaque;
    (void)handle;
    ++fs->close_calls;
    return 0;
}

static void reset_positions(void)
{
    /* mock_read resets each position at end-of-directory. */
}

int main(void)
{
    MockFs fs;
    NexusScanFsOps ops = {mock_open, mock_read, mock_close};
    NexusScanConfig config;
    NexusScanReport report;
    const NexusLibraryEntry *entry;

    memset(&fs, 0, sizeof(fs));
    fs.directories[0].path = "mass:/";
    fs.directories[1].path = "mass:/CD";
    fs.directories[2].path = "mass:/DVD";
    fs.directories[3].path = "mass:/POPS";
    fs.directories[1].count = 3;
    set_entry(&fs.directories[1].entries[0], "RACING.iso",
              NEXUS_SCAN_ENTRY_FILE, UINT64_C(734003200));
    set_entry(&fs.directories[1].entries[1], "not-a-game.txt",
              NEXUS_SCAN_ENTRY_FILE, 1024);
    set_entry(&fs.directories[1].entries[2], "folder.iso",
              NEXUS_SCAN_ENTRY_DIRECTORY, 0);
    fs.directories[2].count = 2;
    set_entry(&fs.directories[2].entries[0], "BIG_GAME.zso",
              NEXUS_SCAN_ENTRY_FILE, UINT64_C(0x100000000));
    set_entry(&fs.directories[2].entries[1], "../escape.iso",
              NEXUS_SCAN_ENTRY_FILE, 2048);
    fs.directories[3].count = 1;
    set_entry(&fs.directories[3].entries[0], "PS1_TITLE.VCD",
              NEXUS_SCAN_ENTRY_FILE, UINT64_C(800000000));

    memset(&config, 0, sizeof(config));
    config.source = NEXUS_LIBRARY_SOURCE_USB;
    config.scan_ps2 = 1;
    config.scan_ps1 = 1;
    strcpy(config.root, "mass:/");

    nexus_library_clear();
    assert(nexus_library_scan_root(&ops, &fs, &config, &report) == 0);
    assert(report.root_available == 1U);
    assert(report.directories_opened == 3U);
    assert(report.entries_seen == 6U);
    assert(report.supported_files == 3U);
    assert(report.indexed == 3U);
    assert(report.rejected == 1U);
    assert(report.missing_directories == 0U);
    assert(nexus_library_count() == 3U);

    entry = nexus_library_find(
        UINT64_C(0), NEXUS_LIBRARY_SOURCE_USB);
    assert(entry == 0);

    entry = nexus_library_visible_at(&(NexusLibraryView){
        0, NEXUS_LIBRARY_FILTER_ANY, NEXUS_LIBRARY_FILTER_ANY, 0,
        NEXUS_LIBRARY_VIEW_GRID}, 0);
    assert(entry != 0);

    entry = nexus_library_find(
        entry->key, NEXUS_LIBRARY_SOURCE_USB);
    assert(entry != 0);
    assert(strcmp(entry->title, "BIG GAME") == 0 ||
           strcmp(entry->title, "PS1 TITLE") == 0 ||
           strcmp(entry->title, "RACING") == 0);

    /* A missing optional CD folder is not fatal; an existing root is enough. */
    fs.directories[1].path = 0;
    assert(nexus_library_scan_root(&ops, &fs, &config, &report) == 0);
    assert(report.missing_directories == 1U);

    /* A failed directory read prevents the staged batch from being committed. */
    fs.directories[1].path = "mass:/CD";
    fs.directories[1].read_error = -9;
    nexus_library_clear();
    assert(nexus_library_scan_root(&ops, &fs, &config, &report) == -9);
    assert(nexus_library_count() == 0U);

    /* Never accept unsafe roots. */
    strcpy(config.root, "mass:/../SYS-CONF");
    assert(nexus_library_scan_root(&ops, &fs, &config, &report) < 0);

    reset_positions();
    nexus_library_clear();
    return 0;
}
