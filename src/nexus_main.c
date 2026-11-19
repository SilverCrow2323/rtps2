#include <kernel.h>
#include <sifrpc.h>
#include <delaythread.h>

#include "nexus_input.h"
#include "nexus_navigation.h"
#include "nexus_actions.h"
#include "nexus_ui.h"

int main(void)
{
    NexusNavState navigation;
    NexusActionStatus actions;
    NexusKey input;
    int input_ready;
    int connected;
    int previous_connected;

    sceSifInitRpc(0);
    nexus_ui_init();

    input_ready = (nexus_input_init() == 0);
    nexus_nav_init(&navigation);
    nexus_actions_init(&actions);
    connected = nexus_input_connected();
    previous_connected = connected;
    nexus_ui_render(&navigation, input_ready, connected, &actions);

    for (;;) {
        input = nexus_input_poll();
        connected = nexus_input_connected();

        if (input != NEXUS_KEY_NONE) {
            nexus_actions_apply(&navigation, &actions, input);
            nexus_ui_render(&navigation, input_ready, connected);
        } else if (connected != previous_connected) {
            /* Refresh the status row when a pad connects or disconnects. */
            nexus_ui_render(&navigation, input_ready, connected);
        }

        previous_connected = connected;
        DelayThread(10000);
    }

    return 0;
}
