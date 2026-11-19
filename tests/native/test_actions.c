#include <assert.h>

#include "nexus_actions.h"

static int adapter_ready;
static int activation_calls;
static int activation_result;

int nexus_filexio_ready(void)
{
    return adapter_ready;
}

int nexus_filexio_activate(void)
{
    ++activation_calls;
    if (activation_result == 0)
        adapter_ready = 1;
    return activation_result;
}

static void navigate_to_system(NexusNavState *navigation)
{
    unsigned int i;
    nexus_nav_init(navigation);
    for (i = 0; i < 5U; ++i)
        nexus_actions_apply(navigation, 0, NEXUS_KEY_DOWN);
    nexus_actions_apply(navigation, 0, NEXUS_KEY_CONFIRM);
    assert(navigation->page == NEXUS_PAGE_SYSTEM);
    assert(navigation->in_section == 1U);
}

int main(void)
{
    NexusNavState navigation;
    NexusActionStatus status;

    adapter_ready = 0;
    activation_calls = 0;
    activation_result = 0;
    navigate_to_system(&navigation);
    nexus_actions_init(&status);

    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.filexio_confirm_pending == 1U);
    assert(activation_calls == 0);

    /* Navigation input cannot accidentally confirm a pending activation. */
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_NEXT);
    assert(status.filexio_confirm_pending == 1U);
    assert(activation_calls == 0);

    nexus_actions_apply(&navigation, &status, NEXUS_KEY_BACK);
    assert(status.filexio_confirm_pending == 0U);
    assert(activation_calls == 0);
    assert(navigation.page == NEXUS_PAGE_SYSTEM);

    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.filexio_confirm_pending == 1U);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_CONFIRM);
    assert(status.filexio_confirm_pending == 0U);
    assert(status.filexio_attempted == 1U);
    assert(status.filexio_ready == 1U);
    assert(status.last_filexio_result == 0);
    assert(activation_calls == 1);

    /* Once ready, Triangle does not trigger a second activation. */
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.filexio_confirm_pending == 0U);
    assert(activation_calls == 1);

    /* A failed explicit attempt is reported and can be retried by the user. */
    adapter_ready = 0;
    activation_result = -3;
    status.filexio_ready = 0;
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_CONFIRM);
    assert(status.filexio_attempted == 1U);
    assert(status.filexio_ready == 0U);
    assert(status.last_filexio_result == -3);
    assert(activation_calls == 2);

    /* Leaving System with START while a prompt is open cancels the prompt. */
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.filexio_confirm_pending == 1U);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_HOME);
    assert(status.filexio_confirm_pending == 0U);
    assert(navigation.page == NEXUS_PAGE_HOME);
    assert(navigation.in_section == 0U);
    assert(activation_calls == 2);
    return 0;
}
