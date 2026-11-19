#include <tamtypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <packet.h>
#include <dma.h>
#include <graph.h>
#include <gs_psm.h>
#include <draw.h>
#include <font.h>

#include "nexus_renderer.h"
#include "nexus_atlas.h"
#include "nexus_cover_cache.h"
#include "nexus_library.h"
#include "nexus_asset_blob.h"
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
#define C_SOFT     10, 25, 39
#define C_LINE     28, 64, 82
#define C_GREEN    91, 210, 153
#define C_AMBER    238, 181, 98

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
    q = fill_rect(q, 0, 0, NEXUS_WIDTH, 55, C_HEADER);
    q = fill_rect(q, 0, 54, NEXUS_WIDTH, 2, C_ACCENT);
    q = fill_rect(q, 18, 13, 28, 28, C_SELECTED);
    q = fill_rect(q, 22, 17, 3, 20, C_ACCENT);
    q = fill_rect(q, 27, 17, 3, 20, C_ACCENT);
    q = fill_rect(q, 32, 17, 3, 20, C_ACCENT);
    q = print_text(q, "SPDW FACTORY", 56, 13, C_ACCENT);
    q = print_text(q, "/  NEXUS", 56, 30, C_TEXT);
    q = fill_rect(q, 428, 14, 1, 26, C_LINE);
    q = print_text(q, "PLAYSTATION 2", 444, 13, C_MUTED);
    if (section != 0)
        q = print_text(q, section, 444, 30, C_TEXT);
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
        q = nexus_atlas_bind(q, NEXUS_CONTEXT);
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

static qword_t *draw_nav_item(qword_t *q, int index, int selected,
                              const char *title, const char *subtitle,
                              int x, int y, int width, int height)
{
    if (selected) {
        q = fill_rect(q, x, y, width, height, C_SELECTED);
        q = fill_rect(q, x, y, 3, height, C_ACCENT);
        q = fill_rect(q, x + 3, y, width - 3, 1, C_LINE);
    } else {
        q = fill_rect(q, x, y, width, height, C_SOFT);
        q = fill_rect(q, x, y + height - 1, width, 1, C_LINE);
    }
    q = draw_icon(q, index, x + 10, y + (height - 28) / 2, selected);
    if (selected) {
        q = print_text(q, title, x + 48, y + 10, C_TEXT);
        q = print_text(q, subtitle, x + 48, y + 27, C_ACCENT);
    } else {
        q = print_text(q, title, x + 48, y + 10, C_MUTED);
        q = print_text(q, subtitle, x + 48, y + 27, C_MUTED);
    }
    return q;
}

static qword_t *draw_large_icon(qword_t *q, int index, int x, int y, int size)
{
    int row, col;
    if (index < 0 || index >= 6)
        return q;
    if (nexus_atlas_ready()) {
        q = nexus_atlas_bind(q, NEXUS_CONTEXT);
        return nexus_atlas_draw(q, NEXUS_CONTEXT, (unsigned int)index,
                                x - (NEXUS_WIDTH / 2),
                                y - (screen_height / 2), (unsigned int)size);
    }
    for (row = 0; row < 5; ++row) {
        for (col = 0; col < 5; ++col) {
            if ((nexus_icon_patterns[index][row] & (1U << (4 - col))) != 0) {
                int px = size / 5;
                q = fill_rect(q, x + col * px, y + row * px,
                              px - 2, px - 2, C_ACCENT);
            }
        }
    }
    return q;
}

static qword_t *draw_metric_card(qword_t *q, int x, int y, int width,
                                 const char *label, const char *value,
                                 int ready)
{
    q = fill_rect(q, x, y, width, 1, C_LINE);
    q = fill_rect(q, x, y + 2, width, 70, C_SOFT);
    q = print_text(q, label, x + 10, y + 12, C_MUTED);
    if (ready) {
        q = fill_rect(q, x, y + 1, width, 1, C_ACCENT);
        q = print_text(q, value, x + 10, y + 34, C_TEXT);
    } else {
        q = fill_rect(q, x, y + 1, width, 1, C_AMBER);
        q = print_text(q, value, x + 10, y + 34, C_AMBER);
    }
    return q;
}

