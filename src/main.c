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
        mtr_hold(LOCK, 400u);
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

    forever {
        st_perform_test();
        task_hold(1000u);
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
