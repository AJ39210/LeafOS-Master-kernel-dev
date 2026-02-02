/**
 * drivers/gpio/gpio.c
 * 
 * LeafOS GPIO Driver Implementation
 * 
 * Supports general purpose digital I/O with configurable pins
 * Includes interrupt support for edge detection
 */

#include "gpio.h"
#include "../../include/serial.h"
#include <stddef.h>

/* Maximum GPIO handlers */
#define GPIO_MAX_HANDLERS 256

/* GPIO Driver State Structure */
typedef struct {
    int initialized;
    gpio_irq_handler_t handlers[GPIO_MAX_HANDLERS];
} gpio_driver_state_t;

static gpio_driver_state_t gpio_driver_state = {0};

/**
 * gpio_is_valid_pin()
 * Validate port and pin combination
 */
int gpio_is_valid_pin(gpio_port_t port, uint8_t pin)
{
    if (port > GPIO_PORT_F) {
        return 0;
    }
    
    if (pin > 15) {  /* 16 pins per port */
        return 0;
    }
    
    return 1;
}

/**
 * gpio_get_port_letter()
 * Get ASCII letter for port
 */
char gpio_get_port_letter(gpio_port_t port)
{
    return (char)('A' + port);
}

/**
 * gpio_get_mode_name()
 * Get string name for mode
 */
const char* gpio_get_mode_name(gpio_mode_t mode)
{
    switch (mode) {
        case GPIO_MODE_INPUT:    return "Input";
        case GPIO_MODE_OUTPUT:   return "Output";
        case GPIO_MODE_ALTERNATE: return "Alternate";
        case GPIO_MODE_ANALOG:   return "Analog";
        default:                 return "Unknown";
    }
}

/**
 * gpio_configure_pin()
 * Configure a GPIO pin with basic settings
 */
int gpio_configure_pin(gpio_port_t port, uint8_t pin, gpio_mode_t mode)
{
    if (!gpio_is_valid_pin(port, pin)) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Configure P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_putchar(SERIAL_PORT_A, pin < 10 ? ('0' + pin) : ('A' + (pin - 10)));
    serial_puts(SERIAL_PORT_A, " as ");
    serial_puts(SERIAL_PORT_A, (char*)gpio_get_mode_name(mode));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // In real implementation:
    // 1. Unlock GPIO port
    // 2. Set MODER register for mode
    // 3. Set OTYPER for push-pull/open-drain
    // 4. Set OSPEEDR for speed
    // 5. Set PUPDR for pull-up/pull-down
    // 6. Lock GPIO port
    
    return 1;
}

/**
 * gpio_configure_advanced()
 * Configure GPIO pin with advanced settings
 */
int gpio_configure_advanced(gpio_pin_config_t *config)
{
    if (!config || !gpio_is_valid_pin(config->port, config->pin)) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Advanced config P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(config->port));
    serial_putchar(SERIAL_PORT_A, config->pin < 10 ? ('0' + config->pin) : ('A' + (config->pin - 10)));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // Configure mode
    gpio_configure_pin(config->port, config->pin, config->mode);
    
    // Configure pull
    // Configure speed
    // Configure interrupt if needed
    if (config->irq_mode != GPIO_IRQ_NONE) {
        gpio_enable_interrupt(config->port, config->pin, config->irq_mode);
    }
    
    return 1;
}

/**
 * gpio_set_pin()
 * Set GPIO pin output state
 */
int gpio_set_pin(gpio_port_t port, uint8_t pin, gpio_state_t state)
{
    if (!gpio_is_valid_pin(port, pin)) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Set P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_putchar(SERIAL_PORT_A, pin < 10 ? ('0' + pin) : ('A' + (pin - 10)));
    serial_puts(SERIAL_PORT_A, " = ");
    serial_puts(SERIAL_PORT_A, state == GPIO_HIGH ? "HIGH" : "LOW");
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // In real implementation:
    // Set BSRR register to set/reset pin
    // BSRR_SET for high, BSRR_RESET for low
    
    return 1;
}

/**
 * gpio_read_pin()
 * Read GPIO pin input state
 */
