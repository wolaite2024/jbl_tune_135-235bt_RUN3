
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "app_flags.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "dlps_util.h"
#include "trace.h"
#include "audio.h"
#include "sysm.h"
#include "remote.h"
#include "gap_legacy.h"
#include "gap_bond_legacy.h"
#include "gap_timer.h"
#include "gap.h"
#include "console_uart.h"
#include "bqb.h"
#include "cli_power.h"
#include "cli_lpm.h"
#if F_APP_CLI_CFG_SUPPORT
#include "cli_cfg.h"
#endif
#if F_APP_CLI_STRING_MP_SUPPORT
#include "cli_mp.h"
#endif
#if F_APP_CLI_BINARY_MP_SUPPORT
#include "mp_cmd.h"
#endif
#include "test_mode.h"
#include "single_tone.h"
#include "platform_utils.h"
#include "engage.h"
#include "app_gap.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_led.h"
#include "app_io_msg.h"
#include "app_ble_gap.h"
#include "app_ble_client.h"
#include "app_ble_service.h"
#include "app_key_process.h"
#include "app_charger.h"
#include "app_dlps.h"
#include "app_key_gpio.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_sdp.h"
#include "app_transfer.h"
#include "app_auto_power_off.h"
#include "app_device.h"
#include "app_audio_policy.h"
#include "app_a2dp.h"
#include "app_hfp.h"
#include "app_avrcp.h"
#if F_APP_BT_PROFILE_PBAP_SUPPORT
#include "app_pbap.h"
#endif
#include "app_iap.h"
#include "app_spp.h"
#include "app_cmd.h"
#include "app_bud_loc.h"
#include "app_ble_bond.h"
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#include "app_io_output.h"
#endif
#include "app_multilink.h"
#include "app_ble_device.h"
#if F_APP_TEST_SUPPORT
#include "app_test.h"
#endif
#include "app_gpio.h"
#include "app_sensor.h"
#include "app_in_out_box.h"
#include "app_mmi.h"
#include "system_status_api.h"
#include "os_ext.h"
#include "app_adc.h"
#include "app_ota.h"
#include "app_third_party_srv.h"
#include "app_bond.h"
#include "app_adp.h"
#include "app_io_output.h"
#include "app_adc.h"
#include "app_amp.h"

#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
#include "app_rtk_fast_pair_adv.h"
#endif

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
#include "app_one_wire_uart.h"
#endif

#if (F_APP_QDECODE_SUPPORT == 1)
#include "app_qdec.h"
#endif

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#endif

#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
#include "rtl876x_rtc.h"
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "version.h"
#include "app_harman_vendor_cmd.h"
#include "app_harman_license.h"
#include "app_harman_behaviors.h"
#endif

#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
#include "app_ext_charger.h"
#endif

#if GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#endif

#if HARMAN_SECURITY_CHECK
#include "hm_security.h"
#endif

#if HARMAN_NTC_DETECT_PROTECT
#include "app_harman_adc.h"
#endif

#define MAX_NUMBER_OF_GAP_MESSAGE       0x20    //!< indicate BT stack message queue size
#define MAX_NUMBER_OF_IO_MESSAGE        0x20    //!< indicate io queue size
#define MAX_NUMBER_OF_GAP_TIMER         0x10    //!< indicate gap timer queue size
#define MAX_NUMBER_OF_DSP_MSG           0x20    //!< number of dsp message reserved for DSP message handling.
#define MAX_NUMBER_OF_CODEC_MSG         0x10    //!< number of codec message reserved for CODEC message handling.
#define MAX_NUMBER_OF_ANC_MSG           0x10    //!< number of anc message reserved for ANC message handling.
#define MAX_NUMBER_OF_SYS_MSG           0x20    //!< indicate SYS timer queue size
#define MAX_NUMBER_OF_LOADER_MSG        0x20    //!< indicate Bin Loader queue size
/** indicate rx event queue size*/
#define MAX_NUMBER_OF_RX_EVENT      \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE  + MAX_NUMBER_OF_GAP_TIMER + \
     MAX_NUMBER_OF_DSP_MSG + MAX_NUMBER_OF_CODEC_MSG + MAX_NUMBER_OF_ANC_MSG + \
     MAX_NUMBER_OF_SYS_MSG + MAX_NUMBER_OF_LOADER_MSG)

#define DEFAULT_PAGESCAN_WINDOW             0x12
#define DEFAULT_PAGESCAN_INTERVAL           0x800 //0x800
#define DEFAULT_PAGE_TIMEOUT                0x2000
#define DEFAULT_SUPVISIONTIMEOUT            0x1f40 //0x7D00
#define DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define DEFAULT_INQUIRYSCAN_INTERVAL        0x1000 //0x800

#define CONSOLE_TX_BUFFER_LARGE             512
#define CONSOLE_RX_BUFFER_LARGE             512
#define CONSOLE_TX_BUFFER_SMALL             256
#define CONSOLE_RX_BUFFER_SMALL             256

void *audio_evt_queue_handle;
void *audio_io_queue_handle;

T_APP_DB app_db;

#if HARMAN_SECURITY_CHECK

#define HARMAN_SECURITY_DEBUG       0
#define HARMAN_PUBLICK_KEY_SIZE     260

#if HARMAN_SECURITY_DEBUG  // for test
uint8_t harman_public_key[HARMAN_PUBLICK_KEY_SIZE] =
// test chipID: [0x43, 0x2B, 0x15, 0xA4, 0x0F, 0xF4, 0xED, 0xAE, 0xA6, 0xFB, 0x48, 0xE3, 0x8D, 0x58, 0x51, 0xAC]
{
    0xdf, 0xd0, 0x30, 0xa5, 0x64, 0xbf, 0xf3, 0x1c, 0x5d, 0xab, 0x4f, 0x85, 0xd2, 0xdc, 0x48, 0x29, 0x08, 0x3f, 0x12, 0x21, 0x5d, 0xf6, 0x0c, 0x06, 0x39, 0x86, 0xaf, 0xca, 0x35, 0x48, 0x2e, 0xac,
    0xde, 0x12, 0xab, 0xa6, 0xb3, 0xe1, 0xf6, 0xd4, 0xa9, 0x66, 0xcd, 0x32, 0xd8, 0x85, 0xd1, 0xbc, 0x76, 0xe0, 0x67, 0x7d, 0x41, 0xfb, 0xd4, 0x0f, 0x65, 0xda, 0x8e, 0xdb, 0x66, 0xb0, 0xa0, 0x1a,
    0x59, 0xab, 0x39, 0x91, 0xf4, 0x33, 0x3f, 0x2d, 0x2d, 0x32, 0x16, 0x70, 0x86, 0xa7, 0x47, 0x80, 0xf3, 0xdf, 0x93, 0x80, 0x8e, 0xc4, 0x4e, 0xb3, 0xa7, 0x39, 0x40, 0x26, 0xba, 0x2a, 0xca, 0xac,
    0xaf, 0x20, 0x5b, 0xba, 0xf1, 0x3a, 0x67, 0x9b, 0xe3, 0x2d, 0x0f, 0xb3, 0x3d, 0xb2, 0xa8, 0x85, 0x36, 0xa2, 0xd2, 0x9a, 0x0f, 0xc2, 0x8f, 0x53, 0xeb, 0xcb, 0xa6, 0xa0, 0x01, 0x81, 0xba, 0x2b,
    0xcb, 0x47, 0xe8, 0xea, 0x0d, 0x27, 0x5e, 0x7f, 0x47, 0x61, 0xbc, 0xa1, 0x7f, 0x96, 0x27, 0xa4, 0xa2, 0xe2, 0x89, 0xd7, 0xcf, 0x29, 0xe3, 0x88, 0x73, 0x27, 0x6b, 0x01, 0x86, 0xd9, 0x03, 0x65,
    0x49, 0x39, 0xcf, 0x65, 0x6e, 0xe0, 0x46, 0x3b, 0xeb, 0x1a, 0xb7, 0x02, 0x79, 0xe9, 0x0b, 0x61, 0xd9, 0x67, 0xc4, 0x23, 0x9b, 0x20, 0xc9, 0xca, 0x13, 0x3d, 0x61, 0x11, 0xae, 0x50, 0x08, 0x2e,
    0x07, 0x6d, 0x77, 0xd6, 0x16, 0xac, 0x85, 0xd4, 0x59, 0xbe, 0x3d, 0x12, 0x93, 0x2e, 0x63, 0x63, 0xb9, 0x13, 0x75, 0x79, 0x98, 0xea, 0x8a, 0x3e, 0xb2, 0xee, 0x7e, 0x6f, 0x82, 0xb8, 0x41, 0x38,
    0xb7, 0xac, 0x15, 0xf1, 0x81, 0x32, 0x2a, 0x6e, 0xfc, 0xcf, 0x48, 0x0b, 0xfc, 0x8e, 0x28, 0xe0, 0xf3, 0xbb, 0x4d, 0xa6, 0xea, 0xad, 0xdd, 0xc7, 0x3f, 0x26, 0xcc, 0x0b, 0x19, 0x81, 0x0e, 0xcf,
    0x01, 0x0, 0x01, 0x0
};
#else // HARMAN_SECURITY_DEBUG

