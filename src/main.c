#include <xc.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "project_clock.h"
#include "motor.h"
#include "rgbw.h"
#include "pwm.h"
#include "display_hw.h"
#include "stdio.h"

#define I_DONT_LIKE_MICROCHIP_SOFTWARE_H 1ul

/* Configuration bits */
#pragma config FNOSC    = FRC
#pragma config IESO     = OFF
#pragma config POSCMD   = NONE
#pragma config OSCIOFNC = ON
#pragma config IOL1WAY  = OFF
#pragma config FWDTEN   = OFF
#pragma config ICS      = PGD1
#pragma config JTAGEN   = OFF
#if PROJECT_USE_PLL
    #pragma config FCKSM    = CSECMD
#else
    #pragma config FCKSM    = CSDCMD
#endif

#define DISPLAY_TASK_STACK  256u
#define DISPLAY_BG ST_RED
#define MTR_SENSE_TEXT_X 105u
#define MTR_SENSE_TEXT_Y 10u
#define MTR_SENSE_DIGITS 3u

static volatile u16 MTR_SENSE_VAL = 0;

static void prvInitClock(void)
{
    #if PROJECT_USE_PLL
        CLKDIVbits.FRCDIV = 0u;
        CLKDIVbits.PLLPRE = (uint16_t)PROJECT_PLLPRE_BITS;
        CLKDIVbits.PLLPOST = (uint16_t)PROJECT_PLLPOST_BITS;
        PLLFBDbits.PLLDIV = (uint16_t)PROJECT_PLLFBD_BITS;

        __builtin_write_OSCCONH(PROJECT_OSC_FRCPLL_NOSC);
        __builtin_write_OSCCONL((uint8_t)(OSCCON | 0x0001u));

        while (OSCCONbits.COSC != PROJECT_OSC_FRCPLL_NOSC) {
        }
        while (!OSCCONbits.LOCK) {
        }
    #endif
}

static void prvInitHardware(void)
{
    rgbw_init();
    mtr_init();
}

static void vHeartbeatTask(void *pvParameters) 
{
    unused(pvParameters);

    forever {
        rgbw_hold_g(ON, 50u);
        rgbw_hold_g(OFF, 100u);
        rgbw_hold_g(ON, 50u);
        rgbw_hold_g(OFF, 800u);
    }
}

static void vMotorTestTask(void *pvParameters) 
{
    unused(pvParameters);

    task_hold(1500u);
    forever {
        rgbw_set_r(ON);
        rgbw_set_b(OFF);
        mtr_hold(LOCK, 2000u);
        mtr_hold(STOP, 300u);

        rgbw_set_r(OFF);
        rgbw_set_b(ON);
        mtr_hold(UNLOCK, 500u);
        mtr_hold(STOP, 2000u);
    }
}

void vDisplayDemoTask(void *pvParameters)
{
    unused(pvParameters);

    task_hold(50u);
    st_init();
    st_fill_color(DISPLAY_BG);
    gfx_draw_string(25, 10, "MTR SENSE: ", Font_7x10, ST_BLACK, DISPLAY_BG);
    task_hold(50u);

    forever {
        static bool upd = 0;
        char buf[6];
        sprintf(buf, "%u", mtr_sense_raw_to_ma(MTR_SENSE_VAL));
        gfx_draw_filled_rectangle(MTR_SENSE_TEXT_X, MTR_SENSE_TEXT_Y,
                (u16)(MTR_SENSE_DIGITS * Font_7x10.width),
                Font_7x10.height, ST_YELLOW);
        gfx_draw_string(MTR_SENSE_TEXT_X, MTR_SENSE_TEXT_Y, buf,
                Font_7x10, ST_BLACK, ST_YELLOW);
        if (upd)
        {
            gfx_draw_filled_rectangle(10, 10, 10, 10, ST_YELLOW);
        } else
        {
            gfx_draw_filled_rectangle(10, 10, 10, 10, DISPLAY_BG);
        }
        upd = !upd;
        task_hold(80u);
    }
}

static void vMotorPollTask(void *pvParameters)
{
    unused(pvParameters);

    forever {
        MTR_SENSE_VAL = mtr_poll_sense_raw();
        task_hold(20u);
    }
}

int main(void)
{
    prvInitClock();
    prvInitHardware();

    xTaskCreate(vHeartbeatTask, "Heartbeat", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vDisplayDemoTask, "Display", DISPLAY_TASK_STACK, NULL, 1, NULL);
    xTaskCreate(vMotorTestTask, "Motor_TEST", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vMotorPollTask, "Motor_Poll", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    forever;
    return 0;
}
