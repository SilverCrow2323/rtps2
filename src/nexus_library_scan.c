#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "nexus_library_scan.h"

#define SCAN_STAGE_CAPACITY NEXUS_LIBRARY_CAPACITY
#define SCAN_DIR_CD 1U
#define SCAN_DIR_DVD 2U
#define SCAN_DIR_POPS 3U

static NexusLibraryEntry staged_entries[SCAN_STAGE_CAPACITY];
static unsigned int staged_count;

static unsigned char lower_ascii(unsigned char c)
{
    if (c >= (unsigned char)'A' && c <= (unsigned char)'Z')
        return (unsigned char)(c + ('a' - 'A'));
    return c;
}

static int equal_ascii_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (lower_ascii((unsigned char)*a) != lower_ascii((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static int safe_root(const char *root)
{
    size_t i, len;
    int colon = -1;
    size_t component_start;

    if (root == 0)
        return 0;
    len = strlen(root);
    if (len < 6U || len >= NEXUS_SCAN_ROOT_CAPACITY)
        return 0;

    for (i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)root[i];
        if (c == ':') {
            if (colon >= 0 || i < 2U)
                return 0;
            colon = (int)i;
            continue;
        }
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.' || c == '/'))
            return 0;
    }
    if (colon < 0 || (size_t)colon + 1U >= len)
        return 0;
    if (root[colon + 1] != '/' &&
        strchr(root + colon + 1, '/') == 0)
        return 0;

    component_start = (size_t)colon + 1U;
    for (i = component_start; i <= len; ++i) {
        if (root[i] == '/' || root[i] == '\0') {
            size_t part_len = i - component_start;
            if ((part_len == 1U && root[component_start] == '.') ||
                (part_len == 2U && root[component_start] == '.' &&
                 root[component_start + 1U] == '.'))
                return 0;
            component_start = i + 1U;
        }
    }
    return 1;
}

static int safe_basename(const char *name)
{
    size_t i;
    const char *end;

    if (name == 0)
        return 0;
    end = (const char *)memchr(name, '\0', NEXUS_SCAN_NAME_CAPACITY);
    if (end == 0 || end == name)
        return 0;
    if ((end - name == 1 && name[0] == '.') ||
        (end - name == 2 && name[0] == '.' && name[1] == '.'))
        return 0;

    for (i = 0; i < (size_t)(end - name); ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c < 0x20U || c == 0x7fU ||
            c == '/' || c == '\\' || c == ':')
            return 0;
    }
    return 1;
}

static const char *extension_of(const char *name)
{
    const char *dot = 0;
    const char *p;

    for (p = name; *p != '\0'; ++p) {
        if (*p == '.')
            dot = p;
    }
    return dot;
}

static int extension_supported(const char *name, uint8_t directory,
                               uint8_t *platform)
{
    const char *ext = extension_of(name);
    if (ext == 0)
        return 0;

    if ((directory == SCAN_DIR_CD || directory == SCAN_DIR_DVD) &&
        (equal_ascii_ci(ext, ".iso") || equal_ascii_ci(ext, ".zso"))) {
        *platform = NEXUS_LIBRARY_PLATFORM_PS2;
        return 1;
    }
    if (directory == SCAN_DIR_POPS && equal_ascii_ci(ext, ".vcd")) {
        *platform = NEXUS_LIBRARY_PLATFORM_PS1;
        return 1;
    }
    return 0;
}

static int join_path(char *output, size_t capacity, const char *root,
                     const char *folder, const char *filename)
{
    size_t len = strlen(root);
    size_t folder_len = strlen(folder);
    size_t file_len = strlen(filename);
    int separator = len > 0U && root[len - 1U] != '/';
    size_t required = len + (size_t)separator + folder_len + 1U + file_len + 1U;

    if (required > capacity)
        return -1;

    memcpy(output, root, len);
    if (separator)
        output[len++] = '/';
    memcpy(output + len, folder, folder_len);
    len += folder_len;
    output[len++] = '/';
    memcpy(output + len, filename, file_len);
    output[len + file_len] = '\0';
    return 0;
}

static uint64_t make_key(uint8_t source, const char *path)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    const unsigned char *p;

    hash ^= source;
    hash *= UINT64_C(1099511628211);
    for (p = (const unsigned char *)path; *p != 0U; ++p) {
        hash ^= *p;
        hash *= UINT64_C(1099511628211);
    }
    return hash == 0U ? 1U : hash;
}

static void make_title(char output[NEXUS_LIBRARY_TITLE_CAPACITY],
                       const char *filename)
{
    const char *ext = extension_of(filename);
    size_t length = ext ? (size_t)(ext - filename) : strlen(filename);
    size_t i, out = 0;
    int previous_space = 1;

    for (i = 0; i < length && out + 1U < NEXUS_LIBRARY_TITLE_CAPACITY; ++i) {
        unsigned char c = (unsigned char)filename[i];
        if (c == '_' || c == '-') {
            c = ' ';
        }
        if (c <= 0x20U) {
            if (!previous_space && out + 1U < NEXUS_LIBRARY_TITLE_CAPACITY) {
                output[out++] = ' ';
                previous_space = 1;
            }
        } else {
            output[out++] = (char)c;
            previous_space = 0;
        }
    }
    if (out > 0U && output[out - 1U] == ' ')
        --out;
    output[out] = '\0';
}

