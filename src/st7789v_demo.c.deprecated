#include <xc.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "st7789v.h"
#include "st7789v_demo.h"
#include "trollface_image.h"

#define DISPLAY_WIDTH              240u
#define DISPLAY_HEIGHT             240u
#define DISPLAY_IMAGE_SCALE        5u
#define DISPLAY_IMAGE_X            40u
#define DISPLAY_IMAGE_Y            40u
#define DISPLAY_FACE_X             28u
#define DISPLAY_FACE_Y             28u
#define DISPLAY_FACE_W             184u
#define DISPLAY_FACE_H             184u
#define DISPLAY_TE_MASK            (1u << 9)
#define DISPLAY_TE_TIMEOUT_LOOPS   (PROJECT_FCY_HZ / 400UL)

static const GPIO DISPLAY_CS        = { &LATD, &PORTD, &TRISD, NULL,    1u << 5 };
static const GPIO DISPLAY_SCK       = { &LATC, &PORTC, &TRISC, NULL,    1u << 3 };
static const GPIO DISPLAY_MOSI      = { &LATA, &PORTA, &TRISA, &ANSELA, 1u << 4 };
static const GPIO DISPLAY_MISO      = { &LATA, &PORTA, &TRISA, NULL,    1u << 9 };
static const GPIO DISPLAY_DC        = { &LATA, &PORTA, &TRISA, NULL,    1u << 7 };
static const GPIO DISPLAY_RST       = { &LATC, &PORTC, &TRISC, NULL,    1u << 6 };
static const GPIO DISPLAY_TE        = { &LATG, &PORTG, &TRISG, NULL,    1u << 9 };
static const GPIO DISPLAY_BACKLIGHT = { &LATC, &PORTC, &TRISC, NULL,    1u << 7 };

static u16 s_display_line[DISPLAY_WIDTH];

static void demo_gpio_input(const GPIO *pin)
{
    gpio_init_dr(pin);
}

static u8 display_te_high(void)
{
    return (PORTG & DISPLAY_TE_MASK) ? 1u : 0u;
}

static void display_wait_te(void)
{
    volatile u32 timeout;

    timeout = DISPLAY_TE_TIMEOUT_LOOPS;
    while (display_te_high() && (timeout > 0UL)) {
        timeout--;
    }

    timeout = DISPLAY_TE_TIMEOUT_LOOPS;
    while ((!display_te_high()) && (timeout > 0UL)) {
        timeout--;
    }
}

static u8 display_demo_init(void)
{
    St7789vConfig cfg;

    demo_gpio_input(&DISPLAY_MISO);
    demo_gpio_input(&DISPLAY_TE);

    cfg.sck = DISPLAY_SCK;
    cfg.mosi = DISPLAY_MOSI;
    cfg.dc = DISPLAY_DC;
    cfg.cs = DISPLAY_CS;
    cfg.reset = DISPLAY_RST;
    cfg.backlight = DISPLAY_BACKLIGHT;

    cfg.sck_rp_num = 0u;
    cfg.mosi_rp_num = 0u;
    cfg.has_cs = 1u;
    cfg.has_reset = 1u;
    cfg.has_backlight = 1u;

    cfg.width = DISPLAY_WIDTH;
    cfg.height = DISPLAY_HEIGHT;
    cfg.x_offset = 0u;
    cfg.y_offset = 0u;
    cfg.spi_hz = ST7789V_DEFAULT_SPI_HZ;

    cfg.bus = ST7789V_BUS_SPI1_FIXED;
    cfg.madctl = ST7789V_MADCTL_RGB;
    cfg.inversion_on = 0u;

    if (!st7789v_init(&cfg)) {
        return 0u;
    }

    st7789v_set_tearing_effect(1u, 0u);
    st7789v_set_rotation(1);
    return 1u;
}

static void display_build_line(u16 y, u16 bg, u16 face, u16 ink)
{
    u16 x;
    u16 image_y0;
    u16 image_y1;
    u16 image_x0;
    u16 image_x1;
    u16 image_x;
    u16 pixel_x;
    u16 color;
    u8 image_y;
    u8 col;
    u8 sx;

    for (x = 0u; x < DISPLAY_WIDTH; x++) {
        s_display_line[x] = bg;
    }

    if ((y >= DISPLAY_FACE_Y) && (y < (DISPLAY_FACE_Y + DISPLAY_FACE_H))) {
        for (x = DISPLAY_FACE_X; x < (DISPLAY_FACE_X + DISPLAY_FACE_W); x++) {
            s_display_line[x] = face;
        }
    }

    image_y0 = DISPLAY_IMAGE_Y;
    image_y1 = (u16)(DISPLAY_IMAGE_Y +
                     (TROLLFACE_IMAGE_HEIGHT * DISPLAY_IMAGE_SCALE));
    if ((y < image_y0) || (y >= image_y1)) {
        return;
    }

    image_y = (u8)((y - DISPLAY_IMAGE_Y) / DISPLAY_IMAGE_SCALE);
    image_x0 = DISPLAY_IMAGE_X;
    image_x1 = (u16)(DISPLAY_IMAGE_X +
                     (TROLLFACE_IMAGE_WIDTH * DISPLAY_IMAGE_SCALE));

    for (col = 0u; col < TROLLFACE_IMAGE_WIDTH; col++) {
        color = trollface_image_pixel(col, image_y) ? ink : face;
        image_x = (u16)(image_x0 + ((u16)col * DISPLAY_IMAGE_SCALE));
        for (sx = 0u; sx < DISPLAY_IMAGE_SCALE; sx++) {
            pixel_x = (u16)(image_x + sx);
            if ((pixel_x >= image_x0) && (pixel_x < image_x1) &&
                (pixel_x < DISPLAY_WIDTH)) {
                s_display_line[pixel_x] = color;
            }
        }
    }
}

static void display_demo_frame(u16 bg, u16 face, u16 ink)
{
    u16 y;

    display_wait_te();
    st7789v_begin_pixels(0u, 0u, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (y = 0u; y < DISPLAY_HEIGHT; y++) {
        display_build_line(y, bg, face, ink);
        st7789v_push_pixels(s_display_line, DISPLAY_WIDTH);
    }
    st7789v_end_pixels();
}

void vDisplayDemoTask(void *pvParameters)
{
    unused(pvParameters);

    vTaskDelayMS(50u);

    if (!display_demo_init()) {
        forever {
            vTaskDelayMS(1000u);
        }
    }

    forever {
        display_demo_frame(ST7789V_RGB565_BLUE,
                           ST7789V_RGB565_WHITE,
                           ST7789V_RGB565_BLACK);
        vTaskDelayMS(1200u);

        display_demo_frame(ST7789V_RGB565_BLACK,
                           st7789v_color565(255u, 230u, 70u),
                           ST7789V_RGB565_BLACK);
        vTaskDelayMS(1200u);
    }
}
