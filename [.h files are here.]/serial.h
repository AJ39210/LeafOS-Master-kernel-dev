#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define SERIAL_PORT_A 0x3F8
#define SERIAL_PORT_B 0x2F8
#define SERIAL_PORT_C 0x3E8
#define SERIAL_PORT_D 0x2E8

#define SERIAL_DATA_PORT(base)     (base)
#define SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base) (base + 5)

void serial_init(uint16_t port);
void serial_putchar(uint16_t port, char c);
void serial_puts(uint16_t port, const char *str);
int serial_received(uint16_t port);
char serial_read(uint16_t port);

#endif
