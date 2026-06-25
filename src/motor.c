#include <xc.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include "motor.h"

static const GPIO MOTOR_LOCK =         { &LATA, &PORTA, &TRISA, NULL, 1u << 1 };
static const GPIO MOTOR_LOCK_PULSE =   { &LATF, &PORTF, &TRISF, NULL, 1u << 0 };
static const GPIO MOTOR_UNLOCK =       { &LATA, &PORTA, &TRISA, NULL, 1u << 0 };
static const GPIO MOTOR_UNLOCK_PULSE = { &LATF, &PORTF, &TRISF, NULL, 1u << 1 };
static const ANPI MOTOR_SENSE =        { { &LATE, &PORTE, &TRISE, &ANSELE, 1u << 13 }, 13u }; /* AN13/RE13 */


void mtr_init(void)
{
    gpio_init_dw(&MOTOR_LOCK);
    gpio_init_dw(&MOTOR_LOCK_PULSE);
    gpio_init_dw(&MOTOR_UNLOCK);
    gpio_init_dw(&MOTOR_UNLOCK_PULSE);
    anpi_init_ar(&MOTOR_SENSE);


    mtr_drive(STOP);
}

void mtr_drive(enum mtr_drive_dir dir)
{
    switch (dir) {
    case LOCK:
        gpio_dw(&MOTOR_LOCK, 1u);
        gpio_dw(&MOTOR_LOCK_PULSE, 1u);
        gpio_dw(&MOTOR_UNLOCK, 0u);
        gpio_dw(&MOTOR_UNLOCK_PULSE, 0u);
        break;
    case UNLOCK:
        gpio_dw(&MOTOR_LOCK, 0u);
        gpio_dw(&MOTOR_LOCK_PULSE, 0u);
        gpio_dw(&MOTOR_UNLOCK, 1u);
        gpio_dw(&MOTOR_UNLOCK_PULSE, 1u);
        break;
    default:
        gpio_dw(&MOTOR_LOCK, 0u);
        gpio_dw(&MOTOR_LOCK_PULSE, 0u);
        gpio_dw(&MOTOR_UNLOCK, 0u);
        gpio_dw(&MOTOR_UNLOCK_PULSE, 0u);
        break;
    }
}

void mtr_hold(enum mtr_drive_dir dir, u32 hold_ms) {
    mtr_drive(dir);
    task_hold(hold_ms);
}

u16 mtr_poll_sense_raw() {
    return anpi_ar(&MOTOR_SENSE);
}

u16 mtr_sense_raw_to_ma(u16 raw)
{
    u32 num;
    u32 den;

    num = (u32)raw * (u32)ADC_VREF_MV * 2u;
    den = 4095u * 9u;

    return (u16)((num + (den / 2u)) / den);
}