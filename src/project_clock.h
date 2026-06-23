#ifndef PROJECT_CLOCK_H
#define PROJECT_CLOCK_H

#define PROJECT_FRC_HZ             7370000UL
#define PROJECT_USE_PLL            0u

#if PROJECT_USE_PLL
#define PROJECT_PLL_N1             2UL
#define PROJECT_PLL_M              76UL
#define PROJECT_PLL_N2             2UL

#define PROJECT_PLLPRE_BITS        (PROJECT_PLL_N1 - 2UL)
#define PROJECT_PLLFBD_BITS        (PROJECT_PLL_M - 2UL)
#define PROJECT_PLLPOST_BITS       0UL
#define PROJECT_OSC_FRCPLL_NOSC    0x01u

#define PROJECT_FOSC_HZ            ((PROJECT_FRC_HZ * PROJECT_PLL_M) / \
                                    (PROJECT_PLL_N1 * PROJECT_PLL_N2))
#define PROJECT_FCY_HZ             (PROJECT_FOSC_HZ / 2UL)
#else
#define PROJECT_FOSC_HZ            PROJECT_FRC_HZ
#define PROJECT_FCY_HZ             (PROJECT_FOSC_HZ / 2UL)
#endif

#ifndef PROJECT_DISPLAY_SPI_HZ
#if PROJECT_USE_PLL
#define PROJECT_DISPLAY_SPI_HZ     10000000UL
#else
#define PROJECT_DISPLAY_SPI_HZ     PROJECT_FCY_HZ
#endif
#endif

#endif /* PROJECT_CLOCK_H */
