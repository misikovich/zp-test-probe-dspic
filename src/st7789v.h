#ifndef ST7789V_H
#define ST7789V_H

#include "amazing_utils.h"
#include "project_clock.h"

#define ST7789V_DEFAULT_WIDTH       240u
#define ST7789V_DEFAULT_HEIGHT      240u
#define ST7789V_DEFAULT_SPI_HZ      PROJECT_DISPLAY_SPI_HZ

#define ST7789V_BUS_SPI2_PPS        0u
#define ST7789V_BUS_SPI1_FIXED      1u

#define ST7789V_MADCTL_MY           0x80u
#define ST7789V_MADCTL_MX           0x40u
#define ST7789V_MADCTL_MV           0x20u
#define ST7789V_MADCTL_ML           0x10u
#define ST7789V_MADCTL_RGB          0x00u
#define ST7789V_MADCTL_BGR          0x08u
#define ST7789V_MADCTL_MH           0x04u

#define ST7789V_RGB565_BLACK        0x0000u
#define ST7789V_RGB565_WHITE        0xFFFFu
#define ST7789V_RGB565_RED          0xF800u
#define ST7789V_RGB565_GREEN        0x07E0u
#define ST7789V_RGB565_BLUE         0x001Fu

typedef struct {
    Pin sck;
    Pin mosi;
    Pin dc;
    Pin cs;
    Pin reset;
    Pin backlight;

    u8 sck_rp_num;
    u8 mosi_rp_num;
    u8 has_cs;
    u8 has_reset;
    u8 has_backlight;

    u16 width;
    u16 height;
    u16 x_offset;
    u16 y_offset;
    u32 spi_hz;

    u8 bus;
    u8 madctl;
    u8 inversion_on;
} St7789vConfig;

/*
 * SPI2 mode uses PPS: sck_rp_num maps SCK2OUT and mosi_rp_num maps SDO2.
 * SPI1 fixed mode uses the device's fixed SCK1/SDO1 pins and ignores RP nums.
 * CS is software-driven and may be omitted when the panel CS pin is tied low.
 */
u8 st7789v_init(const St7789vConfig *cfg);
void st7789v_deinit(void);
u8 st7789v_is_ready(void);

void st7789v_command(u8 cmd, const u8 *data, u16 len);
void st7789v_set_backlight(u8 on);
void st7789v_set_madctl(u8 madctl);
void st7789v_set_inversion(u8 on);
void st7789v_set_tearing_effect(u8 on, u8 mode);

/* Select a RAM window; follow with st7789v_write_pixels() or st7789v_write_color(). */
void st7789v_set_window(u16 x, u16 y, u16 w, u16 h);

void st7789v_begin_pixels(u16 x, u16 y, u16 w, u16 h);
void st7789v_push_pixels(const u16 *rgb565, u32 count);
void st7789v_push_color(u16 rgb565, u32 count);
void st7789v_end_pixels(void);

void st7789v_write_pixels(const u16 *rgb565, u32 count);
void st7789v_write_color(u16 rgb565, u32 count);
void st7789v_draw_pixel(u16 x, u16 y, u16 rgb565);
void st7789v_fill_rect(u16 x, u16 y, u16 w, u16 h, u16 rgb565);
void st7789v_fill_screen(u16 rgb565);

u16 st7789v_color565(u8 r, u8 g, u8 b);

#endif /* ST7789V_H */
