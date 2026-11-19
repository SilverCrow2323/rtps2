#include <tamtypes.h>
#include <stdlib.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <gs_psm.h>
#include <draw.h>
#include <font.h>

#include "nexus_renderer.h"
#include "nexus_atlas.h"
#include "nexus_icon_patterns.h"

#define NEXUS_WIDTH 640
#define NEXUS_NTSC_HEIGHT 448
#define NEXUS_PAL_HEIGHT 512
#define NEXUS_PACKET_QWORDS 60000
#define NEXUS_CONTEXT 0

/* SPDW Factory / NEXUS palette. */
#define C_BG       7, 17, 29
#define C_HEADER   12, 31, 48
#define C_PANEL    15, 39, 58
#define C_PANEL_2  20, 49, 69
#define C_SELECTED 18, 80, 101
#define C_ACCENT   60, 218, 235
#define C_TEXT     232, 243, 249
#define C_MUTED    133, 160, 178
#define C_WARN     223, 183, 105
#define C_DARK     4, 12, 21

static framebuffer_t frames[2];
static packet_t *render_packet;
static fontx_t system_font;
static int screen_height;
static int front_index;
static int renderer_is_ready;

static color_t make_color(u8 r, u8 g, u8 b)
{
    color_t color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = 0x80;
    color.q = 1.0f;
    return color;
}

static qword_t *fill_rect(qword_t *q, int x, int y, int width, int height,
                          u8 r, u8 g, u8 b)
{
    rect_t rect;

    rect.v0.x = (float)(x - (NEXUS_WIDTH / 2));
    rect.v0.y = (float)(y - (screen_height / 2));
    rect.v0.z = 0;
    rect.v1.x = (float)(x + width - (NEXUS_WIDTH / 2));
    rect.v1.y = (float)(y + height - (screen_height / 2));
    rect.v1.z = 0;
    rect.color = make_color(r, g, b);
    return draw_rect_filled(q, NEXUS_CONTEXT, &rect);
}

static qword_t *print_text(qword_t *q, const char *value, int x, int y,
                           u8 r, u8 g, u8 b)
{
    vertex_t position;
    color_t color;

    position.x = (float)(x - (NEXUS_WIDTH / 2));
    position.y = (float)(y - (screen_height / 2));
    position.z = 0;
    color = make_color(r, g, b);
    return fontx_print_ascii(q, NEXUS_CONTEXT,
                             (const unsigned char *)value,
                             LEFT_ALIGN, &position, &color, &system_font);
}

static qword_t *draw_topbar(qword_t *q, const char *section)
{
    q = fill_rect(q, 0, 0, NEXUS_WIDTH, 47, C_HEADER);
    q = fill_rect(q, 0, 46, NEXUS_WIDTH, 3, C_ACCENT);
    q = print_text(q, "SPDW FACTORY", 20, 16, C_ACCENT);
    q = print_text(q, "/  PS2BBN NEXUS", 130, 16, C_TEXT);
    if (section != 0) {
        q = print_text(q, section, 500, 16, C_MUTED);
    }
    return q;
}

static qword_t *draw_status_chip(qword_t *q, int index, const char *label)
{
    int x = 10 + index * 104;
    q = fill_rect(q, x, 104, 98, 40, C_PANEL);
    q = fill_rect(q, x, 104, 3, 40, C_ACCENT);
    q = print_text(q, label, x + 10, 109, C_MUTED);
    q = print_text(q, "NOT PROBED", x + 10, 126, C_WARN);
    return q;
}

static qword_t *draw_icon(qword_t *q, int index, int x, int y, int selected)
{
    int row;
    int col;

    if (index < 0 || index >= 6) {
        return q;
    }
    if (nexus_atlas_ready()) {
        return nexus_atlas_draw(q, NEXUS_CONTEXT, (unsigned int)index,
                                x - (NEXUS_WIDTH / 2),
                                y - (screen_height / 2), 32);
    }

    for (row = 0; row < 5; ++row) {
        for (col = 0; col < 5; ++col) {
            if ((nexus_icon_patterns[index][row] & (1U << (4 - col))) != 0) {
                if (selected) {
                    q = fill_rect(q, x + col * 5, y + row * 5, 4, 4, C_ACCENT);
                } else {
                    q = fill_rect(q, x + col * 5, y + row * 5, 4, 4, C_MUTED);
                }
            }
        }
    }
    return q;
}

