/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_MULTILINK_H_
#define _APP_MULTILINK_H_

#include <stdbool.h>
#include <stdint.h>
#include "app_roleswap.h"
#include "app_relay.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_MULTILINK App Multilink
  * @brief App Multilink
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_MULTILINK_Exported_Macros App Multilink Macros
   * @{
   */
#define UPDATE_ACTIVE_A2DP_INDEX_TIMER  2000
#define MULTILINK_SRC_CONNECTED         2
#define TIMER_TO_DISCONN_ACL            800

#define HARMAN_NOTIFY_DEVICE_INFO_TIME  500

/** End of APP_MULTILINK_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_MULTILINK_Exported_Types App Multilink Types
    * @{
    */
/** @brief Multilink handle connect, disconnect, related player status event */
typedef enum
{
    JUDGE_EVENT_A2DP_CONNECTED,
    JUDGE_EVENT_MEDIAPLAYER_PLAYING,
    JUDGE_EVENT_MEDIAPLAYER_PAUSED,
    JUDGE_EVENT_A2DP_START,
    JUDGE_EVENT_A2DP_DISC,
    JUDGE_EVENT_DSP_SILENT,
    JUDGE_EVENT_A2DP_STOP,
    JUDGE_EVENT_SNIFFING_STOP,

    JUDGE_EVENT_TOTAL
} T_APP_JUDGE_A2DP_EVENT;

typedef enum
{
    APP_REMOTE_MSG_PROFILE_CONNECTED,
    APP_REMOTE_MSG_PHONE_CONNECTED,
    APP_REMOTE_MSG_MUTE_AUDIO_WHEN_MULTI_CALL_NOT_IDLE,
    APP_REMOTE_MSG_UNMUTE_AUDIO_WHEN_MULTI_CALL_IDLE,
    APP_REMOTE_MSG_RESUME_BT_ADDRESS,
    APP_REMOTE_MSG_ACTIVE_BD_ADDR_SYNC,
    APP_REMOTE_MSG_CONNECTED_TONE_NEED,
    APP_REMOTE_MSG_SASS_PREEM_BIT_SYNC,
    APP_REMOTE_MSG_SASS_SWITCH_SYNC,
    APP_REMOTE_MSG_SASS_MULTILINK_STATE_SYNC,
    APP_REMOTE_MSG_SASS_DEVICE_BITMAP_SYNC,
    APP_REMOTE_MSG_SASS_DEVICE_SUPPORT_SYNC,
    APP_REMOTE_MSG_MULTILINK_TOTAL
} T_MULTILINK_REMOTE_MSG;

/** @brief Multilink update active a2dp link, delay role switch timer */
typedef enum
{
    MULTILINK_UPDATE_ACTIVE_A2DP_INDEX,
    MULTILINK_PAUSE_INACTIVE_A2DP,
    MULTILINK_DISCONN_INACTIVE_ACL,
#if F_APP_MUTILINK_VA_PREEMPTIVE
    MULTILINK_DISC_VA_SCO,
#endif
    MULTILINK_SILENCE_PACKET_JUDGE,
    MULTILINK_SILENCE_PACKET_TIMER,
    AU_TIMER_HARMAN_POWER_OFF_OPTION,
    HARMAN_NOTIFY_DEVICEINFO,
    HARMAN_ADC_READ_HARMAN_ADP_VOLTAGE,
    MULTILINK_A2DP_STOP_TIMER,
} T_MULTILINK_TIMER;

typedef enum
{
    MULTI_A2DP_PREEM,
    MULTI_HFP_PREEM,
    MULTI_AVRCP_PREEM,
    MULTI_FORCE_PREEM,
} T_MULTILINK_SASS_ACTON;

//a2dp can preemptive a2dp, bit 0
//a2dp can preemptive sco,  bit 1
//a2dp can preemptive va,   bit 2
//sco can preemptive a2dp,  bit 3
//sco can preemptive sco,   bit 4
//sco can preemptive va,    bit 5
//va can preemptive a2dp,   bit 6
//va can preemptive sco,    bit 7
//va can preemptive va,     bit 8

#define sass_a2dp_a2dp 0x0001
#define sass_sco_sco   0x0002
#define sass_a2dp_sco  0x0004
#define sass_sco_a2dp  0x0008
#define sass_a2dp_va   0x0010
#define sass_sco_va    0x0020
#define sass_va_a2dp   0x0040
#define sass_va_sco    0x0080
#define sass_va_va     0x0100

/** End of APP_MULTILINK_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_MULTILINK_Exported_Functions App Multilink Functions
    * @{
    */
/**
    * @brief  multilink module init
    * @param  void
    * @return void
    */
void app_multilink_init(void);

/**
    * @brief  get current active a2dp link index
    * @param  void
    * @return active a2dp index
    */
uint8_t app_get_active_a2dp_idx(void);

/**
    * @brief  set current active a2dp link index
    * @param  idx
    * @return void
    */
void app_set_active_a2dp_idx(uint8_t idx);

/**
    * @brief  set active a2dp link index
    * @param  bd_addr
    * @return bool
    */
bool app_a2dp_active_link_set(uint8_t *bd_addr);

/**
    * @brief  judge one device a2dp link as active link and avrcp can control it.
    *         config BT qos when multilink connect or disconnect.
    * @param  app_idx BT link index
    * @param  event judge a2dp active link event
    * @return void
    */
void app_judge_active_a2dp_idx_and_qos(uint8_t app_idx, T_APP_JUDGE_A2DP_EVENT event);

void app_multilink_disconnect_inactive_link(uint8_t app_idx);
void app_reconnect_inactive_link(void);
void app_disconnect_inactive_link(void);

bool app_active_link_check(uint8_t *bd_addr);

void app_multilink_remote_msg_handler(uint16_t msg, void *buf, uint16_t len);

void app_handle_sniffing_link_disconnect_event(uint8_t id);

bool app_pause_inactive_a2dp_link_stream(uint8_t keep_active_a2dp_idx, uint8_t resume_fg);

void app_resume_a2dp_link_stream(uint8_t app_idx);

void app_bond_set_priority(uint8_t *bd_addr);
bool app_multi_check_in_sniffing(void);

void app_multilink_switch_by_mmi(bool is_on_by_mmi);
bool app_multilink_is_on_by_mmi(void);

void app_multilink_sco_preemptive(uint8_t inactive_idx);
uint8_t app_multi_get_acl_connect_num(void);
void app_multilink_stop_acl_disconn_timer(void);
void app_multi_active_hfp_transfer(uint8_t idx, bool disc, bool force);

typedef enum
{
    CONNECT_IDLE_POWER_OFF_START    = 0,
    DISC_RESET_TO_COUNT             = 1,
    ACTIVE_NEED_STOP_COUNT          = 2,
} T_CONNECT_IDLE_POWER_OFF;

void au_set_harman_aling_active_idx(uint8_t active_idx);
uint8_t au_get_harman_aling_active_idx(void);
void au_multilink_harman_check_silent(uint8_t set_idx, uint8_t silent_check);
void au_set_record_a2dp_active_ever(bool res);
bool au_get_record_a2dp_active_ever(void);
void au_set_power_on_link_back_fg(bool res);
bool au_get_power_on_link_back_fg(void);
void au_connect_idle_to_power_off(T_CONNECT_IDLE_POWER_OFF action, uint8_t index);
void au_dump_link_information(void);
uint8_t au_ever_link_information(void);
void at_pairing_radio_setting(uint8_t enable);
void au_set_harman_already_connect_one(bool res);
bool au_get_harman_already_connect_one(void);

bool app_multilink_preemptive_judge(uint8_t app_idx, uint8_t type);
bool app_multilink_sass_bitmap(uint8_t app_idx, uint8_t profile, bool a2dp_check);
uint8_t app_multilink_get_highest_priority_index(uint32_t profile_mask);
uint8_t app_multilink_get_active_idx(void);
void app_multilink_set_active_idx(uint8_t idx);

int8_t app_multilink_get_available_connection_num(void);
uint8_t app_multilink_get_inactive_index(uint8_t new_link_idx, uint8_t call_num, bool force);
uint8_t app_multilink_find_other_idx(uint8_t app_idx);
bool app_multilink_get_stream_only(uint8_t idx);
bool app_multilink_pause_inactive_for_sass(void);

void multilink_notify_deviceinfo(uint8_t conn_id, uint32_t time);
void multilink_notify_deviceinfo_stop(void);
bool app_multilink_get_in_silence_packet(void);
/** @} */ /* End of group APP_MULTILINK_Exported_Functions */

/** End of APP_MULTILINK
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MULTILINK_H_ */
