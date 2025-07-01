#include <stdint.h>
#include "usb_std_desc.h"
#include "usb_composite_driver.h"

typedef enum
{
    STRING_ID_UNDEFINED,
    STRING_ID_MANUFACTURER,
    STRING_ID_PRODUCT,
    STRING_ID_SERIALNUM,
} T_STRING_ID;

static const T_USB_DEVICE_DESC usb_dev_desc =
{
    .bLength                = sizeof(T_USB_DEVICE_DESC),
    .bDescriptorType        = USB_DT_DEVICE,

    .bcdUSB                 = 0x0200,
//    .bDeviceClass           = 0xEF,
//    .bDeviceSubClass        = 0x02,
//    .bDeviceProtocol        = 0x01,
    .bDeviceClass          = 0x00,
    .bDeviceSubClass        = 0x00,
    .bDeviceProtocol        = 0x00,
    .bMaxPacketSize0        = 0x40,
    .idVendor               = 0x0bda,
    .idProduct              = 0x8773,
    .bcdDevice              = 0x0200,
    .iManufacturer          = STRING_ID_MANUFACTURER,
    .iProduct               = STRING_ID_PRODUCT,
    .iSerialNumber          = STRING_ID_SERIALNUM,
    .bNumConfigurations     = 1,
};

T_USB_STRING usb_dev_strings[] =
{
    [0] =
    {
        .id     = STRING_ID_MANUFACTURER,
        .string = "RealTek",
    },

    [1] =
    {
        .id     = STRING_ID_PRODUCT,
        .string = "USB Audio",
    },

    [2] =
    {
        .id     = STRING_ID_SERIALNUM,
        .string = "0123456789A",
    },
};

void usb_dev_desc_register(void)
{
    usb_composite_driver_desc_regster((void *)&usb_dev_desc);
    usb_composite_driver_string_regster((void *)&usb_dev_desc);
}

