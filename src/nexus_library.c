#include <stddef.h>
#include <string.h>

#include "nexus_library.h"

static NexusLibraryEntry entries[NEXUS_LIBRARY_CAPACITY];
static unsigned int entry_count;

static int terminated_nonempty(const char *value, size_t capacity, int required)
{
    const char *end;

    if (value == 0)
        return 0;
    end = (const char *)memchr(value, '\0', capacity);
    if (end == 0)
        return 0;
    if (required && end == value)
        return 0;
    return 1;
}

static int valid_entry(const NexusLibraryEntry *entry)
{
    if (entry == 0 || entry->key == 0)
        return 0;
    if (entry->source >= NEXUS_LIBRARY_SOURCE_COUNT ||
        entry->platform >= NEXUS_LIBRARY_PLATFORM_COUNT ||
        entry->backend >= NEXUS_LIBRARY_BACKEND_COUNT ||
        entry->favorite > 1U)
        return 0;
    if (!terminated_nonempty(entry->title, sizeof(entry->title), 1))
        return 0;
    if (!terminated_nonempty(entry->title_id, sizeof(entry->title_id), 0))
        return 0;
    if (!terminated_nonempty(entry->path, sizeof(entry->path), 1))
        return 0;
    return 1;
}

static unsigned char fold_ascii(unsigned char value)
{
    if (value >= (unsigned char)'A' && value <= (unsigned char)'Z')
        return (unsigned char)(value + ('a' - 'A'));
    return value;
}

static int compare_text(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        unsigned char a = fold_ascii((unsigned char)*left);
        unsigned char b = fold_ascii((unsigned char)*right);
        if (a < b)
            return -1;
        if (a > b)
            return 1;
        ++left;
        ++right;
    }
    if (*left == *right)
        return 0;
    return (*left == '\0') ? -1 : 1;
}

static int compare_entry(const NexusLibraryEntry *left,
                         const NexusLibraryEntry *right)
{
    int result = compare_text(left->title, right->title);
    if (result != 0)
        return result;

    result = compare_text(left->title_id, right->title_id);
    if (result != 0)
        return result;

    if (left->source < right->source)
        return -1;
    if (left->source > right->source)
        return 1;
    if (left->key < right->key)
        return -1;
    if (left->key > right->key)
        return 1;
    return 0;
}

static int find_index(uint64_t key, uint8_t source)
{
    unsigned int i;
    for (i = 0; i < entry_count; ++i) {
        if (entries[i].key == key && entries[i].source == source)
            return (int)i;
    }
    return -1;
}

static void remove_index(unsigned int index)
{
    if (index + 1U < entry_count) {
        memmove(&entries[index], &entries[index + 1U],
                (entry_count - index - 1U) * sizeof(entries[0]));
    }
    --entry_count;
}

void nexus_library_clear(void)
{
    memset(entries, 0, sizeof(entries));
    entry_count = 0;
}

unsigned int nexus_library_count(void)
{
    return entry_count;
}

int nexus_library_upsert(const NexusLibraryEntry *entry)
{
    NexusLibraryEntry normalized;
    int old_index;
    unsigned int insert_at;

    if (!valid_entry(entry))
        return -1;

    normalized = *entry;
    old_index = find_index(entry->key, entry->source);
    if (old_index >= 0) {
        /* A metadata refresh must not erase a user's existing favorite. */
        if (entries[old_index].favorite)
            normalized.favorite = 1U;
        remove_index((unsigned int)old_index);
    } else if (entry_count >= NEXUS_LIBRARY_CAPACITY) {
        return -2;
    }

    insert_at = 0;
    while (insert_at < entry_count &&
           compare_entry(&entries[insert_at], &normalized) <= 0) {
        ++insert_at;
    }

    if (insert_at < entry_count) {
        memmove(&entries[insert_at + 1U], &entries[insert_at],
                (entry_count - insert_at) * sizeof(entries[0]));
    }
    entries[insert_at] = normalized;
    ++entry_count;
    return 0;
}

const NexusLibraryEntry *nexus_library_find(uint64_t key, uint8_t source)
{
    int index = find_index(key, source);
    return (index < 0) ? 0 : &entries[index];
}