static qword_t *draw_home_card(qword_t *q, int index, int selected,
                               const char *title, const char *line1,
                               const char *line2)
{
    static const int x_positions[3] = {18, 223, 428};
    static const int y_positions[2] = {166, 282};
    int x = x_positions[index % 3];
    int y = y_positions[index / 3];
    int w = 194;
    int h = 100;

    if (selected) {
        q = fill_rect(q, x, y, w, h, 18, 80, 101);
        q = fill_rect(q, x, y, w, 3, 60, 218, 235);
        q = fill_rect(q, x, y + 3, 4, h - 3, 60, 218, 235);
        q = print_text(q, title, x + 15, y + 16, 232, 243, 249);
        q = print_text(q, "SELECTED", x + 130, y + 16, C_WARN);
    } else {
        q = fill_rect(q, x, y, w, h, C_PANEL);
        q = fill_rect(q, x, y, w, 3, C_PANEL_2);
        q = fill_rect(q, x, y + 3, 4, h - 3, C_PANEL_2);
        q = print_text(q, title, x + 15, y + 16, C_ACCENT);
    }
    q = print_text(q, line1, x + 15, y + 48, C_TEXT);
    q = print_text(q, line2, x + 15, y + 68, C_MUTED);
    q = draw_icon(q, index, x + 156, y + 45, selected);
    return q;
}

static qword_t *render_home(qword_t *q, const NexusNavState *state,
                            int input_ready, int connected)
{
    static const char *const labels[6] = {
        "GAMES", "PS1 LIBRARY", "HOMEBREW",
        "NEXUS STORE", "FILES", "SYSTEM"
    };
    static const char *const line1[6] = {
        "HDD / MMCE / USB", "POPS COLLECTION", "LOCAL ELF LIBRARY",
        "ONLINE CATALOG", "STORAGE BROWSER", "CONSOLE SETTINGS"
    };
    static const char *const line2[6] = {
        "Grid - Coverflow - List", "Artworks and metadata",
        "Apps and utilities", "Packages and updates",
        "Browse and manage files", "Devices and information"
    };

    unsigned int i;

    q = fill_rect(q, 0, 0, NEXUS_WIDTH, screen_height, C_BG);
    if (nexus_atlas_ready()) {
        q = nexus_atlas_bind(q, NEXUS_CONTEXT);
    }
    q = draw_topbar(q, "DASHBOARD");
    q = print_text(q, "YOUR CONSOLE. YOUR LIBRARY.", 20, 64, C_TEXT);
    q = print_text(q, "A new home for the PlayStation 2", 20, 86, C_MUTED);

    q = draw_status_chip(q, 0, "DISC");
    q = draw_status_chip(q, 1, "MC0");
    q = draw_status_chip(q, 2, "MC1");
    q = draw_status_chip(q, 3, "HDD");
    q = draw_status_chip(q, 4, "MMCE");
    q = draw_status_chip(q, 5, "USB");

    q = print_text(q, "EXPLORE NEXUS", 20, 150, C_ACCENT);
    for (i = 0; i < 6; ++i) {
        q = draw_home_card(q, (int)i, state->selected == i,
                           labels[i], line1[i], line2[i]);
    }

    q = fill_rect(q, 18, screen_height - 45, 604, 40, C_HEADER);
    q = print_text(q, "UP / DOWN  SELECT", 28, screen_height - 39, C_TEXT);
    q = print_text(q, "CROSS  OPEN", 190, screen_height - 39, C_ACCENT);
    q = print_text(q, "L1 / R1  MOVE", 315, screen_height - 36, C_TEXT);
    q = print_text(q, "START  HOME", 470, screen_height - 39, C_MUTED);

    if (input_ready) {
        q = print_text(q, "PAD READY", 20, screen_height - 20, C_ACCENT);
    } else {
        q = print_text(q, "PAD OFFLINE", 20, screen_height - 20, C_WARN);
    }
    q = print_text(q, connected ? "CONTROLLER CONNECTED" : "WAITING FOR CONTROLLER",
                   122, screen_height - 20, C_MUTED);
    return q;
}

