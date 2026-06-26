#include "display_hw.h"
#include "FreeRTOS.h"
#include "task.h"
#include "trollface_image.h"

static void st_delay_ms(u16 ms)
{
	task_hold(ms);
}

static void st_gpio_init(void)
{
	gpio_init_dw(&ST_HW_SCK);
	gpio_init_dw(&ST_HW_MOSI);
	gpio_init_dr(&ST_HW_MISO);
	gpio_init_dr(&ST_HW_TE);
	gpio_init_dw(&ST_HW_RST);
	gpio_init_dw(&ST_HW_DC);
	#ifdef ST_CONF_ENABLE_CS_CTL
		gpio_init_dw(&ST_HW_CS);
		ST_OP_UNSELECT();
	#endif
	#ifdef ST_CONF_ENABLE_BLK_CTL
		gpio_init_dw(&ST_HW_BLK);
		gpio_dw(&ST_HW_BLK, 1u);
	#endif
}

/* SPI */
static void st_spi_init(void)
{
    volatile u16 dummy;

    PMD1bits.SPI1MD = 0u;      /* Enable SPI1 module clock */

    SPI1STATbits.SPIEN = 0u;   /* Disable before config */
    SPI1CON1 = 0u;
    SPI1CON2 = 0u;
    SPI1STAT = 0u;
    dummy = SPI1BUF;
    unused(dummy);

    SPI1CON1bits.MSTEN = 1u;   /* Master mode */
    SPI1CON1bits.CKP = 0u;     /* Idle clock low */
    SPI1CON1bits.CKE = 1u;     /* Data changes active-to-idle */
    SPI1CON1bits.SMP = 0u;
    SPI1CON1bits.MODE16 = 0u;  /* 8-bit transfers */
    SPI1CON1bits.SSEN = 0u;    /* CS controlled by GPIO */
    SPI1CON1bits.DISSDO = 0u;
    SPI1CON1bits.DISSCK = 0u;

    SPI1CON1bits.PPRE = 3u;    /* Primary prescale 1:1 */
    SPI1CON1bits.SPRE = 6u;    /* Secondary prescale 2:1 */

    SPI1STATbits.SPIROV = 0u;
    SPI1STATbits.SPIEN = 1u;
}

static void st_spi_drop_rx(void)
{
    volatile u16 dummy;

    while (SPI1STATbits.SPIRBF) {
        dummy = SPI1BUF;
    }
    unused(dummy);

    if (SPI1STATbits.SPIROV) {
        SPI1STATbits.SPIROV = 0u;
    }
}

static void st_spi_write_u8(u8 value)
{
    volatile u16 dummy;

    while (SPI1STATbits.SPITBF) {
    }

    SPI1BUF = (u16)value;

    while (!SPI1STATbits.SPIRBF) {
    }

    dummy = SPI1BUF;
    unused(dummy);

    if (SPI1STATbits.SPIROV) {
        SPI1STATbits.SPIROV = 0u;
    }
}

/*
 * Streaming SPI write primitives.
 *
 * These let a caller assert CS + set DC once and push many bytes without
 * re-toggling CS/DC per byte (the big win for text and address windows).
 *
 * Each byte uses the full handshake: wait for TX room, write, wait for RX,
 * drain RX. It is less glamorous than DMA, but it is the known-good path on
 * the live panel and avoids SPIROV stalls.
 *
 * Usage: st_spi_stream_begin() -> st_spi_stream_byte() xN -> st_spi_stream_flush().
 * Not reentrant: the display is driven from a single task and CS is shared.
 */
static void st_spi_stream_begin(void)
{
    st_spi_drop_rx();
}

static void st_spi_stream_byte(u8 value)
{
    volatile u16 dummy;

    while (SPI1STATbits.SPITBF) {
    }
    SPI1BUF = (u16)value;

    while (!SPI1STATbits.SPIRBF) {
    }
    dummy = SPI1BUF;
    unused(dummy);
}

static void st_spi_stream_flush(void)
{
    if (SPI1STATbits.SPIROV) {
        SPI1STATbits.SPIROV = 0u;
    }
}

