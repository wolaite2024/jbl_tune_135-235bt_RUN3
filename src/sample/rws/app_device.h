/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DEVICE_H_
#define _APP_DEVICE_H_
#include <stdbool.h>
#include <stdint.h>
#include <rtl876x_wdg.h>

#if F_APP_LOSS_BATTERY_PROTECT
#include "app_msg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_DEVICE App Device
  * @brief App Device
  * @{
  */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_DEVICE_Exported_Macros App Device Macros
    * @{
    */

/** End of APP_DEVICE_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_DEVICE_Exported_Types App Device Types
    * @{
    */
#if F_APP_TEAMS_BT_POLICY
extern uint8_t temp_master_device_name[40];
extern uint8_t temp_master_device_name_len;
#endif

typedef enum
{
    APP_DEVICE_TIMER_POWER_OFF_RESET,
    APP_DEVICE_TIMER_TOTAL
} T_APP_DEVICE_TIMER;

typedef enum
{
    BUD_COUPLE_STATE_IDLE      = 0x00,
    BUD_COUPLE_STATE_START     = 0x01,
    BUD_COUPLE_STATE_CONNECTED = 0x02
} T_BUD_COUPLE_STATE;

typedef enum
{
    APP_DEVICE_STATE_OFF       = 0x00,
    APP_DEVICE_STATE_ON        = 0x01,
    APP_DEVICE_STATE_OFF_ING   = 0x02,
} T_APP_DEVICE_STATE;

typedef enum
{
    AUTO_POWER_OFF_MASK_POWER_ON = 0x01,
    AUTO_POWER_OFF_MASK_SOURCE_LINK = 0x02,
    AUTO_POWER_OFF_MASK_IN_BOX = 0x04,
    AUTO_POWER_OFF_MASK_BUD_COUPLING = 0x08,
    AUTO_POWER_OFF_MASK_KEY = 0x10,
    AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF = 0x20,
    AUTO_POWER_OFF_MASK_PAIRING_MODE = 0x40,
    AUTO_POWER_OFF_MASK_ANC_APT_MODE = 0x80,
    AUTO_POWER_OFF_MASK_BLE_LINK_EXIST  = 0x00000100,
    AUTO_POWER_OFF_MASK_PLAYBACK_MODE   = 0x00000200,
    AUTO_POWER_OFF_MASK_LINE_IN = 0x00000400,
    AUTO_POWER_OFF_MASK_ANC_APT_MODE_WITH_PHONE_CONNECTED   = 0x00000800,
    AUTO_POWER_OFF_MASK_AIRPLANE_MODE = 0x00001000,
    AUTO_POWER_OFF_MASK_LINKBACK = 0x00002000,
    AUTO_POWER_OFF_MASK_DUT_MODE = 0x00004000,
    AUTO_POWER_OFF_MASK_USB_AUDIO_MODE   = 0x00008000,
    AUTO_POWER_OFF_MASK_XIAOAI_OTA         = 0x00010000,
    AUTO_POWER_OFF_MASK_OTA_TOOLING      = 0x00020000,
    AUTO_POWER_OFF_MASK_BLE_AUDIO      = 0x00040000,
    AUTO_POWER_OFF_MASK_TUYA_OTA         = 0x00080000,
    AUTO_POWER_OFF_MASK_CONSOLE_CMD = 0x00100000,
    AUTO_POWER_OFF_MASK_LE_CMD      = 0x00200000,
    AUTO_POWER_OFF_MASK_HARMAN_OTA      = 0x00400000,
} T_AUTO_POWER_OFF_MASK;


typedef enum
{
    APP_TONE_VP_STOP      = 0x00,
    APP_TONE_VP_STARTED     = 0x01,
} T_APP_TONE_VP_STATE;

typedef enum
{
    APP_REMOTE_MSG_SYNC_DB                      = 0x00,
    APP_REMOTE_MSG_SYNC_IO_PIN_PULL_HIGH        = 0x01,
    APP_REMOTE_MSG_LINK_RECORD_ADD              = 0x02,
    APP_REMOTE_MSG_LINK_RECORD_DEL              = 0x03,
    APP_REMOTE_MSG_LINK_RECORD_XMIT             = 0x04,
    APP_REMOTE_MSG_LINK_RECORD_PRIORITY_SET     = 0x05,
    APP_REMOTE_MSG_DEVICE_NAME_SYNC             = 0x06,
    APP_REMOTE_MSG_SET_LPS_SYNC                 = 0x07,
    APP_REMOTE_MSG_LE_NAME_SYNC                 = 0x08,
    APP_REMOTE_MSG_POWERING_OFF                 = 0x09,
    APP_REMOTE_MSG_REMOTE_SPK2_PLAY_SYNC        = 0x0a,
    APP_REMOTE_MSG_SPK1_REPLY_SRC_IS_IOS        = 0x0b,

    APP_REMOTE_MSG_DEVICE_TOTAL
} T_DEVICE_REMOTE_MSG;

typedef struct
{
    T_APP_TONE_VP_STATE state;
    uint8_t     index;
} T_APP_TONE_VP_STARTED;

typedef struct test_equipment_info
{
    uint8_t oui[3];
    const char *name;
} T_APP_TEST_EQUIPMENT_INFO;

typedef struct
{
    bool remote_is_8753bau;
} T_ROLESWAP_APP_DB;

#if (F_APP_AVP_INIT_SUPPORT == 1)
extern void (*app_power_on_hook)(void);
extern void (*app_power_off_hook)(void);
extern void (*app_src_connect_hook)(void);
extern void (*app_src_disconnect_hook)(void);
extern void (*app_profile_connect_hook)(void);
extern void (*app_pairing_finish_hook)(void);
extern void (*app_pairing_timeout_hook)(void);
extern void (*app_bud_connect_hook)(void);
extern void (*app_bud_disconnect_hook)(void);
extern void (*app_roleswap_src_connect_hook)(void);
#endif


/* rws speaker channal definitions */
#define RWS_SPK_CHANNEL_LEFT    0x01
#define RWS_SPK_CHANNEL_RIGHT   0x02

/** End of APP_DEVICE_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_DEVICE_Exported_Functions App Device Functions
    * @{
    */
/* @brief  app device module init.
*
* @param  void
* @return none
*/
void app_device_init(void);

/* @brief  app device reboot.
*
* @param  timeout_ms timeout value to do system reboot
* @param  wdg_mode Watch Dog Mode
* @return none
*/
void app_device_reboot(uint32_t timeout_ms, T_WDG_MODE wdg_mode);

void app_device_factory_reset(void);

/* @brief  check device inside/outside box state
*
* @param  void
* @return true/false
*/
bool app_device_is_in_the_box(void);

/* @brief  check device inside/outside ear state
*
* @param  void
* @return true/false
*/
bool app_device_is_in_ear(uint8_t loc);

/* @brief  change device state
*
* @param  state @ref T_APP_DEVICE_STATE
* @return none
*/
void app_device_state_change(T_APP_DEVICE_STATE state);

/* @brief  disconnect_all_link and enter dut mode
*
* @param  none
* @return none
*/
void app_device_enter_dut_mode(void);

/* @brief  get bud physical channel
*
* @param  void
* @return physical channel
*/
uint8_t app_device_get_bud_channel(void);

/* @brief  start up bt policy
*
* @param  at_once_trigger
* @return none
*/
void app_device_bt_policy_startup(bool at_once_trigger);

/** @} */ /* End of group APP_DEVICE_Exported_Functions */


/** End of APP_DEVICE
* @}
*/

/**
    * @brief  unlock vbat disallow power on
    * @param  void
    * @return void
    */
void app_device_unlock_vbat_disallow_power_on(void);

#if F_APP_LOSS_BATTERY_PROTECT
void app_device_loss_battery_gpio_driver_init(void);
void app_device_loss_battery_gpio_detect_handle_msg(T_IO_MSG *io_driver_msg_recv);
#endif

void app_harman_req_remote_device_name_timer_start(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_DEVICE_H_ */
