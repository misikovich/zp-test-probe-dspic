#include <stddef.h>
#include <xc.h>
#include <libpic30.h>
#include "amazing_utils.h"
#include "project_clock.h"
#include "st7789v.h"

#define ST7789V_CYCLES_PER_MS       (PROJECT_FCY_HZ / 1000UL)

#define ST7789V_PPS_NULL            0u
#define ST7789V_PPS_SDO2            8u
#define ST7789V_PPS_SCK2OUT         9u

#define ST7789V_CMD_NOP             0x00u
#define ST7789V_CMD_SWRESET         0x01u
#define ST7789V_CMD_SLPIN           0x10u
#define ST7789V_CMD_SLPOUT          0x11u
#define ST7789V_CMD_NORON           0x13u
#define ST7789V_CMD_INVOFF          0x20u
#define ST7789V_CMD_INVON           0x21u
#define ST7789V_CMD_DISPOFF         0x28u
#define ST7789V_CMD_DISPON          0x29u
#define ST7789V_CMD_TEOFF           0x34u
#define ST7789V_CMD_TEON            0x35u
#define ST7789V_CMD_CASET           0x2Au
#define ST7789V_CMD_RASET           0x2Bu
#define ST7789V_CMD_RAMWR           0x2Cu
#define ST7789V_CMD_MADCTL          0x36u
#define ST7789V_CMD_COLMOD          0x3Au
#define ST7789V_CMD_PORCTRL         0xB2u
#define ST7789V_CMD_GCTRL           0xB7u
#define ST7789V_CMD_VCOMS           0xBBu
#define ST7789V_CMD_LCMCTRL         0xC0u
#define ST7789V_CMD_VDVVRHEN        0xC2u
#define ST7789V_CMD_VRHS            0xC3u
#define ST7789V_CMD_VDVS            0xC4u
#define ST7789V_CMD_FRCTRL2         0xC6u
#define ST7789V_CMD_PWCTRL1         0xD0u
#define ST7789V_CMD_PVGAMCTRL       0xE0u
#define ST7789V_CMD_NVGAMCTRL       0xE1u

static const u16 s_spi_primary_div[4] = { 64u, 16u, 4u, 1u };
static const u8 s_spi_secondary_div[8] = { 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u };
static const u8 s_porctrl[] = { 0x0Cu, 0x0Cu, 0x00u, 0x33u, 0x33u };
static const u8 s_pwctrl1[] = { 0xA4u, 0xA1u };
static const u8 s_pvgam[] = {
    0xD0u, 0x04u, 0x0Du, 0x11u, 0x13u, 0x2Bu, 0x3Fu,
    0x54u, 0x4Cu, 0x18u, 0x0Du, 0x0Bu, 0x1Fu, 0x23u
};
static const u8 s_nvgam[] = {
    0xD0u, 0x04u, 0x0Cu, 0x11u, 0x13u, 0x2Cu, 0x3Fu,
    0x44u, 0x51u, 0x2Fu, 0x1Fu, 0x1Fu, 0x20u, 0x23u
};

static St7789vConfig s_cfg;
static u8 s_ready;
static u8 s_spi_bus;
static u8 s_mosi_mapped;
static u8 s_sck_mapped;
static u8 s_write_active;

static void st7789v_delay_ms(u16 ms)
{
    while (ms > 0u) {
        __delay32(ST7789V_CYCLES_PER_MS);
        ms--;
    }
}

