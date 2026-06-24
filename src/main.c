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
    rgbw_init();
    mtr_init();
}

static void vHeartbeatTask(void *pvParameters) {
    unused(pvParameters);

    forever {
        rgbw_hold_g(ON, 50u)
        rgbw_hold_g(OFF, 100u);
        rgbw_hold_g(ON, 50u)
        rgbw_hold_g(OFF, 800u);
    }
}

static void vMotorTestTask(void *pvParameters) {
    unused(pvParameters);

    vTaskDelayMS(1500u);
    forever {
        rgbw_set_r(ON);
        rgbw_set_b(OFF);
        mtr_drive(LOCK);
        vTaskDelayMS(400);

        mtr_drive(STOP);
        vTaskDelayMS(300);

        rgbw_set_r(OFF);
        rgbw_set_b(ON);
        mtr_drive(UNLOCK);
        vTaskDelayMS(500);

        mtr_drive(STOP);
        vTaskDelayMS(2000);
    }
}

void vDisplayDemoTask(void *pvParameters)
{
    unused(pvParameters);

    vTaskDelayMS(50u);
    st_init();

    forever {
        st_perform_test();
        vTaskDelayMS(1000u);
    }
}

int main(void)
{
    prvInitClock();
    prvInitHardware();

    xTaskCreate(vHeartbeatTask, "Heartbeat", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vDisplayDemoTask, "Display", DISPLAY_TASK_STACK, NULL, 1, NULL);
    xTaskCreate(vMotorTestTask, "Motor_TEST", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    forever;
    return 0;
}
