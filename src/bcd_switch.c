#include <xc.h>
#include <stddef.h>
#include "bcd_switch.h"
#include "hardware.h"

static const GPIO BCD_SWITCH_1 = { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 7  };  /* RP39/INT0/RB7 */
static const GPIO BCD_SWITCH_2 = { &LATC, &PORTC, &TRISC, &ANSELC, 1u << 4  };  /* SDA1/RPI52/RC4 */
static const GPIO BCD_SWITCH_4 = { &LATC, &PORTC, &TRISC, &ANSELC, 1u << 5  };  /* SCL1/RPI53/RC5 */
static const GPIO BCD_SWITCH_8 = { &LATB, &PORTB, &TRISB, &ANSELB, 1u << 12 };  /* RPI44/PWM2H/RB12 */

static bcd_switch_cb bcd_on_switch_callback = NULL;

void bcd_switch_init(void)
{
    gpio_init_dr(&BCD_SWITCH_1);
    gpio_init_dr(&BCD_SWITCH_2);
    gpio_init_dr(&BCD_SWITCH_4);
    gpio_init_dr(&BCD_SWITCH_8);
}

u8 bcd_switch_read(void)
{
    static u8 prev_value = 255;
    u8 value;

    value = 0u;
    if (gpio_dr(&BCD_SWITCH_1) == 0u) {
        value |= 0x01u;
    }
    if (gpio_dr(&BCD_SWITCH_2) == 0u) {
        value |= 0x02u;
    }
    if (gpio_dr(&BCD_SWITCH_4) == 0u) {
        value |= 0x04u;
    }
    if (gpio_dr(&BCD_SWITCH_8) == 0u) {
        value |= 0x08u;
    }
    
    if (prev_value != value && bcd_on_switch_callback != NULL)
    {
        bcd_on_switch_callback(value);
    }

    prev_value = value;
    return value;
}

void bcd_on_switch(bcd_switch_cb fun) {
    bcd_on_switch_callback = fun;
}
