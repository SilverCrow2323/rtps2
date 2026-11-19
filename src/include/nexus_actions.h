#ifndef NEXUS_ACTIONS_H
#define NEXUS_ACTIONS_H

#include <stdint.h>
#include "nexus_navigation.h"
#include "nexus_library_scan.h"

typedef struct {
    uint8_t filexio_confirm_pending;
    uint8_t filexio_attempted;
    uint8_t filexio_ready;
    int last_filexio_result;
    uint8_t scan_confirm_pending;
    uint8_t scan_attempted;
    uint8_t scan_root_index;
    int last_scan_result;
    NexusScanReport last_scan_report;
    NexusLibraryView library_view;
    uint8_t game_detail_open;
} NexusActionStatus;

void nexus_actions_init(NexusActionStatus *status);
void nexus_actions_apply(NexusNavState *navigation,
                         NexusActionStatus *status,
                         NexusKey key);
const char *nexus_actions_scan_root_path(const NexusActionStatus *status);
const char *nexus_actions_scan_root_label(const NexusActionStatus *status);

#endif
