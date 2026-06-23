#include <stddef.h>
#include <xc.h>
#include <libpic30.h>
#include "hardware.h"
#include "project_clock.h"
#include "fonts.h"
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
        gpio_dw(&s_cfg.cs, 0u);
    }
}

static void st7789v_deselect(void)
{
    if (s_cfg.has_cs) {
        gpio_dw(&s_cfg.cs, 1u);
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
    gpio_dw(&s_cfg.dc, 0u);
    st7789v_spi_write_u8_raw(cmd);

    if ((data != NULL) && (len > 0u)) {
        gpio_dw(&s_cfg.dc, 1u);
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
        gpio_dw(&s_cfg.reset, 1u);
        st7789v_delay_ms(10u);
        gpio_dw(&s_cfg.reset, 0u);
        st7789v_delay_ms(20u);
        gpio_dw(&s_cfg.reset, 1u);
        st7789v_delay_ms(120u);
    }
}

static void st7789v_gpio_init(void)
{
    gpio_init_dw(&s_cfg.sck);
    gpio_init_dw(&s_cfg.mosi);
    gpio_init_dw(&s_cfg.dc);
    gpio_dw(&s_cfg.sck, 0u);
    gpio_dw(&s_cfg.mosi, 0u);
    gpio_dw(&s_cfg.dc, 1u);

    if (s_cfg.has_cs) {
        gpio_init_dw(&s_cfg.cs);
        gpio_dw(&s_cfg.cs, 1u);
    }

    if (s_cfg.has_reset) {
        gpio_init_dw(&s_cfg.reset);
        gpio_dw(&s_cfg.reset, 1u);
    }

    if (s_cfg.has_backlight) {
        gpio_init_dw(&s_cfg.backlight);
        gpio_dw(&s_cfg.backlight, 0u);
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

    gpio_dw(&s_cfg.backlight, on);
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

void st7789v_set_rotation(u8 rotation)
{
    static const u8 madctl_map[4] = {
        ST7789V_MADCTL_MY | ST7789V_MADCTL_MX | ST7789V_MADCTL_RGB, /* 0: MY|MX */
        ST7789V_MADCTL_MY | ST7789V_MADCTL_MV | ST7789V_MADCTL_RGB, /* 1: MY|MV */
        ST7789V_MADCTL_RGB,                                          /* 2: normal */
        ST7789V_MADCTL_MX | ST7789V_MADCTL_MV | ST7789V_MADCTL_RGB  /* 3: MX|MV */
    };
    st7789v_set_madctl(madctl_map[rotation & 0x03u]);
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
    gpio_dw(&s_cfg.dc, 1u);
    st7789v_spi_set_mode16(0u);
    s_write_active = 1u;
}

void st7789v_push_pixels(const u16 *rgb565, u32 count)
{
    if ((!s_ready) || (!s_write_active) || (rgb565 == NULL) || (count == 0UL)) {
        return;
    }

    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        volatile u16 dummy;
        u8 hi;
        u8 lo;
        while (count > 0UL) {
            hi = (u8)(*rgb565 >> 8u);
            lo = (u8)(*rgb565 & 0x00FFu);
            rgb565++;
            count--;
            while (SPI1STATbits.SPITBF) {}
            SPI1BUF = (u16)hi;
            while (!SPI1STATbits.SPIRBF) {}
            dummy = SPI1BUF;
            while (SPI1STATbits.SPITBF) {}
            SPI1BUF = (u16)lo;
            while (!SPI1STATbits.SPIRBF) {}
            dummy = SPI1BUF;
        }
        unused(dummy);
    } else {
        while (count > 0UL) {
            st7789v_spi_write_u8_raw((u8)(*rgb565 >> 8u));
            st7789v_spi_write_u8_raw((u8)(*rgb565 & 0x00FFu));
            rgb565++;
            count--;
        }
    }
}

void st7789v_push_color(u16 rgb565, u32 count)
{
    if ((!s_ready) || (!s_write_active) || (count == 0UL)) {
        return;
    }

    if (s_spi_bus == ST7789V_BUS_SPI1_FIXED) {
        volatile u16 dummy;
        u8 hi;
        u8 lo;
        hi = (u8)(rgb565 >> 8u);
        lo = (u8)(rgb565 & 0x00FFu);
        while (count > 0UL) {
            count--;
            while (SPI1STATbits.SPITBF) {}
            SPI1BUF = (u16)hi;
            while (!SPI1STATbits.SPIRBF) {}
            dummy = SPI1BUF;
            while (SPI1STATbits.SPITBF) {}
            SPI1BUF = (u16)lo;
            while (!SPI1STATbits.SPIRBF) {}
            dummy = SPI1BUF;
        }
        unused(dummy);
    } else {
        while (count > 0UL) {
            st7789v_spi_write_u8_raw((u8)(rgb565 >> 8u));
            st7789v_spi_write_u8_raw((u8)(rgb565 & 0x00FFu));
            count--;
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
    gpio_dw(&s_cfg.dc, 1u);
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
    gpio_dw(&s_cfg.dc, 1u);
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

#define ST7789V_ABS(x) ((x) >= 0 ? (x) : -(x))

void st7789v_draw_line(int x0, int y0, int x1, int y1, u16 color)
{
    int steep;
    int dx;
    int dy;
    int err;
    int ystep;
    int tmp;

    if (!s_ready) {
        return;
    }

    steep = ST7789V_ABS(y1 - y0) > ST7789V_ABS(x1 - x0);
    if (steep) {
        tmp = x0; x0 = y0; y0 = tmp;
        tmp = x1; x1 = y1; y1 = tmp;
    }
    if (x0 > x1) {
        tmp = x0; x0 = x1; x1 = tmp;
        tmp = y0; y0 = y1; y1 = tmp;
    }
    dx = x1 - x0;
    dy = ST7789V_ABS(y1 - y0);
    err = dx / 2;
    ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) {
            st7789v_draw_pixel((u16)y0, (u16)x0, color);
        } else {
            st7789v_draw_pixel((u16)x0, (u16)y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void st7789v_draw_rectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    st7789v_draw_line((int)x1, (int)y1, (int)x2, (int)y1, color);
    st7789v_draw_line((int)x1, (int)y1, (int)x1, (int)y2, color);
    st7789v_draw_line((int)x1, (int)y2, (int)x2, (int)y2, color);
    st7789v_draw_line((int)x2, (int)y1, (int)x2, (int)y2, color);
}

void st7789v_draw_circle(u16 x0, u16 y0, u8 r, u16 color)
{
    int f = 1 - (int)r;
    int ddF_x = 1;
    int ddF_y = -2 * (int)r;
    int x = 0;
    int y = (int)r;
    int cx = (int)x0;
    int cy = (int)y0;

    if (!s_ready) {
        return;
    }

    st7789v_draw_pixel((u16)cx, (u16)(cy + y), color);
    st7789v_draw_pixel((u16)cx, (u16)(cy - y), color);
    st7789v_draw_pixel((u16)(cx + y), (u16)cy, color);
    st7789v_draw_pixel((u16)(cx - y), (u16)cy, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        st7789v_draw_pixel((u16)(cx + x), (u16)(cy + y), color);
        st7789v_draw_pixel((u16)(cx - x), (u16)(cy + y), color);
        st7789v_draw_pixel((u16)(cx + x), (u16)(cy - y), color);
        st7789v_draw_pixel((u16)(cx - x), (u16)(cy - y), color);
        st7789v_draw_pixel((u16)(cx + y), (u16)(cy + x), color);
        st7789v_draw_pixel((u16)(cx - y), (u16)(cy + x), color);
        st7789v_draw_pixel((u16)(cx + y), (u16)(cy - x), color);
        st7789v_draw_pixel((u16)(cx - y), (u16)(cy - x), color);
    }
}

void st7789v_draw_filled_circle(int x0, int y0, int r, u16 color)
{
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;

    if (!s_ready) {
        return;
    }

    st7789v_draw_pixel((u16)x0, (u16)(y0 + r), color);
    st7789v_draw_pixel((u16)x0, (u16)(y0 - r), color);
    st7789v_draw_pixel((u16)(x0 + r), (u16)y0, color);
    st7789v_draw_pixel((u16)(x0 - r), (u16)y0, color);
    st7789v_draw_line(x0 - r, y0, x0 + r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        st7789v_draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
        st7789v_draw_line(x0 + x, y0 - y, x0 - x, y0 - y, color);
        st7789v_draw_line(x0 + y, y0 + x, x0 - y, y0 + x, color);
        st7789v_draw_line(x0 + y, y0 - x, x0 - y, y0 - x, color);
    }
}

void st7789v_draw_triangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3, u16 color)
{
    st7789v_draw_line((int)x1, (int)y1, (int)x2, (int)y2, color);
    st7789v_draw_line((int)x2, (int)y2, (int)x3, (int)y3, color);
    st7789v_draw_line((int)x3, (int)y3, (int)x1, (int)y1, color);
}

void st7789v_draw_filled_triangle(u16 x1p, u16 y1p, u16 x2p, u16 y2p,
                                   u16 x3p, u16 y3p, u16 color)
{
    int x1 = (int)x1p;
    int y1 = (int)y1p;
    int x2 = (int)x2p;
    int y2 = (int)y2p;
    int x3 = (int)x3p;
    int y3 = (int)y3p;
    int deltax;
    int deltay;
    int x;
    int y;
    int xinc1;
    int xinc2;
    int yinc1;
    int yinc2;
    int den;
    int num;
    int numadd;
    int numpixels;
    int curpixel;

    deltax = ST7789V_ABS(x2 - x1);
    deltay = ST7789V_ABS(y2 - y1);
    x = x1;
    y = y1;

    xinc1 = (x2 >= x1) ? 1 : -1;
    xinc2 = xinc1;
    yinc1 = (y2 >= y1) ? 1 : -1;
    yinc2 = yinc1;

    if (deltax >= deltay) {
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    } else {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++) {
        st7789v_draw_line(x, y, x3, y3, color);
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

void st7789v_draw_image(u16 x, u16 y, u16 w, u16 h, const u16 *data)
{
    u32 count;

    if ((!s_ready) || (data == NULL) || (w == 0u) || (h == 0u)) {
        return;
    }
    if ((x >= s_cfg.width) || (y >= s_cfg.height)) {
        return;
    }
    if (((u32)x + (u32)w) > (u32)s_cfg.width) {
        return;
    }
    if (((u32)y + (u32)h) > (u32)s_cfg.height) {
        return;
    }

    count = (u32)w * (u32)h;
    st7789v_begin_pixels(x, y, w, h);
    st7789v_push_pixels(data, count);
    st7789v_end_pixels();
}

void st7789v_write_char(u16 x, u16 y, char ch, FontDef font, u16 color, u16 bgcolor)
{
    u32 i;
    u32 j;
    u16 b;

    if (!s_ready) {
        return;
    }
    if ((ch < 32) || (ch > 126)) {
        return;
    }

    st7789v_begin_pixels(x, y, (u16)font.width, (u16)font.height);
    for (i = 0UL; i < (u32)font.height; i++) {
        b = font.data[((u32)(unsigned char)(ch - 32) * (u32)font.height) + i];
        for (j = 0UL; j < (u32)font.width; j++) {
            if (b & (u16)(0x8000u >> j)) {
                st7789v_push_color(color, 1UL);
            } else {
                st7789v_push_color(bgcolor, 1UL);
            }
        }
    }
    st7789v_end_pixels();
}

void st7789v_write_string(u16 x, u16 y, const char *str, FontDef font,
                           u16 color, u16 bgcolor)
{
    if ((!s_ready) || (str == NULL)) {
        return;
    }
    while (*str != '\0') {
        if (((u32)x + (u32)font.width) >= (u32)s_cfg.width) {
            x = 0u;
            y = (u16)((u32)y + (u32)font.height);
            if (((u32)y + (u32)font.height) >= (u32)s_cfg.height) {
                break;
            }
            if (*str == ' ') {
                str++;
                continue;
            }
        }
        st7789v_write_char(x, y, *str, font, color, bgcolor);
        x = (u16)((u32)x + (u32)font.width);
        str++;
    }
}
