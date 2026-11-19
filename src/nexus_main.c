#include <kernel.h>
#include <sifrpc.h>

#include "nexus_input.h"
#include "nexus_navigation.h"
#include "nexus_ui.h"

int main(void)
{
    NexusNavState navigation;
    NexusKey input;
    int input_ready;
    int connected;
    int previous_connected;

    sceSifInitRpc(0);
    nexus_ui_init();

    input_ready = (nexus_input_init() == 0);
    nexus_nav_init(&navigation);
    connected = nexus_input_connected();
    previous_connected = connected;
    nexus_ui_render(&navigation, input_ready, connected);

    for (;;) {
        input = nexus_input_poll();
        connected = nexus_input_connected();

        if (input != NEXUS_KEY_NONE) {
            nexus_nav_apply(&navigation, input);
            nexus_ui_render(&navigation, input_ready, connected);
        } else if (connected != previous_connected) {
            /* Refresh the status row when a pad connects or disconnects. */
            nexus_ui_render(&navigation, input_ready, connected);
        }

        previous_connected = connected;

        /* PS2SDK EE kernel.h provides nopdelay(); DelayThread is IOP-side. */
        nopdelay();
    }

    return 0;
}
