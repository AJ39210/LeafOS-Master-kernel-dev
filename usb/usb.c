#include "usb.h"
#include "serial.h"

/* PCI USB Controller Detection */
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static inline uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    asm volatile("outl %0, %1" : : "a"(address), "Nd"(PCI_CONFIG_ADDRESS));
    uint32_t data;
    asm volatile("inl %1, %0" : "=a"(data) : "Nd"(PCI_CONFIG_DATA));
    return data;
}

static inline void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    asm volatile("outl %0, %1" : : "a"(address), "Nd"(PCI_CONFIG_ADDRESS));
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(PCI_CONFIG_DATA));
}

/* USB Host Controller registry */
static int usb_hc_type = -1;
static uint16_t usb_hc_iobase = 0;

int usb_detect_host_controller(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB] Scanning for host controller...\n");
    #endif
    
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t vendor_device = pci_read_config(0, slot, 0, 0x00);
        if (vendor_device == 0xFFFFFFFF) continue;
        
        uint16_t vendor_id = vendor_device & 0xFFFF;
        uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
        
        uint32_t class_info = pci_read_config(0, slot, 0, 0x08);
        uint8_t class = (class_info >> 24) & 0xFF;
        uint8_t subclass = (class_info >> 16) & 0xFF;
        uint8_t prog_if = (class_info >> 8) & 0xFF;
        
        /* Class 0x0C = Serial Bus, Subclass 0x03 = USB */
        if (class == 0x0C && subclass == 0x03) {
            #ifdef CONFIG_SERIAL_DRIVER
            serial_puts(SERIAL_PORT_A, "[USB] Found USB HC: Vendor=");
            serial_puts(SERIAL_PORT_A, "0x");
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(vendor_id >> 12) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(vendor_id >> 8) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(vendor_id >> 4) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[vendor_id & 0xF]);
            serial_puts(SERIAL_PORT_A, ", Device=0x");
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(device_id >> 12) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(device_id >> 8) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(device_id >> 4) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[device_id & 0xF]);
            serial_puts(SERIAL_PORT_A, ", ProgIF=");
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(prog_if >> 4) & 0xF]);
            serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[prog_if & 0xF]);
            serial_puts(SERIAL_PORT_A, "\n");
            #endif
            
            switch (prog_if) {
                case 0x00:
                    usb_hc_type = USB_HC_UHCI;
                    #ifdef CONFIG_SERIAL_DRIVER
                    serial_puts(SERIAL_PORT_A, "[USB] Detected UHCI Host Controller\n");
                    #endif
                    break;
                case 0x10:
                    usb_hc_type = USB_HC_OHCI;
                    #ifdef CONFIG_SERIAL_DRIVER
                    serial_puts(SERIAL_PORT_A, "[USB] Detected OHCI Host Controller\n");
                    #endif
                    break;
                case 0x20:
                    usb_hc_type = USB_HC_EHCI;
                    #ifdef CONFIG_SERIAL_DRIVER
                    serial_puts(SERIAL_PORT_A, "[USB] Detected EHCI Host Controller\n");
                    #endif
                    break;
            }
            
            if (usb_hc_type != -1) {
                uint32_t bar = pci_read_config(0, slot, 0, 0x10);
                usb_hc_iobase = bar & 0xFFFFFFF0;
                return usb_hc_type;
            }
        }
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB] No USB host controller found\n");
    #endif
    return -1;
}

void usb_init(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB] USB Driver Initializing...\n");
    #endif
    
    int hc_type = usb_detect_host_controller();
    
    if (hc_type == -1) {
        #ifdef CONFIG_SERIAL_DRIVER
        serial_puts(SERIAL_PORT_A, "[USB] Failed to initialize: No controller\n");
        #endif
        return;
    }
    
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB] Host Controller IO Base: 0x");
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 28) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 24) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 20) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 16) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 12) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 8) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[(usb_hc_iobase >> 4) & 0xF]);
    serial_putchar(SERIAL_PORT_A, "0123456789ABCDEF"[usb_hc_iobase & 0xF]);
    serial_puts(SERIAL_PORT_A, "\n");
    #endif
}

void usb_scan_devices(void) {
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB] Scanning for devices...\n");
    #endif
    
    if (usb_hc_type == -1) {
        return;
    }
    
    /* Devices will be enumerated here */
    #ifdef CONFIG_SERIAL_DRIVER
    serial_puts(SERIAL_PORT_A, "[USB] Device scan complete\n");
    #endif
}
