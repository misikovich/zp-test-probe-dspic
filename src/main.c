#include <xc.h>
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "project_clock.h"
#include "motor.h"
#include "sensors.h"
#include "bcd_switch.h"
#include "rgbw.h"
#include "pwm.h"
#include "uart.h"
#include "display_hw.h"
#include "gfx_plotter.h"
#include "startup_logo.h"

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
#define APP_DISPLAY_PERIOD_MS 40u
#define APP_STARTUP_SOUND_DELAY_MS 500u
#define APP_IDLE_SLEEP_MS 60000u

#define APP_PAGE_SENSOR_DASHBOARD 0u
#define APP_PAGE_TEST_HUB 1u
#define APP_PAGE_INVALID 255u

#define DISPLAY_BG ST_RED
#define TEST_PAGE_BG ST_DARKBLUE

#define SENSOR_UPD_X 10u
#define SENSOR_UPD_SIZE 8u
#define SENSOR_VALUE_TEXT_X 105u
#define MTR_SENSE_TEXT_Y 10u
#define TMP_TYPE2_TEXT_Y 22u
#define TMP_RCD_TEXT_Y 34u
#define V_NG_TEXT_Y 46u
#define V_L1L2_TEXT_Y 58u
#define BCD_LABEL_TEXT_X 145u
#define BCD_VALUE_TEXT_X 185u
#define BCD_VALUE_TEXT_Y 10u
#define BCD_BITS_TEXT_Y 22u
#define BCD_VALUE_DIGITS 2u
#define BCD_BITS_DIGITS 4u
#define MTR_SENSE_DIGITS 3u
#define TMP_RAW_DIGITS 4u
#define SENSOR_VOLT_DIGITS 6u
#define MTR_PLOT_X 10u
#define MTR_PLOT_Y 76u
#define MTR_PLOT_W 220u
#define MTR_PLOT_H 146u
#define MTR_PLOT_MIN_MA 0u
#define MTR_PLOT_MAX_MA 500u

#define TEST_TITLE_X 16u
#define TEST_TITLE_Y 12u
#define TEST_LABEL_X 18u
#define TEST_STATUS_X 132u
#define TEST_ROW_FIRST_Y 54u
#define TEST_ROW_STEP_Y 24u
#define TEST_STATUS_DIGITS 8u
#define TEST_UPD_X 212u
#define TEST_UPD_Y 18u
#define TEST_UPD_SIZE 10u

#define ARRAY_LEN_U8(a) ((u8)(sizeof(a) / sizeof((a)[0])))

#define RGBW_DEFAULT_DURATION 350u

typedef struct AppPage AppPage;
typedef struct AppUi AppUi;
typedef void (*AppPageFn)(AppUi *ui, const AppPage *page);
typedef const char *(*TestStatusFn)(void);

typedef struct {
    char text[6];
} StrU16;

typedef struct {
    char text[8];
} StrS32;

typedef struct {
    u16 motor_raw;
    u16 tmp_rcd_raw;
    u16 tmp_type2_raw;
    u16 v_ng_raw;
    u16 v_l1l2_raw;
} AppSensorValues;

typedef struct {
    const char *label;
    TestStatusFn status;
} TestPageRow;

typedef struct {
    const TestPageRow *rows;
    u8 row_count;
} TestPageData;

struct AppUi {
    GfxPlotter motor_plotter;
    bool blink;
};

struct AppPage {
    u8 id;
    const char *title;
    u16 bg;
    AppPageFn enter;
    AppPageFn tick;
    const void *data;
};

static void page_enter_sensor_dashboard(AppUi *ui, const AppPage *page);
static void page_tick_sensor_dashboard(AppUi *ui, const AppPage *page);
static void page_enter_test_page(AppUi *ui, const AppPage *page);
static void page_tick_test_page(AppUi *ui, const AppPage *page);
static const char *test_status_fpga_upload(void);
static const char *test_status_comms(void);
static const char *test_status_relay(void);

