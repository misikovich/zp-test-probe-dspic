#include <stddef.h>
#include <xc.h>
#include "hardware.h"
#include "sensors.h"

static const ANPI SENSOR_MOTOR =     { { &LATE, &PORTE, &TRISE, &ANSELE, 1u << 13 }, 13u }; /* AN13/RE13 */
static const ANPI SENSOR_TMP_RCD =   { { &LATC, &PORTC, &TRISC, &ANSELC, 1u << 0  }, 6u  }; /* AN6/RC0 */
static const ANPI SENSOR_TMP_TYPE2 = { { &LATE, &PORTE, &TRISE, &ANSELE, 1u << 15 }, 15u }; /* AN15/RE15 */

void sensors_init(void)
{
    anpi_init_ar(&SENSOR_MOTOR);
    anpi_init_ar(&SENSOR_TMP_RCD);
    anpi_init_ar(&SENSOR_TMP_TYPE2);
}

u16 sensors_motor_raw(void)
{
    return anpi_ar(&SENSOR_MOTOR);
}

u16 sensors_tmp_rcd_raw(void)
{
    return anpi_ar(&SENSOR_TMP_RCD);
}

u16 sensors_tmp_type2_raw(void)
{
    return anpi_ar(&SENSOR_TMP_TYPE2);
}

void sensors_poll(SensorReadings *readings)
{
    if (readings == NULL) {
        return;
    }

    readings->motor_raw = sensors_motor_raw();
    readings->tmp_rcd_raw = sensors_tmp_rcd_raw();
    readings->tmp_type2_raw = sensors_tmp_type2_raw();
}

u16 sensors_motor_raw_to_ma(u16 raw)
{
    u32 num;
    u32 den;

    num = (u32)raw * (u32)SENSORS_ADC_VREF_MV * 2u;
    den = 4095u * 9u;

    return (u16)((num + (den / 2u)) / den);
}
