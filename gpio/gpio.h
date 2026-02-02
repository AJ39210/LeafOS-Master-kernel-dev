/**
 * drivers/gpio/gpio.h
 * 
 * LeafOS GPIO (General Purpose Input/Output) Driver
 * 
 * Provides support for GPIO pins for basic digital I/O
 * Supports input, output, and interrupt modes
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/* GPIO Pin Modes */
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_ALTERNATE = 2,
    GPIO_MODE_ANALOG = 3
} gpio_mode_t;

/* GPIO Pin States */
typedef enum {
    GPIO_LOW = 0,
    GPIO_HIGH = 1
} gpio_state_t;

/* GPIO Pull Configuration */
typedef enum {
    GPIO_PULL_NONE = 0,
    GPIO_PULL_UP = 1,
    GPIO_PULL_DOWN = 2
} gpio_pull_t;

/* GPIO Speed */
typedef enum {
    GPIO_SPEED_LOW = 0,
    GPIO_SPEED_MEDIUM = 1,
    GPIO_SPEED_HIGH = 2,
    GPIO_SPEED_VERY_HIGH = 3
} gpio_speed_t;

/* GPIO Interrupt Mode */
typedef enum {
    GPIO_IRQ_NONE = 0,
    GPIO_IRQ_RISING = 1,
    GPIO_IRQ_FALLING = 2,
    GPIO_IRQ_BOTH = 3
} gpio_irq_mode_t;

/* GPIO Port Selection */
typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_E = 4,
    GPIO_PORT_F = 5
} gpio_port_t;

/* GPIO Pin Configuration */
typedef struct {
    gpio_port_t port;
    uint8_t pin;
    gpio_mode_t mode;
    gpio_pull_t pull;
    gpio_speed_t speed;
    gpio_irq_mode_t irq_mode;
} gpio_pin_config_t;

/* GPIO Pin Handler */
typedef void (*gpio_irq_handler_t)(gpio_port_t port, uint8_t pin);

/* Core Functions */
int gpio_init(void);
int gpio_configure_pin(gpio_port_t port, uint8_t pin, gpio_mode_t mode);
int gpio_configure_advanced(gpio_pin_config_t *config);

/* I/O Functions */
int gpio_set_pin(gpio_port_t port, uint8_t pin, gpio_state_t state);
gpio_state_t gpio_read_pin(gpio_port_t port, uint8_t pin);
int gpio_toggle_pin(gpio_port_t port, uint8_t pin);

/* Interrupt Functions */
int gpio_enable_interrupt(gpio_port_t port, uint8_t pin, gpio_irq_mode_t mode);
int gpio_disable_interrupt(gpio_port_t port, uint8_t pin);
int gpio_register_handler(gpio_port_t port, uint8_t pin, gpio_irq_handler_t handler);

/* Port Functions */
int gpio_set_port(gpio_port_t port, uint32_t value);
uint32_t gpio_read_port(gpio_port_t port);

/* Utility Functions */
int gpio_is_valid_pin(gpio_port_t port, uint8_t pin);
char gpio_get_port_letter(gpio_port_t port);
const char* gpio_get_mode_name(gpio_mode_t mode);

#endif // GPIO_H
