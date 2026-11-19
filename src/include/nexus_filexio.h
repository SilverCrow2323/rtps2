#ifndef NEXUS_FILEXIO_H
#define NEXUS_FILEXIO_H

#include "nexus_library_scan.h"

/*
 * Optional, explicit activation. The NEXUS bootstrap must already have
 * initialized SIF RPC. This may load the PS2SDK IOMANX and fileXio IRX modules
 * into the current IOP session, but never resets the IOP,
 * mounts/unmounts devices or loads a storage block driver.
 */
int nexus_filexio_activate(void);
int nexus_filexio_ready(void);

/* Scan an already-accessible root such as mass:/ or mmce0:/; no mounting. */
int nexus_filexio_scan_root(const NexusScanConfig *config,
                            NexusScanReport *report);

#endif
