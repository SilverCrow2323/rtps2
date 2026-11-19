#ifndef NEXUS_ATLAS_H
#define NEXUS_ATLAS_H

#include <stddef.h>
#include <tamtypes.h>
#include <draw.h>

/* NXA1 v1, fixed NEXUS section atlas: 256x128 PSMCT32 RGBA. */
int nexus_atlas_validate_pack(const void *pack, size_t pack_size);
int nexus_atlas_upload(const void *pack, size_t pack_size);
int nexus_atlas_ready(void);
qword_t *nexus_atlas_bind(qword_t *q, int context);
qword_t *nexus_atlas_draw(qword_t *q, int context, unsigned int icon,
                          int x, int y, int size);
void nexus_atlas_release(void);

#endif
