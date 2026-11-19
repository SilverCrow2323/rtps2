#ifndef NEXUS_UI_H
#define NEXUS_UI_H

#include "nexus_navigation.h"

void nexus_ui_init(void);
void nexus_ui_render(const NexusNavState *state, int input_ready, int controller_connected);

#endif