static qword_t *render_page(qword_t *q, const NexusNavState *state,
                            int input_ready, int connected)
{
    const char *title = nexus_page_name(state->page);
    const char *eyebrow = "NEXUS MODULE";
    const char *body1 = "This module is part of the NEXUS shell.";
    const char *body2 = "The native provider is not connected yet.";
    const char *body3 = "No storage scan or device modification has been run.";
    const char *tag1 = "STATUS";
    const char *tag2 = "NOT CONNECTED";
    const char *tag3 = "NEXT STEP";
    const char *next = "Provider integration";

    switch (state->page) {
        case NEXUS_PAGE_GAMES:
            body1 = "Browse titles discovered from supported storage sources.";
            body2 = "Planned sources: internal HDD, MMCE and USB.";
            body3 = "Views: Grid, Carousel, Coverflow and List.";
            tag2 = "LIBRARY PENDING";
            next = "Read-only game discovery";
            break;
        case NEXUS_PAGE_PS1:
            body1 = "A dedicated collection for PlayStation 1 titles.";
            body2 = "POPS discovery and title metadata are not wired yet.";
            body3 = "Existing POPS installations will not be modified.";
            tag2 = "POPS PENDING";
            next = "POPS library provider";
            break;
        case NEXUS_PAGE_HOMEBREW:
            body1 = "Keep launchers, utilities and emulators in one place.";
            body2 = "Local ELF discovery and categories are not wired yet.";
            body3 = "Existing homebrew installations are left untouched.";
            tag2 = "DISCOVERY PENDING";
            next = "Local ELF index";
            break;
        case NEXUS_PAGE_STORE:
            body1 = "The host Companion has signed-catalog and ZIP staging support.";
            body2 = "The native PS2 network client is not implemented yet.";
            body3 = "No package can be installed from this screen.";
            tag2 = "HOST PIPELINE ONLY";
            next = "Native network provider";
            break;
        case NEXUS_PAGE_FILES:
            body1 = "Browse detected storage without hiding its source.";
            body2 = "No filesystem is mounted or modified by this bootstrap.";
            body3 = "A read-only volume provider is the next requirement.";
            tag2 = "NO VOLUMES";
            next = "Read-only volume discovery";
            break;
        case NEXUS_PAGE_SYSTEM:
            body1 = input_ready ? "DualShock input backend initialized." : "DualShock input backend unavailable.";
            body2 = connected ? "Controller detected on port 0." : "Waiting for a controller on port 0.";
            body3 = "Device slots remain NOT PROBED until real probes exist.";
            tag2 = connected ? "CONTROLLER READY" : "CONTROLLER WAITING";
            next = "Device provider registry";
            break;
        default:
            break;
    }

    q = fill_rect(q, 0, 0, NEXUS_WIDTH, screen_height, C_BG);
    q = draw_topbar(q, title);
    q = print_text(q, eyebrow, 20, 73, C_ACCENT);
    q = print_text(q, title, 20, 101, C_TEXT);
    q = fill_rect(q, 18, 142, 604, 196, C_PANEL);
    q = fill_rect(q, 18, 142, 4, 196, C_ACCENT);

    q = print_text(q, body1, 42, 170, C_TEXT);
    q = print_text(q, body2, 42, 204, C_MUTED);
    q = print_text(q, body3, 42, 238, C_MUTED);

    q = fill_rect(q, 42, 282, 170, 37, C_HEADER);
    q = fill_rect(q, 224, 282, 374, 37, C_HEADER);
    q = print_text(q, tag1, 54, 291, C_MUTED);
    q = print_text(q, tag2, 236, 291, C_WARN);

    q = print_text(q, tag3, 20, 350, C_ACCENT);
    q = print_text(q, next, 20, 373, C_TEXT);

    q = fill_rect(q, 18, screen_height - 42, 604, 26, C_HEADER);
    q = print_text(q, "L1 / R1  PREVIOUS / NEXT", 28, screen_height - 36, C_TEXT);
    q = print_text(q, "CIRCLE  BACK", 280, screen_height - 36, C_ACCENT);
    q = print_text(q, "START  HOME", 442, screen_height - 36, C_MUTED);
    return q;
}

