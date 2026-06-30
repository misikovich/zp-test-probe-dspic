#include <xc.h>
#include <stdint.h>
#include "project_clock.h"
#include "uart.h"

#define UART_SOUND_TX_RP          36u
#define UART_SOUND_TX_PPS_FN      _RPOUT_U1TX

static uint8_t s_uart_ready;

static uint16_t uart_brg_value(uint32_t baud)
{
    uint32_t divisor;

    divisor = (uint32_t)16u * baud;
    return (uint16_t)(((PROJECT_FCY_HZ + (divisor / 2UL)) / divisor) - 1UL);
}

static void uart_pps_unlock(void)
{
    __builtin_write_OSCCONL((uint16_t)(OSCCON & 0x00BFu));
}

static void uart_pps_lock(void)
{
    __builtin_write_OSCCONL((uint16_t)(OSCCON | 0x0040u));
}

void uart_init(void)
{
    U1MODE = 0u;
    U1STA = 0u;

    IEC0bits.U1RXIE = 0u;
    IEC0bits.U1TXIE = 0u;
    IFS0bits.U1RXIF = 0u;
    IFS0bits.U1TXIF = 0u;

    LATBbits.LATB4 = 1u;
    TRISBbits.TRISB4 = 0u;
    TRISAbits.TRISA8 = 1u;
    ODCBbits.ODCB4 = 0u;
    ODCAbits.ODCA8 = 0u;

    uart_pps_unlock();
    RPOR1bits.RP36R = UART_SOUND_TX_PPS_FN;
    uart_pps_lock();

    U1BRG = uart_brg_value(SOUND_UART_BAUD_RATE);
    U1MODEbits.BRGH = 0u;
    U1MODEbits.PDSEL = 0u;
    U1MODEbits.STSEL = 0u;
    U1MODEbits.UEN = 0u;
    U1MODEbits.UARTEN = 1u;
    U1STAbits.UTXEN = 1u;

    s_uart_ready = 1u;
}

static void uart_write_byte(uint8_t value)
{
    while (U1STAbits.UTXBF) {
    }
    U1TXREG = value;
}

void uart_play_sound(sound_command_t sound_cmd)
{
    if (!s_uart_ready) {
        uart_init();
    }

    uart_write_byte((uint8_t)sound_cmd);

    while (!U1STAbits.TRMT) {
    }
}
