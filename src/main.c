#include <xc.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "project_clock.h"
#include "motor.h"
#include "sensors.h"
#include "rgbw.h"
#include "pwm.h"
#include "display_hw.h"
#include "gfx_plotter.h"

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

#define DISPLAY_TASK_STACK  768u
#define APP_TASK_PRIORITY_NORMAL 1u
#define APP_TASK_PRIORITY_MOTOR 2u
#define DISPLAY_BG ST_RED
#define SENSOR_UPD_X 10u
#define SENSOR_UPD_SIZE 8u
#define SENSOR_VALUE_TEXT_X 105u
#define MTR_SENSE_TEXT_Y 10u
#define TMP_TYPE2_TEXT_Y 22u
#define TMP_RCD_TEXT_Y 34u
#define MTR_SENSE_DIGITS 3u
#define TMP_RAW_DIGITS 4u
#define MTR_PLOT_X 10u
#define MTR_PLOT_Y 52u
#define MTR_PLOT_W 220u
#define MTR_PLOT_H 170u
#define MTR_PLOT_MIN_MA 0u
#define MTR_PLOT_MAX_MA 500u

static volatile SensorReadings SENSOR_VALUES = { 0u, 0u, 0u };

#define RGBW_DEFAULT_DURATION 350u

RGBW_STATE RGBW_STATE_GREEN =   { 0, 255, 0, 0};
RGBW_STATE RGBW_STATE_GREENF =  { 0, 255, 12, 0};
RGBW_STATE RGBW_STATE_BLUE =    { 0, 0, 255, 0};
RGBW_STATE RGBW_STATE_BLUEF =   { 4, 12, 255, 0};
RGBW_STATE RGBW_STATE_RED =     { 255, 0, 0, 0};
RGBW_STATE RGBW_STATE_REDF =    { 255, 0, 5, 0};
RGBW_STATE RGBW_STATE_ERR =     { 220, 40, 22, 0};

typedef struct {
    char text[6];
} StrU16;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    unused(xTask);
    unused(pcTaskName);

    __builtin_disable_interrupts();

    rgbw_new_transition(RGBW_STATE_ERR, RGBW_DEFAULT_DURATION);
    forever {
    }
}

void vApplicationMallocFailedHook(void)
{
    __builtin_disable_interrupts();

    rgbw_new_transition(RGBW_STATE_ERR, RGBW_DEFAULT_DURATION);
    forever {
    }
}

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
    sensors_init();
    rgbw_new_transition(RGBW_STATE_GREENF, RGBW_DEFAULT_DURATION);
}

static void vMotorTestTask(void *pvParameters) 
{
    unused(pvParameters);

    task_hold(1500u);
    forever {
        rgbw_new_transition(RGBW_STATE_REDF, RGBW_DEFAULT_DURATION);
        mtr_hold(LOCK, 2000u);
        mtr_hold(STOP, 300u);

        rgbw_new_transition(RGBW_STATE_BLUEF, RGBW_DEFAULT_DURATION);
        mtr_hold(UNLOCK, 500u);
        mtr_hold(STOP, 2000u);
    }
}

static StrU16 stru16(u16 num)
{
    StrU16 str;
    char reversed[5];
    u8 count;
    u8 out;

    if (num == 0u) {
        str.text[0] = '0';
        str.text[1] = '\0';
        return str;
    }

    count = 0u;
    while ((num != 0u) && (count < 5u)) {
        reversed[count] = (char)('0' + (num % 10u));
        num = (u16)(num / 10u);
        count++;
    }

    out = 0u;
    while (count != 0u) {
        count--;
        str.text[out] = reversed[count];
        out++;
    }
    str.text[out] = '\0';
    return str;
}

static void draw_sensor_upd_indicator(u16 y, bool upd)
{
    gfx_draw_filled_rectangle(SENSOR_UPD_X, (u16)(y + 1u),
            SENSOR_UPD_SIZE, SENSOR_UPD_SIZE,
            upd ? ST_GREEN : DISPLAY_BG);
}

