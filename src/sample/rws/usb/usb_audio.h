#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__
#include <stdint.h>

#include "usb_audio_driver.h"

#define USB_AUDIO_CTRL_CB               BOOL_USB_AUDIO_CB
#define USB_AUDIO_STREAM_CB             BOOL_USB_AUDIO_CB

typedef enum
{
    CTRL_CB_MIN             = USB_AUDIO_DRIVER_CB_SPK_VOL_SET,
    CTRL_CB_SPK_VOL_SET     = USB_AUDIO_DRIVER_CB_SPK_VOL_SET - CTRL_CB_MIN,
    CTRL_CB_SPK_VOL_GET     = USB_AUDIO_DRIVER_CB_SPK_VOL_GET - CTRL_CB_MIN,
    CTRL_CB_SPK_MUTE_SET    = USB_AUDIO_DRIVER_CB_SPK_MUTE_SET - CTRL_CB_MIN,
    CTRL_CB_MIC_VOL_SET     = USB_AUDIO_DRIVER_CB_MIC_VOL_SET - CTRL_CB_MIN,
    CTRL_CB_MIC_VOL_GET     = USB_AUDIO_DRIVER_CB_MIC_VOL_GET - CTRL_CB_MIN,
    CTRL_CB_MIC_MUTE_SET    = USB_AUDIO_DRIVER_CB_MIC_MUTE_SET - CTRL_CB_MIN,
    CTRL_CB_MAX,
} T_CTRL_CB_IDX;

typedef enum
{
    DRIVER_CB_MIN           = USB_AUDIO_DRIVER_CB_ACTIVE,
    DRIVER_CB_ACTIVE        = USB_AUDIO_DRIVER_CB_ACTIVE - DRIVER_CB_MIN,
    DRIVER_CB_DEACTIVE      = USB_AUDIO_DRIVER_CB_DEACTIVE - DRIVER_CB_MIN,
    DRIVER_CB_DATA_TRANS    = USB_AUDIO_DRIVER_CB_DATA_XMIT - DRIVER_CB_MIN,
    DRIVER_CB_MAX,
} T_STREAM_CB_IDX;

typedef enum
{
    USB_AUDIO_VOL_TYPE_CUR,
    USB_AUDIO_VOL_TYPE_RANGE,
} T_USB_AUDIO_VOL_TYPE;

typedef struct _usb_audio_vol
{
    uint32_t      type: 16;
    uint32_t      value: 16;
} T_USB_AUDIO_VOL;

void usb_audio_init(void);
void usb_audio_ctrl_cb_register(USB_AUDIO_CTRL_CB *cbs);
void usb_audio_streaming_cb_register(USB_AUDIO_STREAM_CB *cbs);
#endif
