#ifndef _USB_AUDIO_CONFIG_H_
#define _USB_AUDIO_CONFIG_H_
#include "usb_audio_driver.h"

#define USB_AUDIO_VERSION           USB_AUDIO_VERSION_2

#if(USB_AUDIO_VERSION == USB_AUDIO_VERSION_2)
#define UAC2_BIT_RES_16                      16
#define UAC2_BIT_RES_24                      24

#define UAC2_SPK_BIT_RES                    UAC2_BIT_RES_24

#define UAC2_SPK_SAM_RATE_NUM               3
#define UAC2_SPK_SAM_RATE_16000             16000
#define UAC2_SPK_SAM_RATE_48000             48000
#define UAC2_SPK_SAM_RATE_96000             96000
#define UAC2_SPK_SAM_RATE_192000            192000

#define UAC2_SPK_VOL_RANGE_NUM              1
#define UAC2_SPK_VOL_CUR                    0xF000
#define UAC2_SPK_VOL_MIN                    0xDB00
#define UAC2_SPK_VOL_MAX                    0x0000
#define UAC2_SPK_VOL_RES                    0x0100

#else
#define UAC1_BIT_RES_16                      16
#define UAC1_BIT_RES_24                      24

#define UAC1_SPK_SAM_RATE_NUM               3
#define UAC1_SPK_SAM_RATE_48000             48000
#define UAC1_SPK_SAM_RATE_96000             96000
#define UAC1_SPK_SAM_RATE_192000            192000

#define UAC1_SPK_VOL_CUR                    0xF600
#define UAC1_SPK_VOL_MIN                    0xDB00
#define UAC1_SPK_VOL_MAX                    0x0000
#define UAC1_SPK_VOL_RES                    0x0100
#endif

#endif

