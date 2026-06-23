#ifndef FONTS_H
#define FONTS_H

#include "hardware.h"

typedef struct {
    const u8 width;
    u8 height;
    const u16 *data;
} FontDef;

extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

#endif /* FONTS_H */
