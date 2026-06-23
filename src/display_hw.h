#ifndef __DISPLAY_HW_H
#define __DISPLAY_HW_H

namespace display
#include "fonts.h"


/*
 * ST7789 HW Configuration &
 * Feature Select
*/

/* SPI port selection */
#define ST7789_SPI_PORT hspi1
extern SPI_HandleTypeDef ST7789_SPI_PORT;

/* Pin connection*/
#define HW_RST_PORT ST7789_RST_GPIO_Port
#define HW_RST_PIN  ST7789_RST_Pin

#define HW_DC_PORT  ST7789_DC_GPIO_Port
#define HW_DC_PIN   ST7789_DC_Pin

/* Uncomment enables CS ctl*/
#define CONF_ENABLE_CS_CTL
#define HW_CS_PORT  ST7789_CS_GPIO_Port
#define HW_CS_PIN   ST7789_CS_Pin

/* Uncomment enables backlight ctl */
#define CONF_ENABLE_BLK_CTL
#define BLK_PORT
#define BLK_PIN

/* Uncomment enables Direct Memory Access */
//#define USE_DMA


/*
 * Display Configuration
 * 240x240 (1.3inch)
 * X_SHIFT & Y_SHIFT are used to adapt different display's resolution
*/

#define CONF_WIDTH 240
#define CONF_HEIGHT 240
/* Display rotation (0-3) */
#define CONF_ROTATION 0
//#define CONF_ROTATION 1
//#define CONF_ROTATION 2				// 240x240 normal
//#define CONF_ROTATION 3

#if CONF_ROTATION == 0
    #define X_SHIFT 0
    #define Y_SHIFT 80
#elif CONF_ROTATION == 1
    #define X_SHIFT 80
    #define Y_SHIFT 0
#elif CONF_ROTATION == 2
    #define X_SHIFT 0
    #define Y_SHIFT 0
#elif CONF_ROTATION == 3
    #define X_SHIFT 0
    #define Y_SHIFT 0
#endif


/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */

#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define GRAY        0X8430
#define BRED        0XF81F
#define GRED        0XFFE0
#define GBLUE       0X07FF
#define BROWN       0XBC40
#define BRRED       0XFC07
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458

#define LIGHTGREEN  0X841F
#define LGRAY       0XC618
#define LGRAYBLUE   0XA651
#define LBBLUE      0X2B12

/* Control Registers and constant codes */
#define CMD_NOP     0x00
#define CMD_SWRESET 0x01
#define CMD_RDDID   0x04
#define CMD_RDDST   0x09

#define CMD_SLPIN   0x10
#define CMD_SLPOUT  0x11
#define CMD_PTLON   0x12
#define CMD_NORON   0x13

#define CMD_INVOFF  0x20
#define CMD_INVON   0x21
#define CMD_DISPOFF 0x28
#define CMD_DISPON  0x29
#define CMD_CASET   0x2A
#define CMD_RASET   0x2B
#define CMD_RAMWR   0x2C
#define CMD_RAMRD   0x2E

#define CMD_PTLAR   0x30
#define CMD_COLMOD  0x3A
#define CMD_MADCTL  0x36

/* Advanced options */
#define CMD_COLOR_MODE_18bit 0x66    //  RGB666 (18bit)
#define CMD_COLOR_MODE_16bit 0x55    //  RGB565 (16bit)

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST7789_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST7789_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST7789_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST7789_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD


/* Basic operations */
#define OP_RST_CLR() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define OP_RST_SET() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)

#define OP_DC_CLR() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define OP_DC_SET() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)

#ifdef CONF_ENABLE_CS_CTL
    #define OP_SELECT() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
    #define OP_UNSELECT() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)
#else
    #define OP_SELECT() asm("nop")
    #define OP_UNSELECT() asm("nop")
#endif

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* Basic functions. */
void display_init(void);
void display_set_rotation(uint8_t m);
void display_fill_color(uint16_t color);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void display_draw_pixel_4px(uint16_t x, uint16_t y, uint16_t color);

/* Command functions */
void display_tear(bool tear);

/* Graphical functions. */
void gfx_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gfx_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void gfx_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void gfx_invert_colors(uint8_t invert);

/* Text functions. */
void gfx_draw_Char(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
void gfx_draw_string(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);

/* Extented Graphical functions. */
void gfx_draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gfx_draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void gfx_draw_filled_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void gfx_draw_filled_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/* Simple test function. */
void display_perform_test(void);

#ifndef CONF_ROTATION
    #error CONF_ROTATION must be selected! (display_hw.h)
#endif

#endif