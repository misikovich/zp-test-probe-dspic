#ifndef PROJECT_CLOCK_H
#define PROJECT_CLOCK_H

#include <stdint.h>
#include <xc.h>

#define PROJECT_FRC_HZ             7370000UL
#define PROJECT_USE_PLL            1u

#if PROJECT_USE_PLL
    #define PROJECT_PLL_N1             2UL
    #define PROJECT_PLL_M              78UL
    #define PROJECT_PLL_N2             2UL

    #define PROJECT_PLLPRE_BITS        (PROJECT_PLL_N1 - 2UL)
    #define PROJECT_PLLFBD_BITS        (PROJECT_PLL_M - 2UL)
    #define PROJECT_PLLPOST_BITS       0UL
    #define PROJECT_OSC_FRCPLL_NOSC    0x01u

    #define PROJECT_FOSC_HZ            ((PROJECT_FRC_HZ * PROJECT_PLL_M) / (PROJECT_PLL_N1 * PROJECT_PLL_N2))
    #define PROJECT_FCY_HZ             (PROJECT_FOSC_HZ / 2UL)
#else
    #define PROJECT_FOSC_HZ            PROJECT_FRC_HZ
    #define PROJECT_FCY_HZ             (PROJECT_FOSC_HZ / 2UL)
#endif

#ifndef PROJECT_DISPLAY_SPI_HZ
#if PROJECT_USE_PLL
    #define PROJECT_DISPLAY_SPI_HZ     400000000UL
#else
    #define PROJECT_DISPLAY_SPI_HZ     PROJECT_FCY_HZ
#endif
#endif

static void prvInitClock(void)
{
    #if PROJECT_USE_PLL
        CLKDIVbits.FRCDIV = 0u;
        CLKDIVbits.PLLPRE = (uint16_t)PROJECT_PLLPRE_BITS;
        CLKDIVbits.PLLPOST = (uint16_t)PROJECT_PLLPOST_BITS;
        PLLFBDbits.PLLDIV = (uint16_t)PROJECT_PLLFBD_BITS;

        __builtin_write_OSCCONH(PROJECT_OSC_FRCPLL_NOSC);
        __builtin_write_OSCCONL((uint8_t)(OSCCON | 0x0001u));

        while (OSCCONbits.COSC != PROJECT_OSC_FRCPLL_NOSC) {
        }
        while (!OSCCONbits.LOCK) {
        }
    #endif
}

#endif /* PROJECT_CLOCK_H */
