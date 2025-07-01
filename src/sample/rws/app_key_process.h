/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_KEY_PROCESS_H_
#define _APP_KEY_PROCESS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"
#include "section.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (F_APP_AVP_INIT_SUPPORT == 1)
extern bool (*app_key_press_hook)(uint8_t key_mask, uint8_t mode, uint8_t type);
extern bool (*app_key_press_release_hook)(uint8_t key_mask, uint8_t pressed);
#endif
/** @defgroup APP_KEY_PROCESS App Key Process
  * @brief App Key Process
  * @{
  */

/* Hybrid key type */
typedef enum t_hybrid_key
{
    HYBRID_KEY_SHORT_PRESS,
    HYBRID_KEY_LONG_PRESS,
    HYBRID_KEY_ULTRA_LONG_PRESS,
    HYBRID_KEY_2_CLICK,
    HYBRID_KEY_3_CLICK,
    HYBRID_KEY_4_CLICK,
    HYBRID_KEY_5_CLICK,
    HYBRID_KEY_HALL_SWITCH_HIGH,
    HYBRID_KEY_HALL_SWITCH_LOW,
    HYBRID_KEY_COMBINEKEY_POWER_ONOFF = 9,

    HYBRID_KEY_6_CLICK,
    HYBRID_KEY_7_CLICK,

    HYBRID_KEY_CLICK_AND_PRESS,
    HYBRID_KEY_TOTAL
} T_HYBRID_KEY;

typedef enum
{
    KEY_CLICK_SINGLE            = 0x01,
    KEY_CLICK_MULTI_2           = 0x02,
    KEY_CLICK_MULTI_3           = 0x03,
    KEY_CLICK_LONG_PRESS        = 0x04,
    KEY_CLICK_ULTRA_LONG_PRESS  = 0x05,
    KEY_CLICK_MULTI_4           = 0x06,
    KEY_CLICK_MULTI_5           = 0x07,
    KEY_CLICK_MULTI_6           = 0x08,
    KEY_CLICK_MULTI_7           = 0x09,
    KEY_CLICK_MAXIMUM,
} KEY_CLICK_TYPE;

typedef enum
{
    KEY_CALL_STATUS_CALL_IDLE        = 0x00,
    KEY_CALL_STATUS_NOT_CALL_IDLE    = 0x01,

    KEY_CALL_STATUS_MAXIMUM,
} KEY_CALL_STATUS_TYPE;

typedef enum
{
    KEY_BUD_SIDE_STEREO        = 0x00,
    KEY_BUD_SIDE_LEFT_BUD      = 0x01,
    KEY_BUD_SIDE_RIGHT_BUD     = 0x02,
    KEY_BUD_SIDE_LEFT_AND_RIGHT_BUDS = 0x03,

    KEY_BUD_SIDE__MAXIMUM,
} KEY_CALL_BUD_SIDE;

typedef enum
{
    APP_KEY_MAP              = 0x00,
    APP_KEY_SEC_MMI          = 0x01,
    APP_KEY_RESET            = 0x02,
    APP_KEY_SEC_DEFAULT_MMI  = 0x03,
    APP_KEY_RWS_KEY_MAP              = 0x04,
    APP_KEY_RWS_KEY_SEC_MMI          = 0x05,
    APP_KEY_RWS_KEY_RESET            = 0x06,
    APP_KEY_RWS_KEY_SEC_DEFAULT_MMI  = 0x07,
    APP_KEY_MSG_MAX
} T_APP_KEY_REMOTE_MSG;

typedef struct
{
    uint8_t rsv : 2;
    uint8_t bud_channel : 2;
    uint8_t click_type : 4;
    uint8_t call_status : 4;
    uint8_t key_cmd : 8;
} T_KEY_REMAP_INFO;

typedef struct
{
    uint8_t bud_channel;
    uint8_t call_state;
    uint8_t click_type;
    uint8_t key_mmi;
} T_APP_KEY_MAP;

#define BUD_PHYSICAL_CHANNEL_MAXIMUM 3
#define KEY_REMAPPING_ARRAY_SIZE    ((BUD_PHYSICAL_CHANNEL_MAXIMUM-1) * (KEY_CLICK_MAXIMUM-1) * KEY_CALL_STATUS_MAXIMUM)

typedef struct
{
    uint8_t         app_test_rsv1;
    uint8_t         app_test_rsv2;
    uint8_t         app_test_rsv3;
    uint8_t         app_test_rsv4;
    T_KEY_REMAP_INFO app_key_remap_info[KEY_REMAPPING_ARRAY_SIZE];
} T_CUSTOMER_KEY_INFO;

typedef struct
{
    uint8_t         app_test_rsv1;
    uint8_t         app_test_rsv2;
    uint8_t         app_test_rsv3;
    uint8_t         app_test_rsv4 : 6;
    uint8_t         is_setting_rws_key: 1;
    uint8_t         already_sync: 1;
    T_KEY_REMAP_INFO app_key_remap_info[KEY_REMAPPING_ARRAY_SIZE];
} T_CUSTOMER_RWS_KEY_INFO;

/**
    * @brief  key process initial.
    * @param  void
    * @return void
    */
void app_key_init(void);

/**
    * @brief  judge if now going to pairing mode by long press key.
    * @param  void
    * @return is pairing mode or other mode
    */
bool app_key_is_enter_pairing(void);

/**
    * @brief  reset enter pairing variable.
    * @param  void
    * @return void
    */
