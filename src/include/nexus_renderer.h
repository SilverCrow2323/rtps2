#ifndef NEXUS_RENDERER_H
#define NEXUS_RENDERER_H

#include "nexus_navigation.h"

int nexus_renderer_init(void);
int nexus_renderer_ready(void);
int nexus_renderer_render(const NexusNavState *state, int input_ready, int connected);

#endif
