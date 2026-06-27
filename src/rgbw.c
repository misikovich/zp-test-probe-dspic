#include <xc.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "pwm.h"
#include "rgbw.h"

#define LED_PWM_FREQ        1000u
#define LED_MAX_R           150u
#define LED_MAX_G           100u
#define LED_MAX_B           150u
#define LED_MAX_W           100u

#define RGBW_TPS 30
inline void rgbw_hold_cycle() {
    task_hold(1000 / RGBW_TPS);
}

/* PwmPin = { { LAT, PORT, TRIS, ANSEL, mask }, RP number, OC channel } */
static const PwmPin LED_R = { { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 8  }, 40u, 1u };
static const PwmPin LED_G = { { &LATB, &PORTB, &TRISB, NULL,    1u << 10 }, 42u, 2u };
static const PwmPin LED_B = { { &LATB, &PORTB, &TRISB, NULL,    1u << 6  }, 38u, 3u };
static const PwmPin LED_W = { { &LATB, &PORTB, &TRISB, NULL,    1u << 5  }, 37u, 4u };

RGBW_STATE RGBW_current;
RGBW_TRANSITION rgbw_transition = { { 0u, 0u, 0u, 0u }, 0u, 0u };

void rgbw_new_transition(RGBW_STATE s, u32 d_ms) {
    rgbw_transition.target = s;
    rgbw_transition.duration = d_ms;
    rgbw_transition.left = d_ms;
}

bool rgbw_state_is_equal(RGBW_STATE s1, RGBW_STATE s2) {
    return 
    s1.r == s2.r && 
    s1.g == s2.g && 
    s1.b == s2.b && 
    s1.w == s2.w;
}

/* map a raw 0..255 channel value onto that channel's own duty ceiling.
 *
 * two things happen here:
 *  - per-channel scale (not a shared max + clamp) keeps the whole 0..255
 *    range live, so a fade uses every step instead of saturating early.
 *  - gamma ~2.0 (square law) pre-distorts the output. LED luminance is ~linear
 *    in duty but the eye is ~cube-root of luminance, so a LINEAR duty ramp
 *    looks like it slams bright in the first ~10% then crawls -- reads as a
 *    hard cut. Squaring makes luminance track f^2, so perceived brightness is
 *    ~linear in time and the fade actually looks like a fade. One multiply,
 *    no lookup table. */
static u16 rgbw_scale(u8 raw, u16 max) {
    u32 g = ((u32)raw * raw) / 255u;   /* gamma 2.0: 0..255 -> 0..255 */
    return (u16)((g * max) / 255u);
}

/* push the current raw state out to the four PWM channels */
static void rgbw_apply(void) {
    rgbw_set_r(rgbw_scale(RGBW_current.r, LED_MAX_R));
    rgbw_set_g(rgbw_scale(RGBW_current.g, LED_MAX_G));
    rgbw_set_b(rgbw_scale(RGBW_current.b, LED_MAX_B));
    rgbw_set_w(rgbw_scale(RGBW_current.w, LED_MAX_W));
}

/* move one channel toward target by 1/steps of the remaining delta.
 * recomputed every tick, so the path is linear and converges exactly.
 * a non-zero delta always moves at least 1 so small fades don't stall. */
static u8 rgbw_step_channel(u8 cur, u8 tgt, u32 steps) {
    int delta = (int)tgt - (int)cur;
    int step;

    if (delta == 0) return cur;

    step = delta / (int)steps;
    if (step == 0) step = (delta > 0) ? 1 : -1;

    return (u8)((int)cur + step);
}

void rgbw_interpolate(void) {
    u32 dt = 1000u / RGBW_TPS;   /* ms advanced per tick */
    u32 steps;

    if (rgbw_transition.left == 0u) return;

    if (rgbw_transition.left <= dt) {
        /* final tick: snap exactly onto target */
        RGBW_current = rgbw_transition.target;
        rgbw_transition.left = 0u;
    } else {
        steps = rgbw_transition.left / dt;   /* >= 1 here */
        RGBW_current.r = rgbw_step_channel(RGBW_current.r, rgbw_transition.target.r, steps);
        RGBW_current.g = rgbw_step_channel(RGBW_current.g, rgbw_transition.target.g, steps);
        RGBW_current.b = rgbw_step_channel(RGBW_current.b, rgbw_transition.target.b, steps);
        RGBW_current.w = rgbw_step_channel(RGBW_current.w, rgbw_transition.target.w, steps);
        rgbw_transition.left -= dt;
    }

    rgbw_apply();
}

void rgbw_tick(void) {
    if (rgbw_transition.left != 0u) {
        rgbw_interpolate();
    }
    rgbw_hold_cycle();   /* always pace; never busy-spin when idle */
}


void rgbw_init(void) {
    pwm_timer_init(LED_PWM_FREQ);
    pwm_init(LED_R, 0u);
    pwm_init(LED_G, 0u);
    pwm_init(LED_B, 0u);
    pwm_init(LED_W, 0u);
}

void rgbw_set_r(u16 duty) {
    duty = (duty <= LED_MAX_R) ? duty : LED_MAX_R; 
    pwm_set_duty(LED_R, duty);
}

void rgbw_set_g(u16 duty) {
    duty = (duty <= LED_MAX_G) ? duty : LED_MAX_G; 
    pwm_set_duty(LED_G, duty);
}

void rgbw_set_b(u16 duty) {
    duty = (duty <= LED_MAX_B) ? duty : LED_MAX_B; 
    pwm_set_duty(LED_B, duty);
}

void rgbw_set_w(u16 duty) {
    duty = (duty <= LED_MAX_W) ? duty : LED_MAX_W; 
    pwm_set_duty(LED_W, duty);
}


void rgbw_hold_r(u16 duty, u32 delay_ms) {
    rgbw_set_r(duty);
    task_hold(delay_ms);
}

void rgbw_hold_g(u16 duty, u32 delay_ms) {
    rgbw_set_g(duty);
    task_hold(delay_ms);
}

void rgbw_hold_b(u16 duty, u32 delay_ms) {
    rgbw_set_b(duty);
    task_hold(delay_ms);
}

void rgbw_hold_w(u16 duty, u32 delay_ms) {
    rgbw_set_w(duty);
    task_hold(delay_ms);
}