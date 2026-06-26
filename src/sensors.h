#ifndef SENSORS_H
#define SENSORS_H

#include "hardware.h"

#define SENSORS_ADC_VREF_MV 3300u

typedef struct {
    u16 motor_raw;
    u16 tmp_rcd_raw;
    u16 tmp_type2_raw;
} SensorReadings;

void sensors_init(void);
void sensors_poll(SensorReadings *readings);
u16 sensors_motor_raw_to_ma(u16 raw);
u16 sensors_motor_raw(void);
u16 sensors_tmp_rcd_raw(void);
u16 sensors_tmp_type2_raw(void);

#endif /* SENSORS_H */
