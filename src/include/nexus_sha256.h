#ifndef NEXUS_SHA256_H
#define NEXUS_SHA256_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Computes SHA-256 without heap allocation. Returns 0, or -1 for bad args. */
int nexus_sha256(const void *data, size_t length, unsigned char digest[32]);

#ifdef __cplusplus
}
#endif

#endif
