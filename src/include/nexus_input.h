#ifndef NEXUS_INPUT_H
#define NEXUS_INPUT_H

#include "nexus_navigation.h"

/* PS2SDK/libpad-backed input. Call after SIF RPC initialization. */
int nexus_input_init(void);
NexusKey nexus_input_poll(void);
int nexus_input_connected(void);

#endif