static qword_t *render_home(qword_t *q, const NexusNavState *state,
                            int input_ready, int connected,
                            const NexusActionStatus *actions)
{
    static const char *const labels[6] = {
        "GAMES", "PS1 LIBRARY", "HOMEBREW",
        "NEXUS STORE", "FILES", "SYSTEM"
    };
    static const char *const subtitles[6] = {
        "PS2 COLLECTION", "POPS COLLECTION", "ELF APPLICATIONS",
        "ONLINE CATALOG", "STORAGE BROWSER", "CONSOLE CONTROL"
    };
    static const char *const descriptions[6] = {
        "Your PlayStation 2 library, all in one place.",
        "A dedicated home for PlayStation classics.",
        "Launchers, utilities and homebrew tools.",
        "Discover applications and updates for NEXUS.",
        "Browse connected storage without changing it.",
        "Hardware, video, input and system services."
    };
    static const char *const details[6] = {
        "GRID / CAROUSEL / COVERFLOW / LIST",
        "POPS / VCD / TITLE INFORMATION",
        "LOCAL ELFS / TOOLS / EMULATORS",
        "PACKAGES / DEPENDENCIES / UPDATES",
        "USB / MMCE / INTERNAL STORAGE",
        "VIDEO MODE / CONTROLLER / FILEXIO"
    };
    unsigned int i;
    int row_height;
    int nav_y = 74;
    int nav_x = 18;
    int nav_width = 150;
    int hero_x = 180;
    int hero_y = 74;
    int hero_width = 442;
    int hero_height = screen_height == NEXUS_PAL_HEIGHT ? 246 : 202;
    int footer_y = screen_height - 35;
    int content_bottom = footer_y - 10;
    int nav_available = content_bottom - nav_y;
    int metric_y = hero_y + hero_height + 12;
    int metric_height = content_bottom - metric_y;
    int metric_width = 142;
    char count_line[48];
    char renderer_line[48];
    const char *status_line = "READY TO EXPLORE";
    int selected = state->selected < 6U ? state->selected : 0;

    row_height = (nav_available - 25) / 6;
    if (row_height > 58)
        row_height = 58;
    if (row_height < 42)
        row_height = 42;

    q = fill_rect(q, 0, 0, NEXUS_WIDTH, screen_height, C_BG);
    q = draw_topbar(q, "DASHBOARD");
    q = print_text(q, "YOUR CONSOLE", nav_x, 63, C_ACCENT);
    for (i = 0; i < 6U; ++i) {
        int y = nav_y + (int)i * (row_height + 5);
        q = draw_nav_item(q, (int)i, selected == (int)i,
                          labels[i], subtitles[i],
                          nav_x, y, nav_width, row_height);
    }

    q = fill_rect(q, hero_x, hero_y, hero_width, hero_height, C_PANEL);
    q = fill_rect(q, hero_x, hero_y, hero_width, 3, C_ACCENT);
    q = fill_rect(q, hero_x + 1, hero_y + hero_height - 2,
                  hero_width - 2, 1, C_LINE);
    q = fill_rect(q, hero_x + 284, hero_y + 20, 140,
                  hero_height - 40, C_SOFT);
    q = print_text(q, "NEXUS MODULE", hero_x + 22, hero_y + 18, C_ACCENT);
    q = print_text(q, labels[selected], hero_x + 22, hero_y + 54, C_TEXT);
    q = print_text(q, descriptions[selected], hero_x + 22, hero_y + 92, C_MUTED);
    q = print_text(q, details[selected], hero_x + 22, hero_y + 116, C_TEXT);
    q = fill_rect(q, hero_x + 22, hero_y + 148, 228, 1, C_LINE);

    if (selected == 0) {
        (void)snprintf(count_line, sizeof(count_line), "%u TITLES INDEXED",
                       nexus_library_count());
        status_line = nexus_library_count() == 0U
            ? "LIBRARY WAITING FOR ITS FIRST SCAN" : "LIBRARY READY";
    } else if (selected == 1) {
        (void)snprintf(count_line, sizeof(count_line), "POPS COLLECTION");
        status_line = "PS1 DISCOVERY READY FOR INTEGRATION";
    } else if (selected == 2) {
        (void)snprintf(count_line, sizeof(count_line), "LOCAL ELF INDEX");
        status_line = "HOMEBREW DISCOVERY IN DEVELOPMENT";
    } else if (selected == 3) {
        (void)snprintf(count_line, sizeof(count_line), "STORE COMPANION");
        status_line = "HOST PACKAGE PIPELINE AVAILABLE";
    } else if (selected == 4) {
        (void)snprintf(count_line, sizeof(count_line), "%s",
                       actions != 0 && actions->filexio_ready
                           ? "FILEXIO RPC READY" : "FILEXIO NOT ACTIVE");
        status_line = actions != 0 && actions->filexio_ready
            ? "SELECT A ROOT AND CONFIRM A READ-ONLY SCAN"
            : "ACTIVATE FILEXIO FROM SYSTEM";
    } else {
        (void)snprintf(count_line, sizeof(count_line), "%s",
                       actions != 0 && actions->filexio_ready
                           ? "FILEXIO AVAILABLE" : "FILEXIO INACTIVE");
        status_line = input_ready && connected
            ? "DUALSHOCK CONTROLLER CONNECTED"
            : "WAITING FOR CONTROLLER INPUT";
    }

    q = print_text(q, count_line, hero_x + 22, hero_y + 164, C_ACCENT);
    q = print_text(q, status_line, hero_x + 22, hero_y + 185, C_MUTED);
    q = draw_large_icon(q, selected, hero_x + 319,
                        hero_y + (hero_height - 84) / 2, 84);
    q = print_text(q, selected == 0 ? "01 / 06" :
                      selected == 1 ? "02 / 06" :
                      selected == 2 ? "03 / 06" :
                      selected == 3 ? "04 / 06" :
                      selected == 4 ? "05 / 06" : "06 / 06",
                   hero_x + 326, hero_y + hero_height - 26, C_ACCENT);

    if (metric_height > 50) {
        (void)snprintf(count_line, sizeof(count_line), "%u",
                       nexus_library_count());
        (void)snprintf(renderer_line, sizeof(renderer_line), "%s",
                       nexus_atlas_ready() ? "ATLAS READY" : "ICON FALLBACK");
        q = draw_metric_card(q, hero_x, metric_y, metric_width,
                             "GAME CATALOG", count_line,
                             nexus_library_count() > 0U);
        q = draw_metric_card(q, hero_x + 150, metric_y, metric_width,
                             "GS GRAPHICS", renderer_line,
                             nexus_atlas_ready());
        q = draw_metric_card(q, hero_x + 300, metric_y, metric_width,
                             "CONTROLLER",
                             connected ? "CONNECTED" : "WAITING",
                             input_ready && connected);
    }

    q = fill_rect(q, 18, footer_y, 604, 27, C_HEADER);
    q = fill_rect(q, 18, footer_y, 604, 1, C_LINE);
    q = print_text(q, "UP / DOWN  SELECT", 28, footer_y + 7, C_TEXT);
    q = print_text(q, "CROSS  OPEN", 190, footer_y + 7, C_ACCENT);
    q = print_text(q, "L1 / R1  MODULES", 315, footer_y + 7, C_TEXT);
    q = print_text(q, "START  HOME", 490, footer_y + 7, C_MUTED);
    return q;
}


