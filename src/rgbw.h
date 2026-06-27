#ifndef RGBW_H
#define RGBW_H

#include "hardware.h"

#define RGBW_RED    255u, 0u, 0u, 0u
#define RGBW_REDF   255u, 0u, 15u, 10u
#define RGBW_BLUE   0u, 0u, 255u, 0u
#define RGBW_BLUEF  15u, 0u, 255u, 10u
#define RGBW_GREEN  0u, 255u, 0u, 0u
#define RGBW_GREENF 0u, 255u, 15u, 10u

void rgbw_init(void);

void rgbw_set_r(u16 duty);
void rgbw_set_g(u16 duty);
void rgbw_set_b(u16 duty);
void rgbw_set_w(u16 duty);

void rgbw_hold_r(u16 duty, u32 delay_ms);
void rgbw_hold_g(u16 duty, u32 delay_ms);
void rgbw_hold_b(u16 duty, u32 delay_ms);
void rgbw_hold_w(u16 duty, u32 delay_ms);

typedef struct
{
    u8 r;
    u8 g;
    u8 b;
    u8 w;
} RGBW_STATE;

typedef struct
{
    RGBW_STATE target;
    u32 duration;
    u32 left;
} RGBW_TRANSITION;

void rgbw_new_transition(RGBW_STATE state, u32 d_ms);
void rgbw_tick(void);

#endif /* RGBW_H */