#if HARMAN_T135_SUPPORT
uint8_t harman_public_key[HARMAN_PUBLICK_KEY_SIZE] =
{
    0x83, 0xB1, 0x1E, 0x77, 0xF0, 0xF9, 0x8C, 0x98, 0x2C, 0xAE, 0x47, 0x05, 0xAC, 0x82, 0xE8, 0xD5,
    0x34, 0x41, 0x9E, 0xDE, 0x76, 0x35, 0x9F, 0x10, 0xCE, 0x7D, 0xEB, 0x7C, 0x5E, 0x93, 0xC8, 0xAE,
    0xE9, 0xCA, 0x1A, 0x67, 0xD7, 0x5E, 0x2F, 0x37, 0x4B, 0xBE, 0xC0, 0xDE, 0xC9, 0x95, 0x7A, 0xA,
    0xD5, 0xBC, 0x50, 0xAD, 0xA6, 0xF0, 0x61, 0x62, 0xF6, 0x96, 0xD2, 0x4A, 0xFF, 0xE2, 0xFD, 0x33,
    0x61, 0x6B, 0x5A, 0x29, 0x8D, 0x36, 0xB1, 0x7F, 0x7F, 0xF1, 0xF9, 0xEB, 0x50, 0x1B, 0x85, 0x4E,
    0x7A, 0xA5, 0xD8, 0xFA, 0xFB, 0xB4, 0x13, 0x98, 0x8E, 0x06, 0xD1, 0x5E, 0x8D, 0xB6, 0x28, 0xC8,
    0xB8, 0x6A, 0xFB, 0xAF, 0xFC, 0x6A, 0x4A, 0x92, 0xBA, 0x51, 0xC7, 0xB5, 0x6E, 0x2F, 0x6A, 0x7B,
    0xAE, 0x60, 0x38, 0xF1, 0x04, 0x9A, 0xA7, 0x8D, 0x5A, 0xF0, 0x61, 0x1D, 0x24, 0x13, 0xAF, 0xA,
    0x91, 0xDB, 0x81, 0xB6, 0x1F, 0x97, 0x0F, 0x6A, 0x69, 0x88, 0xDF, 0x70, 0xA1, 0xFB, 0x75, 0x3D,
    0x20, 0x98, 0xFB, 0x9D, 0x2A, 0x0C, 0xAB, 0x3B, 0xA1, 0x18, 0xF3, 0x20, 0xBA, 0xB5, 0x43, 0x65,
    0x14, 0xE8, 0xB3, 0x8F, 0xDA, 0xEA, 0x0B, 0x4C, 0x3C, 0xDF, 0x3F, 0x33, 0xD9, 0x1D, 0x5C, 0x80,
    0x3F, 0xBB, 0x12, 0x9D, 0x9C, 0x19, 0x70, 0xDF, 0xE0, 0x4A, 0xE8, 0x81, 0xA9, 0xCE, 0x74, 0xD7,
    0xE4, 0xD2, 0x23, 0x18, 0xA6, 0x4D, 0x20, 0xA7, 0x4C, 0xE0, 0x54, 0x01, 0x95, 0xFC, 0x84, 0x11,
    0x32, 0x9B, 0x81, 0x73, 0x20, 0x9D, 0xF2, 0xCE, 0x89, 0xF5, 0x3E, 0x37, 0xF7, 0x0D, 0x77, 0xCE,
    0x35, 0x66, 0xF3, 0xA3, 0x18, 0x79, 0xAE, 0x78, 0x0D, 0x94, 0xF1, 0xA5, 0x2E, 0x89, 0xA4, 0x4C,
    0xDB, 0x3F, 0xDF, 0xED, 0x56, 0x37, 0x11, 0x64, 0xD5, 0x32, 0xAA, 0xF0, 0x80, 0x6F, 0x90, 0xC0,
    0x01, 0x00, 0x01, 0x00
};

#elif HARMAN_T235_SUPPORT
uint8_t harman_public_key[HARMAN_PUBLICK_KEY_SIZE] =
{
    0x15, 0x30, 0xF4, 0x5E, 0x07, 0x85, 0x00, 0x22, 0x1C, 0x23, 0x40, 0x94, 0x4D, 0x62, 0xED, 0xAB,
    0x6A, 0x9C, 0x41, 0x39, 0x37, 0x81, 0x64, 0x6E, 0xF0, 0x70, 0x08, 0xF1, 0x74, 0x1F, 0xCB, 0x5E,
    0xAD, 0x50, 0x0A, 0xDA, 0xC8, 0x08, 0x71, 0x46, 0x3C, 0xEB, 0x7B, 0x6C, 0x9E, 0xEB, 0xA6, 0x6F,
    0x2E, 0xE1, 0x3D, 0x79, 0xA4, 0x65, 0x8C, 0xF8, 0x29, 0x72, 0x57, 0xD1, 0x4D, 0xDD, 0x8A, 0x90,
    0x0A, 0x9A, 0x6B, 0xF6, 0x0F, 0xB5, 0xE8, 0x32, 0x81, 0xB4, 0x52, 0x45, 0x41, 0x9C, 0x1F, 0x8E,
    0x56, 0x45, 0xAE, 0xF4, 0x43, 0x14, 0xF6, 0xE3, 0xFC, 0x07, 0x69, 0x05, 0x26, 0x73, 0x60, 0x20,
    0x3F, 0x4E, 0xCD, 0xCB, 0x52, 0x20, 0x97, 0x8D, 0xF3, 0xB4, 0x24, 0x27, 0xCF, 0x38, 0x5A, 0xC6,
    0xB9, 0x1A, 0xCC, 0x91, 0xE3, 0xDE, 0xBD, 0xEF, 0x66, 0x58, 0x5E, 0x77, 0xFA, 0x26, 0xFA, 0xC1,
    0x47, 0x56, 0x10, 0x8F, 0x8B, 0x48, 0x8E, 0x0A, 0x4C, 0x50, 0x19, 0xD4, 0x9A, 0x0F, 0x9A, 0xBC,
    0xDF, 0xA6, 0xA7, 0x29, 0xF3, 0x4A, 0xCC, 0xD5, 0xAC, 0xE4, 0xFB, 0x42, 0x5A, 0x18, 0xD9, 0x56,
    0x38, 0xEE, 0x0F, 0xA0, 0xFE, 0x6F, 0xDD, 0x75, 0x53, 0xC7, 0xA7, 0x4C, 0xB7, 0x9C, 0x3E, 0x02,
    0x3C, 0x13, 0xB8, 0x59, 0xF8, 0xF9, 0x5D, 0x48, 0x61, 0x45, 0x3C, 0x84, 0xEE, 0x8D, 0x67, 0x27,
    0xD6, 0x6C, 0x3A, 0x41, 0xEF, 0x9B, 0x0B, 0x05, 0x82, 0x44, 0xB3, 0xE5, 0xDE, 0x71, 0xBD, 0x63,
    0xD8, 0x41, 0xFE, 0x56, 0x41, 0xD7, 0x28, 0xBD, 0x26, 0xA9, 0x23, 0x1F, 0x74, 0x79, 0x8A, 0x3F,
    0xE7, 0x0E, 0xE6, 0x5F, 0xB3, 0xB8, 0x76, 0x79, 0x68, 0x27, 0xB4, 0x4D, 0x00, 0x33, 0x64, 0xE3,
    0xF5, 0x48, 0xD5, 0x0A, 0xD2, 0x45, 0x79, 0x6F, 0x20, 0x44, 0x46, 0x10, 0xB5, 0x81, 0x3C, 0xB0,
    0x01, 0x00, 0x01, 0x00
};