static u8 st7789v_rpor_for_pin(u8 rp_num, volatile u16 **rpor, u8 *high_byte)
{
    *high_byte = 0u;

    switch (rp_num) {
    case 20u:  *rpor = &RPOR0; break;
    case 35u:  *rpor = &RPOR0; *high_byte = 1u; break;
    case 36u:  *rpor = &RPOR1; break;
    case 37u:  *rpor = &RPOR1; *high_byte = 1u; break;
    case 38u:  *rpor = &RPOR2; break;
    case 39u:  *rpor = &RPOR2; *high_byte = 1u; break;
    case 40u:  *rpor = &RPOR3; break;
    case 41u:  *rpor = &RPOR3; *high_byte = 1u; break;
    case 42u:  *rpor = &RPOR4; break;
    case 43u:  *rpor = &RPOR4; *high_byte = 1u; break;
    case 54u:  *rpor = &RPOR5; break;
    case 55u:  *rpor = &RPOR5; *high_byte = 1u; break;
    case 56u:  *rpor = &RPOR6; break;
    case 57u:  *rpor = &RPOR6; *high_byte = 1u; break;
    case 97u:  *rpor = &RPOR7; *high_byte = 1u; break;
    case 118u: *rpor = &RPOR8; *high_byte = 1u; break;
    case 120u: *rpor = &RPOR9; break;
    default: return 0u;
    }

    return 1u;
}

static u8 st7789v_pps_write(u8 rp_num, u8 fn)
{
    volatile u16 *rpor;
    u8 high_byte;
    u16 mask;
    u16 value;
    u16 unlock;

    if (!st7789v_rpor_for_pin(rp_num, &rpor, &high_byte)) {
        return 0u;
    }

    if (high_byte) {
        mask = 0x3F00u;
        value = (u16)(((u16)fn & 0x3Fu) << 8u);
    } else {
        mask = 0x003Fu;
        value = (u16)((u16)fn & 0x3Fu);
    }

    unlock = (u16)(OSCCON & 0x00BFu);
    __builtin_write_OSCCONL(unlock);
    *rpor = (u16)((*rpor & (u16)(~mask)) | value);
    __builtin_write_OSCCONL((u16)(OSCCON | 0x0040u));

    return 1u;
}

static void st7789v_choose_spi_speed(u32 hz, u8 *ppre, u8 *spre)
{
    u8 i;
    u8 j;
    u32 actual;
    u32 best;

    if ((hz == 0UL) || (hz > PROJECT_FCY_HZ)) {
        hz = PROJECT_FCY_HZ;
    }

    best = 0UL;
    *ppre = 0u;
    *spre = 0u;

    for (i = 0u; i < 4u; i++) {
        for (j = 0u; j < 8u; j++) {
            actual = PROJECT_FCY_HZ /
                ((u32)s_spi_primary_div[i] * (u32)s_spi_secondary_div[j]);
            if ((actual <= hz) && (actual > best)) {
                best = actual;
                *ppre = i;
                *spre = j;
            }
        }
    }
}

static void st7789v_spi2_init(u32 hz)
{
    u8 ppre;
    u8 spre;
    volatile u16 dummy;

    st7789v_choose_spi_speed(hz, &ppre, &spre);

    PMD1bits.SPI2MD = 0u;

    SPI2STATbits.SPIEN = 0u;
    SPI2CON1 = 0u;
    SPI2CON2 = 0u;
    SPI2STAT = 0u;
    dummy = SPI2BUF;
    unused(dummy);

    SPI2CON1bits.MSTEN = 1u;
    SPI2CON1bits.CKP = 0u;
    SPI2CON1bits.CKE = 1u;
    SPI2CON1bits.SMP = 0u;
    SPI2CON1bits.MODE16 = 0u;
    SPI2CON1bits.SSEN = 0u;
    SPI2CON1bits.DISSDO = 0u;
    SPI2CON1bits.DISSCK = 0u;
    SPI2CON1bits.PPRE = ppre;
    SPI2CON1bits.SPRE = spre;

    RPINR22bits.SDI2R = 0u;
    SPI2STATbits.SPIROV = 0u;
    SPI2STATbits.SPIEN = 1u;
}