static const TestPageRow TEST_HUB_ROWS[] = {
    { "FPGA UPLOAD:", test_status_fpga_upload },
    { "COMMS TEST :", test_status_comms },
    { "RELAY TEST :", test_status_relay }
};

static const TestPageData TEST_HUB_DATA = {
    TEST_HUB_ROWS,
    ARRAY_LEN_U8(TEST_HUB_ROWS)
};

/* Add pages here. A matching BCD value selects that page directly. */
static const AppPage APP_PAGES[] = {
    { APP_PAGE_SENSOR_DASHBOARD, "SENSORS", DISPLAY_BG,
            page_enter_sensor_dashboard, page_tick_sensor_dashboard, NULL },
    { APP_PAGE_TEST_HUB, "TESTS", TEST_PAGE_BG,
            page_enter_test_page, page_tick_test_page, &TEST_HUB_DATA }
};

#define APP_PAGE_COUNT ARRAY_LEN_U8(APP_PAGES)

static volatile AppSensorValues SENSOR_VALUES = { 0u, 0u, 0u, 0u, 0u };
static volatile u8 BCD_SWITCH_VALUE = 0u;
static volatile u8 APP_REQUESTED_PAGE_ID = APP_PAGE_SENSOR_DASHBOARD;

RGBW_STATE RGBW_STATE_GREEN =       { 0, 255, 0, 0};
RGBW_STATE RGBW_STATE_GREENF =      { 0, 255, 12, 0};
RGBW_STATE RGBW_STATE_BLUE =        { 0, 0, 255, 0};
RGBW_STATE RGBW_STATE_BLUEF =       { 4, 12, 255, 0};
RGBW_STATE RGBW_STATE_RED =         { 255, 0, 0, 0};
RGBW_STATE RGBW_STATE_REDF =        { 255, 0, 5, 0};
RGBW_STATE RGBW_STATE_ERR =         { 220, 40, 22, 0};
RGBW_STATE RGBW_STATE_UNLOCKED =    { 5, 255, 45, 0};
RGBW_STATE RGBW_STATE_LOCKED =      { 255, 5, 28, 0};

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

static const AppPage *app_page_find(u8 page_id)
{
    u8 i;

    for (i = 0u; i < APP_PAGE_COUNT; i++) {
        if (APP_PAGES[i].id == page_id) {
            return &APP_PAGES[i];
        }
    }

    return NULL;
}

static u8 app_page_id_from_bcd(u8 value)
{
    const AppPage *page;

    page = app_page_find(value);
    if (page != NULL) {
        return page->id;
    }

    if (value == 0u) {
        return APP_PAGE_SENSOR_DASHBOARD;
    }

    return APP_PAGE_TEST_HUB;
}

static void app_page_on_bcd_switch(u8 value)
{
    BCD_SWITCH_VALUE = value;
    APP_REQUESTED_PAGE_ID = app_page_id_from_bcd(value);
    uart_play_sound(SOUND_CMD_SWITCH_PAGE);
}

static void prvInitHardware(void)
{
    rgbw_init();
    mtr_init();
    sensors_init();
    bcd_switch_init();
    uart_init();

    BCD_SWITCH_VALUE = bcd_switch_read();
    APP_REQUESTED_PAGE_ID = app_page_id_from_bcd(BCD_SWITCH_VALUE);
    bcd_on_switch(app_page_on_bcd_switch);

    rgbw_new_transition(RGBW_STATE_GREENF, RGBW_DEFAULT_DURATION);
}