static void st_spi_write(const u8 *data, u32 len)
{
    if ((data == NULL) || (len == 0UL)) {
        return;
    }

    st_spi_stream_begin();
    while (len > 0UL) {
        st_spi_stream_byte(*data);
        data++;
        len--;
    }
    st_spi_stream_flush();
}

#ifdef ST_CONF_USE_SPI16_COLOR_FILL
static void st_spi_set_mode16(u8 mode16)
{
	if (SPI1CON1bits.MODE16 != mode16) {
		while (!SPI1STATbits.SRMPT) {
		}
		SPI1STATbits.SPIEN = 0u;
		SPI1CON1bits.MODE16 = mode16;
		SPI1STATbits.SPIROV = 0u;
		SPI1STATbits.SPIEN = 1u;
	}
}
#endif

static void st_spi_write_color(u16 color, u32 count)
{
	#ifdef ST_CONF_USE_SPI16_COLOR_FILL
	volatile u16 dummy;

	st_spi_set_mode16(1u);

	while (count > 0UL) {
		count--;

		while (SPI1STATbits.SPITBF) {
		}
		SPI1BUF = color;
		while (!SPI1STATbits.SPIRBF) {
		}
		dummy = SPI1BUF;
	}

	st_spi_set_mode16(0u);
	unused(dummy);
	#else
	u8 hi;
	u8 lo;

    if (count == 0UL) {
        return;
    }

	hi = (u8)(color >> 8u);
	lo = (u8)(color & 0x00FFu);

    st_spi_stream_begin();
    while (count > 0UL) {
        count--;
        st_spi_stream_byte(hi);
        st_spi_stream_byte(lo);
    }
    st_spi_stream_flush();
	#endif
}

static void st_pixel_stream_reset(void)
{
    st_spi_stream_begin();
}

static void st_pixel_stream_flush(void)
{
    st_spi_stream_flush();
}

static void st_pixel_stream_put(u16 color)
{
    st_spi_stream_byte((u8)(color >> 8u));
    st_spi_stream_byte((u8)(color & 0x00FFu));
}


