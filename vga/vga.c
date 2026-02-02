#include "printk.h"
#include "string.h"

static uint16_t *vga_buffer = (uint16_t *)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static uint16_t make_vgaentry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | (uint16_t)color << 8;
}

static void scroll_up(void) {
    for (int i = 0; i < VGA_HEIGHT - 1; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
            vga_buffer[i * VGA_WIDTH + j] = vga_buffer[(i + 1) * VGA_WIDTH + j];
        }
    }
    for (int j = 0; j < VGA_WIDTH; j++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + j] = make_vgaentry(' ', make_color(VGA_WHITE, VGA_BLACK));
    }
    cursor_y--;
}

static void putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        cursor_x += 4;
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = make_vgaentry(c, color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll_up();
    }
}

void printk(uint8_t color, const char *str) {
    if (!str) return;
    while (*str) {
        putchar(*str, color);
        str++;
    }
}

void printk_clear(void) {
    uint8_t color = make_color(VGA_WHITE, VGA_BLACK);
    for (int i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = make_vgaentry(' ', color);
    }
    cursor_x = 0;
    cursor_y = 0;
}
