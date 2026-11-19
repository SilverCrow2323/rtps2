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

    sceSifInitRpc(0);
    nexus_ui_init();

    input_ready = (nexus_input_init() == 0);
    nexus_nav_init(&navigation);
    nexus_ui_render(&navigation, input_ready, 0);

    for (;;) {
        input = nexus_input_poll();
        if (input != NEXUS_KEY_NONE) {
            nexus_nav_apply(&navigation, input);
            nexus_ui_render(&navigation, input_ready, nexus_input_connected());
        }

        /* Avoid a tight polling loop while keeping input responsive. */
        DelayThread(10000);
    }

    return 0;
}