void nexus_library_view_init(NexusLibraryView *view)
{
    if (view == 0)
        return;
    memset(view, 0, sizeof(*view));
    view->source_filter = NEXUS_LIBRARY_FILTER_ANY;
    view->platform_filter = NEXUS_LIBRARY_FILTER_ANY;
    view->view_mode = NEXUS_LIBRARY_VIEW_GRID;
}

int nexus_library_set_source(NexusLibraryView *view, uint8_t source_or_any)
{
    if (view == 0 ||
        (source_or_any != NEXUS_LIBRARY_FILTER_ANY &&
         source_or_any >= NEXUS_LIBRARY_SOURCE_COUNT))
        return -1;
    view->source_filter = source_or_any;
    view->selected_index = 0;
    return 0;
}

int nexus_library_set_platform(NexusLibraryView *view, uint8_t platform_or_any)
{
    if (view == 0 ||
        (platform_or_any != NEXUS_LIBRARY_FILTER_ANY &&
         platform_or_any >= NEXUS_LIBRARY_PLATFORM_COUNT))
        return -1;
    view->platform_filter = platform_or_any;
    view->selected_index = 0;
    return 0;
}

int nexus_library_set_favorites_only(NexusLibraryView *view, int enabled)
{
    if (view == 0 || (enabled != 0 && enabled != 1))
        return -1;
    view->favorites_only = (uint8_t)enabled;
    view->selected_index = 0;
    return 0;
}

int nexus_library_set_view_mode(NexusLibraryView *view, uint8_t mode)
{
    if (view == 0 || mode >= NEXUS_LIBRARY_VIEW_COUNT)
        return -1;
    view->view_mode = mode;
    view->selected_index = 0;
    return 0;
}

static int visible_entry(const NexusLibraryView *view,
                         const NexusLibraryEntry *entry)
{
    if (view == 0 || entry == 0)
        return 0;
    if (view->source_filter != NEXUS_LIBRARY_FILTER_ANY &&
        entry->source != view->source_filter)
        return 0;
    if (view->platform_filter != NEXUS_LIBRARY_FILTER_ANY &&
        entry->platform != view->platform_filter)
        return 0;
    if (view->favorites_only && !entry->favorite)
        return 0;
    return 1;
}

unsigned int nexus_library_visible_count(const NexusLibraryView *view)
{
    unsigned int i, count = 0;
    if (view == 0)
        return 0;
    for (i = 0; i < entry_count; ++i) {
        if (visible_entry(view, &entries[i]))
            ++count;
    }
    return count;
}

const NexusLibraryEntry *nexus_library_visible_at(
    const NexusLibraryView *view, unsigned int visible_index)
{
    unsigned int i, count = 0;
    if (view == 0)
        return 0;

    for (i = 0; i < entry_count; ++i) {
        if (!visible_entry(view, &entries[i]))
            continue;
        if (count == visible_index)
            return &entries[i];
        ++count;
    }
    return 0;
}

const NexusLibraryEntry *nexus_library_selected(const NexusLibraryView *view)
{
    if (view == 0)
        return 0;
    return nexus_library_visible_at(view, view->selected_index);
}

int nexus_library_move(NexusLibraryView *view, int delta)
{
    unsigned int count;
    int step, next;

    if (view == 0)
        return -1;

    count = nexus_library_visible_count(view);
    if (count == 0) {
        view->selected_index = 0;
        return 0;
    }

    step = delta % (int)count;
    next = ((int)(view->selected_index % count) + step) % (int)count;
    if (next < 0)
        next += (int)count;
    view->selected_index = (uint16_t)next;
    return 0;
}

int nexus_library_toggle_favorite(NexusLibraryView *view)
{
    const NexusLibraryEntry *selected;
    int index;

    if (view == 0)
        return -1;
    selected = nexus_library_selected(view);
    if (selected == 0)
        return -2;
    index = find_index(selected->key, selected->source);
    if (index < 0)
        return -2;
    entries[index].favorite = entries[index].favorite ? 0U : 1U;
    return 0;
}
