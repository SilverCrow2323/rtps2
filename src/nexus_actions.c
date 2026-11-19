#include <string.h>

#include "nexus_actions.h"
#include "nexus_filexio.h"

typedef struct {
    const char *path;
    const char *label;
    uint8_t source;
} NexusScanRoot;

static const NexusScanRoot scan_roots[] = {
    {"mass:/", "USB MASS STORAGE", NEXUS_LIBRARY_SOURCE_USB},
    {"mmce0:/", "MMCE SLOT 0", NEXUS_LIBRARY_SOURCE_MMCE},
    {"mmce1:/", "MMCE SLOT 1", NEXUS_LIBRARY_SOURCE_MMCE}
};

static const NexusScanRoot *selected_root(const NexusActionStatus *status)
{
    unsigned int count = sizeof(scan_roots) / sizeof(scan_roots[0]);
    unsigned int index;

    if (status == 0)
        return &scan_roots[0];
    index = (unsigned int)status->scan_root_index % count;
    return &scan_roots[index];
}

const char *nexus_actions_scan_root_path(const NexusActionStatus *status)
{
    return selected_root(status)->path;
}

const char *nexus_actions_scan_root_label(const NexusActionStatus *status)
{
    return selected_root(status)->label;
}

void nexus_actions_init(NexusActionStatus *status)
{
    if (status == 0)
        return;

    memset(status, 0, sizeof(*status));
    status->filexio_ready = (uint8_t)(nexus_filexio_ready() != 0);
}

static void move_scan_root(NexusActionStatus *status, int delta)
{
    const unsigned int count = sizeof(scan_roots) / sizeof(scan_roots[0]);
    int index;

    index = (int)(status->scan_root_index % count);
    index = (index + (delta % (int)count) + (int)count) % (int)count;
    status->scan_root_index = (uint8_t)index;
    status->scan_attempted = 0;
    status->last_scan_result = 0;
    memset(&status->last_scan_report, 0, sizeof(status->last_scan_report));
}

static void execute_read_only_scan(NexusActionStatus *status)
{
    const NexusScanRoot *root = selected_root(status);
    NexusScanConfig config;

    memset(&config, 0, sizeof(config));
    config.source = root->source;
    config.scan_ps2 = 1U;
    config.scan_ps1 = 1U;
    strncpy(config.root, root->path, sizeof(config.root) - 1U);

    memset(&status->last_scan_report, 0, sizeof(status->last_scan_report));
    status->last_scan_result =
        nexus_filexio_scan_root(&config, &status->last_scan_report);
    status->scan_attempted = 1U;
}

void nexus_actions_apply(NexusNavState *navigation,
                         NexusActionStatus *status,
                         NexusKey key)
{
    if (navigation == 0)
        return;

    if (status == 0) {
        nexus_nav_apply(navigation, key);
        return;
    }

    status->filexio_ready = (uint8_t)(nexus_filexio_ready() != 0);

    if (navigation->in_section && navigation->page == NEXUS_PAGE_SYSTEM) {
        if (status->filexio_confirm_pending) {
            if (key == NEXUS_KEY_CONFIRM) {
                status->filexio_confirm_pending = 0;
                status->filexio_attempted = 1;
                status->last_filexio_result = nexus_filexio_activate();
                status->filexio_ready = (uint8_t)(nexus_filexio_ready() != 0);
                return;
            }
            if (key == NEXUS_KEY_BACK) {
                status->filexio_confirm_pending = 0;
                return;
            }
            if (key == NEXUS_KEY_HOME) {
                status->filexio_confirm_pending = 0;
                nexus_nav_apply(navigation, key);
                return;
            }
            return;
        }

        if (key == NEXUS_KEY_ACTION) {
            if (!status->filexio_ready) {
                status->filexio_confirm_pending = 1;
                status->filexio_attempted = 0;
                status->last_filexio_result = 0;
            }
            return;
        }
    }

    if (navigation->in_section && navigation->page == NEXUS_PAGE_FILES) {
        if (status->scan_confirm_pending) {
            if (key == NEXUS_KEY_CONFIRM) {
                status->scan_confirm_pending = 0;
                if (status->filexio_ready)
                    execute_read_only_scan(status);
                else {
                    status->last_scan_result = -1;
                    status->scan_attempted = 1U;
                }
                return;
            }
            if (key == NEXUS_KEY_BACK) {
                status->scan_confirm_pending = 0;
                return;
            }
            if (key == NEXUS_KEY_HOME) {
                status->scan_confirm_pending = 0;
                nexus_nav_apply(navigation, key);
                return;
            }
            return;
        }

        if (key == NEXUS_KEY_UP) {
            move_scan_root(status, -1);
            return;
        }
        if (key == NEXUS_KEY_DOWN) {
            move_scan_root(status, 1);
            return;
        }
        if (key == NEXUS_KEY_ACTION) {
            if (status->filexio_ready) {
                status->scan_confirm_pending = 1U;
                status->scan_attempted = 0U;
                status->last_scan_result = 0;
                memset(&status->last_scan_report, 0,
                       sizeof(status->last_scan_report));
            }
            return;
        }
    }

    nexus_nav_apply(navigation, key);
}
