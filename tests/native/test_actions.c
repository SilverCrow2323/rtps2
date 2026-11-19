#include <assert.h>

#include "nexus_actions.h"

static int adapter_ready;
static int activation_calls;
static int activation_result;
static int scan_calls;
static int scan_result;
static NexusScanConfig last_scan_config;

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

int nexus_filexio_scan_root(const NexusScanConfig *config,
                            NexusScanReport *report)
{
    ++scan_calls;
    last_scan_config = *config;
    if (report != 0) {
        report->root_available = 1U;
        report->directories_opened = 3U;
        report->entries_seen = 5U;
        report->supported_files = 3U;
        report->indexed = 3U;
    }
    return scan_result;
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

    /* Triangle is inert everywhere outside System. */
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.filexio_confirm_pending == 0U);
    assert(activation_calls == 2);
    assert(navigation.page == NEXUS_PAGE_HOME);

    /* Files root selection is harmless until FileXio is explicitly ready. */
    adapter_ready = 0;
    scan_calls = 0;
    scan_result = 0;
    nexus_nav_init(&navigation);
    nexus_actions_init(&status);
    for (unsigned int i = 0; i < 4U; ++i)
        nexus_actions_apply(&navigation, &status, NEXUS_KEY_DOWN);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_CONFIRM);
    assert(navigation.page == NEXUS_PAGE_FILES);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.scan_confirm_pending == 0U);
    assert(scan_calls == 0);

    /* The root list is a hard-coded allowlist, never free-form text. */
    assert(strcmp(nexus_actions_scan_root_path(&status), "mass:/") == 0);
    adapter_ready = 1;
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_DOWN);
    assert(status.filexio_ready == 1U);
    assert(strcmp(nexus_actions_scan_root_path(&status), "mmce0:/") == 0);
    assert(strcmp(nexus_actions_scan_root_label(&status), "MMCE SLOT 0") == 0);

    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.scan_confirm_pending == 1U);
    assert(scan_calls == 0);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_DOWN);
    assert(strcmp(nexus_actions_scan_root_path(&status), "mmce0:/") == 0);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_BACK);
    assert(status.scan_confirm_pending == 0U);
    assert(scan_calls == 0);

    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.scan_confirm_pending == 1U);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_CONFIRM);
    assert(status.scan_confirm_pending == 0U);
    assert(status.scan_attempted == 1U);
    assert(status.last_scan_result == 0);
    assert(status.last_scan_report.indexed == 3U);
    assert(scan_calls == 1);
    assert(strcmp(last_scan_config.root, "mmce0:/") == 0);
    assert(last_scan_config.source == NEXUS_LIBRARY_SOURCE_MMCE);
    assert(last_scan_config.scan_ps2 == 1U);
    assert(last_scan_config.scan_ps1 == 1U);

    /* Next root selection invalidates the previous result; START cancels. */
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_DOWN);
    assert(strcmp(nexus_actions_scan_root_path(&status), "mmce1:/") == 0);
    assert(status.scan_attempted == 0U);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_ACTION);
    assert(status.scan_confirm_pending == 1U);
    nexus_actions_apply(&navigation, &status, NEXUS_KEY_HOME);
    assert(status.scan_confirm_pending == 0U);
    assert(navigation.page == NEXUS_PAGE_HOME);
    assert(scan_calls == 1);
    return 0;
}
