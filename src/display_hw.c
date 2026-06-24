#include "display_hw.h"
#include "FreeRTOS.h"
#include "task.h"
#include "trollface_image.h"

static void st_delay_ms(u16 ms)
{
	vTaskDelayMS(ms);
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
 * Each byte uses the proven full handshake: wait for TX room, write, wait for
 * the byte to fully shift out (SPIRBF), drain RX. This deliberately does NOT
 * try to overlap byte N+1 with byte N: on this part a transient RX overflow
 * halts the SPI module, which hung the bus and blanked the display. Correct
 * and bus-rate-limited beats fast-but-dead.
 *
 * Usage: st_spi_stream_begin() -> st_spi_stream_byte() xN -> st_spi_stream_flush().
 * Not reentrant: the display is driven from a single task and CS is shared.
 */
static void st_spi_stream_begin(void)
{
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

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in display_hw.h)
 * @return none
 */
void st_set_rotation(uint8_t m)
{
	st_write_command(ST_CMD_MADCTL);	/* MADCTL */
	switch (m) {
	case 0:
		st_write_small_data(ST_MADCTL_MX | ST_MADCTL_MY | ST_MADCTL_RGB);
		break;
	case 1:
		st_write_small_data(ST_MADCTL_MY | ST_MADCTL_MV | ST_MADCTL_RGB);
		break;
	case 2:
		st_write_small_data(ST_MADCTL_RGB);
		break;
	case 3:
		st_write_small_data(ST_MADCTL_MX | ST_MADCTL_MV | ST_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void st_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	uint16_t x_start = x0 + ST_X_SHIFT, x_end = x1 + ST_X_SHIFT;
	uint16_t y_start = y0 + ST_Y_SHIFT, y_end = y1 + ST_Y_SHIFT;
	uint8_t caset[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
	uint8_t raset[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};

	/* Hold CS low for the whole CASET/RASET/RAMWR sequence instead of
	 * toggling it around each sub-command (was 3 CS pairs, now 1). */
	ST_OP_SELECT();

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
	st_set_address_window(0, 0, ST_CONF_WIDTH - 1, ST_CONF_HEIGHT - 1);
	ST_OP_SELECT();
	ST_OP_DC_SET();
	st_spi_write_color(color, (u32)ST_CONF_WIDTH * (u32)ST_CONF_HEIGHT);
	ST_OP_UNSELECT();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void st_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x < 0) || (x >= ST_CONF_WIDTH) ||
		 (y < 0) || (y >= ST_CONF_HEIGHT))	return;
	
	st_set_address_window(x, y, x, y);
	{
		uint8_t data[] = {color >> 8, color & 0xFF};
		st_write_data(data, sizeof(data));	/* st_write_data manages CS */
	}
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

	if ((xEnd < 0) || (xEnd >= ST_CONF_WIDTH) ||
		 (yEnd < 0) || (yEnd >= ST_CONF_HEIGHT))	return;

	count = ((u32)xEnd - (u32)xSta + 1UL) * ((u32)yEnd - (u32)ySta + 1UL);
	st_set_address_window(xSta, ySta, xEnd, yEnd);
	ST_OP_SELECT();
	ST_OP_DC_SET();
	st_spi_write_color(color, count);
	ST_OP_UNSELECT();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void st_draw_pixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x <= 0) || (x > ST_CONF_WIDTH) ||
		 (y <= 0) || (y > ST_CONF_HEIGHT))	return;
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
	uint16_t swap;
    uint16_t steep;

    /* Axis-aligned fast paths: stream the run as a fill instead of
     * setting a fresh address window per pixel. */
    if (y0 == y1) {
        if (x0 <= x1) st_fill(x0, y0, x1, y1, color);
        else          st_fill(x1, y0, x0, y1, color);
        return;
    }
    if (x0 == x1) {
        if (y0 <= y1) st_fill(x0, y0, x1, y1, color);
        else          st_fill(x0, y1, x1, y0, color);
        return;
    }

    steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
        /* _swap_int16_t(x0, y0); */
        /* _swap_int16_t(x1, y1); */
    }

    if (x0 > x1) {
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
        /* _swap_int16_t(x0, x1); */
        /* _swap_int16_t(y0, y1); */
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            st_draw_pixel(y0, x0, color);
        } else {
            st_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
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
	u8 fg_hi = (u8)(color >> 8u);
	u8 fg_lo = (u8)(color & 0x00FFu);
	u8 bg_hi = (u8)(bgcolor >> 8u);
	u8 bg_lo = (u8)(bgcolor & 0x00FFu);

	st_set_address_window(x, y, x + font.width - 1, y + font.height - 1);

	/* Stream the whole glyph under a single CS assertion instead of
	 * toggling CS + DC and stalling per pixel (was 2 SPI round-trips and
	 * 2 GPIO pairs for every pixel). */
	ST_OP_SELECT();
	ST_OP_DC_SET();
	st_spi_stream_begin();

	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				st_spi_stream_byte(fg_hi);
				st_spi_stream_byte(fg_lo);
			}
			else {
				st_spi_stream_byte(bg_hi);
				st_spi_stream_byte(bg_lo);
			}
		}
	}

	st_spi_stream_flush();
	ST_OP_UNSELECT();
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
	/* gfx_draw_char manages CS per glyph. */
	while (*str) {
		if (x + font.width >= ST_CONF_WIDTH) {
			x = 0;
			y += font.height;
			if (y + font.height >= ST_CONF_HEIGHT) {
				break;
			}

			if (*str == ' ') {
				/* skip spaces in the beginning of the new line */
				str++;
				continue;
			}
		}
		gfx_draw_char(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
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
	if (x >= ST_CONF_WIDTH ||
		y >= ST_CONF_HEIGHT) {
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

	st_fill(x, y, x + w, y + h, color);
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
