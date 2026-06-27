#include <xc.h>
#include "hardware.h"
#include "project_clock.h"
#include "pwm.h"

#define PWM_DEFAULT_FREQ_HZ    1000UL
#define PWM_MIN_TIMER_TICKS    2UL

#define PWM_OC_OFF             0x0000u
#define PWM_OC_PWM_TIMER2      0x0006u
#define PWM_OC_MODE_MASK       0x0007u
#define PWM_OC_SYNC_TIMER2     0x000Cu

#define PWM_PPS_NULL           0u
#define PWM_PPS_OC1            16u
#define PWM_PPS_OC2            17u
#define PWM_PPS_OC3            18u
#define PWM_PPS_OC4            19u

#define PWM_BAD_INDEX          0xFFu

typedef struct {
    volatile u16 *con1;
    volatile u16 *con2;
    volatile u16 *r;
    volatile u16 *rs;
    u8 pps_fn;
} PwmOcRegs;

static PwmOcRegs s_oc[4] = {
    { &OC1CON1, &OC1CON2, &OC1R, &OC1RS, PWM_PPS_OC1 },
    { &OC2CON1, &OC2CON2, &OC2R, &OC2RS, PWM_PPS_OC2 },
    { &OC3CON1, &OC3CON2, &OC3R, &OC3RS, PWM_PPS_OC3 },
    { &OC4CON1, &OC4CON2, &OC4R, &OC4RS, PWM_PPS_OC4 }
};

static const u16 s_prescale[4] = { 1u, 8u, 64u, 256u };

static u32 s_period_ticks;
static u16 s_duty[4];
static u8 s_duty_valid[4];
static u8 s_timer_ready;
static u8 s_pps_mapped[4];

static u8 pwm_index(u8 ch)
{
    if ((ch < 1u) || (ch > 4u)) {
        return PWM_BAD_INDEX;
    }
    return (u8)(ch - 1u);
}

static u16 pwm_clamp_duty(u16 duty)
{
    if (duty > PWM_DUTY_MAX) {
        return PWM_DUTY_MAX;
    }
    return duty;
}

static void pwm_choose_timer(u32 freq_hz, u8 *tckps, u16 *period)
{
    u8 i;
    u32 denom;
    u32 ticks;
    u32 max_freq;

    if (freq_hz == 0UL) {
        freq_hz = PWM_DEFAULT_FREQ_HZ;
    }

    max_freq = PROJECT_FCY_HZ / PWM_MIN_TIMER_TICKS;
    if (freq_hz > max_freq) {
        freq_hz = max_freq;
    }

    for (i = 0u; i < 4u; i++) {
        denom = (u32)s_prescale[i] * freq_hz;
        ticks = (PROJECT_FCY_HZ + (denom / 2UL)) / denom;

        if ((ticks >= PWM_MIN_TIMER_TICKS) && (ticks <= 65536UL)) {
            *tckps = i;
            *period = (u16)(ticks - 1UL);
            s_period_ticks = ticks;
            return;
        }
    }

    *tckps = 3u;
    *period = 65535u;
    s_period_ticks = 65536UL;
}

static u16 pwm_compare_from_duty(u16 duty)
{
    u32 cmp;

    cmp = ((s_period_ticks * (u32)duty) + (PWM_DUTY_MAX / 2u)) / PWM_DUTY_MAX;
    if (cmp == 0UL) {
        cmp = 1UL;
    }
    if (cmp >= s_period_ticks) {
        cmp = s_period_ticks - 1UL;
    }
    return (u16)cmp;
}

static u8 pwm_rpor_for_pin(u8 rp_num, volatile u16 **rpor, u8 *high_byte)
{
    *high_byte = 0u;

    switch (rp_num) {
    case 20u: *rpor = &RPOR0; break;
    case 35u: *rpor = &RPOR0; *high_byte = 1u; break;
    case 36u: *rpor = &RPOR1; break;
    case 37u: *rpor = &RPOR1; *high_byte = 1u; break;
    case 38u: *rpor = &RPOR2; break;
    case 39u: *rpor = &RPOR2; *high_byte = 1u; break;
    case 40u: *rpor = &RPOR3; break;
    case 41u: *rpor = &RPOR3; *high_byte = 1u; break;
    case 42u: *rpor = &RPOR4; break;
    case 43u: *rpor = &RPOR4; *high_byte = 1u; break;
    case 54u: *rpor = &RPOR5; break;
    case 55u: *rpor = &RPOR5; *high_byte = 1u; break;
    case 56u: *rpor = &RPOR6; break;
    case 57u: *rpor = &RPOR6; *high_byte = 1u; break;
    case 97u: *rpor = &RPOR7; *high_byte = 1u; break;
    case 118u: *rpor = &RPOR8; *high_byte = 1u; break;
    case 120u: *rpor = &RPOR9; break;
    default: return 0u;
    }

    return 1u;
}

