#include <stddef.h>
#include <xc.h>
#include "hardware.h"
#include "sensors.h"

#define SENSOR_OFFSET_MV 1650L
#define SENSOR_GAIN_SCALE 1000000L
#define SENSOR_V_NG_GAIN_PPM 3680L
#define SENSOR_V_L1L2_GAIN_PPM 2450L

static const ANPI SENSOR_MOTOR =     { { &LATE, &PORTE, &TRISE, &ANSELE, 1u << 13 }, 13u }; /* AN13/RE13 */
static const ANPI SENSOR_TMP_RCD =   { { &LATC, &PORTC, &TRISC, &ANSELC, 1u << 0  }, 6u  }; /* AN6/RC0 */
static const ANPI SENSOR_TMP_TYPE2 = { { &LATE, &PORTE, &TRISE, &ANSELE, 1u << 15 }, 15u }; /* AN15/RE15 */
static const ANPI SENSOR_V_NG =      { { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 1  }, 3u  }; /* AN3/RB1 */
static const ANPI SENSOR_V_L1L2 =    { { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 0  }, 2u  }; /* AN2/RB0 */

void sensors_init(void)
{
    anpi_init_ar(&SENSOR_MOTOR);
    anpi_init_ar(&SENSOR_TMP_RCD);
    anpi_init_ar(&SENSOR_TMP_TYPE2);
    anpi_init_ar(&SENSOR_V_NG);
    anpi_init_ar(&SENSOR_V_L1L2);
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

u16 sensors_v_ng_raw(void)
{
    return anpi_ar(&SENSOR_V_NG);
}

u16 sensors_v_l1l2_raw(void)
{
    return anpi_ar(&SENSOR_V_L1L2);
}

s32 sensors_v_ng_mv(void)
{
    return sensors_v_ng_raw_to_mv(sensors_v_ng_raw());
}

s32 sensors_v_l1l2_mv(void)
{
    return sensors_v_l1l2_raw_to_mv(sensors_v_l1l2_raw());
}

void sensors_poll(SensorReadings *readings)
{
    if (readings == NULL) {
        return;
    }

    readings->motor_raw = sensors_motor_raw();
    readings->tmp_rcd_raw = sensors_tmp_rcd_raw();
    readings->tmp_type2_raw = sensors_tmp_type2_raw();
    readings->v_ng_raw = sensors_v_ng_raw();
    readings->v_l1l2_raw = sensors_v_l1l2_raw();
}

u16 sensors_motor_raw_to_ma(u16 raw)
{
    u32 num;
    u32 den;

    num = (u32)raw * (u32)SENSORS_ADC_VREF_MV * 2u;
    den = (u32)ADC1_RESULT_MAX * 9u;

    return (u16)((num + (den / 2u)) / den);
}

static s32 sensors_raw_to_input_mv(u16 raw, s32 gain_ppm)
{
    s32 adc_mv;
    s32 delta_mv;
    s32 round;

    /* gain_ppm is ADC volts per input volt, scaled by 1e6. */
    adc_mv = (s32)(((u32)raw * (u32)SENSORS_ADC_VREF_MV +
            ((u32)ADC1_RESULT_MAX / 2u)) / (u32)ADC1_RESULT_MAX);
    delta_mv = adc_mv - SENSOR_OFFSET_MV;
    round = gain_ppm / 2L;

    if (delta_mv < 0L) {
        return (s32)(((delta_mv * SENSOR_GAIN_SCALE) - round) / gain_ppm);
    }

    return (s32)(((delta_mv * SENSOR_GAIN_SCALE) + round) / gain_ppm);
}

s32 sensors_v_ng_raw_to_mv(u16 raw)
{
    return sensors_raw_to_input_mv(raw, SENSOR_V_NG_GAIN_PPM);
}

s32 sensors_v_l1l2_raw_to_mv(u16 raw)
{
    return sensors_raw_to_input_mv(raw, SENSOR_V_L1L2_GAIN_PPM);
}
