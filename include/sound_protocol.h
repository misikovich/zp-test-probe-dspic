#ifndef SOUND_PROTOCOL_H
#define SOUND_PROTOCOL_H

#include <stdint.h>

#define SOUND_UART_BAUD_RATE 115200

typedef enum {
    SOUND_CMD_STARTUP = 0x01,
    SOUND_CMD_SWITCH_PAGE = 0x02,
    SOUND_CMD_GENERIC_BEEP = 0x03,
    SOUND_CMD_OK_BEEP = 0x04,
    SOUND_CMD_ERR_BEEP = 0x05,
} sound_command_t;

#endif