void app_key_reset_enter_pairing(void);

/**
    * @brief  set volume by press key.
    * @param  volume be set by press key;
    * @return void
    */
void app_key_set_volume_status(bool volume_status);

/**
    * @brief  judge if set volume by press key.
    * @param  void
    * @return set volume by press key or not
    */
bool app_key_is_set_volume(void);

/**
    * @brief  play unmute tone only by press key.
    * @param  play unmute tone enable or not;
    * @return void
    */
void app_key_enable_play_mic_unmute_tone(bool enable);

/**
    * @brief  judge if play unmute tone by press key.
    * @param  void
    * @return enable or not
    */
bool app_key_is_enable_play_mic_unmute_tone(void);

/**
    * @brief  judge if set vol change mmi.
    * @param  void
    * @return void
    */
void app_key_check_vol_mmi(void);

/**
    * @brief  disallow sync play vol changed tone.
    * @param  disallow sync
    * @return void
    */
void app_key_set_sync_play_vol_changed_tone_disallow(bool disallow_sync);

/**
    * @brief  disallow sync play vol changed tone.
    * @param  void
    * @return sync play or not
    */
bool app_key_is_sync_play_vol_changed_tone_disallow(void);

/**
    * @brief  key single click process.
    * @param  key:This parameter is from KEY0_MASK to KEY7_MASK
    * @return void
    */
void app_key_single_click(uint8_t key);

/**
    * @brief  key single click process.
    * @param  key:This parameter is from KEY0_MASK to KEY7_MASK
    * @param  clicks:click counts
    * @return void
    */
void app_key_hybrid_multi_clicks(uint8_t key, uint8_t clicks);

/**
    * @brief  key module handle message.
    *         when app_io_handle_msg recv msg IO_MSG_GPIO_KEY, it will be called.
    * @param  void
    * @return void
    */
void app_key_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  check key index is combine key power on off.
    * @param  key number
    * @return key index is combine key or not
    */
ISR_TEXT_SECTION bool app_key_is_combinekey_power_on_off(uint8_t key);

/**
    * @brief  check gpio index is combine key power on off.
    * @param  key number
    * @return gpio index is combine key or not
    */
ISR_TEXT_SECTION bool app_key_is_gpio_combinekey_power_on_off(uint8_t gpio_index);

/**
    * @brief  Read current status of Hall Switch sensor.
    * @param  void
    * @return status
    */
uint8_t app_key_read_hall_switch_status(void);

/**
    * @brief  App parses if there are hall switch function definitions in the hybrid key behaviors.
    *         Hall switch function is defined as one type of hybrid key.
    *         Execute this function to check if there's hall switch function used.
    *         Hall switch function actions are different from general hybrid key actions.
    * @param  void
    * @return void
    */
void app_key_hybird_parse_hall_switch_setting(void);

/**
    * @brief  power onoff combinekey process before enter dlps.
    * @param  void
    * @return void
    */
void app_power_onoff_combinekey_dlps_process(void);

/**
    * @brief  Execute the corresponding action with Hall Switch status.
    *         Once Hall Switch status changed. It required a stable timer for debouncing.
    *         After the debouncing process, related action will be executed.
    *         Note that action only be executed in the correct status.
    * @param  hall_switch_action Hall Switch high or Hall Switch Low.
    * @return void
    */
void app_key_execute_hall_switch_action(uint8_t hall_switch_action);

/*
 * Find out the key index by key mask.
 * For example: The key index 0 correspond to key0, such as KEY0_MASK, KEY1_MASK etc.
 */
uint8_t key_search_index(uint8_t key);

void key_execute_action(uint8_t action);


#if F_APP_KEY_EXTEND_FEATURE
/**
    * @brief  key remap vendor command.
    * @param  length command length
    * @param  p_value command type
    * @param  app_idx received rx command device index
    * @param  cmd_path command path use for distinguish uart,or le,or spp channel.
    * @return cmd execute result @ref T_AU_CMD_SET_STATUS
    */
uint8_t app_key_handle_key_remapping_cmd(uint16_t length, uint8_t *p_value, uint8_t app_idx,
                                         uint8_t cmd_path);
uint32_t app_key_reset_key_customized_parameter(void);

#if F_APP_RWS_KEY_SUPPORT
uint32_t app_key_reset_rws_key_customized_parameter(bool reset_all_setting);

/**
    * @brief  judge if set rws key.
    * @param  void
    * @return set rws key or not
    */
bool app_key_is_rws_key_setting(void);

#endif
#endif

void app_key_stop_timer(void);

/**
    * @brief  set long key power off key precessed or release.
    * @param  value: true pressed; flase released.
    * @return void
    */
void app_key_long_key_power_off_pressed_set(bool value);

/**
    * @brief  get long key power off key precessed or release.
    * @param  void
    * @return true pressed; flase released.
    */
bool app_key_long_key_power_off_pressed_get(void);

#if APP_CAP_TOUCH_SUPPORT
/**
    * @brief  start check cap touch slide event
    * @param  void
    * @return void
    */
void app_key_start_cap_touch_check_timer(void);

/**
    * @brief  process cap touch press/release event
    * @param  press or release
    * @return void
    */
void app_key_cap_touch_key_process(uint8_t key_chek_press);

/**
    * @brief  process cap touch need stop timer
    * @param  void
    * @return void
    */
void app_key_cap_touch_stop_timer(void);

#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_KEY_PROCESS_H_ */