static const char *games_view_label(unsigned int mode)
{
    static const char *const names[] = {"GRID", "CAROUSEL", "COVERFLOW", "LIST"};
    return mode < NEXUS_LIBRARY_VIEW_COUNT ? names[mode] : "GRID";
}

static const char *games_source_label(unsigned int source)
{
    switch (source) {
        case NEXUS_LIBRARY_SOURCE_HDD: return "HDD";
        case NEXUS_LIBRARY_SOURCE_MMCE: return "MMCE";
        case NEXUS_LIBRARY_SOURCE_USB: return "USB";
        default: return "ANY";
    }
}

static const char *games_platform_label(unsigned int platform)
{
    switch (platform) {
        case NEXUS_LIBRARY_PLATFORM_PS2: return "PS2";
        case NEXUS_LIBRARY_PLATFORM_PS1: return "PS1";
        default: return "PS1 + PS2";
    }
}

static qword_t *draw_game_card(qword_t *q, const NexusLibraryEntry *entry,
                               int selected, int x, int y, int w, int h)
{
    char title[28];
    char meta[32];
    int cover_h;
    int icon_size;
    int precision;

    if (entry == 0 || w < 48 || h < 60)
        return q;

    if (selected) {
        q = fill_rect(q, x, y, w, h, C_SELECTED);
        q = fill_rect(q, x, y, w, 3, C_ACCENT);
        q = fill_rect(q, x, y, 3, h, C_ACCENT);
    } else {
        q = fill_rect(q, x, y, w, h, C_PANEL);
        q = fill_rect(q, x, y, w, 1, C_LINE);
        q = fill_rect(q, x, y, 1, h, C_LINE);
    }

    cover_h = h - 38;
    if (cover_h < 30)
        cover_h = 30;
    q = fill_rect(q, x + 5, y + 5, w - 10, cover_h, C_SOFT);
    q = fill_rect(q, x + 5, y + 5, w - 10, 2, C_LINE);
    q = fill_rect(q, x + 5, y + 5, 3, cover_h, C_ACCENT);
    q = fill_rect(q, x + 12, y + cover_h - 9, w - 24, 1, C_LINE);

    if (w >= 170)
        icon_size = cover_h > 95 ? 72 : 52;
    else if (w >= 120)
        icon_size = 44;
    else
        icon_size = 30;
    if (icon_size > cover_h - 16)
        icon_size = cover_h - 16;
    if (icon_size < 18)
        icon_size = 18;

    q = draw_large_icon(q,
                        entry->platform == NEXUS_LIBRARY_PLATFORM_PS1 ? 1 : 0,
                        x + (w - icon_size) / 2,
                        y + 12 + (cover_h - icon_size) / 2,
                        icon_size);

    q = print_text(q,
                   entry->platform == NEXUS_LIBRARY_PLATFORM_PS1 ? "PS1" : "PS2",
                   x + 12, y + 9, C_TEXT);
    if (entry->favorite)
        q = print_text(q, "FAV", x + w - 38, y + 9, C_AMBER);

    q = print_text(q, "ARTWORK NOT CACHED",
                   x + 12, y + cover_h - 12, C_MUTED);

    precision = w >= 180 ? 20 : (w >= 130 ? 14 : 9);
    (void)snprintf(title, sizeof(title), "%.*s", precision, entry->title);
    (void)snprintf(meta, sizeof(meta), "%s / %s",
                   games_platform_label(entry->platform),
                   games_source_label(entry->source));

    q = fill_rect(q, x + 5, y + cover_h + 10, w - 10, h - cover_h - 15,
                  C_HEADER);
    if (selected)
        q = print_text(q, title, x + 10, y + h - 24, C_TEXT);
    else
        q = print_text(q, title, x + 10, y + h - 24, C_ACCENT);
    if (h >= 175)
        q = print_text(q, meta, x + 10, y + h - 10, C_MUTED);
    return q;
}