static int stage_entry(const NexusScanConfig *config,
                       const NexusScanDirEntry *dir_entry,
                       const char *folder, uint8_t directory,
                       NexusScanReport *report)
{
    NexusLibraryEntry entry;
    uint8_t platform;

    if (dir_entry->type != NEXUS_SCAN_ENTRY_FILE)
        return 0;
    if (!safe_basename(dir_entry->name)) {
        ++report->rejected;
        return 0;
    }
    if (!extension_supported(dir_entry->name, directory, &platform))
        return 0;
    ++report->supported_files;

    if (dir_entry->size_bytes == 0U ||
        join_path(entry.path, sizeof(entry.path), config->root,
                  folder, dir_entry->name) != 0) {
        ++report->rejected;
        return 0;
    }
    if (staged_count >= SCAN_STAGE_CAPACITY) {
        report->truncated = 1U;
        return 0;
    }

    memset(&entry, 0, sizeof(entry));
    if (join_path(entry.path, sizeof(entry.path), config->root,
                  folder, dir_entry->name) != 0) {
        ++report->rejected;
        return 0;
    }
    entry.key = make_key(config->source, entry.path);
    entry.size_bytes = dir_entry->size_bytes;
    entry.source = config->source;
    entry.platform = platform;
    entry.backend = platform == NEXUS_LIBRARY_PLATFORM_PS1
                        ? NEXUS_LIBRARY_BACKEND_POPS
                        : NEXUS_LIBRARY_BACKEND_AUTO;
    make_title(entry.title, dir_entry->name);
    staged_entries[staged_count++] = entry;
    return 0;
}

static int scan_directory(const NexusScanFsOps *ops, void *context,
                          const NexusScanConfig *config, const char *folder,
                          uint8_t directory, NexusScanReport *report)
{
    char path[NEXUS_LIBRARY_PATH_CAPACITY];
    NexusScanDirEntry entry;
    int handle = -1;
    int result;
    int close_result;

    if (join_path(path, sizeof(path), config->root, folder, "") != 0)
        return -10;
    /* Remove the final slash inserted for the empty filename. */
    path[strlen(path) - 1U] = '\0';

    result = ops->open_dir(context, path, &handle);
    if (result == NEXUS_SCAN_IO_NOT_FOUND) {
        ++report->missing_directories;
        return 0;
    }
    if (result < 0)
        return result;

    ++report->directories_opened;
    for (;;) {
        memset(&entry, 0, sizeof(entry));
        result = ops->read_dir(context, handle, &entry);
        if (result == 0)
            break;
        if (result < 0) {
            report->first_error = result;
            (void)ops->close_dir(context, handle);
            return result;
        }
        ++report->entries_seen;
        (void)stage_entry(config, &entry, folder, directory, report);
    }

    close_result = ops->close_dir(context, handle);
    if (close_result < 0) {
        report->first_error = close_result;
        return close_result;
    }
    return 0;
}

int nexus_library_scan_root(const NexusScanFsOps *ops, void *context,
                            const NexusScanConfig *config,
                            NexusScanReport *report)
{
    int root_handle = -1;
    int result;
    int close_result;
    unsigned int i, additions = 0;

    static const struct {
        const char *name;
        uint8_t id;
        uint8_t flag;
    } directories[] = {
        {"CD", SCAN_DIR_CD, 1U},
        {"DVD", SCAN_DIR_DVD, 1U},
        {"POPS", SCAN_DIR_POPS, 2U}
    };

    if (report == 0)
        return -1;
    memset(report, 0, sizeof(*report));
    staged_count = 0;

    if (ops == 0 || ops->open_dir == 0 || ops->read_dir == 0 ||
        ops->close_dir == 0 || config == 0 ||
        config->source >= NEXUS_LIBRARY_SOURCE_COUNT ||
        (config->scan_ps2 == 0U && config->scan_ps1 == 0U) ||
        !safe_root(config->root))
        return -1;

    result = ops->open_dir(context, config->root, &root_handle);
    if (result < 0) {
        report->first_error = result;
        return result;
    }
    close_result = ops->close_dir(context, root_handle);
    if (close_result < 0) {
        report->first_error = close_result;
        return close_result;
    }
    report->root_available = 1U;

    for (i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i) {
        int enabled = directories[i].flag == 1U
                          ? config->scan_ps2
                          : config->scan_ps1;
        if (!enabled)
            continue;
        result = scan_directory(ops, context, config,
                                directories[i].name, directories[i].id,
                                report);
        if (result < 0) {
            report->first_error = result;
            return result;
        }
    }

    for (i = 0; i < staged_count; ++i) {
        if (nexus_library_find(staged_entries[i].key,
                               staged_entries[i].source) == 0)
            ++additions;
    }
    if (nexus_library_count() + additions > NEXUS_LIBRARY_CAPACITY) {
        report->first_error = -11;
        return -11;
    }

    for (i = 0; i < staged_count; ++i) {
        result = nexus_library_upsert(&staged_entries[i]);
        if (result < 0) {
            report->first_error = result;
            return result;
        }
        ++report->indexed;
    }
    return 0;
}