void vDisplayDemoTask(void *pvParameters)
{
    GfxPlotter plotter;
    unused(pvParameters);

    task_hold(50u);
    st_init();


    st_fill_color(DISPLAY_BG);
    plotter = gfx_plotter_create("MTR mA",
            MTR_PLOT_X, MTR_PLOT_Y, MTR_PLOT_W, MTR_PLOT_H,
            MTR_PLOT_MIN_MA, MTR_PLOT_MAX_MA,
            ST_BLACK, ST_LIGHTBLUE);

    gfx_draw_string(25, 10, "MTR SENSE: ", Font_7x10, ST_BLACK, DISPLAY_BG);
    gfx_draw_string(25, 22, "TMP TYPE2: ", Font_7x10, ST_BLACK, DISPLAY_BG);
    gfx_draw_string(25, 34, "TMP RCD  : ", Font_7x10, ST_BLACK, DISPLAY_BG);
    task_hold(50u);

    forever {
        static bool upd = 0;
        SensorReadings readings;
        u16 ma;
        StrU16 chars_ma;
        StrU16 chars_tmp_type2;
        StrU16 chars_tmp_rcd;

        upd = !upd;
        readings.motor_raw = SENSOR_VALUES.motor_raw;
        readings.tmp_rcd_raw = SENSOR_VALUES.tmp_rcd_raw;
        readings.tmp_type2_raw = SENSOR_VALUES.tmp_type2_raw;

        ma = sensors_motor_raw_to_ma(readings.motor_raw);
        chars_ma = stru16(ma);
        chars_tmp_type2 = stru16(readings.tmp_type2_raw);
        chars_tmp_rcd = stru16(readings.tmp_rcd_raw);

        gfx_draw_text(SENSOR_VALUE_TEXT_X, MTR_SENSE_TEXT_Y, chars_ma.text,
                Font_7x10, ST_BLACK, ST_YELLOW, MTR_SENSE_DIGITS);
        gfx_draw_text(SENSOR_VALUE_TEXT_X, TMP_TYPE2_TEXT_Y, chars_tmp_type2.text,
                Font_7x10, ST_BLACK, ST_YELLOW, TMP_RAW_DIGITS);
        gfx_draw_text(SENSOR_VALUE_TEXT_X, TMP_RCD_TEXT_Y, chars_tmp_rcd.text,
                Font_7x10, ST_BLACK, ST_YELLOW, TMP_RAW_DIGITS);
        gfx_plotter_push(&plotter, ma);
        draw_sensor_upd_indicator(MTR_SENSE_TEXT_Y, upd);
        draw_sensor_upd_indicator(TMP_TYPE2_TEXT_Y, upd);
        draw_sensor_upd_indicator(TMP_RCD_TEXT_Y, upd);

        task_hold(40u);
    }
}

static void vSensorPollTask(void *pvParameters)
{
    SensorReadings readings;

    unused(pvParameters);

    forever {
        sensors_poll(&readings);
        SENSOR_VALUES.motor_raw = readings.motor_raw;
        SENSOR_VALUES.tmp_rcd_raw = readings.tmp_rcd_raw;
        SENSOR_VALUES.tmp_type2_raw = readings.tmp_type2_raw;
        task_hold(20u);
    }
}

static void vRGBWTickTask(void *pvParameters)
{
    unused(pvParameters);
    task_hold(100);

    forever {
        rgbw_tick();
    }
}
int main(void)
{
    prvInitClock();
    prvInitHardware();

    configASSERT(xTaskCreate(vRGBWTickTask, "RGBW",
            configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);
    configASSERT(xTaskCreate(vDisplayDemoTask, "Display",
            DISPLAY_TASK_STACK, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);
    configASSERT(xTaskCreate(vMotorTestTask, "Motor_TEST",
            configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_MOTOR, NULL) == pdPASS);
    configASSERT(xTaskCreate(vSensorPollTask, "Sensors",
            configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);


    vTaskStartScheduler();

    forever;
    return 0;
}
