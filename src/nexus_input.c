#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <string.h>

#include <libpad.h>

#include "nexus_input.h"

#define NEXUS_PAD_PORT 0
#define NEXUS_PAD_SLOT 0

/* libpad requires a 256-byte work area aligned to at least 64 bytes. */
static unsigned char pad_buffer[256] __attribute__((aligned(64)));
static int pad_open;
static int pad_connected;
static u16 previous_buttons;

static int load_pad_modules(void)
{
    int result;

    result = SifLoadModule("rom0:SIO2MAN", 0, 0);
    if (result < 0) {
        return result;
    }

    result = SifLoadModule("rom0:PADMAN", 0, 0);
    if (result < 0) {
        return result;
    }

    return 0;
}

int nexus_input_init(void)
{
    int result;

    pad_open = 0;
    pad_connected = 0;
    previous_buttons = 0;

    result = load_pad_modules();
    if (result < 0) {
        return result;
    }

    if (padInit(0) != 1) {
        return -1;
    }

    result = padPortOpen(NEXUS_PAD_PORT, NEXUS_PAD_SLOT, pad_buffer);
    if (result == 0) {
        return -2;
    }

    pad_open = 1;
    return 0;
}

NexusKey nexus_input_poll(void)
{
    struct padButtonStatus status;
    u16 pressed;
    u16 held;
    int pad_state;

    if (!pad_open) {
        return NEXUS_KEY_NONE;
    }

    pad_state = padGetState(NEXUS_PAD_PORT, NEXUS_PAD_SLOT);
    if (pad_state != PAD_STATE_STABLE) {
        if (pad_state == PAD_STATE_DISCONN) {
            pad_connected = 0;
            previous_buttons = 0;
        }
        return NEXUS_KEY_NONE;
    }

    memset(&status, 0, sizeof(status));
    if (padRead(NEXUS_PAD_PORT, NEXUS_PAD_SLOT, &status) == 0) {
        return NEXUS_KEY_NONE;
    }

    pad_connected = 1;
    held = (u16)(0xffffU ^ status.btns);
    pressed = (u16)(held & (u16)(~previous_buttons));
    previous_buttons = held;

    if ((pressed & PAD_START) != 0) return NEXUS_KEY_HOME;
    if ((pressed & PAD_CIRCLE) != 0) return NEXUS_KEY_BACK;
    if ((pressed & PAD_CROSS) != 0) return NEXUS_KEY_CONFIRM;
    if ((pressed & PAD_UP) != 0) return NEXUS_KEY_UP;
    if ((pressed & PAD_DOWN) != 0) return NEXUS_KEY_DOWN;
    if ((pressed & PAD_LEFT) != 0) return NEXUS_KEY_LEFT;
    if ((pressed & PAD_RIGHT) != 0) return NEXUS_KEY_RIGHT;
    if ((pressed & PAD_L1) != 0) return NEXUS_KEY_PREVIOUS;
    if ((pressed & PAD_R1) != 0) return NEXUS_KEY_NEXT;

    return NEXUS_KEY_NONE;
}

int nexus_input_connected(void)
{
    return pad_connected;
}