static void vMotorTestTask(void *pvParameters)
{
    unused(pvParameters);

    task_hold(1500u);
    forever {
        if (APP_REQUESTED_PAGE_ID != APP_PAGE_SENSOR_DASHBOARD) {
            task_hold(100u);
            continue;
        }

        rgbw_new_transition(RGBW_STATE_LOCKED, RGBW_DEFAULT_DURATION);
        mtr_hold(LOCK, 2000u);
        mtr_hold(STOP, 300u);

        rgbw_new_transition(RGBW_STATE_UNLOCKED, RGBW_DEFAULT_DURATION);
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

static StrS32 strs32(s32 num)
{
    StrS32 str;
    char reversed[6];
    u8 negative;
    u8 count;
    u8 out;
    u32 mag;

    if (num == 0L) {
        str.text[0] = '0';
        str.text[1] = '\0';
        return str;
    }

    negative = 0u;
    if (num < 0L) {
        negative = 1u;
        mag = (u32)(-num);
    } else {
        mag = (u32)num;
    }

    count = 0u;
    while ((mag != 0u) && (count < 6u)) {
        reversed[count] = (char)('0' + (mag % 10u));
        mag = (u32)(mag / 10u);
        count++;
    }

    out = 0u;
    if (negative) {
        str.text[out] = '-';
        out++;
    }

    while (count != 0u) {
        count--;
        str.text[out] = reversed[count];
        out++;
    }
    str.text[out] = '\0';
    return str;
}

static void draw_update_indicator(u16 x, u16 y, u16 size, bool upd, u16 bg)
{
    gfx_draw_filled_rectangle(x, y, size, size, upd ? ST_GREEN : bg);
}

static void bcd_bits_text(u8 value, char *text)
{
    text[0] = (value & 0x08u) ? '1' : '0';
    text[1] = (value & 0x04u) ? '1' : '0';
    text[2] = (value & 0x02u) ? '1' : '0';
    text[3] = (value & 0x01u) ? '1' : '0';
    text[4] = '\0';
}

static void page_enter_sensor_dashboard(AppUi *ui, const AppPage *page)
{
    ui->blink = 0u;
    st_fill_color(page->bg);
    ui->motor_plotter = gfx_plotter_create("MTR mA",
            MTR_PLOT_X, MTR_PLOT_Y, MTR_PLOT_W, MTR_PLOT_H,
            MTR_PLOT_MIN_MA, MTR_PLOT_MAX_MA,
            ST_BLACK, ST_LIGHTBLUE);

    gfx_draw_string(25, 10, "MTR SENSE: ", Font_7x10, ST_BLACK, page->bg);
    gfx_draw_string(25, 22, "TMP TYPE2: ", Font_7x10, ST_BLACK, page->bg);
    gfx_draw_string(25, 34, "TMP RCD  : ", Font_7x10, ST_BLACK, page->bg);
    gfx_draw_string(25, 46, "V_NG mV  : ", Font_7x10, ST_BLACK, page->bg);
    gfx_draw_string(25, 58, "V_L1L2 mV: ", Font_7x10, ST_BLACK, page->bg);
    gfx_draw_string(BCD_LABEL_TEXT_X, BCD_VALUE_TEXT_Y, "BCD:", Font_7x10,
            ST_BLACK, page->bg);
    gfx_draw_string(BCD_LABEL_TEXT_X, BCD_BITS_TEXT_Y, "BITS:", Font_7x10,
            ST_BLACK, page->bg);
}

static void page_tick_sensor_dashboard(AppUi *ui, const AppPage *page)
{
    SensorReadings readings;
    u16 ma;
    StrU16 chars_ma;
    StrU16 chars_tmp_type2;
    StrU16 chars_tmp_rcd;
    StrU16 chars_bcd;
    StrS32 chars_v_ng;
    StrS32 chars_v_l1l2;
    char bcd_bits[5];
    u8 bcd_value;

    ui->blink = ui->blink ? 0u : 1u;
    readings.motor_raw = SENSOR_VALUES.motor_raw;
    readings.tmp_rcd_raw = SENSOR_VALUES.tmp_rcd_raw;
    readings.tmp_type2_raw = SENSOR_VALUES.tmp_type2_raw;
    readings.v_ng_raw = SENSOR_VALUES.v_ng_raw;
    readings.v_l1l2_raw = SENSOR_VALUES.v_l1l2_raw;
    bcd_value = BCD_SWITCH_VALUE;

    ma = sensors_motor_raw_to_ma(readings.motor_raw);
    chars_ma = stru16(ma);
    chars_tmp_type2 = stru16(readings.tmp_type2_raw);
    chars_tmp_rcd = stru16(readings.tmp_rcd_raw);
    chars_v_ng = strs32(sensors_v_ng_raw_to_mv(readings.v_ng_raw));
    chars_v_l1l2 = strs32(sensors_v_l1l2_raw_to_mv(readings.v_l1l2_raw));
    chars_bcd = stru16((u16)bcd_value);
    bcd_bits_text(bcd_value, bcd_bits);

    gfx_draw_text(SENSOR_VALUE_TEXT_X, MTR_SENSE_TEXT_Y, chars_ma.text,
            Font_7x10, ST_BLACK, ST_YELLOW, MTR_SENSE_DIGITS);
    gfx_draw_text(SENSOR_VALUE_TEXT_X, TMP_TYPE2_TEXT_Y, chars_tmp_type2.text,
            Font_7x10, ST_BLACK, ST_YELLOW, TMP_RAW_DIGITS);
    gfx_draw_text(SENSOR_VALUE_TEXT_X, TMP_RCD_TEXT_Y, chars_tmp_rcd.text,
            Font_7x10, ST_BLACK, ST_YELLOW, TMP_RAW_DIGITS);
    gfx_draw_text(SENSOR_VALUE_TEXT_X, V_NG_TEXT_Y, chars_v_ng.text,
            Font_7x10, ST_BLACK, ST_YELLOW, SENSOR_VOLT_DIGITS);
    gfx_draw_text(SENSOR_VALUE_TEXT_X, V_L1L2_TEXT_Y, chars_v_l1l2.text,
            Font_7x10, ST_BLACK, ST_YELLOW, SENSOR_VOLT_DIGITS);
    gfx_draw_text(BCD_VALUE_TEXT_X, BCD_VALUE_TEXT_Y, chars_bcd.text,
            Font_7x10, ST_BLACK, ST_YELLOW, BCD_VALUE_DIGITS);
    gfx_draw_text(BCD_VALUE_TEXT_X, BCD_BITS_TEXT_Y, bcd_bits,
            Font_7x10, ST_BLACK, ST_YELLOW, BCD_BITS_DIGITS);

    gfx_plotter_push(&ui->motor_plotter, ma);
    draw_update_indicator(SENSOR_UPD_X, (u16)(MTR_SENSE_TEXT_Y + 1u),
            SENSOR_UPD_SIZE, ui->blink, page->bg);
    draw_update_indicator(SENSOR_UPD_X, (u16)(TMP_TYPE2_TEXT_Y + 1u),
            SENSOR_UPD_SIZE, ui->blink, page->bg);
    draw_update_indicator(SENSOR_UPD_X, (u16)(TMP_RCD_TEXT_Y + 1u),
            SENSOR_UPD_SIZE, ui->blink, page->bg);
    draw_update_indicator(SENSOR_UPD_X, (u16)(V_NG_TEXT_Y + 1u),
            SENSOR_UPD_SIZE, ui->blink, page->bg);
    draw_update_indicator(SENSOR_UPD_X, (u16)(V_L1L2_TEXT_Y + 1u),
            SENSOR_UPD_SIZE, ui->blink, page->bg);
}

static const char *test_status_fpga_upload(void)
{
    return "IDLE";
}

static const char *test_status_comms(void)
{
    return "IDLE";
}

static const char *test_status_relay(void)
{
    return "IDLE";
}

static void page_draw_test_status(const TestPageRow *row, u16 y)
{
    const char *status;

    status = "UNKNOWN";
    if ((row != NULL) && (row->status != NULL)) {
        status = row->status();
    }

    gfx_draw_text(TEST_STATUS_X, y, status, Font_7x10,
            ST_BLACK, ST_YELLOW, TEST_STATUS_DIGITS);
}

static void page_enter_test_page(AppUi *ui, const AppPage *page)
{
    const TestPageData *data;
    u8 i;
    u16 y;

    ui->blink = 0u;
    data = (const TestPageData *)page->data;

    st_fill_color(page->bg);
    gfx_draw_string(TEST_TITLE_X, TEST_TITLE_Y, page->title, Font_16x26,
            ST_YELLOW, page->bg);

    if (data == NULL) {
        return;
    }

    for (i = 0u; i < data->row_count; i++) {
        y = (u16)(TEST_ROW_FIRST_Y + ((u16)i * TEST_ROW_STEP_Y));
        gfx_draw_string(TEST_LABEL_X, y, data->rows[i].label, Font_7x10,
                ST_WHITE, page->bg);
        page_draw_test_status(&data->rows[i], y);
    }
}

static void page_tick_test_page(AppUi *ui, const AppPage *page)
{
    const TestPageData *data;
    u8 i;
    u16 y;

    ui->blink = ui->blink ? 0u : 1u;
    draw_update_indicator(TEST_UPD_X, TEST_UPD_Y,
            TEST_UPD_SIZE, ui->blink, page->bg);

    data = (const TestPageData *)page->data;
    if (data == NULL) {
        return;
    }

    for (i = 0u; i < data->row_count; i++) {
        y = (u16)(TEST_ROW_FIRST_Y + ((u16)i * TEST_ROW_STEP_Y));
        page_draw_test_status(&data->rows[i], y);
    }
}

void vDisplayDemoTask(void *pvParameters)
{
    AppUi ui;
    const AppPage *active_page;
    u8 displayed_page_id;
    unused(pvParameters);

    task_hold(50u);
    st_init();
    task_hold(50u);
    gfx_draw_image(80, 80, STARTUP_LOGO_WIDTH, STARTUP_LOGO_HEIGHT, startup_logo);
    task_hold(300u);
    gfx_draw_text(10, 26, "ZapTest 67", Font_16x26, ST_BLUE, ST_BLACK, 11);
    task_hold(600u);

    active_page = NULL;
    displayed_page_id = APP_PAGE_INVALID;
    ui.blink = 0u;

    forever {
        u8 requested_page_id;

        requested_page_id = APP_REQUESTED_PAGE_ID;
        if (requested_page_id != displayed_page_id) {
            active_page = app_page_find(requested_page_id);
            if (active_page == NULL) {
                active_page = app_page_find(APP_PAGE_TEST_HUB);
            }

            displayed_page_id = requested_page_id;
            if ((active_page != NULL) && (active_page->enter != NULL)) {
                active_page->enter(&ui, active_page);
            }
        }

        if ((active_page != NULL) && (active_page->tick != NULL)) {
            active_page->tick(&ui, active_page);
        }

        task_hold(APP_DISPLAY_PERIOD_MS);
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
        SENSOR_VALUES.v_ng_raw = readings.v_ng_raw;
        SENSOR_VALUES.v_l1l2_raw = readings.v_l1l2_raw;
        BCD_SWITCH_VALUE = bcd_switch_read();
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

static void vStartupSoundTask(void *pvParameters)
{
    unused(pvParameters);

    task_hold(APP_STARTUP_SOUND_DELAY_MS);
    uart_play_sound(SOUND_CMD_STARTUP);

    forever {
        task_hold(APP_IDLE_SLEEP_MS);
    }
}

int main(void)
{
    prvInitClock();
    prvInitHardware();

    configASSERT(xTaskCreate(vRGBWTickTask, "RGBW", configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);
    configASSERT(xTaskCreate(vDisplayDemoTask, "Display", DISPLAY_TASK_STACK, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);
    configASSERT(xTaskCreate(vMotorTestTask, "Motor_TEST", configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_MOTOR, NULL) == pdPASS);
    configASSERT(xTaskCreate(vSensorPollTask, "Sensors", configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);
    configASSERT(xTaskCreate(vStartupSoundTask, "SoundInit", configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY_NORMAL, NULL) == pdPASS);

    vTaskStartScheduler();

    forever;
    return 0;
}