static void st7789v_spi1_init(u32 hz)
{
    u8 ppre;
    u8 spre;
    volatile u16 dummy;

    st7789v_choose_spi_speed(hz, &ppre, &spre);

    PMD1bits.SPI1MD = 0u;

    SPI1STATbits.SPIEN = 0u;
    SPI1CON1 = 0u;
    SPI1CON2 = 0u;
    SPI1STAT = 0u;
    dummy = SPI1BUF;
    unused(dummy);

    SPI1CON1bits.MSTEN = 1u;
    SPI1CON1bits.CKP = 0u;
    SPI1CON1bits.CKE = 1u;
    SPI1CON1bits.SMP = 0u;
    SPI1CON1bits.MODE16 = 0u;
    SPI1CON1bits.SSEN = 0u;
    SPI1CON1bits.DISSDO = 0u;
    SPI1CON1bits.DISSCK = 0u;
    SPI1CON1bits.PPRE = ppre;
    SPI1CON1bits.SPRE = spre;

    SPI1STATbits.SPIROV = 0u;
    SPI1STATbits.SPIEN = 1u;
}

static void st7789v_spi_init(u32 hz)
{
    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        st7789v_spi1_init(hz);
    } else {
        st7789v_spi2_init(hz);
    }
}

static void st7789v_spi_set_mode16(u8 mode16)
{
    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        if (SPI1CON1bits.MODE16 != mode16) {
            while (!SPI1STATbits.SRMPT) {
            }
            SPI1STATbits.SPIEN = 0u;
            SPI1CON1bits.MODE16 = mode16;
            SPI1STATbits.SPIROV = 0u;
            SPI1STATbits.SPIEN = 1u;
        }
    } else {
        if (SPI2CON1bits.MODE16 != mode16) {
            while (!SPI2STATbits.SRMPT) {
            }
            SPI2STATbits.SPIEN = 0u;
            SPI2CON1bits.MODE16 = mode16;
            SPI2STATbits.SPIROV = 0u;
            SPI2STATbits.SPIEN = 1u;
        }
    }
}

static void st7789v_spi_write_u8_raw(u8 value)
{
    volatile u16 dummy;

    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
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
    } else {
        while (SPI2STATbits.SPITBF) {
        }

        SPI2BUF = (u16)value;

        while (!SPI2STATbits.SPIRBF) {
        }

        dummy = SPI2BUF;
        unused(dummy);

        if (SPI2STATbits.SPIROV) {
            SPI2STATbits.SPIROV = 0u;
        }
    }
}

static void st7789v_select(void)
{
    if (s_cfg.has_cs) {
        pin_low(s_cfg.cs);
    }
}

static void st7789v_deselect(void)
{
    if (s_cfg.has_cs) {
        pin_high(s_cfg.cs);
    }
}

static void st7789v_finish_pixels(void)
{
    if (s_write_active) {
        st7789v_spi_set_mode16(0u);
        st7789v_deselect();
        s_write_active = 0u;
    }
}

static void st7789v_write_command_selected(u8 cmd, const u8 *data, u16 len)
{
    u16 i;

    st7789v_spi_set_mode16(0u);
    pin_low(s_cfg.dc);
    st7789v_spi_write_u8_raw(cmd);

    if ((data != NULL) && (len > 0u)) {
        pin_high(s_cfg.dc);
        for (i = 0u; i < len; i++) {
            st7789v_spi_write_u8_raw(data[i]);
        }
    }
}

static void st7789v_write_command_data_raw(u8 cmd, const u8 *data, u16 len)
{
    st7789v_select();
    st7789v_write_command_selected(cmd, data, len);

    st7789v_deselect();
}

static void st7789v_write_one_raw(u8 cmd, u8 data)
{
    st7789v_write_command_data_raw(cmd, &data, 1u);
}

static void st7789v_reset_panel(void)
{
    if (s_cfg.has_reset) {
        pin_high(s_cfg.reset);
        st7789v_delay_ms(10u);
        pin_low(s_cfg.reset);
        st7789v_delay_ms(20u);
        pin_high(s_cfg.reset);
        st7789v_delay_ms(120u);
    }
}

