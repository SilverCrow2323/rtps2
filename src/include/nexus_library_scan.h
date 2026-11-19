#ifndef NEXUS_LIBRARY_SCAN_H
#define NEXUS_LIBRARY_SCAN_H

#include <stdint.h>

#include "nexus_library.h"

#define NEXUS_SCAN_ROOT_CAPACITY 96U
#define NEXUS_SCAN_NAME_CAPACITY 128U

#define NEXUS_SCAN_ENTRY_FILE 1U
#define NEXUS_SCAN_ENTRY_DIRECTORY 2U

/* Adapters must map their native not-found result to this value. */
#define NEXUS_SCAN_IO_NOT_FOUND (-2)

typedef struct {
    char name[NEXUS_SCAN_NAME_CAPACITY];
    uint64_t size_bytes;
    uint8_t type;
} NexusScanDirEntry;

/*
 * Read-only directory adapter contract:
 * open_dir: 0 on success, negative error on failure.
 * read_dir: 1 = entry, 0 = end of directory, negative = error.
 * close_dir: 0 on success, negative error on failure.
 * These callbacks must not mount, format, create, rename or write anything.
 */
typedef struct {
    int (*open_dir)(void *context, const char *path, int *handle);
    int (*read_dir)(void *context, int handle, NexusScanDirEntry *entry);
    int (*close_dir)(void *context, int handle);
} NexusScanFsOps;

typedef struct {
    uint8_t source;
    uint8_t scan_ps2;
    uint8_t scan_ps1;
    char root[NEXUS_SCAN_ROOT_CAPACITY];
} NexusScanConfig;

typedef struct {
    uint32_t root_available;
    uint32_t directories_opened;
    uint32_t missing_directories;
    uint32_t entries_seen;
    uint32_t supported_files;
    uint32_t indexed;
    uint32_t rejected;
    uint32_t truncated;
    int32_t first_error;
} NexusScanReport;

/*
 * Scan only existing <root>/CD, <root>/DVD and <root>/POPS directories.
 * PS2 accepts .iso/.zso in CD/DVD; PS1 accepts .vcd in POPS.
 * No recursion, mount operation, filesystem write or loader handoff occurs.
 *
 * Entries are staged until all requested directory reads succeed; a broken
 * adapter/read does not partially modify the visible catalog. Old entries
 * not encountered on a later scan are retained for now (stale cleanup is a
 * separate library transaction operation).
 */
int nexus_library_scan_root(const NexusScanFsOps *ops, void *context,
                            const NexusScanConfig *config,
                            NexusScanReport *report);

#endif
