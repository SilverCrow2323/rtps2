#include <debug.h>

#include "nexus_devices.h"
#include "nexus_ui.h"

static const char *const home_labels[] = {
    "GAMES",
    "PS1 LIBRARY",
    "HOMEBREW",
    "NEXUS STORE",
    "FILES",
    "SYSTEM"
};

void nexus_ui_init(void)
{
    init_scr();
    scr_setbgcolor(0x00131d);
    scr_setfontcolor(0x00d9ff);
    scr_clear();
}

static void render_home(const NexusNavState *state, int input_ready, int connected)
{
    NexusDeviceSnapshot devices;
    unsigned int i;

    nexus_devices_snapshot(&devices);

    scr_printf("SPDW FACTORY  /  PS2BBN NEXUS\n");
    scr_printf("==============================================\n");
    scr_printf("A new shell for your PlayStation 2\n\n");
    scr_printf("SYSTEM STATUS\n");
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
    for (i = 0; i < sizeof(home_labels) / sizeof(home_labels[0]); ++i) {
        scr_printf("%s %s\n",
                   (state->selected == i) ? ">" : " ",
                   home_labels[i]);
    }

    scr_printf("\n----------------------------------------------\n");
    scr_printf("PAD: %s   CONTROLLER: %s\n",
               input_ready ? "LIBPAD READY" : "UNAVAILABLE",
               connected ? "CONNECTED" : "WAITING");
    scr_printf("UP/DOWN: SELECT   CROSS: OPEN   L1/R1: MOVE\n");
    scr_printf("START: HOME   CIRCLE: BACK\n");
}

static void render_section(const NexusNavState *state, int input_ready, int connected)
{
    scr_printf("SPDW FACTORY  /  NEXUS  /  %s\n", nexus_page_name(state->page));
    scr_printf("==============================================\n\n");

    switch (state->page) {
        case NEXUS_PAGE_GAMES:
            scr_printf("GAME LIBRARY\n\n");
            scr_printf("Storage providers: not connected yet\n");
            scr_printf("Planned sources: HDD / MMCE / USB\n");
            scr_printf("Views: GRID / CAROUSEL / COVERFLOW / LIST\n");
            scr_printf("Launch adapters: OPL / NEUTRINO (pending)\n");
            break;
        case NEXUS_PAGE_PS1:
            scr_printf("PLAYSTATION 1 LIBRARY\n\n");
            scr_printf("POPS discovery and title database: pending\n");
            scr_printf("No storage scan has been performed.\n");
            break;
        case NEXUS_PAGE_HOMEBREW:
            scr_printf("HOMEBREW HUB\n\n");
            scr_printf("Local ELF discovery: pending\n");
            scr_printf("Categories and metadata provider: pending\n");
            break;
        case NEXUS_PAGE_STORE:
            scr_printf("NEXUS STORE\n\n");
            scr_printf("Signed catalog and package pipeline exist on host.\n");
            scr_printf("Native PS2 network transport: not connected\n");
            scr_printf("Native installer: not implemented\n");
            break;
        case NEXUS_PAGE_FILES:
            scr_printf("FILE EXPLORER\n\n");
            scr_printf("Mounted-volume provider: not connected\n");
            scr_printf("No filesystem is mounted or modified.\n");
            break;
        case NEXUS_PAGE_SYSTEM:
            scr_printf("SYSTEM\n\n");
            scr_printf("DualShock input: %s\n", input_ready ? "initialized" : "unavailable");
            scr_printf("Controller: %s\n", connected ? "connected" : "waiting");
            scr_printf("Device probing: not implemented\n");
            scr_printf("Renderer: PS2SDK debug text (bootstrap)\n");
            scr_printf("Graphics UI: next native milestone\n");
            break;
        default:
            scr_printf("Unknown page.\n");
            break;
    }

    scr_printf("\n----------------------------------------------\n");
    scr_printf("L1/R1: PREVIOUS/NEXT SECTION   CIRCLE: BACK\n");
    scr_printf("START: HOME\n");
}

void nexus_ui_render(const NexusNavState *state, int input_ready, int controller_connected)
{
    if (state == 0) {
        return;
    }

    scr_clear();
    if (!state->in_section || state->page == NEXUS_PAGE_HOME) {
        render_home(state, input_ready, controller_connected);
    } else {
        render_section(state, input_ready, controller_connected);
    }
}
