#ifndef NEXUS_ACTIONS_H
#define NEXUS_ACTIONS_H

#include <stdint.h>
#include "nexus_navigation.h"

typedef struct {
    uint8_t filexio_confirm_pending;
    uint8_t filexio_attempted;
    uint8_t filexio_ready;
    int last_filexio_result;
} NexusActionStatus;

void nexus_actions_init(NexusActionStatus *status);
void nexus_actions_apply(NexusNavState *navigation,
                         NexusActionStatus *status,
                         NexusKey key);

#endif