/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void st_write_command(u8 cmd)
{
	ST_OP_SELECT();
	ST_OP_DC_CLR();
	st_spi_write_u8(cmd);
	ST_OP_UNSELECT();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void st_write_data(const u8 *buff, u32 buff_size)
{
	ST_OP_SELECT();
	ST_OP_DC_SET();

	st_spi_write(buff, buff_size);

	ST_OP_UNSELECT();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void st_write_small_data(u8 data)
{
	ST_OP_SELECT();
	ST_OP_DC_SET();
	st_spi_write_u8(data);
	ST_OP_UNSELECT();
}

static void st_write_command_data(u8 cmd, const u8 *data, u16 len)
{
    ST_OP_SELECT();
    ST_OP_DC_CLR();
    st_spi_write_u8(cmd);
    if ((data != NULL) && (len > 0u)) {
        ST_OP_DC_SET();
        st_spi_write(data, len);
    }
    ST_OP_UNSELECT();
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in display_hw.h)
 * @return none
 */
void st_set_rotation(uint8_t m)
{
    u8 madctl;

	switch (m) {
	case 0:
		madctl = ST_MADCTL_MX | ST_MADCTL_MY | ST_MADCTL_RGB;
		break;
	case 1:
		madctl = ST_MADCTL_MY | ST_MADCTL_MV | ST_MADCTL_RGB;
		break;
	case 2:
		madctl = ST_MADCTL_RGB;
		break;
	case 3:
		madctl = ST_MADCTL_MX | ST_MADCTL_MV | ST_MADCTL_RGB;
		break;
	default:
		return;
	}

    st_write_command_data(ST_CMD_MADCTL, &madctl, 1u);
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void st_set_address_window_selected(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	uint16_t x_start = x0 + ST_X_SHIFT, x_end = x1 + ST_X_SHIFT;
	uint16_t y_start = y0 + ST_Y_SHIFT, y_end = y1 + ST_Y_SHIFT;
	uint8_t caset[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
	uint8_t raset[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};

	ST_OP_DC_CLR();
	st_spi_write_u8(ST_CMD_CASET);
	ST_OP_DC_SET();
	st_spi_write(caset, sizeof(caset));

	ST_OP_DC_CLR();
	st_spi_write_u8(ST_CMD_RASET);
	ST_OP_DC_SET();
	st_spi_write(raset, sizeof(raset));

	ST_OP_DC_CLR();
	st_spi_write_u8(ST_CMD_RAMWR);
}

static void st_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST_OP_SELECT();
    st_set_address_window_selected(x0, y0, x1, y1);
	ST_OP_UNSELECT();
}

static void st_begin_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST_OP_SELECT();
    st_set_address_window_selected(x0, y0, x1, y1);
    ST_OP_DC_SET();
}

static void st_end_pixels(void)
{
    st_spi_stream_flush();
	ST_OP_UNSELECT();
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void st_init(void)
{
	st_gpio_init();
	st_spi_init();
	st_delay_ms(10u);
    ST_OP_RST_CLR();
    st_delay_ms(10u);
    ST_OP_RST_SET();
    st_delay_ms(20u);

    st_write_command(ST_CMD_COLMOD);		/* Set color mode */
    st_write_small_data(ST_CMD_COLOR_MODE_16bit);
	st_write_command(0xB2);				/* Porch control */
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		st_write_data(data, sizeof(data));
	}
	st_set_rotation(ST_CONF_ROTATION);	/* MADCTL (Display Rotation) */
	
	/* Internal LCD Voltage generator settings */
    st_write_command(0XB7);				/* Gate Control */
    st_write_small_data(0x35);			/* Default value */
    st_write_command(0xBB);				/* VCOM setting */
    st_write_small_data(0x19);			/* 0.725v (default 0.75v for 0x20) */
    st_write_command(0xC0);				/* LCMCTRL */
    st_write_small_data (0x2C);			/* Default value */
    st_write_command (0xC2);				/* VDV and VRH command Enable */
    st_write_small_data (0x01);			/* Default value */
    st_write_command (0xC3);				/* VRH set */
    st_write_small_data (0x12);			/* +-4.45v (defalut +-4.1v for 0x0B) */
    st_write_command (0xC4);				/* VDV set */
    st_write_small_data (0x20);			/* Default value */
    st_write_command (0xC6);				/* Frame rate control in normal mode */
    st_write_small_data (0x0F);			/* Default value (60HZ) */
    st_write_command (0xD0);				/* Power control */
    st_write_small_data (0xA4);			/* Default value */
    st_write_small_data (0xA1);			/* Default value */
	/**************** Division line ****************/

	st_write_command(0xE0);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		st_write_data(data, sizeof(data));
	}

    st_write_command(0xE1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		st_write_data(data, sizeof(data));
	}
    st_write_command (ST_CMD_INVON);		/* Inversion ON */
	st_write_command (ST_CMD_SLPOUT);	/* Out of sleep mode */
	st_write_command (ST_CMD_NORON);		/* Normal Display on */
	st_write_command (ST_CMD_DISPON);	/* Main screen turned on */

	st_delay_ms(50u);
	st_fill_color(ST_BLACK);				/* Fill with Black. */
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void st_fill_color(uint16_t color)
{
	st_begin_pixels(0, 0, ST_CONF_WIDTH - 1, ST_CONF_HEIGHT - 1);
	st_spi_write_color(color, (u32)ST_CONF_WIDTH * (u32)ST_CONF_HEIGHT);
	st_end_pixels();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void st_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= ST_CONF_WIDTH) || (y >= ST_CONF_HEIGHT)) {
        return;
    }
	
    st_begin_pixels(x, y, x, y);
    st_spi_write_color(color, 1UL);
    st_end_pixels();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void st_fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	u32 count;
    uint16_t swap;

    if (xSta > xEnd) {
        swap = xSta;
        xSta = xEnd;
        xEnd = swap;
    }
    if (ySta > yEnd) {
        swap = ySta;
        ySta = yEnd;
        yEnd = swap;
    }

    if ((xSta >= ST_CONF_WIDTH) || (ySta >= ST_CONF_HEIGHT)) {
        return;
    }

    if (xEnd >= ST_CONF_WIDTH) {
        xEnd = ST_CONF_WIDTH - 1u;
    }
    if (yEnd >= ST_CONF_HEIGHT) {
        yEnd = ST_CONF_HEIGHT - 1u;
    }

	count = ((u32)xEnd - (u32)xSta + 1UL) * ((u32)yEnd - (u32)ySta + 1UL);
	st_begin_pixels(xSta, ySta, xEnd, yEnd);
	st_spi_write_color(color, count);
	st_end_pixels();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void st_draw_pixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x == 0u) || (x >= (ST_CONF_WIDTH - 1u)) ||
		 (y == 0u) || (y >= (ST_CONF_HEIGHT - 1u)))	return;
	st_fill(x - 1, y - 1, x + 1, y + 1, color);	/* st_fill manages CS */
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void gfx_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        uint16_t color) {
	int16_t ix0;
    int16_t iy0;
    int16_t ix1;
    int16_t iy1;
    int16_t swap;
    int16_t steep;
    int16_t span_start;
    int16_t span_axis;
    int16_t dx;
    int16_t dy;
    int16_t err;
    int16_t ystep;

    ix0 = (int16_t)x0;
    iy0 = (int16_t)y0;
    ix1 = (int16_t)x1;
    iy1 = (int16_t)y1;

    /* Axis-aligned fast paths: stream the run as a fill instead of
     * setting a fresh address window per pixel. */
    if (iy0 == iy1) {
        if (ix0 <= ix1) st_fill((u16)ix0, (u16)iy0, (u16)ix1, (u16)iy1, color);
        else            st_fill((u16)ix1, (u16)iy0, (u16)ix0, (u16)iy1, color);
        return;
    }
    if (ix0 == ix1) {
        if (iy0 <= iy1) st_fill((u16)ix0, (u16)iy0, (u16)ix1, (u16)iy1, color);
        else            st_fill((u16)ix0, (u16)iy1, (u16)ix1, (u16)iy0, color);
        return;
    }

    steep = ABS(iy1 - iy0) > ABS(ix1 - ix0);
    if (steep) {
		swap = ix0;
		ix0 = iy0;
		iy0 = swap;

		swap = ix1;
		ix1 = iy1;
		iy1 = swap;
        /* _swap_int16_t(x0, y0); */
        /* _swap_int16_t(x1, y1); */
    }

    if (ix0 > ix1) {
		swap = ix0;
		ix0 = ix1;
		ix1 = swap;

		swap = iy0;
		iy0 = iy1;
		iy1 = swap;
        /* _swap_int16_t(x0, x1); */
        /* _swap_int16_t(y0, y1); */
    }

    dx = ix1 - ix0;
    dy = ABS(iy1 - iy0);
    err = dx / 2;

    if (iy0 < iy1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    span_start = ix0;
    span_axis = iy0;

    for (; ix0 <= ix1; ix0++) {
        err -= dy;
        if (err < 0) {
            if (steep) {
                st_fill((u16)span_axis, (u16)span_start, (u16)span_axis, (u16)ix0, color);
            } else {
                st_fill((u16)span_start, (u16)span_axis, (u16)ix0, (u16)span_axis, color);
            }
            iy0 = (int16_t)(iy0 + ystep);
            err += dx;
            span_start = (int16_t)(ix0 + 1);
            span_axis = iy0;
        }
    }

    if (span_start <= ix1) {
        if (steep) {
            st_fill((u16)span_axis, (u16)span_start, (u16)span_axis, (u16)ix1, color);
        } else {
            st_fill((u16)span_start, (u16)span_axis, (u16)ix1, (u16)span_axis, color);
        }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void gfx_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	/* Each gfx_draw_line manages its own CS. */
	gfx_draw_line(x1, y1, x2, y1, color);
	gfx_draw_line(x1, y1, x1, y2, color);
	gfx_draw_line(x1, y2, x2, y2, color);
	gfx_draw_line(x2, y1, x2, y2, color);
}

/** 
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void gfx_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	/* st_draw_pixel manages its own CS. */
	st_draw_pixel(x0, y0 + r, color);
	st_draw_pixel(x0, y0 - r, color);
	st_draw_pixel(x0 + r, y0, color);
	st_draw_pixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		st_draw_pixel(x0 + x, y0 + y, color);
		st_draw_pixel(x0 - x, y0 + y, color);
		st_draw_pixel(x0 + x, y0 - y, color);
		st_draw_pixel(x0 - x, y0 - y, color);

		st_draw_pixel(x0 + y, y0 + x, color);
		st_draw_pixel(x0 - y, y0 + x, color);
		st_draw_pixel(x0 + y, y0 - x, color);
		st_draw_pixel(x0 - y, y0 - x, color);
	}
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void gfx_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
	if ((x >= ST_CONF_WIDTH) || (y >= ST_CONF_HEIGHT))
		return;
    if ((data == NULL) || (w == 0u) || (h == 0u))
        return;
	if ((x + w - 1) >= ST_CONF_WIDTH)
		return;
	if ((y + h - 1) >= ST_CONF_HEIGHT)
		return;

	/* st_set_address_window and st_write_data each manage CS. */
	st_set_address_window(x, y, x + w - 1, y + h - 1);
	st_write_data((const u8 *)data, (u32)sizeof(u16) * (u32)w * (u32)h);
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void gfx_invert_colors(uint8_t invert)
{
	/* st_write_command manages CS. */
	st_write_command(invert ? ST_CMD_INVON : ST_CMD_INVOFF);
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void gfx_draw_char(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
	uint32_t i, b, j;

    if ((x >= ST_CONF_WIDTH) || (y >= ST_CONF_HEIGHT) ||
            (font.width == 0u) || (font.height == 0u)) {
        return;
    }
    if (((u32)x + (u32)font.width) > (u32)ST_CONF_WIDTH) {
        return;
    }
    if (((u32)y + (u32)font.height) > (u32)ST_CONF_HEIGHT) {
        return;
    }
    if ((ch < 32) || (ch > 126)) {
        ch = '?';
    }

	st_begin_pixels(x, y, x + font.width - 1u, y + font.height - 1u);

	/* Stream the whole glyph under a single CS assertion instead of
	 * toggling CS + DC and stalling per pixel (was 2 SPI round-trips and
	 * 2 GPIO pairs for every pixel). */
	st_pixel_stream_reset();

	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				st_pixel_stream_put(color);
			}
			else {
				st_pixel_stream_put(bgcolor);
			}
		}
	}

	st_pixel_stream_flush();
	st_end_pixels();
}

