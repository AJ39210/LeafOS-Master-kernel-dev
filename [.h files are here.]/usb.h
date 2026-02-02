#ifndef USB_H
#define USB_H

#include <stdint.h>

/* USB Host Controller Types */
#define USB_HC_UHCI 0x00
#define USB_HC_OHCI 0x01
#define USB_HC_EHCI 0x20

/* USB Device Classes */
#define USB_CLASS_HID     0x03
#define USB_CLASS_MASS_STORAGE 0x08

/* USB Request Types */
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_ADDRESS    0x05
#define USB_REQ_SET_CONFIG     0x09

/* USB Descriptor Types */
#define USB_DESC_DEVICE 0x01
#define USB_DESC_CONFIG 0x02
#define USB_DESC_STRING 0x03
#define USB_DESC_INTERFACE 0x04
#define USB_DESC_ENDPOINT 0x05

/* USB Device States */
#define USB_STATE_NOTATTACHED 0
#define USB_STATE_ATTACHED    1
#define USB_STATE_POWERED     2
#define USB_STATE_RECONNECT   3
#define USB_STATE_DEFAULT     4
#define USB_STATE_ADDRESS     5
#define USB_STATE_CONFIGURED  6

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t state;
    uint8_t address;
    uint8_t speed;
    void *hc_private;
} usb_device_t;

void usb_init(void);
void usb_scan_devices(void);
int usb_detect_host_controller(void);

#endif