static qword_t *render_games_page(qword_t *q, const NexusActionStatus *actions)
{
    NexusLibraryView view;
    unsigned int count, total, selected, start, i;
    const NexusLibraryEntry *entry;
    char summary[88];
    char detail[88];
    int detail_open = actions != 0 && actions->game_detail_open;

    if (actions != 0) view = actions->library_view;
    else nexus_library_view_init(&view);
    total = nexus_library_count();
    count = nexus_library_visible_count(&view);
    if (count == 0U) view.selected_index = 0;
    else if (view.selected_index >= count) view.selected_index = 0;
    selected = view.selected_index;

    q = fill_rect(q, 0, 0, NEXUS_WIDTH, screen_height, C_BG);
    q = draw_topbar(q, "GAME LIBRARY");
    q = print_text(q, "YOUR PLAYSTATION COLLECTION", 20, 65, C_TEXT);
    (void)snprintf(summary, sizeof(summary), "%u TITLES  /  %s  /  %s  /  %s",
                   count, games_view_label(view.view_mode),
                   games_source_label(view.source_filter),
                   games_platform_label(view.platform_filter));
    q = print_text(q, summary, 20, 88, C_MUTED);

    if (detail_open) {
        entry = nexus_library_selected(&view);
        if (entry != 0) {
            q = fill_rect(q, 18, 118, 604, screen_height - 183, C_PANEL);
            q = fill_rect(q, 18, 118, 4, screen_height - 183, C_ACCENT);
            q = print_text(q, "TITLE DETAILS", 40, 136, C_ACCENT);
            q = print_text(q, entry->title, 40, 165, C_TEXT);
            (void)snprintf(detail, sizeof(detail), "%s / %s / %s",
                           games_platform_label(entry->platform),
                           games_source_label(entry->source),
                           entry->backend == NEXUS_LIBRARY_BACKEND_POPS ? "POPS" :
                           entry->backend == NEXUS_LIBRARY_BACKEND_NEUTRINO ? "NEUTRINO" :
                           entry->backend == NEXUS_LIBRARY_BACKEND_OPL ? "OPL" : "AUTO");
            q = print_text(q, detail, 40, 193, C_MUTED);
            (void)snprintf(detail, sizeof(detail), "SIZE: %llu BYTES",
                           (unsigned long long)entry->size_bytes);
            q = print_text(q, detail, 40, 220, C_TEXT);
            q = print_text(q, entry->title_id[0] ? entry->title_id : "TITLE ID: NOT READ",
                           40, 247, C_MUTED);
            (void)snprintf(detail, sizeof(detail), "%.68s", entry->path);
            q = print_text(q, detail, 40, 274, C_MUTED);
            q = print_text(q, entry->favorite ? "FAVORITE: YES" : "FAVORITE: NO",
                           40, 302, C_WARN);
            q = print_text(q, "LAUNCH HANDOFF NOT CONNECTED - NO GAME STARTED.",
                           40, 337, C_TEXT);
        }
    } else if (count == 0U) {
        q = fill_rect(q, 18, 122, 604, screen_height - 200, C_PANEL);
        q = fill_rect(q, 18, 122, 4, screen_height - 200, C_ACCENT);
        q = print_text(q, total == 0U ? "YOUR LIBRARY IS WAITING."
                                      : "NO TITLES MATCH THIS FILTER.",
                       42, 160, C_TEXT);
        q = print_text(q, total == 0U
                           ? "OPEN FILES AND CONFIRM A READ-ONLY SCAN."
                           : "CHANGE SOURCE WITH L2 OR PLATFORM WITH R2.",
                       42, 199, C_MUTED);
        q = print_text(q, "SUPPORTED: CD/DVD ISO, ZSO AND POPS VCD.",
                       42, 235, C_MUTED);
        q = print_text(q, "COVER ART AND GAME LAUNCH REMAIN FUTURE WORK.",
                       42, 271, C_WARN);
    } else if (view.view_mode == NEXUS_LIBRARY_VIEW_LIST) {
        unsigned int visible = count < 6U ? count : 6U;
        start = selected > visible / 2U ? selected - visible / 2U : 0U;
        if (start + visible > count) start = count - visible;
        for (i = 0; i < visible; ++i) {
            unsigned int slot = start + i;
            int y = 122 + (int)i * 39;
            entry = nexus_library_visible_at(&view, slot);
            if (entry == 0) continue;
            if (slot == selected) {
                q = fill_rect(q, 18, y, 604, 34, C_SELECTED);
                q = fill_rect(q, 18, y, 3, 34, C_ACCENT);
                q = print_text(q, entry->title, 30, y + 9, C_TEXT);
            } else {
                q = fill_rect(q, 18, y, 604, 34, C_PANEL);
                q = print_text(q, entry->title, 30, y + 9, C_ACCENT);
            }
            (void)snprintf(detail, sizeof(detail), "%s / %s",
                           games_platform_label(entry->platform),
                           games_source_label(entry->source));
            q = print_text(q, detail, 380, y + 9, C_MUTED);
            if (entry->favorite) q = print_text(q, "FAV", 575, y + 9, C_WARN);
        }
    } else if (view.view_mode == NEXUS_LIBRARY_VIEW_CAROUSEL) {
        if (count == 1U) {
            q = draw_game_card(q, nexus_library_visible_at(&view, selected),
                               1, 220, 140, 200, 214);
        } else {
            unsigned int left = selected == 0U ? count - 1U : selected - 1U;
            unsigned int right = selected + 1U >= count ? 0U : selected + 1U;
            q = draw_game_card(q, nexus_library_visible_at(&view, left),
                               0, 36, 154, 164, 186);
            q = draw_game_card(q, nexus_library_visible_at(&view, selected),
                               1, 220, 140, 200, 214);
            q = draw_game_card(q, nexus_library_visible_at(&view, right),
                               0, 440, 154, 164, 186);
        }
    } else if (view.view_mode == NEXUS_LIBRARY_VIEW_COVERFLOW) {
        static const int offsets[5] = {-2, -1, 0, 1, 2};
        static const int xs[5] = {20, 126, 244, 400, 520};
        static const int widths[5] = {92, 108, 152, 108, 92};
        static const int ys[5] = {178, 158, 137, 158, 178};
        static const int heights[5] = {140, 176, 214, 176, 140};
        if (count == 1U) {
            q = draw_game_card(q, nexus_library_visible_at(&view, selected),
                               1, 244, 137, 152, 214);
        } else {
            for (i = 0; i < 5U; ++i) {
                int target = (int)selected + offsets[i];
                while (target < 0) target += (int)count;
                target %= (int)count;
                if (count == 2U && (i == 0U || i == 4U)) continue;
                q = draw_game_card(q,
                    nexus_library_visible_at(&view, (unsigned int)target),
                    offsets[i] == 0, xs[i], ys[i], widths[i], heights[i]);
            }
        }
    } else {
        unsigned int visible = count < 6U ? count : 6U;
        int card_height = (screen_height - 182) / 2;
        start = selected > visible / 2U ? selected - visible / 2U : 0U;
        if (start + visible > count) start = count - visible;
        for (i = 0; i < visible; ++i) {
            unsigned int slot = start + i;
            int x = 18 + (int)(i % 3U) * 205;
            int y = 118 + (int)(i / 3U) * (card_height + 10);
            q = draw_game_card(q, nexus_library_visible_at(&view, slot),
                               slot == selected, x, y, 194, card_height);
        }
    }

    q = fill_rect(q, 18, screen_height - 48, 604, 44, C_HEADER);
    if (detail_open) {
        q = print_text(q, "CIRCLE BACK TO LIBRARY", 28, screen_height - 39, C_ACCENT);
        q = print_text(q, "START HOME", 470, screen_height - 39, C_MUTED);
    } else {
        q = print_text(q, "UP/DOWN SELECT  CROSS DETAILS  TRIANGLE VIEW",
                       28, screen_height - 39, C_TEXT);
        q = print_text(q, "SQUARE FAV  L2 SOURCE  R2 PLATFORM  L1/R1 MODULES",
                       28, screen_height - 20, C_ACCENT);
    }
    return q;
}

