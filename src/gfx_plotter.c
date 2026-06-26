#include <stddef.h>
#include "display_hw.h"
#include "gfx_plotter.h"

static u16 gfx_plotter_data_x(const GfxPlotter *plotter)
{
    return (u16)(plotter->x + 1u);
}

static u16 gfx_plotter_data_y(const GfxPlotter *plotter)
{
    return (u16)(plotter->y + 1u + (u16)plotter->title_h);
}

static u16 gfx_plotter_data_w(const GfxPlotter *plotter)
{
    if (plotter->w <= 2u) {
        return 0u;
    }

    return (u16)(plotter->w - 2u);
}

static u16 gfx_plotter_data_h(const GfxPlotter *plotter)
{
    u16 used_h;

    used_h = (u16)(2u + (u16)plotter->title_h);
    if (plotter->h <= used_h) {
        return 0u;
    }

    return (u16)(plotter->h - used_h);
}

static u16 gfx_plotter_active_bg(const GfxPlotter *plotter)
{
    return plotter->palette_swap ? plotter->color_value : plotter->color_bg;
}

static u16 gfx_plotter_active_value(const GfxPlotter *plotter)
{
    return plotter->palette_swap ? plotter->color_bg : plotter->color_value;
}

static u16 gfx_plotter_map_y(const GfxPlotter *plotter, u16 value)
{
    u16 data_y;
    u16 data_h;
    u16 range;
    u16 clamped;
    u32 scaled;

    data_y = gfx_plotter_data_y(plotter);
    data_h = gfx_plotter_data_h(plotter);

    if (data_h <= 1u) {
        return data_y;
    }

    if (value < plotter->min) {
        clamped = plotter->min;
    } else if (value > plotter->max) {
        clamped = plotter->max;
    } else {
        clamped = value;
    }

    if (plotter->max <= plotter->min) {
        range = 1u;
    } else {
        range = (u16)(plotter->max - plotter->min);
    }

    scaled = ((u32)(clamped - plotter->min) * (u32)(data_h - 1u)) / (u32)range;
    return (u16)(data_y + data_h - 1u - (u16)scaled);
}

static void gfx_plotter_draw_name(const GfxPlotter *plotter)
{
    const char *text;
    u16 x;
    u16 y;
    u16 right;

    if ((plotter->name == NULL) || (plotter->title_h == 0u) ||
            (plotter->w <= 4u)) {
        return;
    }

    text = plotter->name;
    x = (u16)(plotter->x + 2u);
    y = (u16)(plotter->y + 1u);
    right = (u16)(plotter->x + plotter->w - 2u);

    while ((*text != '\0') && ((u16)(x + Font_7x10.width) <= right)) {
        gfx_draw_char(x, y, *text, Font_7x10,
                plotter->color_value, plotter->color_bg);
        x = (u16)(x + Font_7x10.width);
        text++;
    }
}

static void gfx_plotter_draw_frame(const GfxPlotter *plotter)
{
    if ((plotter->w == 0u) || (plotter->h == 0u)) {
        return;
    }

    gfx_draw_filled_rectangle(plotter->x, plotter->y,
            plotter->w, plotter->h, plotter->color_bg);

    if ((plotter->w > 1u) && (plotter->h > 1u)) {
        gfx_draw_rectangle(plotter->x, plotter->y,
                (u16)(plotter->x + plotter->w - 1u),
                (u16)(plotter->y + plotter->h - 1u),
                plotter->color_value);
    }

    gfx_plotter_draw_name(plotter);
}

GfxPlotter gfx_plotter_create(const char *name,
        u16 x, u16 y, u16 w, u16 h,
        u16 min, u16 max,
        u16 color_bg, u16 color_value)
{
    GfxPlotter plotter;

    plotter.name = name;
    plotter.x = x;
    plotter.y = y;
    plotter.w = w;
    plotter.h = h;
    plotter.min = min;
    plotter.max = max;
    plotter.color_bg = color_bg;
    plotter.color_value = color_value;
    plotter.cursor = 0u;
    plotter.last_x = 0u;
    plotter.last_y = 0u;
    plotter.has_last = 0u;
    plotter.title_h = 0u;
    plotter.palette_swap = 0u;
    plotter.push = gfx_plotter_push;

    if ((name != NULL) && (name[0] != '\0') &&
            (h > (u16)(Font_7x10.height + 4u))) {
        plotter.title_h = (u8)(Font_7x10.height + 2u);
    }

    gfx_plotter_draw_frame(&plotter);
    return plotter;
}

void gfx_plotter_push(GfxPlotter *plotter, u16 value)
{
    u16 data_x;
    u16 data_y;
    u16 data_w;
    u16 data_h;
    u16 x;
    u16 y;
    u16 active_bg;
    u16 active_value;

    if (plotter == NULL) {
        return;
    }

    data_x = gfx_plotter_data_x(plotter);
    data_y = gfx_plotter_data_y(plotter);
    data_w = gfx_plotter_data_w(plotter);
    data_h = gfx_plotter_data_h(plotter);

    if ((data_w == 0u) || (data_h == 0u)) {
        return;
    }

    if (plotter->cursor >= data_w) {
        plotter->cursor = 0u;
        plotter->has_last = 0u;
        plotter->palette_swap = plotter->palette_swap ? 0u : 1u;
    }

    x = (u16)(data_x + plotter->cursor);
    y = gfx_plotter_map_y(plotter, value);
    active_bg = gfx_plotter_active_bg(plotter);
    active_value = gfx_plotter_active_value(plotter);

    gfx_draw_filled_rectangle(x, data_y, 1u, data_h, active_bg);

    if ((plotter->has_last != 0u) && (plotter->cursor != 0u)) {
        gfx_draw_line(plotter->last_x, plotter->last_y, x, y,
                active_value);
    } else {
        st_draw_pixel(x, y, active_value);
    }

    plotter->last_x = x;
    plotter->last_y = y;
    plotter->has_last = 1u;
    plotter->cursor++;
}
