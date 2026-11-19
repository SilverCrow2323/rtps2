#ifndef NEXUS_UI_H
#define NEXUS_UI_H

#include "nexus_navigation.h"
#include "nexus_actions.h"

void nexus_ui_init(void);
void nexus_ui_render(const NexusNavState *state, int input_ready,
                     int controller_connected, const NexusActionStatus *actions);

#endif
