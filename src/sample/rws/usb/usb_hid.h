#ifndef _HID_H_
#define _HID_H_
#include <stdint.h>
#include <stdbool.h>

bool usb_hid_data_send(void *handle, void *buf, uint32_t len, void(*cb)());
void usb_hid_init(void);
#endif
