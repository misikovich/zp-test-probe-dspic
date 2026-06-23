#include <xc.h>
#include <stddef.h>
#include "amazing_utils.h"
#include "motor.h"

static const Pin MOTOR_LOCK =         { &LATA, &TRISA, NULL, 1u << 1 };
static const Pin MOTOR_LOCK_PULSE =   { &LATF, &TRISF, NULL, 1u << 0 };
static const Pin MOTOR_UNLOCK =       { &LATA, &TRISA, NULL, 1u << 0 };
static const Pin MOTOR_UNLOCK_PULSE = { &LATF, &TRISF, NULL, 1u << 1 };

void mtr_init(void)
{
    pin_output(MOTOR_LOCK);
    pin_output(MOTOR_LOCK_PULSE);
    pin_output(MOTOR_UNLOCK);
    pin_output(MOTOR_UNLOCK_PULSE);

    pin_ansel(MOTOR_LOCK, 0u);
    pin_ansel(MOTOR_LOCK_PULSE, 0u);
    pin_ansel(MOTOR_UNLOCK, 0u);
    pin_ansel(MOTOR_UNLOCK_PULSE, 0u);

    mtr_drive(STOP);
}

void mtr_drive(enum mtr_drive_dir dir)
{
    switch (dir) {
    case LOCK:
        pin_high(MOTOR_LOCK);
        pin_high(MOTOR_LOCK_PULSE);
        pin_low(MOTOR_UNLOCK);
        pin_low(MOTOR_UNLOCK_PULSE);
        break;
    case UNLOCK:
        pin_low(MOTOR_LOCK);
        pin_low(MOTOR_LOCK_PULSE);
        pin_high(MOTOR_UNLOCK);
        pin_high(MOTOR_UNLOCK_PULSE);
        break;
    default:
        pin_low(MOTOR_LOCK);
        pin_low(MOTOR_LOCK_PULSE);
        pin_low(MOTOR_UNLOCK);
        pin_low(MOTOR_UNLOCK_PULSE);
        break;
    }
}