static void st7789v_gpio_init(void)
{
    pin_ansel(s_cfg.sck, 0u);
    pin_ansel(s_cfg.mosi, 0u);
    pin_ansel(s_cfg.dc, 0u);

    pin_low(s_cfg.sck);
    pin_low(s_cfg.mosi);
    pin_high(s_cfg.dc);
    pin_output(s_cfg.sck);
    pin_output(s_cfg.mosi);
    pin_output(s_cfg.dc);

    if (s_cfg.has_cs) {
        pin_ansel(s_cfg.cs, 0u);
        pin_high(s_cfg.cs);
        pin_output(s_cfg.cs);
    }

    if (s_cfg.has_reset) {
        pin_ansel(s_cfg.reset, 0u);
        pin_high(s_cfg.reset);
        pin_output(s_cfg.reset);
    }

    if (s_cfg.has_backlight) {
        pin_ansel(s_cfg.backlight, 0u);
        pin_low(s_cfg.backlight);
        pin_output(s_cfg.backlight);
    }
}

static void st7789v_init_sequence(void)
{
    st7789v_write_command_data_raw(ST7789V_CMD_SWRESET, NULL, 0u);
    st7789v_delay_ms(150u);

    st7789v_write_command_data_raw(ST7789V_CMD_SLPOUT, NULL, 0u);
    st7789v_delay_ms(120u);

    st7789v_write_one_raw(ST7789V_CMD_COLMOD, 0x55u);
    st7789v_write_one_raw(ST7789V_CMD_MADCTL, s_cfg.madctl);
    st7789v_write_command_data_raw(ST7789V_CMD_PORCTRL,
        s_porctrl, (u16)sizeof(s_porctrl));
    st7789v_write_one_raw(ST7789V_CMD_GCTRL, 0x35u);
    st7789v_write_one_raw(ST7789V_CMD_VCOMS, 0x19u);
    st7789v_write_one_raw(ST7789V_CMD_LCMCTRL, 0x2Cu);
    st7789v_write_one_raw(ST7789V_CMD_VDVVRHEN, 0x01u);
    st7789v_write_one_raw(ST7789V_CMD_VRHS, 0x12u);
    st7789v_write_one_raw(ST7789V_CMD_VDVS, 0x20u);
    st7789v_write_one_raw(ST7789V_CMD_FRCTRL2, 0x0Fu);
    st7789v_write_command_data_raw(ST7789V_CMD_PWCTRL1,
        s_pwctrl1, (u16)sizeof(s_pwctrl1));
    st7789v_write_command_data_raw(ST7789V_CMD_PVGAMCTRL,
        s_pvgam, (u16)sizeof(s_pvgam));
    st7789v_write_command_data_raw(ST7789V_CMD_NVGAMCTRL,
        s_nvgam, (u16)sizeof(s_nvgam));

    if (s_cfg.inversion_on) {
        st7789v_write_command_data_raw(ST7789V_CMD_INVON, NULL, 0u);
    } else {
        st7789v_write_command_data_raw(ST7789V_CMD_INVOFF, NULL, 0u);
    }

    st7789v_write_command_data_raw(ST7789V_CMD_NORON, NULL, 0u);
    st7789v_delay_ms(10u);
    st7789v_write_command_data_raw(ST7789V_CMD_DISPON, NULL, 0u);
    st7789v_delay_ms(100u);
}

static u8 st7789v_map_outputs(void)
{
    if (!st7789v_pps_write(s_cfg.mosi_rp_num, ST7789V_PPS_SDO2)) {
        return 0u;
    }
    s_mosi_mapped = 1u;

    if (!st7789v_pps_write(s_cfg.sck_rp_num, ST7789V_PPS_SCK2OUT)) {
        (void)st7789v_pps_write(s_cfg.mosi_rp_num, ST7789V_PPS_NULL);
        s_mosi_mapped = 0u;
        return 0u;
    }
    s_sck_mapped = 1u;

    return 1u;
}