static void gfx_draw_string_run(uint16_t x, uint16_t y, const char *str,
        uint16_t len, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint16_t row;
    uint16_t col;
    uint16_t idx;
    uint16_t run_width;
    uint16_t bits;
    char ch;

    if (len == 0u) {
        return;
    }

    run_width = (uint16_t)(len * (uint16_t)font.width);
    st_begin_pixels(x, y, (uint16_t)(x + run_width - 1u),
            (uint16_t)(y + (uint16_t)font.height - 1u));
    st_pixel_stream_reset();

    for (row = 0u; row < (uint16_t)font.height; row++) {
        for (idx = 0u; idx < len; idx++) {
            ch = str[idx];
            if ((ch < 32) || (ch > 126)) {
                ch = '?';
            }
            bits = font.data[((uint16_t)(ch - 32) * (uint16_t)font.height) + row];
            for (col = 0u; col < (uint16_t)font.width; col++) {
                if ((bits << col) & 0x8000u) {
                    st_pixel_stream_put(color);
                } else {
                    st_pixel_stream_put(bgcolor);
                }
            }
        }
    }

    st_pixel_stream_flush();
    st_end_pixels();
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void gfx_draw_string(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint16_t chars_fit;
    uint16_t run_len;
    uint16_t run_width;

    if ((str == NULL) || (font.width == 0u) || (font.height == 0u)) {
        return;
    }

	while (*str) {
		if (((u32)x + (u32)font.width) > (u32)ST_CONF_WIDTH) {
			x = 0;
			y += font.height;
			if (((u32)y + (u32)font.height) > (u32)ST_CONF_HEIGHT) {
				break;
			}

			if (*str == ' ') {
				/* skip spaces in the beginning of the new line */
				str++;
				continue;
			}
		}

        chars_fit = (uint16_t)(((u32)ST_CONF_WIDTH - (u32)x) / (u32)font.width);
        run_len = 0u;
        while ((str[run_len] != '\0') && (run_len < chars_fit)) {
            run_len++;
        }
        if (run_len == 0u) {
            break;
        }

        gfx_draw_string_run(x, y, str, run_len, font, color, bgcolor);
        run_width = (uint16_t)(run_len * (uint16_t)font.width);
		x = (uint16_t)(x + run_width);
		str += run_len;
	}
}

/**
 * @brief Write text and optionally reserve a fixed-width background field.
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string and reserved field
 * @param reserve_chars -> minimum number of character cells to clear first
 * @return  none
 */
void gfx_draw_text(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor, uint16_t reserve_chars)
{
    uint16_t chars_fit;
    uint16_t clear_chars;

    if ((str == NULL) || (font.width == 0u) || (font.height == 0u)) {
        return;
    }
    if ((x >= ST_CONF_WIDTH) || (y >= ST_CONF_HEIGHT)) {
        return;
    }
    if (((u32)y + (u32)font.height) > (u32)ST_CONF_HEIGHT) {
        return;
    }

    if (reserve_chars != 0u) {
        chars_fit = (uint16_t)(((u32)ST_CONF_WIDTH - (u32)x) / (u32)font.width);
        clear_chars = reserve_chars;
        if (clear_chars > chars_fit) {
            clear_chars = chars_fit;
        }
        if (clear_chars != 0u) {
            gfx_draw_filled_rectangle(x, y,
                    (uint16_t)(clear_chars * (uint16_t)font.width),
                    font.height, bgcolor);
        }
    }

    gfx_draw_string(x, y, str, font, color, bgcolor);
}

/** 
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void gfx_draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	/* Check input parameters */
	if ((x >= ST_CONF_WIDTH) || (y >= ST_CONF_HEIGHT) ||
        (w == 0u) || (h == 0u)) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= ST_CONF_WIDTH) {
		w = ST_CONF_WIDTH - x;
	}
	if ((y + h) >= ST_CONF_HEIGHT) {
		h = ST_CONF_HEIGHT - y;
	}

	st_fill(x, y, (uint16_t)(x + w - 1u), (uint16_t)(y + h - 1u), color);
}