gpio_state_t gpio_read_pin(gpio_port_t port, uint8_t pin)
{
    if (!gpio_is_valid_pin(port, pin)) {
        return GPIO_LOW;
    }
    
    // In real implementation:
    // Read IDR register and check bit
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Read P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_putchar(SERIAL_PORT_A, pin < 10 ? ('0' + pin) : ('A' + (pin - 10)));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return GPIO_LOW;  /* Stub */
}

/**
 * gpio_toggle_pin()
 * Toggle GPIO pin state
 */
int gpio_toggle_pin(gpio_port_t port, uint8_t pin)
{
    if (!gpio_is_valid_pin(port, pin)) {
        return 0;
    }
    
    // Read current state and toggle
    gpio_state_t current = gpio_read_pin(port, pin);
    gpio_state_t new_state = (current == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH;
    
    return gpio_set_pin(port, pin, new_state);
}

/**
 * gpio_set_port()
 * Set all pins on a port
 */
int gpio_set_port(gpio_port_t port, uint32_t value)
{
    if (port > GPIO_PORT_F) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Set port ");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // In real implementation:
    // Set ODR register with value
    
    return 1;
}

/**
 * gpio_read_port()
 * Read all pins on a port
 */
uint32_t gpio_read_port(gpio_port_t port)
{
    if (port > GPIO_PORT_F) {
        return 0;
    }
    
    // In real implementation:
    // Read IDR register
    
    return 0;  /* Stub */
}

/**
 * gpio_enable_interrupt()
 * Enable interrupt on GPIO pin
 */
int gpio_enable_interrupt(gpio_port_t port, uint8_t pin, gpio_irq_mode_t mode)
{
    if (!gpio_is_valid_pin(port, pin)) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Enable interrupt P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_putchar(SERIAL_PORT_A, pin < 10 ? ('0' + pin) : ('A' + (pin - 10)));
    serial_puts(SERIAL_PORT_A, " mode ");
    serial_putchar(SERIAL_PORT_A, '0' + mode);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // In real implementation:
    // 1. Configure EXTI line
    // 2. Select GPIO port in EXTI source register
    // 3. Set rising/falling/both edge triggers
    // 4. Enable EXTI line interrupt
    // 5. Enable NVIC for EXTI interrupt
    
    return 1;
}

/**
 * gpio_disable_interrupt()
 * Disable interrupt on GPIO pin
 */
int gpio_disable_interrupt(gpio_port_t port, uint8_t pin)
{
    if (!gpio_is_valid_pin(port, pin)) {
        return 0;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Disable interrupt P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_putchar(SERIAL_PORT_A, pin < 10 ? ('0' + pin) : ('A' + (pin - 10)));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    // In real implementation:
    // Disable EXTI line interrupt
    
    return 1;
}

/**
 * gpio_register_handler()
 * Register callback for GPIO interrupt
 */
int gpio_register_handler(gpio_port_t port, uint8_t pin, gpio_irq_handler_t handler)
{
    if (!gpio_is_valid_pin(port, pin) || !handler) {
        return 0;
    }
    
    uint16_t index = (port << 4) | pin;
    if (index >= GPIO_MAX_HANDLERS) {
        return 0;
    }
    
    gpio_driver_state.handlers[index] = handler;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Registered handler P");
    serial_putchar(SERIAL_PORT_A, gpio_get_port_letter(port));
    serial_putchar(SERIAL_PORT_A, pin < 10 ? ('0' + pin) : ('A' + (pin - 10)));
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
    
    return 1;
}

/**
 * gpio_init()
 * Initialize GPIO subsystem
 */
int gpio_init(void)
{
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] Initializing GPIO driver\n");
    #endif
    
    // Initialize handler array
    for (int i = 0; i < GPIO_MAX_HANDLERS; i++) {
        gpio_driver_state.handlers[i] = NULL;
    }
    
    // In real implementation:
    // 1. Enable GPIO port clocks
    // 2. Enable EXTI clocks
    // 3. Enable NVIC for GPIO interrupts
    // 4. Configure any default pins
    
    gpio_driver_state.initialized = 1;
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[GPIO] GPIO driver initialized\n");
    serial_puts(SERIAL_PORT_A, "[GPIO] Supports 6 ports (A-F) with 16 pins each\n");
    #endif
    
    return 1;
}