static void st7789v_set_window_selected(u16 x, u16 y, u16 w, u16 h)
{
    u16 x0;
    u16 y0;
    u16 x1;
    u16 y1;
    u8 data[4];

    x0 = (u16)(x + s_cfg.x_offset);
    y0 = (u16)(y + s_cfg.y_offset);
    x1 = (u16)(x0 + w - 1u);
    y1 = (u16)(y0 + h - 1u);

    data[0] = (u8)(x0 >> 8u);
    data[1] = (u8)(x0 & 0x00FFu);
    data[2] = (u8)(x1 >> 8u);
    data[3] = (u8)(x1 & 0x00FFu);
    st7789v_write_command_selected(ST7789V_CMD_CASET, data, 4u);

    data[0] = (u8)(y0 >> 8u);
    data[1] = (u8)(y0 & 0x00FFu);
    data[2] = (u8)(y1 >> 8u);
    data[3] = (u8)(y1 & 0x00FFu);
    st7789v_write_command_selected(ST7789V_CMD_RASET, data, 4u);

    st7789v_write_command_selected(ST7789V_CMD_RAMWR, NULL, 0u);
}

u8 st7789v_init(const St7789vConfig *cfg)
{
    if (cfg == NULL) {
        return 0u;
    }

    s_ready = 0u;
    s_mosi_mapped = 0u;
    s_sck_mapped = 0u;
    s_write_active = 0u;
    s_cfg = *cfg;
    s_spi_bus = s_cfg.bus;

    if (s_cfg.width == 0u) {
        s_cfg.width = ST7789V_DEFAULT_WIDTH;
    }
    if (s_cfg.height == 0u) {
        s_cfg.height = ST7789V_DEFAULT_HEIGHT;
    }
    if (s_cfg.spi_hz == 0UL) {
        s_cfg.spi_hz = ST7789V_DEFAULT_SPI_HZ;
    }
    if ((s_spi_bus != ST7789V_BUS_SPI1_FIXED) &&
        (s_spi_bus != ST7789V_BUS_SPI2_PPS)) {
        s_spi_bus = ST7789V_BUS_SPI2_PPS;
        s_cfg.bus = ST7789V_BUS_SPI2_PPS;
    }

    st7789v_gpio_init();

    if (s_spi_bus == ST7789V_BUS_SPI2_PPS) {
        if (!st7789v_map_outputs()) {
            return 0u;
        }
    }

    st7789v_spi_init(s_cfg.spi_hz);
    st7789v_reset_panel();
    st7789v_init_sequence();

    s_ready = 1u;
    st7789v_set_backlight(1u);

    return 1u;
}

void st7789v_deinit(void)
{
    st7789v_finish_pixels();

    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        SPI1STATbits.SPIEN = 0u;
    } else {
        SPI2STATbits.SPIEN = 0u;
    }

    if (s_spi_bus == ST7789V_BUS_SPI2_PPS) {
        if (s_mosi_mapped) {
            (void)st7789v_pps_write(s_cfg.mosi_rp_num, ST7789V_PPS_NULL);
            s_mosi_mapped = 0u;
        }
        if (s_sck_mapped) {
            (void)st7789v_pps_write(s_cfg.sck_rp_num, ST7789V_PPS_NULL);
            s_sck_mapped = 0u;
        }
    }

    st7789v_deselect();
    s_ready = 0u;
}

u8 st7789v_is_ready(void)
{
    return s_ready;
}

void st7789v_command(u8 cmd, const u8 *data, u16 len)
{
    if (!s_ready) {
        return;
    }

    st7789v_finish_pixels();
    st7789v_write_command_data_raw(cmd, data, len);
}

void st7789v_set_backlight(u8 on)
{
    if (!s_cfg.has_backlight) {
        return;
    }

    pin_set(s_cfg.backlight, on);
}

