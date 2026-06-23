#ifndef PWM_H
#define PWM_H

#include "amazing_utils.h"

/*
 * Simple edge-aligned PWM via OC1-OC4.
 *
 * All channels share Timer2, so they share one frequency and have separate
 * duty registers. Timer1 is reserved for FreeRTOS. Timer2 is owned here.
 *
 * duty uses 0..PWM_DUTY_MAX:
 *   0             = 0 %
 *   PWM_DUTY_MAX = 100 %
 */

#define PWM_DUTY_MAX 1000u

typedef struct {
    Pin pin;
    u8 rp_num;
    u8 ch;
} PwmPin;

void pwm_timer_init(u32 freq_hz);

void pwm_init(PwmPin p, u16 duty);
void pwm_set_duty(PwmPin p, u16 duty);

void pwm_stop(PwmPin p);
void pwm_start(PwmPin p);

#endif /* PWM_H */
