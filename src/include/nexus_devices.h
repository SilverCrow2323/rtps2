#ifndef NEXUS_DEVICES_H
#define NEXUS_DEVICES_H

typedef enum {
    NEXUS_DEVICE_UNKNOWN = 0,
    NEXUS_DEVICE_ABSENT,
    NEXUS_DEVICE_PRESENT,
    NEXUS_DEVICE_ERROR
} NexusDeviceState;

typedef struct {
    NexusDeviceState disc;
    NexusDeviceState mc0;
    NexusDeviceState mc1;
    NexusDeviceState hdd;
    NexusDeviceState mmce;
    NexusDeviceState usb;
} NexusDeviceSnapshot;

/* Returns UNKNOWN until a real backend has probed the device. */
void nexus_devices_snapshot(NexusDeviceSnapshot *snapshot);
const char *nexus_device_state_name(NexusDeviceState state);

#endif