/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void gfx_draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	/* Each gfx_draw_line manages its own CS. */
	gfx_draw_line(x1, y1, x2, y2, color);
	gfx_draw_line(x2, y2, x3, y3, color);
	gfx_draw_line(x3, y3, x1, y1, color);
}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void gfx_draw_filled_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	/* gfx_draw_line manages CS per span. */
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	}
	else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	}
	else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		gfx_draw_line(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void gfx_draw_filled_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	/* st_draw_pixel / gfx_draw_line manage CS. */
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	st_draw_pixel(x0, y0 + r, color);
	st_draw_pixel(x0, y0 - r, color);
	st_draw_pixel(x0 + r, y0, color);
	st_draw_pixel(x0 - r, y0, color);
	gfx_draw_line(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		gfx_draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
		gfx_draw_line(x0 + x, y0 - y, x0 - x, y0 - y, color);

		gfx_draw_line(x0 + y, y0 + x, x0 - y, y0 + x, color);
		gfx_draw_line(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void st_tear(bool tear)
{
	/* st_write_command manages CS. */
	st_write_command(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
}

static void st_draw_test_image(u16 x0, u16 y0)
{
	u8 x;
	u8 y;
	u16 color;
	const u16 scale = 4u;

	for (y = 0u; y < TROLLFACE_IMAGE_HEIGHT; y++) {
		for (x = 0u; x < TROLLFACE_IMAGE_WIDTH; x++) {
			color = trollface_image_pixel(x, y) ? ST_BLACK : ST_WHITE;
			gfx_draw_filled_rectangle((u16)(x0 + ((u16)x * scale)),
				(u16)(y0 + ((u16)y * scale)), scale, scale, color);
		}
	}
}


/** 
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void st_perform_test(void)
{
	st_fill_color(ST_WHITE);
	gfx_draw_string(10, 20, "Speed Test", Font_11x18, ST_RED, ST_WHITE);
	st_delay_ms(1000u);
	st_fill_color(ST_CYAN);
	st_delay_ms(100u);
	st_fill_color(ST_RED);
	st_delay_ms(100u);
	st_fill_color(ST_BLUE);
	st_delay_ms(100u);
	st_fill_color(ST_GREEN);
	st_delay_ms(100u);
	st_fill_color(ST_YELLOW);
	st_delay_ms(100u);
	st_fill_color(ST_BROWN);
	st_delay_ms(100u);
	st_fill_color(ST_DARKBLUE);
	st_delay_ms(100u);
	st_fill_color(ST_MAGENTA);
	st_delay_ms(100u);
	st_fill_color(ST_LIGHTGREEN);
	st_delay_ms(100u);
	st_fill_color(ST_LGRAY);
	st_delay_ms(100u);
	st_fill_color(ST_LBBLUE);
	st_delay_ms(100u);
	st_fill_color(ST_WHITE);
	st_delay_ms(100u);
	st_fill_color(ST_DARKBLUE);

	gfx_draw_string(10, 10, "Font test.", Font_16x26, ST_GBLUE, ST_DARKBLUE);
	gfx_draw_string(10, 50, "Hello Steve!", Font_7x10, ST_RED, ST_DARKBLUE);
	gfx_draw_string(10, 75, "Hello Steve!", Font_11x18, ST_YELLOW, ST_DARKBLUE);
	gfx_draw_string(10, 100, "Hello Steve!", Font_16x26, ST_MAGENTA, ST_DARKBLUE);
	st_delay_ms(2000u);

	st_fill_color(ST_RED);
	gfx_draw_string(10, 10, "Rect./Line.", Font_11x18, ST_YELLOW, ST_BLACK);
	gfx_draw_rectangle(30, 30, 100, 100, ST_WHITE);
	st_delay_ms(1000u);

	st_fill_color(ST_RED);
	gfx_draw_string(10, 10, "Filled Rect.", Font_11x18, ST_YELLOW, ST_BLACK);
	gfx_draw_filled_rectangle(30, 30, 50, 50, ST_WHITE);
	st_delay_ms(1000u);

	st_fill_color(ST_RED);
	gfx_draw_string(10, 10, "Circle.", Font_11x18, ST_YELLOW, ST_BLACK);
	gfx_draw_circle(60, 60, 25, ST_WHITE);
	st_delay_ms(1000u);

	st_fill_color(ST_RED);
	gfx_draw_string(10, 10, "Filled Cir.", Font_11x18, ST_YELLOW, ST_BLACK);
	gfx_draw_filled_circle(60, 60, 25, ST_WHITE);
	st_delay_ms(1000u);

	st_fill_color(ST_RED);
	gfx_draw_string(10, 10, "Triangle", Font_11x18, ST_YELLOW, ST_BLACK);
	gfx_draw_triangle(30, 30, 30, 70, 60, 40, ST_WHITE);
	st_delay_ms(1000u);

	st_fill_color(ST_RED);
	gfx_draw_string(10, 10, "Filled Tri", Font_11x18, ST_YELLOW, ST_BLACK);
	gfx_draw_filled_triangle(30, 30, 30, 70, 60, 40, ST_WHITE);
	st_delay_ms(1000u);

	/* If FLASH cannot storage anymore datas, please delete codes below. */
	st_fill_color(ST_WHITE);
	st_draw_test_image(0u, 0u);
	st_delay_ms(3000u);
}
