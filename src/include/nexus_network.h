#ifndef NEXUS_NETWORK_H
#define NEXUS_NETWORK_H
#include <stddef.h>
#include <stdint.h>

/* Provider-neutral contract. This header does not load drivers or imply support. */
typedef enum {
    NEXUS_NET_UNKNOWN = 0,
    NEXUS_NET_OFFLINE,
    NEXUS_NET_LINK_UP,
    NEXUS_NET_ADDRESS_READY,
    NEXUS_NET_ERROR
} nexus_net_state_t;

enum {
    NEXUS_NET_CAP_DHCP       = 1u << 0,
    NEXUS_NET_CAP_DNS        = 1u << 1,
    NEXUS_NET_CAP_HTTP       = 1u << 2,
    NEXUS_NET_CAP_HTTPS      = 1u << 3,
    NEXUS_NET_CAP_RANGE      = 1u << 4,
    NEXUS_NET_CAP_WIFI       = 1u << 5
};

typedef struct {
    const char *provider_id;
    uint32_t capabilities;
    nexus_net_state_t (*state)(void);
    int (*connect)(void);
    void (*disconnect)(void);
    int (*http_get_range)(const char *url, uint64_t offset,
                          void *buffer, size_t capacity,
                          size_t *received, int *status_code);
} nexus_network_provider_t;

/* LINK_UP is not ADDRESS_READY; HTTP is not HTTPS; Wi-Fi is not Bluetooth HID. */
#endif
