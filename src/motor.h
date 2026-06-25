#ifndef MOTOR_H
#define MOTOR_H

enum mtr_drive_dir {
    STOP,
    LOCK,
    UNLOCK
};

#define ADC_VREF_MV 3300u


void mtr_init(void);
void mtr_drive(enum mtr_drive_dir dir);
void mtr_hold(enum mtr_drive_dir dir, u32 hold_ms);
u16 mtr_poll_sense_raw();
u16 mtr_sense_raw_to_ma(u16 raw);

#endif /* MOTOR_H */
