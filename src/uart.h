#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "include/sound_protocol.h"

void uart_init(void);
void uart_play_sound(sound_command_t sound_cmd);

#endif /* UART_H */