void st7789v_set_madctl(u8 madctl)
{
    if (!s_ready) {
        return;
    }

    st7789v_finish_pixels();
    s_cfg.madctl = madctl;
    st7789v_write_one_raw(ST7789V_CMD_MADCTL, madctl);
}

void st7789v_set_inversion(u8 on)
{
    if (!s_ready) {
        return;
    }

    st7789v_finish_pixels();
    s_cfg.inversion_on = on ? 1u : 0u;
    if (s_cfg.inversion_on) {
        st7789v_write_command_data_raw(ST7789V_CMD_INVON, NULL, 0u);
    } else {
        st7789v_write_command_data_raw(ST7789V_CMD_INVOFF, NULL, 0u);
    }
}

void st7789v_set_tearing_effect(u8 on, u8 mode)
{
    u8 param;

    if (!s_ready) {
        return;
    }

    st7789v_finish_pixels();

    if (on) {
        param = mode ? 1u : 0u;
        st7789v_write_command_data_raw(ST7789V_CMD_TEON, &param, 1u);
    } else {
        st7789v_write_command_data_raw(ST7789V_CMD_TEOFF, NULL, 0u);
    }
}

void st7789v_set_window(u16 x, u16 y, u16 w, u16 h)
{
    if ((!s_ready) || (w == 0u) || (h == 0u)) {
        return;
    }

    st7789v_finish_pixels();
    st7789v_select();
    st7789v_set_window_selected(x, y, w, h);
    st7789v_deselect();
}

void st7789v_begin_pixels(u16 x, u16 y, u16 w, u16 h)
{
    if ((!s_ready) || (w == 0u) || (h == 0u)) {
        return;
    }

    st7789v_finish_pixels();
    st7789v_select();
    st7789v_set_window_selected(x, y, w, h);
    pin_high(s_cfg.dc);
    st7789v_spi_set_mode16(0u);
    s_write_active = 1u;
}

void st7789v_push_pixels(const u16 *rgb565, u32 count)
{
    volatile u16 dummy;
    u16 pixel;

    if ((!s_ready) || (!s_write_active) || (rgb565 == NULL) || (count == 0UL)) {
        return;
    }

    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        while (count > 0UL) {
            pixel = *rgb565;
            rgb565++;

            while (SPI1STATbits.SPITBF) {
            }

            SPI1BUF = (u16)(pixel >> 8u);

            while (!SPI1STATbits.SPIRBF) {
            }

            dummy = SPI1BUF;
            unused(dummy);

            while (SPI1STATbits.SPITBF) {
            }

            SPI1BUF = (u16)(pixel & 0x00FFu);

            while (!SPI1STATbits.SPIRBF) {
            }

            dummy = SPI1BUF;
            unused(dummy);
            count--;
        }

        if (SPI1STATbits.SPIROV) {
            SPI1STATbits.SPIROV = 0u;
        }
    } else {
        while (count > 0UL) {
            pixel = *rgb565;
            rgb565++;

            while (SPI2STATbits.SPITBF) {
            }

            SPI2BUF = (u16)(pixel >> 8u);

            while (!SPI2STATbits.SPIRBF) {
            }

            dummy = SPI2BUF;
            unused(dummy);

            while (SPI2STATbits.SPITBF) {
            }

            SPI2BUF = (u16)(pixel & 0x00FFu);

            while (!SPI2STATbits.SPIRBF) {
            }

            dummy = SPI2BUF;
            unused(dummy);
            count--;
        }

        if (SPI2STATbits.SPIROV) {
            SPI2STATbits.SPIROV = 0u;
        }
    }
}