static qword_t *print_text_fit(qword_t *q, const char *value,
                               int x, int y, int max_width,
                               u8 r, u8 g, u8 b)
{
    char buffer[96];
    size_t max_chars = max_width > 0 ? (size_t)(max_width / 9) : 0U;
    size_t length;

    if (max_chars > sizeof(buffer) - 1U)
        max_chars = sizeof(buffer) - 1U;
    if (value == 0)
        value = "";
    length = strlen(value);
    if (length <= max_chars)
        return print_text(q, value, x, y, r, g, b);
    if (max_chars < 4U)
        max_chars = 4U;
    if (max_chars > sizeof(buffer) - 1U)
        max_chars = sizeof(buffer) - 1U;
    memcpy(buffer, value, max_chars - 3U);
    buffer[max_chars - 3U] = '.';
    buffer[max_chars - 2U] = '.';
    buffer[max_chars - 1U] = '.';
    buffer[max_chars] = '\0';
    return print_text(q, buffer, x, y, r, g, b);
}

static qword_t *draw_info_card(qword_t *q, int x, int y, int width,
                               const char *label, const char *value,
                               u8 value_r, u8 value_g, u8 value_b)
{
    q = fill_rect(q, x, y, width, 74, C_SOFT);
    q = fill_rect(q, x, y, 3, 74, C_ACCENT);
    q = print_text(q, label, x + 12, y + 10, C_MUTED);
    q = print_text_fit(q, value, x + 12, y + 35, width - 24,
                       value_r, value_g, value_b);
    return q;
}

