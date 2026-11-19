#include "nexus_devices.h"

void nexus_devices_snapshot(NexusDeviceSnapshot *snapshot)
{
    if (snapshot == 0) {
        return;
    }

    /* Never infer hardware presence from a model or a previous test report. */
    snapshot->disc = NEXUS_DEVICE_UNKNOWN;
    snapshot->mc0 = NEXUS_DEVICE_UNKNOWN;
    snapshot->mc1 = NEXUS_DEVICE_UNKNOWN;
    snapshot->hdd = NEXUS_DEVICE_UNKNOWN;
    snapshot->mmce = NEXUS_DEVICE_UNKNOWN;
    snapshot->usb = NEXUS_DEVICE_UNKNOWN;
}

const char *nexus_device_state_name(NexusDeviceState state)
{
    switch (state) {
        case NEXUS_DEVICE_ABSENT: return "NOT DETECTED";
        case NEXUS_DEVICE_PRESENT: return "PRESENT";
        case NEXUS_DEVICE_ERROR: return "ERROR";
        case NEXUS_DEVICE_UNKNOWN:
        default: return "NOT PROBED";
    }
}