void st7789v_push_color(u16 rgb565, u32 count)
{
    volatile u16 dummy;
    u16 high;
    u16 low;

    if ((!s_ready) || (!s_write_active) || (count == 0UL)) {
        return;
    }

    high = (u16)(rgb565 >> 8u);
    low = (u16)(rgb565 & 0x00FFu);

    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        while (count > 0UL) {
            while (SPI1STATbits.SPITBF) {
            }

            SPI1BUF = high;

            while (!SPI1STATbits.SPIRBF) {
            }

            dummy = SPI1BUF;
            unused(dummy);

            while (SPI1STATbits.SPITBF) {
            }

            SPI1BUF = low;

            while (!SPI1STATbits.SPIRBF) {
            }

            dummy = SPI1BUF;
            unused(dummy);
            count--;
        }

        if (SPI1STATbits.SPIROV) {
            SPI1STATbits.SPIROV = 0u;
        }
    } else {
        while (count > 0UL) {
            while (SPI2STATbits.SPITBF) {
            }

            SPI2BUF = high;

            while (!SPI2STATbits.SPIRBF) {
            }

            dummy = SPI2BUF;
            unused(dummy);

            while (SPI2STATbits.SPITBF) {
            }

            SPI2BUF = low;

            while (!SPI2STATbits.SPIRBF) {
            }

            dummy = SPI2BUF;
            unused(dummy);
            count--;
        }

        if (SPI2STATbits.SPIROV) {
            SPI2STATbits.SPIROV = 0u;
        }
    }
}

void st7789v_end_pixels(void)
{
    st7789v_finish_pixels();
}

void st7789v_write_pixels(const u16 *rgb565, u32 count)
{
    if ((!s_ready) || (rgb565 == NULL) || (count == 0UL)) {
        return;
    }

    if (s_write_active) {
        st7789v_push_pixels(rgb565, count);
        return;
    }

    st7789v_select();
    pin_high(s_cfg.dc);
    st7789v_spi_set_mode16(0u);
    s_write_active = 1u;
    st7789v_push_pixels(rgb565, count);
    st7789v_finish_pixels();
}

void st7789v_write_color(u16 rgb565, u32 count)
{
    if ((!s_ready) || (count == 0UL)) {
        return;
    }

    if (s_write_active) {
        st7789v_push_color(rgb565, count);
        return;
    }

    st7789v_select();
    pin_high(s_cfg.dc);
    st7789v_spi_set_mode16(0u);
    s_write_active = 1u;
    st7789v_push_color(rgb565, count);
    st7789v_finish_pixels();
}

void st7789v_draw_pixel(u16 x, u16 y, u16 rgb565)
{
    if ((!s_ready) || (x >= s_cfg.width) || (y >= s_cfg.height)) {
        return;
    }

    st7789v_fill_rect(x, y, 1u, 1u, rgb565);
}

void st7789v_fill_rect(u16 x, u16 y, u16 w, u16 h, u16 rgb565)
{
    u32 right;
    u32 bottom;
    u32 count;

    if ((!s_ready) || (w == 0u) || (h == 0u)) {
        return;
    }
    if ((x >= s_cfg.width) || (y >= s_cfg.height)) {
        return;
    }

    right = (u32)x + (u32)w;
    bottom = (u32)y + (u32)h;

    if (right > (u32)s_cfg.width) {
        w = (u16)((u32)s_cfg.width - (u32)x);
    }
    if (bottom > (u32)s_cfg.height) {
        h = (u16)((u32)s_cfg.height - (u32)y);
    }

    count = (u32)w * (u32)h;
    st7789v_begin_pixels(x, y, w, h);
    st7789v_push_color(rgb565, count);
    st7789v_end_pixels();
}

void st7789v_fill_screen(u16 rgb565)
{
    st7789v_fill_rect(0u, 0u, s_cfg.width, s_cfg.height, rgb565);
}

u16 st7789v_color565(u8 r, u8 g, u8 b)
{
    u16 rr;
    u16 gg;
    u16 bb;

    rr = (u16)((u16)(r & 0xF8u) << 8u);
    gg = (u16)((u16)(g & 0xFCu) << 3u);
    bb = (u16)((u16)b >> 3u);

    return (u16)(rr | gg | bb);
}
