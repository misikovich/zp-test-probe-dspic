#ifndef __DISPLAY_HW_H
#define __DISPLAY_HW_H

#include "fonts.h"
#include <stddef.h>


/*
 * ST7789 HW Configuration &
 * Feature Select
*/

/* Pin connection*/

static const GPIO ST_HW_RST = { &LATC, &PORTC, &TRISC, NULL, 1u << 6 };
static const GPIO ST_HW_DC  = { &LATA, &PORTA, &TRISA, NULL, 1u << 7 };
static const GPIO ST_HW_CS  = { &LATD, &PORTD, &TRISD, NULL, 1u << 5 };
static const GPIO ST_HW_SCK = { &LATC, &PORTC, &TRISC, NULL, 1u << 3 };
static const GPIO ST_HW_MOSI = { &LATA, &PORTA, &TRISA, &ANSELA, 1u << 4 };
static const GPIO ST_HW_MISO = { &LATA, &PORTA, &TRISA, NULL, 1u << 9 };
static const GPIO ST_HW_TE  = { &LATG, &PORTG, &TRISG, NULL, 1u << 9 };
static const GPIO ST_HW_BLK = { &LATC, &PORTC, &TRISC, NULL, 1u << 7 };

/* Uncomment enables CS ctl*/
#define ST_CONF_ENABLE_CS_CTL

/* Uncomment enables backlight ctl */
#define ST_CONF_ENABLE_BLK_CTL

/* DMA path disabled: polled SPI is the known-good display path. */
/* #define USE_DMA */

/* Keep RGB565 color fills as explicit high-byte/low-byte SPI writes. */
/* #define ST_CONF_USE_SPI16_COLOR_FILL */

/*
 * Display Configuration
 * 240x240 (1.3inch)
 * X_SHIFT & Y_SHIFT are used to adapt different display's resolution
*/

#define ST_CONF_WIDTH 240
#define ST_CONF_HEIGHT 240

/* Display rotation (0-3) */
/* 240x240 default: 2 */
#define ST_CONF_ROTATION 1

#if ST_CONF_ROTATION == 0
    #define ST_X_SHIFT 0
    #define ST_Y_SHIFT 80
#elif ST_CONF_ROTATION == 1
    #define ST_X_SHIFT 80
    #define ST_Y_SHIFT 0
#elif ST_CONF_ROTATION == 2
    #define ST_X_SHIFT 0
    #define ST_Y_SHIFT 0
#elif ST_CONF_ROTATION == 3
    #define ST_X_SHIFT 0
    #define ST_Y_SHIFT 0
#endif


/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */

#define ST_WHITE       0xFFFF
#define ST_BLACK       0x0000
#define ST_BLUE        0x001F
#define ST_RED         0xF800
#define ST_MAGENTA     0xF81F
#define ST_GREEN       0x07E0
#define ST_CYAN        0x7FFF
#define ST_YELLOW      0xFFE0
#define ST_GRAY        0X8430
#define ST_BRED        0XF81F
#define ST_GRED        0XFFE0
#define ST_GBLUE       0X07FF
#define ST_BROWN       0XBC40
#define ST_BRRED       0XFC07
#define ST_DARKBLUE    0X01CF
#define ST_LIGHTBLUE   0X7D7C
#define ST_GRAYBLUE    0X5458

#define ST_LIGHTGREEN  0X841F
#define ST_LGRAY       0XC618
#define ST_LGRAYBLUE   0XA651
#define ST_LBBLUE      0X2B12

/* Control Registers and constant codes */
#define ST_CMD_NOP     0x00
#define ST_CMD_SWRESET 0x01
#define ST_CMD_RDDID   0x04
#define ST_CMD_RDDST   0x09

#define ST_CMD_SLPIN   0x10
#define ST_CMD_SLPOUT  0x11
#define ST_CMD_PTLON   0x12
#define ST_CMD_NORON   0x13

#define ST_CMD_INVOFF  0x20
#define ST_CMD_INVON   0x21
#define ST_CMD_DISPOFF 0x28
#define ST_CMD_DISPON  0x29
#define ST_CMD_CASET   0x2A
#define ST_CMD_RASET   0x2B
#define ST_CMD_RAMWR   0x2C
#define ST_CMD_RAMRD   0x2E

#define ST_CMD_PTLAR   0x30
#define ST_CMD_COLMOD  0x3A
#define ST_CMD_MADCTL  0x36

/* Advanced options */
#define ST_CMD_COLOR_MODE_18bit 0x66    /* RGB666 (18bit) */
#define ST_CMD_COLOR_MODE_16bit 0x55    /* RGB565 (16bit) */

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST_MADCTL_RGB 0x00

#define ST_RDID1   0xDA
#define ST_RDID2   0xDB
#define ST_RDID3   0xDC
#define ST_RDID4   0xDD


/* Basic operations */
#define ST_OP_RST_CLR() gpio_dw(&ST_HW_RST, 0u)
#define ST_OP_RST_SET() gpio_dw(&ST_HW_RST, 1u)

#define ST_OP_DC_CLR() gpio_dw(&ST_HW_DC, 0u)
#define ST_OP_DC_SET() gpio_dw(&ST_HW_DC, 1u)

#ifdef ST_CONF_ENABLE_CS_CTL
    #define ST_OP_SELECT() gpio_dw(&ST_HW_CS, 0u)
    #define ST_OP_UNSELECT() gpio_dw(&ST_HW_CS, 1u)
#else
    #define ST_OP_SELECT() asm("nop")
    #define ST_OP_UNSELECT() asm("nop")
#endif

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* Basic functions. */
void st_init(void);
void st_set_rotation(uint8_t m);
void st_fill_color(uint16_t color);
void st_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void st_fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void st_draw_pixel_4px(uint16_t x, uint16_t y, uint16_t color);

/* Command functions */
void st_tear(bool tear);

/* Graphical functions. */
void gfx_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void gfx_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void gfx_invert_colors(uint8_t invert);

/* Text functions. */
void gfx_draw_char(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
void gfx_draw_string(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);

/* Extented Graphical functions. */
void gfx_draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gfx_draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void gfx_draw_filled_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void gfx_draw_filled_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/* Simple test function. */
void st_perform_test(void);

#ifndef ST_CONF_ROTATION
    #error ST_CONF_ROTATION must be selected! (display_hw.h)
#endif

#endif