#elif HARMAN_RUN3_SUPPORT
uint8_t harman_public_key[HARMAN_PUBLICK_KEY_SIZE] =
{
    0xD3, 0x93, 0x2B, 0xED, 0x3F, 0xB5, 0xA4, 0x5F, 0xD7, 0x86, 0x16, 0xE7, 0x87, 0x0F, 0xF5, 0x06, 0xB4, 0x7C, 0xAB, 0xA4, 0xE9, 0xC9, 0x14, 0xCA, 0x0E, 0x9E, 0xAE, 0xBB, 0x0E, 0x0C, 0x0A, 0x15,
    0x4D, 0xAF, 0xA7, 0xAD, 0xB0, 0xD1, 0xD8, 0x12, 0x6B, 0x42, 0xDB, 0x7A, 0x9F, 0xB7, 0x09, 0xC7, 0xF5, 0x9B, 0xA6, 0x93, 0xCF, 0xB5, 0xE4, 0xA6, 0x58, 0xCD, 0x5E, 0x59, 0x87, 0x0C, 0x50, 0xE9,
    0xC5, 0xDA, 0x48, 0xCA, 0x44, 0xC6, 0xD5, 0x3A, 0x5C, 0xBA, 0x66, 0xB3, 0xDE, 0xD4, 0x3C, 0x00, 0x83, 0xEA, 0xA8, 0x5F, 0x64, 0xA3, 0x62, 0x31, 0x5A, 0x9D, 0xFC, 0x29, 0x8E, 0xEA, 0x41, 0x74,
    0xDF, 0x0A, 0x99, 0x3A, 0xDC, 0xE2, 0x76, 0xAD, 0x76, 0x7A, 0x85, 0x39, 0x98, 0xE0, 0xF8, 0x3C, 0x3B, 0x1F, 0x0F, 0xE7, 0xE5, 0xF4, 0xDF, 0x65, 0x1D, 0xC5, 0x3F, 0xF8, 0xF1, 0xCB, 0xDB, 0x7C,
    0x4E, 0xAD, 0xC7, 0x8B, 0xF4, 0xB0, 0xE1, 0xDF, 0xE1, 0xD7, 0xA6, 0x28, 0xD8, 0xCA, 0x88, 0xAD, 0x29, 0xD5, 0x9C, 0x11, 0xF4, 0xCB, 0x09, 0x95, 0xA5, 0x65, 0xBC, 0x4A, 0x1E, 0x2A, 0xB5, 0x78,
    0xBE, 0x9B, 0xD9, 0x00, 0x0D, 0x12, 0xDD, 0xDD, 0x9F, 0xF9, 0x22, 0x5C, 0x8A, 0xF7, 0xDA, 0x3B, 0x9D, 0xD8, 0x89, 0x0C, 0x04, 0x61, 0x9C, 0xAD, 0x85, 0xAC, 0x76, 0x62, 0x8B, 0x9E, 0x41, 0xF0,
    0xC5, 0x81, 0xBE, 0x34, 0xE5, 0xB9, 0x6D, 0x63, 0xFA, 0x76, 0xD5, 0xD8, 0x5E, 0x8E, 0x58, 0x47, 0xA5, 0xDF, 0x33, 0xC2, 0xC5, 0x15, 0xE7, 0x43, 0x0D, 0x30, 0x71, 0x9A, 0x00, 0x3E, 0x7E, 0xA4,
    0x50, 0x03, 0xBE, 0xC5, 0x19, 0x00, 0x05, 0x64, 0xBB, 0xF8, 0x1B, 0x63, 0x74, 0x4D, 0x59, 0x6D, 0xF1, 0x78, 0x6A, 0xA4, 0x31, 0xA9, 0x4E, 0xF5, 0xDB, 0x3E, 0x48, 0x64, 0xD0, 0xA2, 0xA9, 0xC4,
    0x01, 0x00, 0x01, 0x00
};

#elif HARMAN_PACE_SUPPORT
uint8_t harman_public_key[HARMAN_PUBLICK_KEY_SIZE] =
{
    0x07, 0x3D, 0x4C, 0xF9, 0x06, 0xDF, 0xCC, 0x24, 0xC0, 0x99, 0xAF, 0xF7, 0xB9, 0x02, 0x6D, 0x45, 0xDE, 0x4B, 0x20, 0x34, 0x8B, 0x88, 0x65, 0x51, 0xA6, 0x9A, 0x3F, 0x6D, 0x8D, 0xC3, 0x93, 0xCD,
    0xC8, 0x8B, 0xB0, 0xD0, 0xAF, 0x13, 0xB6, 0x6D, 0xD2, 0xF9, 0x80, 0x7F, 0x8D, 0xE8, 0x50, 0x76, 0x3B, 0x72, 0x6F, 0xF6, 0xB2, 0x72, 0x89, 0xA7, 0x7E, 0x6B, 0xCC, 0x62, 0x75, 0x39, 0xFD, 0x5B,
    0x08, 0xB7, 0xBC, 0xCE, 0xCF, 0xDF, 0x30, 0xB6, 0x4C, 0x43, 0x89, 0x35, 0x48, 0xE4, 0x9D, 0x76, 0x5E, 0x37, 0x5F, 0x0D, 0x98, 0x07, 0xE2, 0x31, 0x5D, 0x80, 0x7C, 0x72, 0x8B, 0x84, 0x9F, 0xC9,
    0x85, 0x71, 0x2A, 0x37, 0xE6, 0x36, 0xC2, 0xFC, 0x07, 0x05, 0xA4, 0x5F, 0x4D, 0x05, 0xF1, 0xD6, 0xF9, 0x81, 0xF4, 0x6B, 0x02, 0xDA, 0x35, 0xE7, 0x81, 0x39, 0x92, 0x6D, 0xAF, 0x06, 0xB2, 0x10,
    0x02, 0xB4, 0xC1, 0x3B, 0x71, 0x37, 0x8D, 0xD5, 0xA9, 0xE0, 0x8D, 0xCD, 0x3E, 0x46, 0x1F, 0x4A, 0x11, 0xEC, 0xB2, 0x5D, 0x33, 0x53, 0x54, 0x2D, 0xED, 0x00, 0x12, 0xBA, 0xCD, 0x00, 0xF5, 0xEC,
    0x25, 0x6B, 0x80, 0xBF, 0x26, 0x37, 0xD3, 0x37, 0xB6, 0x7F, 0x63, 0xFC, 0x3B, 0x9E, 0x22, 0x38, 0x00, 0xCB, 0x42, 0x19, 0xD7, 0x15, 0xF8, 0x2D, 0xC0, 0xC7, 0x82, 0xA3, 0x93, 0x3E, 0xAA, 0x99,
    0x81, 0x2A, 0x72, 0x85, 0xA3, 0x06, 0x3D, 0x28, 0x50, 0x92, 0xA5, 0x6B, 0x15, 0x8A, 0xE3, 0x72, 0x16, 0x05, 0xC3, 0xF6, 0xEC, 0xE9, 0x32, 0x82, 0xC7, 0x78, 0xA6, 0xAE, 0x30, 0x1C, 0xA8, 0xF8,
    0x3D, 0x13, 0x13, 0xC2, 0xF2, 0xFE, 0x2B, 0x9F, 0x84, 0xC3, 0x20, 0x01, 0x8E, 0x3E, 0x73, 0x5C, 0x48, 0xF7, 0x4B, 0x25, 0xAB, 0x91, 0x62, 0x2B, 0x83, 0xB0, 0x2D, 0x16, 0x39, 0xFC, 0xED, 0x98,
    0x01, 0x00, 0x01, 0x00
};
#endif  // HARMAN_T135_SUPPORT

#endif // HARMAN_SECURITY_DEBUG

int32_t harmain_public_key_read(uint8_t *public_key_buf, uint16_t buf_size)
{
    if (public_key_buf == NULL || buf_size < HARMAN_PUBLICK_KEY_SIZE)
    {
        return 0;
    }
    else
    {
        memcpy(public_key_buf, harman_public_key, HARMAN_PUBLICK_KEY_SIZE);
        return HARMAN_PUBLICK_KEY_SIZE;
    }
}
#endif // HARMAN_SECURITY_CHECK


