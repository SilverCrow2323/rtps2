#include <debug.h>

#include "nexus_devices.h"
#include "nexus_renderer.h"
#include "nexus_ui.h"

static const char *const home_sections[] = {
    "GAMES",
    "PS1 LIBRARY",
    "HOMEBREW",
    "NEXUS STORE",
    "FILES",
    "SYSTEM"
};

static int gs_renderer_ready;

void nexus_ui_init(void)
{
    init_scr();
    scr_setbgcolor(0x00131d);
    scr_setfontcolor(0x00d9ff);
    scr_clear();

    gs_renderer_ready = (nexus_renderer_init() == 0);
    if (!gs_renderer_ready) {
        scr_printf("NEXUS GS renderer unavailable.\n");
        scr_printf("Using PS2SDK debug renderer fallback.\n");
    }
}

static void render_fallback_home(const NexusNavState *state, int input_ready, int connected)
{
    NexusDeviceSnapshot devices;
    unsigned int i;

    nexus_devices_snapshot(&devices);
    scr_printf("SPDW FACTORY  /  PS2BBN NEXUS\n");
    scr_printf("==============================================\n");
    scr_printf("SYSTEM STATUS - READ ONLY\n");
    scr_printf("DISC   %-14s  MC0    %-14s\n",
               nexus_device_state_name(devices.disc),
               nexus_device_state_name(devices.mc0));
    scr_printf("HDD    %-14s  MMCE   %-14s\n",
               nexus_device_state_name(devices.hdd),
               nexus_device_state_name(devices.mmce));
    scr_printf("MC1    %-14s  USB    %-14s\n\n",
               nexus_device_state_name(devices.mc1),
               nexus_device_state_name(devices.usb));

    scr_printf("YOUR LIBRARY\n");
    for (i = 0; i < sizeof(home_sections) / sizeof(home_sections[0]); ++i) {
        scr_printf("%s %s\n", (state->selected == i) ? ">" : " ", home_sections[i]);
    }
    scr_printf("\nPAD: %s  CONTROLLER: %s\n",
               input_ready ? "READY" : "UNAVAILABLE",
               connected ? "CONNECTED" : "WAITING");
    scr_printf("UP/DOWN SELECT  CROSS OPEN  L1/R1 MOVE\n");
    scr_printf("START HOME  CIRCLE BACK\n");
}

static void render_fallback_page(const NexusNavState *state, int input_ready, int connected,
                                 const NexusActionStatus *actions)
{
    scr_printf("SPDW FACTORY  /  NEXUS  /  %s\n", nexus_page_name(state->page));
    scr_printf("==============================================\n\n");

    switch (state->page) {
        case NEXUS_PAGE_GAMES:
            scr_printf("GAME LIBRARY\nStorage providers: not connected yet\n");
            break;
        case NEXUS_PAGE_PS1:
            scr_printf("PLAYSTATION 1 LIBRARY\nPOPS discovery: pending\n");
            break;
        case NEXUS_PAGE_HOMEBREW:
            scr_printf("HOMEBREW HUB\nLocal ELF discovery: pending\n");
            break;
        case NEXUS_PAGE_STORE:
            scr_printf("NEXUS STORE\nHost signed catalog exists; native transport pending.\n");
            break;
        case NEXUS_PAGE_FILES:
            scr_printf("FILE EXPLORER - READ ONLY\n");
            if (actions != 0 && actions->scan_confirm_pending) {
                scr_printf("Scan root: %s\n",
                           nexus_actions_scan_root_path(actions));
                scr_printf("CROSS confirm scan / CIRCLE cancel / START home\n");
                scr_printf("Only CD/DVD/POPS folders are inspected.\n");
            } else if (actions != 0 && actions->scan_attempted) {
                scr_printf("Root: %s\n",
                           nexus_actions_scan_root_path(actions));
                scr_printf("Result: %d  Indexed: %lu  Seen: %lu  Rejected: %lu\n",
                           actions->last_scan_result,
                           (unsigned long)actions->last_scan_report.indexed,
                           (unsigned long)actions->last_scan_report.entries_seen,
                           (unsigned long)actions->last_scan_report.rejected);
                scr_printf("No game was launched and no files were written.\n");
            } else if (actions != 0 && actions->filexio_ready) {
                scr_printf("FileXio ready. Root: %s (%s)\n",
                           nexus_actions_scan_root_path(actions),
                           nexus_actions_scan_root_label(actions));
                scr_printf("UP/DOWN choose root. TRIANGLE requests scan.\n");
                scr_printf("CROSS confirms; CIRCLE cancels.\n");
            } else {
                scr_printf("FileXio inactive. No driver or volume is mounted.\n");
                scr_printf("Activate services in System first.\n");
            }
            break;
        case NEXUS_PAGE_SYSTEM:
            scr_printf("SYSTEM\nInput: %s\nController: %s\n",
                       input_ready ? "READY" : "UNAVAILABLE",
                       connected ? "CONNECTED" : "WAITING");
            if (actions != 0 && actions->filexio_confirm_pending) {
                scr_printf("Activate missing IOMANX/FileXio modules?\n");
                scr_printf("May apply SBV hooks; no IOP reset, mount or scan. CROSS confirm / CIRCLE cancel.\n");
            } else if (actions != 0 && actions->filexio_ready) {
                scr_printf("FileXio RPC: READY (no volumes mounted)\n");
            } else if (actions != 0 && actions->filexio_attempted) {
                scr_printf("FileXio activation result: %d\n",
                           actions->last_filexio_result);
            } else {
                scr_printf("Triangle: request explicit FileXio activation\n");
            }
            scr_printf("Storage probing: not started\n");
            break;
        default:
            scr_printf("Unknown page.\n");
            break;
    }

    scr_printf("\nL1/R1 PREVIOUS/NEXT  CIRCLE BACK  START HOME\n");
}

void nexus_ui_render(const NexusNavState *state, int input_ready,
                     int controller_connected, const NexusActionStatus *actions)
{
    if (state == 0) {
        return;
    }

    if (gs_renderer_ready &&
        nexus_renderer_render(state, input_ready, controller_connected, actions) == 0) {
        return;
    }

    scr_clear();
    if (!state->in_section || state->page == NEXUS_PAGE_HOME) {
        render_fallback_home(state, input_ready, controller_connected);
    } else {
        render_fallback_page(state, input_ready, controller_connected, actions);
    }
}
