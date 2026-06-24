#ifndef RGBW_H
#define RGBW_H

#include "hardware.h"

void rgbw_init(void);

void rgbw_set_r(u16 duty);
void rgbw_set_g(u16 duty);
void rgbw_set_b(u16 duty);
void rgbw_set_w(u16 duty);

void rgbw_hold_r(u16 duty, u32 delay_ms);
void rgbw_hold_g(u16 duty, u32 delay_ms);
void rgbw_hold_b(u16 duty, u32 delay_ms);
void rgbw_hold_w(u16 duty, u32 delay_ms);

#endif /* RGBW_H */