static u8 pwm_pps_write(u8 rp_num, u8 fn)
{
    volatile u16 *rpor;
    u8 high_byte;
    u16 mask;
    u16 value;
    u16 unlock;

    if (!pwm_rpor_for_pin(rp_num, &rpor, &high_byte)) {
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

static void pwm_disable_output(PwmPin p, u8 idx)
{
    *s_oc[idx].con1 = PWM_OC_OFF;

    if (s_pps_mapped[idx]) {
        (void)pwm_pps_write(p.rp_num, PWM_PPS_NULL);
        s_pps_mapped[idx] = 0u;
    }
}

void pwm_timer_init(u32 freq_hz)
{
    u8 tckps;
    u16 period;

    pwm_choose_timer(freq_hz, &tckps, &period);

    T2CON = 0u;
    TMR2 = 0u;
    PR2 = period;
    T2CONbits.TCKPS = tckps;
    T2CONbits.TON = 1u;
    s_timer_ready = 1u;
}

void pwm_init(PwmPin p, u16 duty)
{
    u8 idx;
    PwmOcRegs *oc;

    idx = pwm_index(p.ch);
    if (idx == PWM_BAD_INDEX) {
        return;
    }

    if (!s_timer_ready) {
        pwm_timer_init(PWM_DEFAULT_FREQ_HZ);
    }

    oc = &s_oc[idx];
    *oc->con1 = PWM_OC_OFF;
    *oc->con2 = PWM_OC_SYNC_TIMER2;
    *oc->r = 0u;
    *oc->rs = 0u;
    s_pps_mapped[idx] = 0u;
    s_duty_valid[idx] = 0u;

    gpio_init_dw(&p.gpio);
    gpio_dw(&p.gpio, 0u);

    pwm_set_duty(p, duty);
}

void pwm_set_duty(PwmPin p, u16 duty)
{
    u8 idx;
    u16 cmp;
    PwmOcRegs *oc;

    idx = pwm_index(p.ch);
    if (idx == PWM_BAD_INDEX) {
        return;
    }

    if (!s_timer_ready) {
        pwm_timer_init(PWM_DEFAULT_FREQ_HZ);
    }

    oc = &s_oc[idx];
    duty = pwm_clamp_duty(duty);

    if (s_duty_valid[idx] && (s_duty[idx] == duty)) {
        return;
    }

    s_duty[idx] = duty;
    s_duty_valid[idx] = 1u;

    gpio_init_dw(&p.gpio);

    if (duty == 0u) {
        pwm_disable_output(p, idx);
        gpio_dw(&p.gpio, 0u);
        return;
    }

    if (duty >= PWM_DUTY_MAX) {
        pwm_disable_output(p, idx);
        gpio_dw(&p.gpio, 1u);
        return;
    }

    if (!s_pps_mapped[idx]) {
        if (!pwm_pps_write(p.rp_num, oc->pps_fn)) {
            s_duty_valid[idx] = 0u;
            gpio_dw(&p.gpio, 0u);
            return;
        }
        s_pps_mapped[idx] = 1u;
    }

    cmp = pwm_compare_from_duty(duty);
    /* With SYNCSEL=Timer2 the duty compare is OCxR, so it must be rewritten on
     * every update -- not only at (re)start. OCxR is buffered (transfers at the
     * Timer2 period boundary), so live writes don't glitch. OCxRS kept in sync
     * to stay correct under either buffering model. Previously OCxR was set only
     * when the channel was off, so duty froze at its first value while running
     * and transitions held one brightness instead of fading. */
    *oc->r = cmp;
    *oc->rs = cmp;
    if (*oc->con2 != PWM_OC_SYNC_TIMER2) {
        *oc->con2 = PWM_OC_SYNC_TIMER2;
    }
    if ((*oc->con1 & PWM_OC_MODE_MASK) != PWM_OC_PWM_TIMER2) {
        *oc->con1 = PWM_OC_PWM_TIMER2;
    }
}

void pwm_stop(PwmPin p)
{
    u8 idx;

    idx = pwm_index(p.ch);
    if (idx == PWM_BAD_INDEX) {
        return;
    }

    pwm_disable_output(p, idx);
    s_duty_valid[idx] = 0u;
    gpio_dw(&p.gpio, 0u);
}

void pwm_start(PwmPin p)
{
    u8 idx;

    idx = pwm_index(p.ch);
    if (idx == PWM_BAD_INDEX) {
        return;
    }

    pwm_set_duty(p, s_duty[idx]);
}
