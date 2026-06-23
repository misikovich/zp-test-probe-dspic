#include <xc.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "project_clock.h"
#include "motor.h"
#include "pwm.h"
#include "st7789v_demo.h"

/* Configuration bits */
#pragma config FNOSC    = FRC
#pragma config IESO     = OFF
#pragma config POSCMD   = NONE
#pragma config OSCIOFNC = ON
#pragma config IOL1WAY  = OFF
#if PROJECT_USE_PLL
#pragma config FCKSM    = CSECMD
#else
#pragma config FCKSM    = CSDCMD
#endif
#pragma config FWDTEN   = OFF
#pragma config ICS      = PGD1
#pragma config JTAGEN   = OFF

#define LED_PWM_FREQ     1000u
#define LEDR_BRIGHTNESS   10u
#define LEDG_BRIGHTNESS   100u
#define LEDB_BRIGHTNESS   50u
#define LEDW_BRIGHTNESS   200u
#define DISPLAY_TASK_STACK 256u

/* PwmPin = { { LAT, PORT, TRIS, ANSEL, mask }, RP number, OC channel } */
static const PwmPin LED_R = { { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 8  }, 40u, 1u };
static const PwmPin LED_G = { { &LATB, &PORTB, &TRISB, NULL,    1u << 10 }, 42u, 2u };
static const PwmPin LED_B = { { &LATB, &PORTB, &TRISB, NULL,    1u << 6  }, 38u, 3u };
static const PwmPin LED_W = { { &LATB, &PORTB, &TRISB, NULL,    1u << 5  }, 37u, 4u };

static void prvInitClock(void)
{
#if PROJECT_USE_PLL
    CLKDIVbits.FRCDIV = 0u;
    CLKDIVbits.PLLPRE = (u16)PROJECT_PLLPRE_BITS;
    CLKDIVbits.PLLPOST = (u16)PROJECT_PLLPOST_BITS;
    PLLFBDbits.PLLDIV = (u16)PROJECT_PLLFBD_BITS;

    __builtin_write_OSCCONH(PROJECT_OSC_FRCPLL_NOSC);
    __builtin_write_OSCCONL((u8)(OSCCON | 0x0001u));

    while (OSCCONbits.COSC != PROJECT_OSC_FRCPLL_NOSC) {
    }
    while (!OSCCONbits.LOCK) {
    }
#endif
}

static void prvInitHardware(void)
{
    pwm_timer_init(LED_PWM_FREQ);
    pwm_init(LED_R, 0u);
    pwm_init(LED_G, 0u);
    pwm_init(LED_B, 0u);
    pwm_init(LED_W, 0u);
    mtr_init();
}

static void vHeartbeatTask(void *pvParameters)
{
    unused(pvParameters);

    forever {
        pwm_set_duty(LED_G, LEDG_BRIGHTNESS); vTaskDelayMS(50);
        pwm_set_duty(LED_G, 0u);             vTaskDelayMS(100);
        pwm_set_duty(LED_G, LEDG_BRIGHTNESS); vTaskDelayMS(50);
        pwm_set_duty(LED_G, 0u);             vTaskDelayMS(800);
    }
}

static void vMotorTestTask(void *pvParameters) {
    unused(pvParameters);

    vTaskDelayMS(1500);
    forever {
        pwm_set_duty(LED_R, 200u);
        pwm_set_duty(LED_B, 0u);

        mtr_drive(LOCK);
        vTaskDelayMS(400);
        mtr_drive(STOP);
        vTaskDelayMS(300);

        pwm_set_duty(LED_R, 0u);
        pwm_set_duty(LED_B, 200u);

        mtr_drive(UNLOCK);
        vTaskDelayMS(500);
        mtr_drive(STOP);
        vTaskDelayMS(2000);
    }
}

int main(void)
{
    prvInitClock();
    prvInitHardware();

    xTaskCreate(vHeartbeatTask, "Blue", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vDisplayDemoTask, "Display", DISPLAY_TASK_STACK, NULL, 1, NULL);
    xTaskCreate(vMotorTestTask, "MTR_TEST", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    forever;
    return 0;
}
