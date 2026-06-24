#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <xc.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t bool;

#define task_hold(ms) vTaskDelay(pdMS_TO_TICKS((ms)))
#define unused(v) (void)(v)
#define forever for (;;)

#define ADC1_RESULT_MAX 4095u

typedef struct {
    volatile u16 *lat;
    volatile u16 *port;
    volatile u16 *tris;
    volatile u16 *ansel;
    u16 mask;
} GPIO;

typedef struct {
    GPIO gpio;
    u8 adc_ch;
} ANPI;

typedef struct {
    GPIO gpio;
    volatile u16 *reg;
    u16 max;
} ANPO;

static inline void gpio_init_dr(const GPIO *pin) {
    *pin->tris |= pin->mask;
    if (pin->ansel) {
        *pin->ansel &= (u16)~pin->mask;
    }
}

static inline u8 gpio_dr(const GPIO *pin) {
    return (*pin->port & pin->mask) ? 1u : 0u;
}

static inline void gpio_init_dw(const GPIO *pin) {
    *pin->tris &= (u16)~pin->mask;
    if (pin->ansel) {
        *pin->ansel &= (u16)~pin->mask;
    }
}

static inline void gpio_dw(const GPIO *pin, u8 state) {
    if (state) {
        *pin->lat |= pin->mask;
    } else {
        *pin->lat &= (u16)~pin->mask;
    }
}

static inline void adc1_init(void) {
    AD1CON1bits.ADON = 0u;

    AD1CON1bits.AD12B = 1u;
    AD1CON1bits.FORM = 0u;
    AD1CON1bits.SSRC = 7u;
    AD1CON1bits.ASAM = 0u;

    AD1CON2bits.VCFG = 0u;
    AD1CON2bits.CSCNA = 0u;
    AD1CON2bits.CHPS = 0u;
    AD1CON2bits.SMPI = 0u;
    AD1CON2bits.ALTS = 0u;

    AD1CON3bits.ADRC = 0u;
    AD1CON3bits.ADCS = 63u;
    AD1CON3bits.SAMC = 15u;

    AD1CHS0bits.CH0NA = 0u;
    AD1CON1bits.ADON = 1u;
}

static inline void anpi_init_ar(const ANPI *pin) {
    *pin->gpio.tris |= pin->gpio.mask;
    if (pin->gpio.ansel) {
        *pin->gpio.ansel |= pin->gpio.mask;
    }
    adc1_init();
}

static inline u16 anpi_ar(const ANPI *pin) {
    anpi_init_ar(pin);

    AD1CHS0bits.CH0SA = pin->adc_ch & 0x1Fu;
    AD1CON1bits.DONE = 0u;
    AD1CON1bits.SAMP = 1u;

    while (!AD1CON1bits.DONE) {
    }

    return ADC1BUF0 & ADC1_RESULT_MAX;
}

static inline void anpo_init_aw(const ANPO *pin) {
    gpio_init_dw(&pin->gpio);
}

static inline void anpo_aw(const ANPO *pin, u16 value) {
    u16 max;

    anpo_init_aw(pin);
    max = pin->max ? pin->max : ADC1_RESULT_MAX;

    if (value > max) {
        value = max;
    }

    if (pin->reg) {
        *pin->reg = value;
    }
}

#endif /* HARDWARE_H */