int nexus_renderer_init(void)
{
    qword_t *q;
    zbuffer_t zbuffer;
    int i;

    renderer_is_ready = 0;
    screen_height = (graph_get_region() == GRAPH_MODE_PAL)
                        ? NEXUS_PAL_HEIGHT : NEXUS_NTSC_HEIGHT;

    graph_vram_clear();
    for (i = 0; i < 2; ++i) {
        frames[i].width = NEXUS_WIDTH;
        frames[i].height = (unsigned int)screen_height;
        frames[i].mask = 0;
        frames[i].psm = GS_PSM_32;
        frames[i].address = (unsigned int)graph_vram_allocate(
            NEXUS_WIDTH, screen_height, GS_PSM_32, GRAPH_ALIGN_PAGE);
        if (frames[i].address == (unsigned int)-1) {
            return -1;
        }
    }

    if (dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0) < 0) {
        return -2;
    }
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    if (graph_initialize((int)frames[0].address, NEXUS_WIDTH,
                         screen_height, GS_PSM_32, 0, 0) < 0) {
        return -3;
    }

    if (fontx_load("rom0:KROM", &system_font, SINGLE_BYTE, 1, 1, 0) != 0) {
        return -4;
    }

    render_packet = packet_init(NEXUS_PACKET_QWORDS, PACKET_NORMAL);
    if (render_packet == 0) {
        fontx_unload(&system_font);
        return -5;
    }

    zbuffer.enable = DRAW_DISABLE;
    zbuffer.mask = 1;
    zbuffer.method = ZTEST_METHOD_GREATER;
    zbuffer.address = 0;
    zbuffer.zsm = GS_ZBUF_32;

    q = render_packet->data;
    q = draw_setup_environment(q, NEXUS_CONTEXT, &frames[0], &zbuffer);
    q = draw_primitive_xyoffset(q, NEXUS_CONTEXT,
                                (float)(2048 - (NEXUS_WIDTH / 2)),
                                (float)(2048 - (screen_height / 2)));
    q = draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF, render_packet->data,
                            (int)(q - render_packet->data), 0, 0);
    dma_wait_fast();
    graph_set_framebuffer_filtered((int)frames[0].address,
                                   NEXUS_WIDTH, GS_PSM_32, 0, 0);

    front_index = 0;
    renderer_is_ready = 1;
    return 0;
}

int nexus_renderer_ready(void)
{
    return renderer_is_ready;
}

int nexus_renderer_render(const NexusNavState *state, int input_ready, int connected)
{
    qword_t *q;
    int back_index;

    if (!renderer_is_ready || state == 0 || render_packet == 0) {
        return -1;
    }

    back_index = front_index ^ 1;
    q = render_packet->data;
    q = draw_framebuffer(q, NEXUS_CONTEXT, &frames[back_index]);
    q = draw_clear(q, NEXUS_CONTEXT,
                   (float)(2048 - (NEXUS_WIDTH / 2)),
                   (float)(2048 - (screen_height / 2)),
                   (float)NEXUS_WIDTH, (float)screen_height,
                   C_BG);

    if (!state->in_section || state->page == NEXUS_PAGE_HOME) {
        q = render_home(q, state, input_ready, connected);
    } else {
        q = render_page(q, state, input_ready, connected);
    }

    q = draw_finish(q);
    dma_wait_fast();
    dma_channel_send_normal(DMA_CHANNEL_GIF, render_packet->data,
                            (int)(q - render_packet->data), 0, 0);
    draw_wait_finish();
    graph_wait_vsync();
    graph_set_framebuffer_filtered((int)frames[back_index].address,
                                   NEXUS_WIDTH, GS_PSM_32, 0, 0);
    front_index = back_index;
    return 0;
}
