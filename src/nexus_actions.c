#include <string.h>

#include "nexus_actions.h"
#include "nexus_filexio.h"

void nexus_actions_init(NexusActionStatus *status)
{
    if (status == 0)
        return;

    memset(status, 0, sizeof(*status));
    status->filexio_ready = (uint8_t)(nexus_filexio_ready() != 0);
}

void nexus_actions_apply(NexusNavState *navigation,
                         NexusActionStatus *status,
                         NexusKey key)
{
    if (navigation == 0)
        return;

    if (status == 0) {
        nexus_nav_apply(navigation, key);
        return;
    }

    status->filexio_ready = (uint8_t)(nexus_filexio_ready() != 0);

    if (navigation->in_section && navigation->page == NEXUS_PAGE_SYSTEM) {
        if (status->filexio_confirm_pending) {
            if (key == NEXUS_KEY_CONFIRM) {
                status->filexio_confirm_pending = 0;
                status->filexio_attempted = 1;
                status->last_filexio_result = nexus_filexio_activate();
                status->filexio_ready = (uint8_t)(nexus_filexio_ready() != 0);
                return;
            }

            if (key == NEXUS_KEY_BACK) {
                status->filexio_confirm_pending = 0;
                return;
            }

            if (key == NEXUS_KEY_HOME) {
                status->filexio_confirm_pending = 0;
                nexus_nav_apply(navigation, key);
                return;
            }

            /* While the warning is open, unrelated input cannot activate. */
            return;
        }

        if (key == NEXUS_KEY_ACTION) {
            if (!status->filexio_ready) {
                status->filexio_confirm_pending = 1;
                status->filexio_attempted = 0;
                status->last_filexio_result = 0;
            }
            return;
        }
    }

    nexus_nav_apply(navigation, key);
}
