#ifndef TROLLFACE_IMAGE_H
#define TROLLFACE_IMAGE_H

#include "hardware.h"

#define TROLLFACE_IMAGE_WIDTH       32u
#define TROLLFACE_IMAGE_HEIGHT      32u

static const u32 TROLLFACE_IMAGE_BITS[TROLLFACE_IMAGE_HEIGHT] = {
    0xffffffffu, 0xffffffffu, 0xff0001ffu, 0xfefffc3fu,
    0xfdffff9fu, 0xfdffffefu, 0xfbffffefu, 0xfbf1ffefu,
    0xf3c078f7u, 0xe7c06073u, 0xe87667fbu, 0xdb8ff60bu,
    0xd67ff67bu, 0xd43e7bbbu, 0xdac63b93u, 0xee50e717u,
    0xe70c0057u, 0xf741f697u, 0xfb900017u, 0xfbde0017u,
    0xfdeb8017u, 0xfcf3b437u, 0xfe383437u, 0xff3f81f7u,
    0xffcfff7u, 0xffe7fff7u, 0xfff9fff7u, 0xfffe7fe7u,
    0xffff87cfu, 0xfffff83fu, 0xffffffffu, 0xffffffffu
};

static inline u8 trollface_image_pixel(u8 x, u8 y)
{
    u32 row;

    if ((x >= TROLLFACE_IMAGE_WIDTH) || (y >= TROLLFACE_IMAGE_HEIGHT)) {
        return 0u;
    }

    row = TROLLFACE_IMAGE_BITS[y];
    return (u8)((row >> (31u - x)) & 1UL);
}

#endif /* TROLLFACE_IMAGE_H */
