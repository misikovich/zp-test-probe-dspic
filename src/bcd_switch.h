#ifndef BCD_SWITCH_H
#define BCD_SWITCH_H

#include "hardware.h"

typedef void (*bcd_switch_cb)(u8 value);

void bcd_switch_init(void);
u8 bcd_switch_read(void);
void bcd_on_switch(bcd_switch_cb fun);

#endif /* BCD_SWITCH_H */
