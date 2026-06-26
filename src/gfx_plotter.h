#ifndef GFX_PLOTTER_H
#define GFX_PLOTTER_H

#include "hardware.h"

typedef struct GfxPlotter GfxPlotter;

struct GfxPlotter {
    const char *name;
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u16 min;
    u16 max;
    u16 color_bg;
    u16 color_value;
    u16 cursor;
    u16 last_x;
    u16 last_y;
    u8 has_last;
    u8 title_h;
    u8 palette_swap;
    void (*push)(GfxPlotter *plotter, u16 value);
};

GfxPlotter gfx_plotter_create(const char *name,
        u16 x, u16 y, u16 w, u16 h,
        u16 min, u16 max,
        u16 color_bg, u16 color_value);
void gfx_plotter_push(GfxPlotter *plotter, u16 value);

#endif /* GFX_PLOTTER_H */
