#include "serial.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_init(uint16_t port) {
    outb(SERIAL_LINE_COMMAND_PORT(port), 0x80);
    outb(SERIAL_DATA_PORT(port), 0x01);
    outb(SERIAL_DATA_PORT(port) + 1, 0x00);
    outb(SERIAL_LINE_COMMAND_PORT(port), 0x03);
    outb(SERIAL_FIFO_COMMAND_PORT(port), 0xC7);
    outb(SERIAL_MODEM_COMMAND_PORT(port), 0x0B);
}

static int serial_is_transmit_fifo_empty(uint16_t port) {
    return inb(SERIAL_LINE_STATUS_PORT(port)) & 0x20;
}

void serial_putchar(uint16_t port, char c) {
    while (!serial_is_transmit_fifo_empty(port));
    outb(SERIAL_DATA_PORT(port), c);
}

void serial_puts(uint16_t port, const char *str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            serial_putchar(port, '\r');
            serial_putchar(port, '\n');
        } else {
            serial_putchar(port, *str);
        }
        str++;
    }
}

int serial_received(uint16_t port) {
    return inb(SERIAL_LINE_STATUS_PORT(port)) & 1;
}

char serial_read(uint16_t port) {
    while (!serial_received(port));
    return inb(SERIAL_DATA_PORT(port));
}
