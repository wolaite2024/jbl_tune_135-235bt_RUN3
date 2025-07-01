/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <stdint.h>

#include "app_link_util.h"
#include "app_device.h"
#include "voice_prompt.h"
#include "engage.h"
#include "remote.h"
#include "app_charger.h"
#include "app_bt_policy_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_MAIN App Main
  * @brief Main entry function for audio sample application.
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Macros App Main Macros
    * @{
    */
#define UART_BUD_RX     (P3_0)

#define RWS_PRIMARY_VALID_OK             0x01
#define RWS_SECONDARY_VALID_OK           0x02

/** End of APP_MAIN_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Types App Main Types
    * @{
    */

/** @brief Bud location */
typedef enum
{
    BUD_LOC_UNKNOWN     = 0x00,
    BUD_LOC_IN_CASE     = 0x01,
    BUD_LOC_IN_AIR      = 0x02,
    BUD_LOC_IN_EAR      = 0x03,
} T_BUD_LOCATION;

/** @brief Bud location */
typedef enum
{
    POWER_OFF_CAUSE_UNKNOWN                 = 0x00,
    POWER_OFF_CAUSE_HYBRID_KEY              = 0x01,
    POWER_OFF_CAUSE_LONG_KEY                = 0x02,
    POWER_OFF_CAUSE_SHORT_KEY               = 0x03,
    POWER_OFF_CAUSE_SLIDE_SWITCH            = 0x04,
    POWER_OFF_CAUSE_CMD_SET                 = 0x05,
    POWER_OFF_CAUSE_ADP_CMD                 = 0x06,
    POWER_OFF_CAUSE_FACTORY_RESET           = 0x07,
    POWER_OFF_CAUSE_LOW_VOLTAGE             = 0x08,
    POWER_OFF_CAUSE_SEC_LINKLOST_TIMEOUT    = 0x09,
    POWER_OFF_CAUSE_EXIT_PAIRING_MODE       = 0x10,
    POWER_OFF_CAUSE_AUTO_POWER_OFF          = 0x11,
    POWER_OFF_CAUSE_ANC_TOOL                = 0x12,
    POWER_OFF_CAUSE_OTA_TOOL                = 0x13,
    POWER_OFF_CAUSE_NORMAL_INBOX            = 0x20,
    POWER_OFF_CAUSE_ADP_CLOSE_CASE_TIMEOUT  = 0x21,
    POWER_OFF_CAUSE_ADP_IN_CASE_TIMEOUT     = 0x22,
    POWER_OFF_CAUSE_RELAY                   = 0x30,
} T_POWER_OFF_CAUSE;

typedef enum
{
    FACTORY_RESET_CLEAR_CFG     = 0x01,
    FACTORY_RESET_CLEAR_PHONE   = 0x02,
    FACTORY_RESET_CLEAR_NORMAL  = 0x03,
    FACTORY_RESET_CLEAR_ALL     = 0x04,
} T_FACTORY_RESET_CLEAR_MODE;


typedef enum
{
    WAKE_UP_RTC                        = 0x0001,
    WAKE_UP_PAD                        = 0x0002,
    WAKE_UP_MFB                        = 0x0004,
    WAKE_UP_ADP                        = 0x0008,
    WAKE_UP_CTC                        = 0x0010,
    WAKE_UP_KEY0                       = 0x0020,
    WAKE_UP_COMBINE_KEY_POWER_ONOFF    = 0x0040,
    WAKE_UP_3PIN                       = 0x0080,
} T_WAKE_UP_REASON;

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_BR_LINK               br_link[MAX_BR_LINK_NUM];
    T_APP_LE_LINK               le_link[MAX_BLE_LINK_NUM];

    uint16_t                    external_mcu_mtu;
    uint8_t                     local_loc;                 /**< local bud location */
    uint8_t                     remote_loc;                /**< remote bud location */
    uint8_t                     local_loc_pre;             /**< local pre bud location */
    uint8_t                     remote_loc_pre;            /**< remote pre bud location */
    bool                        remote_loc_received;       /**< remote loc is received */
    bool                        local_in_ear;              /**< local ear location */
    bool                        local_in_case;             /**< local case location */
    bool                        local_case_closed;         /**< local case closed */
    bool                        remote_case_closed;        /**< remote case closed */
    bool                        out_case_at_least;         /**< bud is out case at least */

    uint8_t                     local_batt_level;          /**< local battery level */
    uint8_t                     remote_batt_level;         /**< remote battery level */
    uint8_t                     case_battery;              /**< rtk case battery level*/
    uint8_t                     disallow_charging_led;

    uint8_t                     factory_addr[6];            /**< local factory address */
    uint8_t                     avrcp_play_status;
    uint8_t                     a2dp_play_status;
    uint8_t                     hall_switch_status;         /**< hall switch status */

    uint8_t                    *dynamic_eq_buf;            /**< store eq param from phone */
    uint16_t                    dynamic_eq_len;            /**< for dynamic_eq */
    uint8_t                     wait_resume_a2dp_idx;
    uint8_t                     update_active_a2dp_idx;

    T_APP_DEVICE_STATE          device_state;
    T_BUD_COUPLE_STATE          bud_couple_state;
    uint8_t                     remote_session_state;
    bool                        key0_wake_up;
    bool                        peri_wake_up;
    bool                        combine_poweron_key_wake_up;
    bool                        reject_call_by_key;                   /*reject incoming by key*/
    bool                        listen_save_mode;

    uint8_t                     first_decide_role;
    uint8_t                     executing_charger_box_special_cmd;
    uint8_t                     jig_subcmd : 3;
    uint8_t                     jig_dongle_id : 5;
    uint8_t                     power_off_cause;

    APP_CHARGER_STATE           local_charger_state;                  /**< local charger state */
    APP_CHARGER_STATE           remote_charger_state;                 /**< remote charger  state */
    uint8_t                     first_hf_index;
    uint8_t                     last_hf_index;

    uint8_t                     *final_listening_state;
    uint8_t                     last_anc_on_state;
    uint8_t                     last_llapt_on_state;
    uint8_t                     current_listening_state;
    bool                        delay_apply_listening_mode;
    uint8_t                     power_on_delay_open_apt_time;
    uint8_t                     last_listening_state;       /*Last state in listening mode cycle*/

    uint8_t                     sd_playback_switch;
    uint8_t                     ota_tooling_start;
    bool                        hfp_report_batt;

    uint8_t                     bt_addr_disconnect[6];
    bool                        ignore_led_mode;
    uint8_t                     batt_roleswap;

    bool                        play_min_max_vol_tone;
    bool                        is_circular_vol_up;
    bool                        gaming_mode;
    bool                        playback_muted;
    bool                        voice_muted;
    bool                        power_on_by_cmd;
    bool                        power_off_by_cmd;

    T_APP_TONE_VP_STARTED       tone_vp_status;
    bool                        wait_resume_a2dp_flag;
    bool                        sco_wait_a2dp_sniffing_stop;
    uint8_t                     pending_sco_idx;
    bool                        a2dp_wait_a2dp_sniffing_stop;
    uint8_t                     pending_a2dp_idx;
    uint8_t                     active_media_paused;

    bool                        bt_is_ready;
    uint8_t                     remote_spk_channel;
    uint8_t                     in_ear_recover_a2dp;
    bool                        detect_suspend_by_out_ear;
    uint8_t                     b2s_connected_num;
    uint8_t                     rsv6;
    uint8_t                     key_action_disallow_too_close;

    uint8_t                     adp_high_wake_up_for_zero_box_bat_vol : 1;
    uint8_t                     need_sync_case_battery_to_pri : 1;
    uint8_t                     rsv5 : 6;

    uint8_t                     remote_default_channel;
    bool                        remote_is_8753bau;
    bool                        gaming_mode_request_is_received;
    bool                        ignore_bau_first_gaming_cmd;
    bool                        restore_gaming_mode;
    bool                        dongle_is_enable_mic;
    bool                        dongle_is_disable_apt;
    bool                        disallow_play_gaming_mode_vp;

    T_AUDIO_EFFECT_INSTANCE     apt_eq_instance;

    // multilink
    uint8_t                     resume_addr[6]; // reconnected addr after roleswap

    uint8_t                     connected_num_before_roleswap : 2;
    uint8_t                     disallow_connected_tone_after_inactive_connected : 1;
    uint8_t                     rsv7 : 5;

    uint8_t                     pending_mmi_action_in_roleswap;

    uint8_t                     remote_bat_vol;
    bool                        src_roleswitch_fail_no_sniffing;
    bool                        rsv8;
    bool                        force_enter_dut_mode_when_acl_connected;
    bool                        rsv3;
    uint8_t                     spk_eq_mode;
    uint8_t                     rsv9;
    bool                        disallow_sniff;
    bool                        recover_param;

    uint8_t                     factory_reset_clear_mode;
    bool                        waiting_factory_reset;
    uint8_t                     ble_common_adv_after_roleswap;
    uint8_t                     down_count;

#if F_APP_ADJUST_TONE_VOLUME_SUPPORT
    uint8_t                     local_tone_volume_min_level;
    uint8_t                     local_tone_volume_max_level;
    uint8_t                     remote_tone_volume_min_level;
    uint8_t                     remote_tone_volume_max_level;
    uint8_t                     remote_voice_prompt_volume_out_level;
    uint8_t                     remote_ringtone_volume_out_level;
#endif

    uint8_t                     remote_apt_volume_out_level;
    uint8_t                     remote_main_brightness_level;

    uint16_t                    remote_apt_sub_volume_out_level;
    uint16_t                    remote_sub_brightness_level;

    uint8_t                     apt_volume_type;
    uint8_t                     brightness_type;
    uint8_t                     sw_eq_type;
    uint8_t                     a2dp_preemptive_flag;
    uint16_t                    last_gaming_mode_audio_track_latency;

    /* Need to remove to relay module */
    T_REMOTE_RELAY_HANDLE       relay_handle;
    T_OS_QUEUE                  relay_cback_list;

#if F_APP_QOL_MONITOR_SUPPORT
    uint16_t                     buffer_level_remote;
    uint16_t                     buffer_level_local;
    int8_t                       rssi_local;
    int8_t                       rssi_remote;
    uint8_t                      src_going_away_multilink_enabled;
    uint8_t                      sec_going_away : 1;
    uint8_t                      rsv2 : 7;
#endif

#if F_APP_MUTILINK_VA_PREEMPTIVE
    uint8_t                     active_hfp_addr[6];
#endif

    uint8_t                     b2b_role_switch_led_pending : 1;
    uint8_t                     pending_handle_power_off: 1;
    uint8_t                     is_long_press_power_on_play_vp : 1;
    uint8_t                     disconnect_inactive_link_actively : 1;
    uint8_t                     factory_reset_ignore_led_and_vp : 1;
    uint8_t                     ble_is_ready : 1;
    uint8_t                     rsv1 : 2;

    uint8_t                     mic_mp_verify_pri_sel_ori : 3;
    uint8_t                     mic_mp_verify_pri_type_ori : 2;
    uint8_t                     mic_mp_verify_sec_sel_ori : 3;

    uint8_t                     mic_mp_verify_sec_type_ori : 2;
    uint8_t                     ignore_set_pri_audio_eq : 1;
    uint8_t                     eq_ctrl_by_src : 1;
    uint8_t                     last_bud_loc_event : 4;

#if F_APP_LINEIN_SUPPORT
    T_AUDIO_EFFECT_INSTANCE     line_in_eq_instance;
#endif
    bool                        connected_tone_need;
    uint8_t                     conn_bit_map;
    uint8_t                     sass_target_drop_device[6];

    T_WAKE_UP_REASON            wake_up_reason;

    uint32_t                    playback_start_time;
    uint32_t                    power_on_start_time;

    bool                        need_delay_power_on;

} T_APP_DB;
/** End of APP_MAIN_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Variables App Main Variables
    * @{
    */
extern T_APP_DB app_db;

extern void *audio_evt_queue_handle;
extern void *audio_io_queue_handle;
/** End of APP_MAIN_Exported_Variables
    * @}
    */

/** End of APP_MAIN
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MAIN_H_ */
