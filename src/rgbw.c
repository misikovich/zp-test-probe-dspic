#include <xc.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "pwm.h"

#define LED_PWM_FREQ    1000u
#define LED_MAX_R       150u
#define LED_MAX_G       100u
#define LED_MAX_B       150u
#define LED_MAX_W       100u

/* PwmPin = { { LAT, PORT, TRIS, ANSEL, mask }, RP number, OC channel } */
static const PwmPin LED_R = { { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 8  }, 40u, 1u };
static const PwmPin LED_G = { { &LATB, &PORTB, &TRISB, NULL,    1u << 10 }, 42u, 2u };
static const PwmPin LED_B = { { &LATB, &PORTB, &TRISB, NULL,    1u << 6  }, 38u, 3u };
static const PwmPin LED_W = { { &LATB, &PORTB, &TRISB, NULL,    1u << 5  }, 37u, 4u };

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