static qword_t *render_page(qword_t *q, const NexusNavState *state,
                            int input_ready, int connected,
                            const NexusActionStatus *actions)
{
    const char *title = nexus_page_name(state->page);
    const char *body1 = "This module is part of the NEXUS shell.";
    const char *body2 = "The native provider is not connected yet.";
    const char *body3 = "No storage scan or device modification has been run.";
    const char *tag2 = "NOT CONNECTED";
    const char *next = "Provider integration";
    char scan_summary[64];
    char scan_detail[64];
    char session[48];
    int icon_index = (int)state->page - 1;
    int main_height = screen_height - 184;
    int footer_y = screen_height - 35;

    scan_summary[0] = '\0';
    scan_detail[0] = '\0';

    switch (state->page) {
        case NEXUS_PAGE_GAMES:
            body1 = nexus_library_count() == 0U
                ? "NATIVE CATALOG READY - NO TITLES INDEXED YET."
                : "INDEXED GAME ENTRIES ARE AVAILABLE.";
            body2 = "Scanner core: CD/DVD ISO/ZSO + POPS VCD (read-only).";
            body3 = "Waiting for an explicitly configured mounted-volume adapter.";
            tag2 = nexus_library_count() == 0U ? "INDEX EMPTY" : "CATALOG READY";
            next = "Open the Games library";
            break;
        case NEXUS_PAGE_PS1:
            body1 = "A dedicated collection for PlayStation 1 titles.";
            body2 = "POPS discovery and title metadata are not wired yet.";
            body3 = "Existing POPS installations will not be modified.";
            tag2 = "POPS PENDING";
            next = "Connect the POPS library provider";
            break;
        case NEXUS_PAGE_HOMEBREW:
            body1 = "Keep launchers, utilities and emulators in one place.";
            body2 = "Local ELF discovery and categories are not wired yet.";
            body3 = "Existing homebrew installations are left untouched.";
            tag2 = "DISCOVERY PENDING";
            next = "Build the local ELF index";
            break;
        case NEXUS_PAGE_STORE:
            body1 = "The host Companion has signed-catalog and ZIP staging support.";
            body2 = "The native PS2 network client is not implemented yet.";
            body3 = "No package can be installed from this screen.";
            tag2 = "HOST PIPELINE ONLY";
            next = "Connect the native network provider";
            break;
        case NEXUS_PAGE_FILES:
            if (actions != 0 && actions->scan_confirm_pending) {
                (void)snprintf(scan_summary, sizeof(scan_summary),
                               "SCAN ROOT: %.36s",
                               nexus_actions_scan_root_path(actions));
                body1 = scan_summary;
                body2 = "Read-only Dopen/Dread; no mount or writes.";
                body3 = "CROSS confirms; CIRCLE cancels.";
                tag2 = "CONFIRM REQUIRED";
                next = "Only direct CD/DVD/POPS folders are checked.";
            } else if (actions != 0 && actions->scan_attempted) {
                (void)snprintf(scan_summary, sizeof(scan_summary),
                               "SCAN RESULT: %d", actions->last_scan_result);
                (void)snprintf(scan_detail, sizeof(scan_detail),
                               "%u indexed / %u seen / %u rejected",
                               (unsigned int)actions->last_scan_report.indexed,
                               (unsigned int)actions->last_scan_report.entries_seen,
                               (unsigned int)actions->last_scan_report.rejected);
                body1 = scan_summary;
                body2 = nexus_actions_scan_root_path(actions);
                body3 = scan_detail;
                tag2 = actions->last_scan_result == 0
                    ? "SCAN COMPLETE" : "SCAN ERROR";
                next = actions->last_scan_result == 0
                    ? "Read-only scan finished; no title launched."
                    : "Check that the selected root is already accessible.";
            } else if (actions != 0 && actions->filexio_ready) {
                (void)snprintf(scan_summary, sizeof(scan_summary),
                               "ROOT: %.40s",
                               nexus_actions_scan_root_label(actions));
                body1 = "FILEXIO READY - ROOT NOT SCANNED.";
                body2 = scan_summary;
                body3 = "UP/DOWN select root; TRIANGLE requests scan.";
                tag2 = "READY - NOT SCANNED";
                next = "Choose a root and request a scan";
            } else {
                body1 = "FILEXIO IS NOT ACTIVE.";
                body2 = "System: Triangle warning, Cross confirmation.";
                body3 = "No device driver or volume is mounted here.";
                tag2 = "ACTIVATE IN SYSTEM";
                next = "Activate FileXio from System";
            }
            break;
        case NEXUS_PAGE_SYSTEM:
            if (actions != 0 && actions->filexio_confirm_pending) {
                body1 = "ACTIVATE FILEXIO / IOMANX SERVICES?";
                body2 = "Loads IOP modules and may apply SBV hooks.";
                body3 = "No reset or mount. CROSS confirms; CIRCLE cancels.";
                tag2 = "CONFIRM REQUIRED";
                next = "No device scan or filesystem write will run.";
            } else if (actions != 0 && actions->filexio_ready) {
                body1 = "FILEXIO RPC READY - NO VOLUMES MOUNTED.";
                body2 = "Only existing registered roots can be scanned.";
                body3 = "USB/MMCE drivers are not started by this action.";
                tag2 = "FILEXIO READY";
                next = "Open Files to select an accessible root";
            } else if (actions != 0 && actions->filexio_attempted) {
                body1 = actions->last_filexio_result == 0
                    ? "FILEXIO ACTIVATION COMPLETED."
                    : "FILEXIO ACTIVATION FAILED.";
                body2 = "No mount, scan, format or storage write was run.";
                body3 = "Review the module/IOP environment before retrying.";
                tag2 = actions->last_filexio_result == 0 ? "READY" : "ERROR";
                next = actions->last_filexio_result == 0
                    ? "FileXio is ready for a selected root"
                    : "Triangle retries after another confirmation";
            } else {
                body1 = input_ready ? "DUALSHOCK INPUT BACKEND INITIALIZED."
                                    : "DUALSHOCK INPUT BACKEND UNAVAILABLE.";
                body2 = screen_height == NEXUS_PAL_HEIGHT
                    ? (nexus_atlas_ready() ? "GS VIDEO: PAL 640x512 / ATLAS READY."
                                           : "GS VIDEO: PAL 640x512 / ICON FALLBACK.")
                    : (nexus_atlas_ready() ? "GS VIDEO: NTSC 640x448 / ATLAS READY."
                                           : "GS VIDEO: NTSC 640x448 / ICON FALLBACK.");
                body3 = nexus_cover_cache_ready()
                    ? "FOUR 128x128 COVER SLOTS RESERVED."
                    : "ARTWORK CACHE UNAVAILABLE.";
                tag2 = nexus_atlas_ready() ? "ATLAS READY" : "ICON FALLBACK";
                next = connected ? "Triangle: explicit FileXio activation"
                                 : "Connect a controller to continue";
            }
            break;
        default:
            break;
    }

    q = fill_rect(q, 0, 0, NEXUS_WIDTH, screen_height, C_BG);
    q = draw_topbar(q, title);
    q = print_text(q, "NEXUS MODULE", 20, 67, C_ACCENT);
    q = print_text(q, title, 20, 86, C_TEXT);

    q = fill_rect(q, 18, 116, 374, main_height, C_PANEL);
    q = fill_rect(q, 18, 116, 374, 3, C_ACCENT);
    q = fill_rect(q, 19, 119, 1, main_height - 3, C_LINE);
    q = print_text(q, "OVERVIEW", 38, 136, C_ACCENT);
    q = draw_large_icon(q, icon_index, 302, 138, 66);
    q = print_text_fit(q, body1, 38, 184, 330, C_TEXT);
    q = print_text_fit(q, body2, 38, 220, 330, C_MUTED);
    q = print_text_fit(q, body3, 38, 256, 330, C_MUTED);
    q = fill_rect(q, 38, 292, 330, 1, C_LINE);
    q = print_text(q, "NEXT ACTION", 38, 311, C_MUTED);
    q = print_text_fit(q, next, 38, 336, 330, C_TEXT);

    q = draw_info_card(q, 406, 116, 216, "CURRENT STATUS",
                       tag2,
                       state->page == NEXUS_PAGE_FILES &&
                       actions != 0 && actions->scan_attempted &&
                       actions->last_scan_result != 0
                           ? 238 : 60,
                       state->page == NEXUS_PAGE_FILES &&
                       actions != 0 && actions->scan_attempted &&
                       actions->last_scan_result != 0
                           ? 181 : 218,
                       state->page == NEXUS_PAGE_FILES &&
                       actions != 0 && actions->scan_attempted &&
                       actions->last_scan_result != 0
                           ? 98 : 235);
    q = draw_info_card(q, 406, 198, 216, "SESSION INPUT",
                       connected ? "CONTROLLER CONNECTED" : "CONTROLLER WAITING",
                       input_ready && connected ? 91 : 238,
                       input_ready && connected ? 210 : 181,
                       input_ready && connected ? 153 : 98);
    (void)snprintf(session, sizeof(session), "%s / %s",
                   screen_height == NEXUS_PAL_HEIGHT ? "PAL" : "NTSC",
                   nexus_cover_cache_ready() ? "GS CACHE READY" : "GS CACHE LIMITED");
    q = draw_info_card(q, 406, 280, 216, "VIDEO / GRAPHICS",
                       session, 232, 243, 249);

    q = fill_rect(q, 18, footer_y, 604, 27, C_HEADER);
    q = fill_rect(q, 18, footer_y, 604, 1, C_LINE);
    if (state->page == NEXUS_PAGE_FILES && state->in_section &&
        actions != 0 && actions->scan_confirm_pending) {
        q = print_text(q, "CROSS  SCAN", 28, footer_y + 7, C_ACCENT);
        q = print_text(q, "CIRCLE  CANCEL", 220, footer_y + 7, C_AMBER);
        q = print_text(q, "START  HOME", 490, footer_y + 7, C_MUTED);
    } else if (state->page == NEXUS_PAGE_FILES && state->in_section &&
               actions != 0 && actions->filexio_ready) {
        q = print_text(q, "UP/DOWN  ROOT", 28, footer_y + 7, C_TEXT);
        q = print_text(q, "TRIANGLE  SCAN", 250, footer_y + 7, C_ACCENT);
        q = print_text(q, "CIRCLE  BACK", 490, footer_y + 7, C_MUTED);
    } else if (state->page == NEXUS_PAGE_FILES && state->in_section) {
        q = print_text(q, "GO TO SYSTEM FIRST", 28, footer_y + 7, C_AMBER);
        q = print_text(q, "CIRCLE  BACK", 490, footer_y + 7, C_MUTED);
    } else if (state->page == NEXUS_PAGE_SYSTEM && state->in_section &&
        actions != 0 && actions->filexio_confirm_pending) {
        q = print_text(q, "CROSS  CONFIRM", 28, footer_y + 7, C_ACCENT);
        q = print_text(q, "CIRCLE  CANCEL", 220, footer_y + 7, C_AMBER);
        q = print_text(q, "START  HOME", 490, footer_y + 7, C_MUTED);
    } else if (state->page == NEXUS_PAGE_SYSTEM && state->in_section &&
               actions != 0 && actions->filexio_ready) {
        q = print_text(q, "FILEXIO  READY", 28, footer_y + 7, C_ACCENT);
        q = print_text(q, "L1 / R1  MODULES", 250, footer_y + 7, C_TEXT);
        q = print_text(q, "CIRCLE  BACK", 490, footer_y + 7, C_MUTED);
    } else if (state->page == NEXUS_PAGE_SYSTEM && state->in_section) {
        q = print_text(q, "TRIANGLE  FILEXIO", 28, footer_y + 7, C_ACCENT);
        q = print_text(q, "L1 / R1  MODULES", 250, footer_y + 7, C_TEXT);
        q = print_text(q, "CIRCLE  BACK", 490, footer_y + 7, C_MUTED);
    } else {
        q = print_text(q, "L1 / R1  PREVIOUS / NEXT", 28, footer_y + 7, C_TEXT);
        q = print_text(q, "CIRCLE  BACK", 310, footer_y + 7, C_ACCENT);
        q = print_text(q, "START  HOME", 490, footer_y + 7, C_MUTED);
    }
    (void)tag2;
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

    if (fontx_load("rom0:KROM", &system_font, SINGLE_BYTE, 1, 1, 1) != 0) {
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

    /* The pack is linked into .rodata; failure keeps the procedural fallback. */
    (void)nexus_atlas_upload(nexus_icon_pack_start,
                             (size_t)(nexus_icon_pack_end - nexus_icon_pack_start));

    /* Fixed LRU artwork slots are optional; the dashboard works without them. */
    (void)nexus_cover_cache_init();

    front_index = 0;
    renderer_is_ready = 1;
    return 0;
}

int nexus_renderer_ready(void)
{
    return renderer_is_ready;
}

int nexus_renderer_render(const NexusNavState *state, int input_ready, int connected,
                          const NexusActionStatus *actions)
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
        q = render_home(q, state, input_ready, connected, actions);
    } else if (state->page == NEXUS_PAGE_GAMES) {
        q = render_games_page(q, actions);
    } else {
        q = render_page(q, state, input_ready, connected, actions);
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
