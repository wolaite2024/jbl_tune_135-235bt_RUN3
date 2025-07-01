/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_BT_POLICY_H_
#define _APP_BT_POLICY_H_

#include <stdint.h>
#include <stdbool.h>
#include "btm.h"
#include "app_linkback.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BT_POLICY App BT Policy
  * @brief App BLE Device
  * @{
  */

typedef enum
{
    BP_EVENT_STATE_CHANGED             = 0x00,
    BP_EVENT_RADIO_MODE_CHANGED        = 0x10,
    BP_EVENT_ROLE_DECIDED              = 0x20,
    BP_EVENT_BUD_CONN_START            = 0x30,
    BP_EVENT_BUD_AUTH_SUC              = 0x31,
    BP_EVENT_BUD_CONN_FAIL             = 0x32,
    BP_EVENT_BUD_AUTH_FAIL             = 0x33,
    BP_EVENT_BUD_DISCONN_NORMAL        = 0x34,
    BP_EVENT_BUD_DISCONN_LOST          = 0x35,
    BP_EVENT_BUD_REMOTE_CONN_CMPL      = 0x36,
    BP_EVENT_BUD_REMOTE_DISCONN_CMPL   = 0x37,
    BP_EVENT_BUD_LINKLOST_TIMEOUT      = 0x38,
    BP_EVENT_SRC_AUTH_SUC              = 0x40,
    BP_EVENT_SRC_AUTH_FAIL             = 0x41,
    BP_EVENT_SRC_DISCONN_LOST          = 0x42,
    BP_EVENT_SRC_DISCONN_NORMAL        = 0x43,
    BP_EVENT_SRC_DISCONN_ROLESWAP      = 0x44,
    BP_EVENT_PROFILE_CONN_SUC          = 0x50,
    BP_EVENT_PROFILE_DISCONN           = 0x51,
    BP_EVENT_PAIRING_MODE_TIMEOUT      = 0x60,
    BP_EVENT_DEDICATED_PAIRING         = 0x61,
} T_BP_EVENT;

typedef struct
{
    uint8_t *bd_addr;
    bool is_first_prof;
    bool is_last_prof;
    bool is_ignore;
    bool is_first_src;
    bool is_shut_down;
    uint32_t prof;
    uint16_t cause;
} T_BP_EVENT_PARAM;

typedef enum
{
    BP_STATE_IDLE                = 0x00,
    BP_STATE_FIRST_ENGAGE        = 0x01,
    BP_STATE_ENGAGE              = 0x02,
    BP_STATE_LINKBACK            = 0x03,
    BP_STATE_CONNECTED           = 0x04,
    BP_STATE_PAIRING_MODE        = 0x05,
    BP_STATE_STANDBY             = 0x06,
    BP_STATE_SECONDARY           = 0x07,
    BP_STATE_PREPARE_ROLESWAP    = 0x08,
} T_BP_STATE;

typedef enum
{
    BP_TPOLL_ACL_CONN_EVENT         = 0x00,
    BP_TPOLL_ACL_DISC_EVENT         = 0x01,
    BP_TPOLL_LOW_LATENCY_EVENT      = 0x02,
    BP_TPOLL_A2DP_PLAY_EVENT        = 0x03,
    BP_TPOLL_A2DP_STOP_EVENT        = 0x04,
    BP_TPOLL_SCO_CONN_EVENT         = 0x05,
    BP_TPOLL_SCO_DISC_EVENT         = 0x06,
    BP_TPOLL_B2B_CONN_EVENT         = 0x07,
    BP_TPOLL_B2B_DISC_EVENT         = 0x08,
    BP_TPOLL_CFU_START_EVENT        = 0x09,
    BP_TPOLL_CFU_END_EVENT          = 0x0A,
    BP_TPOLL_VP_UPDATE_START_EVENT  = 0x0B,
    BP_TPOLL_VP_UPDATE_END_EVENT    = 0x0C,
    BP_TPOLL_LINKBACK_START         = 0x0D,
    BP_TPOLL_LINKBACK_STOP          = 0x0E,
    BP_TPOLL_START_ROLESWAP         = 0x0F,
} T_BP_TPOLL_EVENT;

typedef enum
{
    BP_TPOLL_INIT                       = 0x00,
    BP_TPOLL_IDLE                       = 0x01,
    BP_TPOLL_GAMING_A2DP                = 0x02,
    BP_TPOLL_GAMING_8753BAU_RWS         = 0x03,
    BP_TPOLL_GAMING_8753BAU_RWS_SINGLE  = 0x04,
    BP_TPOLL_GAMING_8753BAU_STEREO      = 0x05,
    BP_TPOLL_A2DP                       = 0x06,
    BP_TPOLL_TEAMS_UPDATE               = 0x07,
    BP_TPOLL_IDLE_SINGLE_LINKBACK       = 0x08,
    BP_TPOLL_MAX                        = 0x09,
} T_BP_TPOLL_STATE;

typedef struct
{
    T_BP_TPOLL_STATE state;
    uint8_t tpoll_value;
} T_BP_TPOLL_MAPPING;

extern void (*app_roleswap_src_connect_delay_hook)(void);
void mmi_disconnect_all_link_event_handle(void);
typedef void (*T_BP_IND_FUN)(T_BP_EVENT event, T_BP_EVENT_PARAM *event_param);

void app_bt_policy_init(void);

void app_bt_stop_a2dp_and_sco(void);

void app_bt_policy_startup(T_BP_IND_FUN fun, bool at_once_trigger);
void app_bt_policy_shutdown(void);
void app_bt_policy_stop(void);
void app_bt_policy_restore(void);
void app_bt_policy_prepare_for_roleswap(void);

void app_bt_policy_msg_prof_conn(uint8_t *bd_addr, uint32_t prof);
void app_bt_policy_msg_prof_disconn(uint8_t *bd_addr, uint32_t prof, uint16_t cause);

void app_bt_policy_default_connect(uint8_t *bd_addr, uint32_t plan_profs, bool check_bond_flag);
void app_bt_policy_special_connect(uint8_t *bd_addr, uint32_t plan_prof,
                                   T_LINKBACK_SEARCH_PARAM *search_param);
void app_bt_policy_disconnect(uint8_t *bd_addr, uint32_t plan_profs);
void app_bt_policy_disconnect_all_link(void);

void app_bt_policy_enter_pairing_mode(bool force, bool visiable);
void app_bt_policy_exit_pairing_mode(void);

void app_bt_policy_enter_dut_test_mode(void);

void app_bt_policy_start_ota_shaking(void);
void app_bt_policy_enter_ota_mode(bool connectable);

T_BP_STATE app_bt_policy_get_state(void);

T_BT_DEVICE_MODE app_bt_policy_get_radio_mode(void);

bool app_bt_policy_get_b2b_connected(void);
uint8_t app_bt_policy_get_b2s_connected_num(void);
uint8_t app_bt_policy_get_b2s_connected_num_with_profile(void);
void app_bt_policy_set_b2s_connected_num_max(uint8_t num_max);
void app_bt_policy_sync_b2s_connected(void);

bool app_bt_policy_get_first_connect_sync_default_vol_flag(void);
void app_bt_policy_set_first_connect_sync_default_vol_flag(bool flag);
void app_bt_policy_primary_engage_action_adjust(void);

void app_bt_policy_qos_param_update(uint8_t *bd_addr, T_BP_TPOLL_EVENT event);
void app_stop_reconnect_timer(void);
void app_bt_policy_b2s_conn_start_timer(void);

void app_bt_policy_abandon_engage(void);
void app_bt_update_pair_idx_mapping(void);
uint8_t *app_bt_policy_get_linkback_device(void);
bool src_conn_ind_check(uint8_t *bd_addr);

#if F_APP_LE_AUDIO_SM
void app_bt_policy_set_legacy(uint8_t para);
bool app_bt_policy_is_pairing(void);
bool app_bt_policy_listening_allow_poweroff(void);
#endif

/** End of APP_BT_POLICY
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_POLICY_H_ */