/**
* @brief board_init() contains the initialization of pinmux settings and pad settings.
*
*   All the pinmux settings and pad settings shall be initiated in this function.
*   But if legacy driver is used, the initialization of pinmux setting and pad setting
*   should be peformed with the IO initializing.
*
* @return void
*/
static void board_init(void)
{
#if FPGA_CPU_CLK_ALLOW_COMPLEMENT
    //platform_set_cpu_freq(app_cfg_const.cpu_freq_option);
//    set_clock_gen_rate(CLK_CPU, CLK_80MHZ, CLK_40MHZ);
    uint32_t cpu_freq;
    int32_t ret = pm_cpu_freq_set(80, &cpu_freq);
    if (ret != 0)
    {
        APP_PRINT_ERROR2("cpu freq config CLK_80MHZ fail ret %x real freq %dMHz", ret, cpu_freq);
    }
    else
    {
        APP_PRINT_INFO1("cpu freq %dMHz ", cpu_freq);
    }
#endif

#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
    app_dlps_system_wakeup_clear_rtc_int();
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
    app_harman_set_cpu_clk(true);
    app_harman_cpu_clk_improve();
#endif

    //Config DATA UART0 pinmux. For external mcu
    if (app_cfg_const.enable_data_uart)
    {
        //when select 4pogo download and uart tx shares the same pin with 3pin gpio, don't config uart tx pinmux in here
        if ((app_cfg_const.enable_4pogo_pin == 0) ||
            (app_cfg_const.gpio_box_detect_pinmux != app_cfg_const.data_uart_tx_pinmux))
        {
            Pinmux_Config(app_cfg_const.data_uart_tx_pinmux, UART0_TX);
            Pad_Config(app_cfg_const.data_uart_tx_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }

        Pinmux_Config(app_cfg_const.data_uart_rx_pinmux, UART0_RX);
        Pad_Config(app_cfg_const.data_uart_rx_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        if (app_cfg_const.enable_rx_wake_up)
        {
            Pinmux_Config(app_cfg_const.rx_wake_up_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.rx_wake_up_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }

        if (app_cfg_const.enable_tx_wake_up)
        {
            Pinmux_Config(app_cfg_const.tx_wake_up_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.tx_wake_up_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        }
    }

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
    if (app_cfg_const.one_wire_uart_support)
    {
        if (app_cfg_const.one_wire_uart_gpio_support)
        {
            Pad_Config(app_cfg_const.one_wire_uart_gpio_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, ONE_WIRE_GPIO_5V_PULL, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_PullConfigValue(app_cfg_const.one_wire_uart_gpio_pinmux, PAD_STRONG_PULL);
        }

        Pinmux_Config(app_cfg_const.one_wire_uart_data_pinmux, IDLE_MODE);
        Pad_Config(app_cfg_const.one_wire_uart_data_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
#endif

    //app_case_detect_gpio_board_init();
#if F_APP_GPIO_ONOFF_SUPPORT
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        app_gpio_detect_onoff_board_init(app_cfg_const.gpio_box_detect_pinmux);
    }
#endif
    //Config  UART3 pinmux. For DSP debug
    if (app_cfg_const.dsp_log_output_select == DSP_OUTPUT_LOG_BY_UART)
    {
        Pinmux_Config(app_cfg_const.dsp_log_pin, UART2_TX);
        Pad_Config(app_cfg_const.dsp_log_pin,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }

    //Config I2C0 pinmux. For CP control
    if ((app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK) ||
        (app_cfg_const.gsensor_support) ||
        (app_cfg_const.psensor_support && (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS773)) ||
        (app_cfg_const.psensor_support && (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS873)) ||
        (app_cfg_const.sensor_support && (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)) ||
        (app_cfg_const.sensor_support && (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225)) ||
        (app_cfg_const.sensor_support && (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)) ||
        (app_cfg_const.sensor_support && (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_SC7A20)))
    {
        Pinmux_Config(app_cfg_const.i2c_0_dat_pinmux, I2C0_DAT);
        Pinmux_Config(app_cfg_const.i2c_0_clk_pinmux, I2C0_CLK);

        if (app_cfg_const.chip_supply_power_to_light_sensor_digital_VDD)
        {
            Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
        else
        {
            Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pad_PullConfigValue(app_cfg_const.i2c_0_dat_pinmux, PAD_STRONG_PULL);
            Pad_PullConfigValue(app_cfg_const.i2c_0_clk_pinmux, PAD_STRONG_PULL);
        }
    }

    //Config Key GPIO pinmux
    if (app_cfg_const.key_gpio_support)
    {
        uint8_t i = 0;

        for (i = 0; i < MAX_KEY_NUM; i++)
        {
            if (app_cfg_const.key_enable_mask & BIT(i))
            {
                Pinmux_Config(app_cfg_const.key_pinmux[i], DWGPIO);
                if (app_cfg_const.key_high_active_mask & BIT(i)) // high active
                {
                    Pad_Config(app_cfg_const.key_pinmux[i],
                               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
                }
                else  // low active
                {
                    Pad_Config(app_cfg_const.key_pinmux[i],
                               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
                }
            }
        }
    }

#if F_APP_LINEIN_SUPPORT
    //Config Line-in pinmux
    if (app_cfg_const.line_in_support)
    {
        Pinmux_Config(app_cfg_const.line_in_pinmux, DWGPIO);
        Pad_Config(app_cfg_const.line_in_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
#endif

#if F_APP_NFC_SUPPORT
    //Config NFC pinmux
    if (app_cfg_const.nfc_support)
    {
        app_gpio_nfc_board_init();
    }
#endif
#if F_APP_LINEIN_SUPPORT
    //config line-in pinmux
    if (app_cfg_const.line_in_support)
    {
        app_line_in_board_init();
    }
#endif
    //config sensor pinmux
    if (app_cfg_const.sensor_support) //sensor_support
    {
        if (0)
        {
            /* for feature define; do nothing */
        }
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX) //sensor_vendor
        {
            //Light detect trigger pad config, High: disable light detect
            Pad_Config(app_cfg_const.sensor_detect_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                       PAD_OUT_ENABLE,
                       PAD_OUT_HIGH);
            Pad_SetPinDrivingCurrent(app_cfg_const.sensor_detect_pinmux, MODE_16MA);

            //Light detect result pad config, Low: out ear
            Pinmux_Config(app_cfg_const.sensor_result_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.sensor_result_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);
        }
#endif
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225 ||
                 app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
        {
            Pinmux_Config(app_cfg_const.sensor_detect_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.sensor_detect_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
                       PAD_OUT_DISABLE, PAD_OUT_HIGH);
        }
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IO)
        {
            Pinmux_Config(app_cfg_const.sensor_detect_pinmux, DWGPIO);

            if (app_cfg_const.iosensor_active)
            {
                Pad_Config(app_cfg_const.sensor_detect_pinmux,
                           PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
            }
            else
            {
                Pad_Config(app_cfg_const.sensor_detect_pinmux,
                           PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
            }
        }
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)
        {
            Pinmux_Config(app_cfg_const.sensor_detect_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.sensor_detect_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
                       PAD_OUT_DISABLE, PAD_OUT_HIGH);
        }
    }

    //config gsensor pinmux
    if ((app_cfg_const.gsensor_support) || (app_cfg_const.psensor_support))
    {
        //INT pad config
        Pinmux_Config(app_cfg_const.gsensor_int_pinmux, DWGPIO);
        Pad_Config(app_cfg_const.gsensor_int_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    }

    //config output 10hz pinmux
    if (app_cfg_const.enable_external_mcu_reset && app_cfg_const.external_mcu_input_pinmux != 0xff)
    {
        //pad output no need Pinmux_Config
        Pad_Config(app_cfg_const.external_mcu_input_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN,
                   PAD_OUT_ENABLE, PAD_OUT_LOW);
    }

    //config amp pinmux
    if (app_cfg_const.enable_ctrl_ext_amp)
    {
        Pinmux_Config(app_cfg_const.ext_amp_pinmux, DWGPIO);
        if (app_cfg_const.enable_ext_amp_high_active)
        {
            Pad_Config(app_cfg_const.ext_amp_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }
        else
        {
            Pad_Config(app_cfg_const.ext_amp_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
        }
    }

    //config thermistor power pinmux
    if (app_cfg_const.thermistor_power_support)
    {
        if (app_cfg_const.thermistor_power_pinmux != 0xFF)
        {
            Pad_Config(app_cfg_const.thermistor_power_pinmux,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
        }
    }

    //config pcba shipping mode pinmux
    if (app_cfg_const.pcba_shipping_mode_pinmux != 0xFF)
    {
        Pad_Config(app_cfg_const.pcba_shipping_mode_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }

#if F_APP_IO_OUTPUT_SUPPORT
    //config out indication pinmux
    app_io_output_out_indication_pinmux_init();
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
    app_slide_switch_board_init();
#endif

#if (F_APP_QDECODE_SUPPORT == 1)
    if (app_cfg_const.wheel_support)
    {
        app_qdec_pad_config();
    }
#endif

#if (F_APP_GPIO_MICBIAS_SUPPORT == 1)
    //for no micbias pin IC , do micbias control gpio pad init in app
    if (app_cfg_const.micbias_pinmux != 0xFF && app_cfg_const.micbias_pinmux != MICBIAS)
    {
        Pad_Config(app_cfg_const.micbias_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
#endif
}

static void driver_init(void)
{
    if (app_cfg_const.key_gpio_support)
    {
        key_gpio_initial();
        app_key_init();

        if (app_cfg_const.enable_hall_switch_function) // hall switch support
        {
            // Check if there are hall switch definitions
            app_key_hybird_parse_hall_switch_setting();

            // Read the hall switch status
            app_db.hall_switch_status = app_key_read_hall_switch_status();
        }
    }

    if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
    {
#if F_APP_IAP_SUPPORT
        app_cp_hw_init(I2C0);
#endif
    }

    if (app_cfg_const.enable_data_uart || app_cfg_const.one_wire_uart_support)
    {
#if F_APP_CONSOLE_SUPPORT
        T_CONSOLE_PARAM console_param;
        T_CONSOLE_OP    console_op;

#if F_APP_ANC_SUPPORT
        if (anc_tool_check_resp_meas_mode() == ANC_RESP_MEAS_MODE_ENTER)
        {
            console_param.tx_buf_size   = CONSOLE_TX_BUFFER_LARGE;
            console_param.rx_buf_size   = CONSOLE_RX_BUFFER_LARGE;
        }
        else
#endif
        {
            console_param.tx_buf_size   = CONSOLE_TX_BUFFER_SMALL;
            console_param.rx_buf_size   = CONSOLE_RX_BUFFER_SMALL;
        }

        console_param.tx_wakeup_pin = app_cfg_const.tx_wake_up_pinmux;
        console_param.rx_wakeup_pin = app_cfg_const.rx_wake_up_pinmux;

        console_op.init = console_uart_init;
        console_op.tx_wakeup_enable = NULL; //console_uart_tx_wakeup_enable;
        console_op.rx_wakeup_enable = NULL; //console_uart_rx_wakeup_enable;
#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_const.one_wire_uart_support)
        {
            console_op.write = console_one_wire_uart_write;
        }
        else
#endif
        {
            console_op.write = console_uart_write;
        }
        console_op.wakeup = console_uart_wakeup;

        console_init(&console_param, &console_op);
#if ISOC_AUDIO_SUPPORT
        le_audio_cmd_register();
#endif
#endif

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_const.one_wire_uart_support)
        {
            console_set_mode(CONSOLE_MODE_BINARY);
            app_one_wire_init();
        }
#endif

#if F_APP_CLI_BINARY_MP_SUPPORT
        mp_cmd_module_init();
#endif

#if F_APP_BQB_CLI_SUPPORT
        bqb_cmd_register();
#endif
        power_cmd_register();

        lpm_cmd_register();

#if F_APP_CLI_BINARY_MP_SUPPORT
#if F_APP_TUYA_SUPPORT
        tuya_cmd_register();
        console_set_mode(CONSOLE_MODE_BINARY);
#else
        mp_cmd_register();
#endif
#endif

#if F_APP_CLI_CFG_SUPPORT
        cfg_cmd_register();
#endif

#if F_APP_CLI_STRING_MP_SUPPORT
        mp_cmd_str_register();
#endif
#if ISOC_TEST_SUPPORT
        isoc_cmd_register();
#endif

#if F_APP_SS_SUPPORT
        app_ss_cmd_register();
#endif
    }

#if F_APP_GPIO_ONOFF_SUPPORT
    //app_case_detect_gpio_driver_init();
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        app_gpio_detect_onoff_driver_init(app_cfg_const.gpio_box_detect_pinmux);
    }
#endif

#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
    if (app_cfg_const.charger_support)
    {
        app_ext_charger_init();
    }
#endif
    if (app_cfg_const.charger_support)
    {
        app_charger_init();
    }

    if (app_cfg_const.led_support)
    {
        led_init();
    }

#if F_APP_USB_AUDIO_SUPPORT
    if (app_cfg_const.usb_audio_support)
    {
//        app_usb_audio_init();
    }
#endif
#if F_APP_NFC_SUPPORT
    if (app_cfg_const.nfc_support)
    {
        app_nfc_init();
    }
#endif
#if F_APP_LINEIN_SUPPORT
    if (app_cfg_const.line_in_support)
    {
        app_line_in_driver_init();
        app_dlps_pad_wake_up_polarity_invert(app_cfg_const.line_in_pinmux);
    }
#endif
#if F_APP_AMP_SUPPORT
    if (app_cfg_const.enable_ctrl_ext_amp)
    {
        app_amp_init();
    }
#endif

#if (F_APP_SENSOR_SUPPORT == 1)
    if (app_cfg_const.sensor_support) //sensor_support
    {
        if (0)
        {
            /* for feature define; do nothing */
        }
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX) //sensor_vendor
        {
            app_sensor_ld_init();
        }
#endif
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225)
        {
            if (app_cfg_const.chip_supply_power_to_light_sensor_digital_VDD)
            {
                /* Init JSA1225 after output power supply pin pull high */
            }
            else
            {
                sensor_i2c_init(SENSOR_ADDR_JSA);
                app_sensor_jsa1225_init();
                sensor_int_gpio_init(app_cfg_const.sensor_detect_pinmux);
            }
        }
#endif
#if (F_APP_SENSOR_JSA1227_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
        {
            if (app_cfg_const.chip_supply_power_to_light_sensor_digital_VDD)
            {
                /* Init JSA1227 after output power supply pin pull high */
            }
            else
            {
                sensor_i2c_init(SENSOR_ADDR_JSA1227);
                app_sensor_jsa1227_init();
                sensor_int_gpio_init(app_cfg_const.sensor_detect_pinmux);
            }
        }
#endif
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IO)
        {
            app_io_sensor_ld_init();
            app_dlps_pad_wake_up_polarity_invert(app_cfg_const.sensor_detect_pinmux);
        }
#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)
        {
            if (app_cfg_const.chip_supply_power_to_light_sensor_digital_VDD)
            {
                /* Init PX318J after output power supply pin pull high */
            }
            else
            {
                sensor_i2c_init(PX318J_ID);
                app_sensor_px318j_init();
                sensor_int_gpio_init(app_cfg_const.sensor_detect_pinmux);
            }
        }
#endif
#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_SC7A20)
        {
            sensor_i2c_init(GSENSOR_I2C_SLAVE_ADDR_SILAN);
            app_sensor_sc7a20_as_ls_init();
        }
#endif
    }

#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
    if (app_cfg_const.gsensor_support)
    {
        //P sensor and G sensor will not coexist
        //Re-use gsensor INT pinmux
        app_gsensor_init();
        app_dlps_pad_wake_up_polarity_invert(app_cfg_const.gsensor_int_pinmux);
    }
#endif

    if (app_cfg_const.psensor_support)
    {
        //P sensor and G sensor will not coexist
        //Re-use gsensor INT pinmux

        if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IO)
        {
            app_int_psensor_init();
            app_dlps_pad_wake_up_polarity_invert(app_cfg_const.gsensor_int_pinmux);
        }
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
        else if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS773)
        {
            sensor_i2c_init(I2C_AZQ_ADDR);
            if (i2c_iqs_check_device() != IQS773_MODULE)
            {
                APP_PRINT_ERROR0("No IQS773 module detected");
                //assert(0);
            }
            else
            {
                i2c_iqs773_setup();
                sensor_int_gpio_init(app_cfg_const.gsensor_int_pinmux);
                app_dlps_pad_wake_up_polarity_invert(app_cfg_const.gsensor_int_pinmux);
                i2c_iqs_initial();
            }

        }
        else if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS873)
        {
            sensor_i2c_init(I2C_AZQ_ADDR);
            if (i2c_iqs_check_device() != IQS873_MODULE)
            {
                APP_PRINT_ERROR0("No IQS873 module detected");
                //assert(0);
            }
            else
            {
                i2c_iqs873_setup();
                sensor_int_gpio_init(app_cfg_const.gsensor_int_pinmux);
                app_dlps_pad_wake_up_polarity_invert(app_cfg_const.gsensor_int_pinmux);
                i2c_iqs_initial();
            }

        }
#endif
    }

    if ((app_cfg_const.sensor_support) || (app_cfg_const.gsensor_support))
    {
        app_sensor_init();
    }
#endif

    if (!(app_cfg_const.key_enable_mask & KEY0_MASK))
    {
        key_mfb_init();
    }

#if (APP_CAP_TOUCH_SUPPORT == 1)
    if (CapTouch_IsSystemEnable())
    {
        app_cap_touch_init(true);
    }
#endif

#if F_APP_GPIO_ONOFF_SUPPORT
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        app_gpio_on_off_set_pad_wkup_polarity();
    }
#endif

#if F_APP_LOSS_BATTERY_PROTECT
    app_device_loss_battery_gpio_driver_init();
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
    app_slide_switch_driver_init();
#endif

#if (F_APP_QDECODE_SUPPORT == 1)
    if (app_cfg_const.wheel_support)
    {
        app_qdec_init_status_read();
        app_qdec_driver_init();
    }
#endif

}

static void bt_gap_init(void)
{
    uint32_t class_of_device = (uint32_t)((app_cfg_const.class_of_device[0]) |
                                          (app_cfg_const.class_of_device[1] << 8) |
                                          (app_cfg_const.class_of_device[2] << 16));

    uint16_t supervision_timeout = DEFAULT_SUPVISIONTIMEOUT;

    uint16_t link_policy = GAP_LINK_POLICY_ROLE_SWITCH | GAP_LINK_POLICY_SNIFF_MODE;

    uint8_t radio_mode = GAP_RADIO_MODE_NONE_DISCOVERABLE;
    bool limited_discoverable = false;
    bool auto_accept_acl = false;

    uint8_t pagescan_type = GAP_PAGE_SCAN_TYPE_INTERLACED;
    uint16_t pagescan_interval = DEFAULT_PAGESCAN_INTERVAL;
    uint16_t pagescan_window = DEFAULT_PAGESCAN_WINDOW;
    uint16_t page_timeout = DEFAULT_PAGE_TIMEOUT;

    uint8_t inquiryscan_type = GAP_INQUIRY_SCAN_TYPE_INTERLACED;
    uint16_t inquiryscan_window = DEFAULT_INQUIRYSCAN_WINDOW;
    uint16_t inquiryscan_interval = DEFAULT_INQUIRYSCAN_INTERVAL;
    uint8_t inquiry_mode = GAP_INQUIRY_MODE_EXTENDED_RESULT;

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_GENERAL_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t bt_mode = GAP_BT_MODE_21ENABLED;

    if ((app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE) && (app_cfg_nv.factory_reset_done == 0))
    {
        /* change the scan interval to 100ms(0xA0 * 0.625) before factory reset */
        pagescan_interval = 0xA0;
        inquiryscan_interval = 0xA0;
    }
#if GFPS_FEATURE_SUPPORT
#if GFPS_SASS_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        pagescan_interval = SASS_PAGESCAN_INTERVAL;
        pagescan_window = SASS_PAGESCAN_WINDOW;
    }
#endif
#endif
    legacy_gap_init();
    gap_lib_init();

    //0: to be master
    legacy_cfg_accept_role(1);

    legacy_set_gap_param(GAP_PARAM_LEGACY_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_legacy);

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_set_bt_name();
    }
#endif

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);

    legacy_set_gap_param(GAP_PARAM_BT_MODE, sizeof(uint8_t), &bt_mode);
    legacy_set_gap_param(GAP_PARAM_COD, sizeof(uint32_t), &class_of_device);
    legacy_set_gap_param(GAP_PARAM_LINK_POLICY, sizeof(uint16_t), &link_policy);
    legacy_set_gap_param(GAP_PARAM_SUPV_TOUT, sizeof(uint16_t), &supervision_timeout);
    legacy_set_gap_param(GAP_PARAM_AUTO_ACCEPT_ACL, sizeof(bool), &auto_accept_acl);


    legacy_set_gap_param(GAP_PARAM_RADIO_MODE, sizeof(uint8_t), &radio_mode);
    legacy_set_gap_param(GAP_PARAM_LIMIT_DISCOV, sizeof(bool), &limited_discoverable);

    legacy_set_gap_param(GAP_PARAM_PAGE_SCAN_TYPE, sizeof(uint8_t), &pagescan_type);
    legacy_set_gap_param(GAP_PARAM_PAGE_SCAN_INTERVAL, sizeof(uint16_t), &pagescan_interval);
    legacy_set_gap_param(GAP_PARAM_PAGE_SCAN_WINDOW, sizeof(uint16_t), &pagescan_window);
    legacy_set_gap_param(GAP_PARAM_PAGE_TIMEOUT, sizeof(uint16_t), &page_timeout);

    legacy_set_gap_param(GAP_PARAM_INQUIRY_SCAN_TYPE, sizeof(uint8_t), &inquiryscan_type);
    legacy_set_gap_param(GAP_PARAM_INQUIRY_SCAN_INTERVAL, sizeof(uint16_t), &inquiryscan_interval);
    legacy_set_gap_param(GAP_PARAM_INQUIRY_SCAN_WINDOW, sizeof(uint16_t), &inquiryscan_window);
    legacy_set_gap_param(GAP_PARAM_INQUIRY_MODE, sizeof(uint8_t), &inquiry_mode);
    bt_pairing_tx_power_set(-2);

    app_ble_gap_param_init();
}

static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(audio_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    remote_local_addr_set(app_cfg_nv.bud_local_addr);
    remote_peer_addr_set(app_cfg_nv.bud_peer_addr);

    /* Bluetooth Manager */
    bt_mgr_init();

    /* Audio Manager */
    audio_mgr_init(PLAYBACK_POOL_SIZE, VOICE_POOL_SIZE, RECORD_POOL_SIZE, NOTIFICATION_POOL_SIZE);
#if F_APP_HARMAN_FEATURE_SUPPORT
    //DBG_DIRECT("MAX_PLC_CNT: %d", app_cfg_const.maximum_packet_lost_compensation_count);
    app_cfg_const.maximum_packet_lost_compensation_count = 0;
#endif
    audio_mgr_set_max_plc_count(app_cfg_const.maximum_packet_lost_compensation_count);

#if F_APP_DUAL_AUDIO_EFFECT
    audio_remote_join_set(false, false);
#endif
}

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
static void app_ble_audio_init(void)
{
    app_le_profile_init();
    app_le_audio_role_init();
}
#endif

static void app_task(void *pvParameters)
{
    uint8_t event;
    driver_init();
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
    if (app_ext_charger_check_support())
    {
        app_adp_detect();
    }
#endif
    if (app_cfg_const.charger_support)
    {
        app_adp_detect();
    }

    APP_PRINT_WARN1("unused memory: %d", mem_peek());
    gap_start_bt_stack(audio_evt_queue_handle, audio_io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    /* means reset in box happened */
    if (app_device_is_in_the_box() &&
        (((app_cfg_const.box_detect_method == ADAPTOR_DETECT) && (app_cfg_nv.adaptor_changed == 0))
#if F_APP_GPIO_ONOFF_SUPPORT
         || ((app_cfg_const.box_detect_method == GPIO_DETECT) && (app_cfg_nv.pre_3pin_status_unplug == 0))
#endif
        ))
    {
        /* disable charger according by the box bat volume */
        if (app_cfg_const.smart_charger_control)
        {
            if (app_cfg_nv.factory_reset_done && app_cfg_nv.disable_charger_by_box_battery)
            {
                app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_BOX_BATTERY);
            }
        }
    }

    app_adp_set_disable_charger_by_box_battery(false);

    if (app_cfg_const.smart_charger_control)
    {
        /* this flag is set before get box bat lv before last power off;
           we need to apply the low box bat lv to disable charger after sw reset */
        if (app_cfg_nv.report_box_bat_lv_again_after_sw_reset)
        {
            app_cfg_nv.report_box_bat_lv_again_after_sw_reset = false;
            app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
            app_cfg_store(&app_cfg_nv.case_battery, 4);
            app_adp_smart_box_update_battery_lv(app_cfg_nv.case_battery, true);
        }
    }

    while (true)
    {
        if (os_msg_recv(audio_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(audio_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        app_io_handle_msg(io_msg);
                    }
                }
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_STACK)
            {
                gap_handle_msg(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_FRAMEWORK)
            {
                sys_mgr_event_handle(event);
            }

            if (app_cfg_const.led_support)
            {
                led_check_charging_mode(0); //LED mode has higher priority
                led_check_mode();
            }

            app_dlps_enable(APP_DLPS_ENTER_CHECK_MSG);
        }
    }
}
#ifdef FPGA_DEV_APP_CUSTOM_CFG
#define DATA_UART_TX_PIN    P3_1
#define DATA_UART_RX_PIN    P3_0

void app_temp_proces(uint8_t role, uint8_t option)
{
    uint8_t single_addr[6] = {0x31, 0xee, 0xff, 0xad, 0xdc, 0xb1};
    uint8_t device_name_legacy[40] = "One_bb2_br";

    if (role == 1)
    {
        //uint8_t single_addr[6] = {0x31, 0xee, 0xf3, 0xad, 0xdc, 0x31};
        single_addr[2] = 0xf3;
        single_addr[5] = 0x31;
        //uint8_t device_name_legacy[40] = "pri_bb2_br";
        device_name_legacy[0] = 'p';
        device_name_legacy[1] = 'r';
        device_name_legacy[2] = 'i';
        app_cfg_nv.bud_role = 1;
        app_cfg_const.bud_role = 1;
    }
    else if (role == 2)
    {
        //uint8_t single_addr[6] = {0x41, 0xee, 0xf4, 0xad, 0xdc, 0x41};
        single_addr[0] = 0x41;
        single_addr[2] = 0xf4;
        single_addr[5] = 0x41;
        //uint8_t device_name_legacy[40] = "sec_bb2_br";
        device_name_legacy[0] = 's';
        device_name_legacy[1] = 'e';
        device_name_legacy[2] = 'c';
        app_cfg_nv.bud_role = 2;
        app_cfg_const.bud_role = 2;
    }
    else
    {
        app_cfg_nv.bud_role = 0;
        app_cfg_const.bud_role = 0;
    }
    memcpy(&app_cfg_nv.device_name_legacy[0], device_name_legacy, 40);
    memcpy(app_cfg_nv.bud_local_addr, single_addr, 6);
    gap_set_bd_addr(single_addr);


    app_cfg_nv.app_is_power_on = 1;
    app_cfg_nv.factory_reset_done = 1;
    app_cfg_const.supported_profile_mask = (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK |
                                            RDTP_PROFILE_MASK | SPP_PROFILE_MASK);
    app_cfg_const.class_of_device[0] = 0x18;
    app_cfg_const.class_of_device[1] = 0x4;
    app_cfg_const.class_of_device[2] = 0x24;
    app_cfg_const.a2dp_link_number = 3;
    app_cfg_const.hfp_link_number = 3;
    app_cfg_const.avrcp_link_number = 3;
    app_cfg_const.timer_first_engage = 15;
    app_cfg_const.hfp_brsf_capability = 0x2BF;
    app_cfg_const.a2dp_codec_type_sbc = 1;
    app_cfg_const.a2dp_codec_type_aac = 1;
    app_cfg_const.sbc_sampling_frequency = 0xF0;
    app_cfg_const.sbc_channel_mode = 0xF;
    app_cfg_const.sbc_block_length = 0xF0;
    app_cfg_const.sbc_subbands = 0xC;
    app_cfg_const.sbc_allocation_method = 0x3;
    app_cfg_const.sbc_min_bitpool = 2;
    app_cfg_const.sbc_max_bitpool = 53;
    app_cfg_const.aac_sampling_frequency = 0x180;//        384
    app_cfg_const.aac_object_type = 0x1C;//              128
    app_cfg_const.aac_channel_number = 12;
    app_cfg_const.aac_vbr_supported  = 1;
    //app_cfg_const.aac_bit_rate;           0
    app_cfg_const.enable_dlps = 0;//Must wait for verified
    app_cfg_const.sensor_support = 0;
    app_cfg_const.enable_rtk_charging_box = 0;
    app_cfg_const.rws_pairing_required_rssi = 60;
    if (option)
    {
        app_cfg_const.data_uart_tx_pinmux = DATA_UART_TX_PIN;
        app_cfg_const.data_uart_rx_pinmux = DATA_UART_RX_PIN;
        //app_cfg_const.i2c_0_dat_pinmux = P1_6;
        //app_cfg_const.i2c_0_clk_pinmux = P1_7;
        app_cfg_const.enable_data_uart = 1;
        app_cfg_const.data_uart_baud_rate = BAUD_RATE_115200;
        app_cfg_const.timer_pairing_timeout = 0;
    }

}
#endif

#if F_APP_LE_AUDIO_TEST_TEMP
#define DATA_UART_TX_PIN    P3_1
#define DATA_UART_RX_PIN    P3_0
void app_iso_proces(void)
{
    app_cfg_const.data_uart_tx_pinmux = DATA_UART_TX_PIN;
    app_cfg_const.data_uart_rx_pinmux = DATA_UART_RX_PIN;

    app_cfg_const.enable_data_uart = 1;
    app_cfg_const.data_uart_baud_rate = BAUD_RATE_115200;
    app_cfg_const.timer_pairing_timeout = 120;
    app_cfg_const.cis_autolink = T_LEA_CIS_MMI;
    app_cfg_const.bis_policy = BIS_POLICY_SPECIFIC;
    app_cfg_const.bis_mode = T_LEA_BROADCAST_SINK;
}
#endif

static void app_wakeup_reason_check(void)
{
    uint8_t pm_wakeup_reason = 0;

    pm_wakeup_reason = power_down_check_wake_up_reason();

    if (pm_wakeup_reason & POWER_DOWN_WAKEUP_RTC)
    {
        app_db.wake_up_reason |= WAKE_UP_RTC;
    }

    if (pm_wakeup_reason & POWER_DOWN_WAKEUP_MFB)
    {
        app_db.wake_up_reason |= WAKE_UP_MFB;
    }

    if (pm_wakeup_reason & POWER_DOWN_WAKEUP_ADP)
    {
        app_db.wake_up_reason |= WAKE_UP_ADP;
    }

    if (pm_wakeup_reason & POWER_DOWN_WAKEUP_CTC)
    {
        app_db.wake_up_reason |= WAKE_UP_CTC;
    }

    if (pm_wakeup_reason & POWER_DOWN_WAKEUP_PAD)
    {
        uint8_t i = 0;

        app_db.wake_up_reason |= WAKE_UP_PAD;

        if (System_WakeUpInterruptValue(app_cfg_const.gpio_box_detect_pinmux))
        {
            app_db.wake_up_reason |= WAKE_UP_3PIN;
        }

        if (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[0]))
        {
            app_db.wake_up_reason |= WAKE_UP_KEY0;
        }

        for (i = 1; i < MAX_KEY_NUM; i++)
        {
            if (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[i]))
            {
                if (app_key_is_combinekey_power_on_off(i))
                {
                    app_db.wake_up_reason |= WAKE_UP_COMBINE_KEY_POWER_ONOFF;
                }
            }
        }
    }

    APP_PRINT_INFO2("app_wakeup_reason_check: pm_wakeup_reason: 0x%x, app_wake_up_reason: 0x%x",
                    pm_wakeup_reason, app_db.wake_up_reason);
}

int main(void)
{
    uint32_t time_entry_APP;
    void *app_task_handle;

    time_entry_APP = sys_timestamp_get();
    APP_PRINT_INFO1("TIME FROM PATCH TO APP: %d ms", time_entry_APP);
    APP_PRINT_INFO2("APP COMPILE TIME: [%s - %s]", TRACE_STRING(__DATE__), TRACE_STRING(__TIME__));
    uint16_t stack_size = 1024 * 3 - 256;

    memset(&app_db, 0, sizeof(T_APP_DB));
    if (sys_hall_get_reset_status())
    {
        APP_PRINT_INFO0("APP RESTART FROM WDT_SOFTWARE_RESET");
    }
    else
    {
        APP_PRINT_INFO0("APP START FROM HW_RESET");
    }

// for harman chipid signature check
#if HARMAN_SECURITY_CHECK
    // Check Secure Boot
    if (sys_hall_get_secure_state() == true)
    {
        APP_PRINT_INFO0("APP START FROM SECURE BOOT");
    }
    else
    {
        DBG_DIRECT("APP START FROM Non SECURE");
        chip_reset(RESET_ALL);
    }
    register_chip_id_pub_key_read_func(harmain_public_key_read);
#endif

    if (is_single_tone_test_mode())
    {
        os_msg_queue_create(&audio_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
        os_msg_queue_create(&audio_evt_queue_handle, "evQ", MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));

        gap_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_GAP_TIMER);
        app_cfg_init();

        if (app_cfg_const.mfb_replace_key0)
        {
            app_cfg_const.key_enable_mask &= 0xfe;
            app_cfg_const.key_pinmux[0] = 0xff;
        }


        if (app_cfg_const.disable_key_when_dut_mode == 1)
        {
            app_cfg_const.key_gpio_support = 0;

            if ((app_cfg_const.key_enable_mask & KEY0_MASK) == 0) //Use MFB key
            {
                app_cfg_const.key_enable_mask |= KEY0_MASK; //Disable MFB key init
            }
        }

        board_init();

        bt_gap_init();

        app_dlps_init();
        framework_init();
        driver_init();
        app_auto_power_off_init();
        app_audio_init();
        app_in_out_box_init();
        app_adp_init();
        reset_single_tone_test_mode();
        mp_hci_test_init(MP_HCI_TEST_DUT_MODE);
    }
    else
    {
        os_msg_queue_create(&audio_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
        os_msg_queue_create(&audio_evt_queue_handle, "evtQ", MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));

        gap_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_GAP_TIMER);

        app_cfg_init();
        app_wakeup_reason_check();

#if F_APP_HARMAN_FEATURE_SUPPORT
        // app_cfg_const.disallow_adjust_volume_when_idle = true;
        DBG_DIRECT("version: %d.%d.%d.%d, active_bank: %d, sass_preemptive: 0x%x, finder_support: %d",
                   VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILDNUM,
                   app_ota_get_active_bank(),
                   app_cfg_nv.sass_preemptive,
                   extend_app_cfg_const.gfps_finder_support);
        app_harman_license_get();
#endif

#ifdef FPGA_DEV_APP_CUSTOM_CFG
        app_temp_proces(0, 0);//0:single, 1:pri, 2:sec; 0:no uart
#endif
        if (app_cfg_const.mfb_replace_key0)
        {
            app_cfg_const.key_enable_mask &= 0xfe;
            app_cfg_const.key_pinmux[0] = 0xff;
        }

        board_init();

        bt_gap_init();
        app_dlps_init();
        framework_init();

        app_harman_license_init();

#if F_APP_ERWS_SUPPORT
        app_relay_init();
#endif

        app_auto_power_off_init();

#if F_APP_TEST_SUPPORT
        app_test_init();
#endif

        app_gap_init();
        app_ble_gap_init();
        app_bt_policy_init();
        app_ble_client_init();

#if F_APP_TUYA_SUPPORT
        le_tuya_adv_init();
#endif

#if F_APP_IO_OUTPUT_SUPPORT
        app_io_output_init();
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
        app_cfg_const.enter_pairing_while_only_one_device_connected = 0;
        app_cfg_const.play_max_min_tone_when_adjust_vol_from_phone = 1;
#endif

#if F_APP_ANC_SUPPORT
        if (anc_tool_check_resp_meas_mode() == ANC_RESP_MEAS_MODE_ENTER)
        {
            app_cfg_const.bud_role = REMOTE_SESSION_ROLE_SINGLE;
            app_cfg_nv.app_is_power_on = 1;
            app_cfg_const.enable_power_on_linkback = 1;
            app_cfg_const.supported_profile_mask = SPP_PROFILE_MASK;
            app_cfg_const.link_scenario = LINKBACK_SCENARIO_SPP_BASE;
            app_cfg_const.listening_mode_power_on_status = POWER_ON_LISTENING_MODE_ALL_OFF;
            app_cfg_const.enable_pairing_timeout_to_power_off = 0;
            app_cfg_const.timer_pairing_timeout = 600;
#if FPGA_CPU_CLK_ALLOW_COMPLEMENT

            uint32_t cpu_freq;
            int32_t ret = pm_cpu_freq_set(80, &cpu_freq);
            if (ret != 0)
            {
                APP_PRINT_ERROR2("cpu freq config CLK_80MHZ fail ret %x real freq %dMHz", ret, cpu_freq);
            }
            else
            {
                APP_PRINT_INFO1("cpu freq %dMHz ", cpu_freq);
            }
#endif
            app_mmi_init();
            APP_PRINT_TRACE0("Enter anc_resp_meas_mode");
        }
        else
#endif
        {
#if F_APP_ANC_SUPPORT
            if (anc_tool_check_resp_meas_mode() == ANC_RESP_MEAS_MODE_EXIT)
            {
                app_cfg_const.bud_role = REMOTE_SESSION_ROLE_SINGLE;
                app_cfg_const.enable_power_on_linkback = 1;
                app_cfg_const.link_scenario = LINKBACK_SCENARIO_SPP_BASE;
                app_cfg_nv.app_is_power_on = 1;
                app_cfg_const.listening_mode_power_on_status = POWER_ON_LISTENING_MODE_ALL_OFF;
                app_cfg_const.enable_pairing_timeout_to_power_off = 0;
                app_cfg_const.timer_pairing_timeout = 600;

                APP_PRINT_TRACE0("Exit anc_resp_meas_mode");
            }
#endif

#if F_APP_ERWS_SUPPORT
            app_rdtp_init();
#endif

            app_hfp_init();
            app_avrcp_init();
            app_a2dp_init();
#if F_APP_BT_PROFILE_PBAP_SUPPORT
            app_pbap_init();
#endif

#if F_APP_HID_SUPPORT
            app_hid_init();
#endif

#if F_APP_IAP_SUPPORT
            app_iap_init();
#endif

#if F_APP_HFP_AG_SUPPORT
            app_hfp_ag_init();
#endif
        }

        app_spp_init();
        app_multilink_init();

#if F_APP_ERWS_SUPPORT
        app_bt_sniffing_init();
        app_roleswap_init();
#endif
        app_roleswap_ctrl_init();
        app_ble_service_init();

        app_bud_loc_init();
        app_adp_init();
#if (F_APP_AVP_INIT_SUPPORT == 1)
        app_avp_init();
#endif


#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
        app_rtk_fast_pair_adv_init();
#endif

#if (F_APP_DUAL_AUDIO_EFFECT == 1)
        app_cfg_const.rws_disable_codec_mute_when_linkback = 1;
        dual_audio_effect_init();
#endif
        app_sdp_init();
        app_audio_init();
#if F_APP_ADC_SUPPORT
        app_adc_init();
#endif
#if HARMAN_NTC_DETECT_PROTECT
        app_harman_adc_init();
#endif
        app_device_init();
        app_ble_device_init();
        app_transfer_init();
//        app_ota_init();
        app_in_out_box_init();

#if F_APP_LISTENING_MODE_SUPPORT
        app_listening_mode_init();
#endif
#if F_APP_ANC_SUPPORT
        app_anc_init();
#endif
#if F_APP_APT_SUPPORT
        app_apt_init();
#endif
#if F_APP_LINEIN_SUPPORT
        app_line_in_init();
#endif
#if (F_APP_AIRPLANE_SUPPORT == 1)
        app_airplane_init();
#endif
        app_cmd_init();
        app_mmi_init();
        app_bond_init();

#if F_APP_BLE_BOND_SYNC_SUPPORT
        app_ble_bond_init();
#endif

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
        app_ble_audio_init();
#endif
//        if (app_cfg_nv.ota_tooling_start)
//        {
//            app_ota_adv_init();
//        }

#if AMA_FEATURE_SUPPORT | BISTO_FEATURE_SUPPORT | F_APP_IAP_RTK_SUPPORT
        spp_stream_init(0xff);
        ble_stream_init(0xff);
        iap_stream_init(0xff);
#endif

        app_third_party_srv_init();

#if F_APP_RWS_MULTI_SPK_SUPPORT
        msbc_sync_init();
#endif


#if F_APP_CFU_SUPPORT
        app_cfu_init();//cfu_init must before usb start
#endif

#if F_APP_USB_AUDIO_SUPPORT
        usb_hid_init();
        app_usb_audio_init();
//        extern void app_usb_audio_hid_init(void);
//        app_usb_audio_hid_init();
#endif

#if F_APP_IAP_RTK_SUPPORT
        app_iap_rtk_init();
#endif

#if F_APP_BRIGHTNESS_SUPPORT
        app_apt_brightness_init();
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
        app_slide_switch_init();
#endif

#if (F_APP_QDECODE_SUPPORT == 1)
        if (app_cfg_const.wheel_support)
        {
            app_qdec_init();
        }
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
        app_harman_vendor_cmd_init();
        app_harman_ble_ota_init();
#endif

#if GFPS_SASS_SUPPORT
        if (app_sass_policy_support_easy_connection_switch())
        {
            app_sass_policy_init();
        }
#endif
        // end of BBPro2 specialized feature
        //increase app task priority to 2 so that user could implement some background daemon task
        os_task_create(&app_task_handle, "app_task", app_task, NULL, stack_size, 2);

#if F_APP_DEBUG_HIT_RATE_PRINT
        cache_hit_count_init(10000);
#endif
    }
    monitor_memory_and_timer(app_cfg_const.timer_monitor_heap_and_timer_timeout);

    APP_PRINT_INFO1("TIME FROM APP TO OS SCHED: %d ms", sys_timestamp_get() - time_entry_APP);

    os_sched_start();

    return 0;
}
