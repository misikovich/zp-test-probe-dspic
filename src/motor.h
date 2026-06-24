#ifndef MOTOR_H
#define MOTOR_H

enum mtr_drive_dir {
    STOP,
    LOCK,
    UNLOCK
};

void mtr_init(void);
void mtr_drive(enum mtr_drive_dir dir);
void mtr_hold(enum mtr_drive_dir dir, u32 hold_ms);

#endif /* MOTOR_H */
