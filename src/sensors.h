#ifndef SENSORS_H
#define SENSORS_H

#include "hardware.h"

#define SENSORS_ADC_VREF_MV 3300u

typedef struct {
    u16 motor_raw;
    u16 tmp_rcd_raw;
    u16 tmp_type2_raw;
    u16 v_ng_raw;
    u16 v_l1l2_raw;
} SensorReadings;

void sensors_init(void);
void sensors_poll(SensorReadings *readings);
u16 sensors_motor_raw_to_ma(u16 raw);
u16 sensors_motor_raw(void);
u16 sensors_tmp_rcd_raw(void);
u16 sensors_tmp_type2_raw(void);
u16 sensors_v_ng_raw(void);
u16 sensors_v_l1l2_raw(void);
s32 sensors_v_ng_mv(void);
s32 sensors_v_l1l2_mv(void);
s32 sensors_v_ng_raw_to_mv(u16 raw);
s32 sensors_v_l1l2_raw_to_mv(u16 raw);

#endif /* SENSORS_H */
