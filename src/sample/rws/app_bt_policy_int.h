/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_BT_POLICY_INT_H_
#define _APP_BT_POLICY_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "gap_legacy.h"
#include "btm.h"
#include "app_link_util.h"
#include "app_linkback.h"
#include "app_bt_policy_api.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FE_MP_RSSI                            (-60)
#define FE_TO_MIN                             (1)
#define FE_TO_UNIT                            (5000)
#define PEER_VALID_MAGIC                      (0x1234)

#define APP_BT_POLICY_B2S_CONN_TIMER_MS          5000
typedef enum
{
    STATE_STARTUP                             = 0x00,
    STATE_FE_SHAKING                          = 0x10,
    STATE_FE_COUPLING                         = 0x11,
    STATE_AFE_TIMEOUT_SHAKING                 = 0x20,
    STATE_AFE_COUPLING                        = 0x21,
    STATE_AFE_LINKBACK                        = 0x22,
    STATE_AFE_CONNECTED                       = 0x23,
    STATE_AFE_PAIRING_MODE                    = 0x24,
    STATE_AFE_STANDBY                         = 0x25,
    STATE_AFE_SECONDARY                       = 0x26,
    STATE_AFE_WAIT_COUPLING                   = 0x27,
    STATE_DUT_TEST_MODE                       = 0x28,
    STATE_OTA_SHAKING                         = 0x30,
    STATE_OTA_MODE                            = 0x31,
    STATE_PREPARE_FOR_ROLESWAP                = 0X32,
    STATE_SHUTDOWN_STEP                       = 0x40,
    STATE_SHUTDOWN                            = 0x41,
    STATE_STOP                                = 0x42,
} T_STATE;

typedef enum
{
    EVENT_STARTUP                             = 0x00,
    EVENT_FE_TIMEOUT                          = 0x10,
    EVENT_FE_SHAKING_DONE                     = 0x11,
    EVENT_AFE_SHAKING_DONE                    = 0x12,
    EVENT_BUD_CONN_SUC                        = 0x20,
    EVENT_BUD_CONN_FAIL                       = 0x21,
    EVENT_BUD_AUTH_SUC                        = 0x22,
    EVENT_BUD_AUTH_FAIL                       = 0x23,
    EVENT_BUD_DISCONN_LOCAL                   = 0x24,
    EVENT_BUD_DISCONN_NORMAL                  = 0x25,
    EVENT_BUD_DISCONN_LOST                    = 0x26,
    EVENT_BUD_REMOTE_CONN_CMPL                = 0x27,
    EVENT_BUD_REMOTE_DISCONN_CMPL             = 0x28,
    EVENT_BUD_LINKLOST_TIMEOUT                = 0x29,
    EVENT_BUD_WAIT_COUPLING_TO                = 0x2a,
    EVENT_BUD_ENGAGE_ACTION_ADJUST            = 0x2b,
    EVENT_SRC_CONN_SUC                        = 0x30,
    EVENT_SRC_CONN_FAIL                       = 0x31,
    EVENT_SRC_CONN_FAIL_ACL_EXIST             = 0x32,
    EVENT_SRC_CONN_FAIL_CONTROLLER_BUSY       = 0x33,
    EVENT_SRC_CONN_TIMEOUT                    = 0x34,
    EVENT_SRC_AUTH_LINK_KEY_INFO              = 0x35,
    EVENT_SRC_AUTH_LINK_KEY_REQ               = 0x36,
    EVENT_SRC_AUTH_LINK_PIN_CODE_REQ          = 0x37,
    EVENT_SRC_AUTH_SUC                        = 0x38,
    EVENT_SRC_AUTH_FAIL                       = 0x39,
    EVENT_SRC_DISCONN_LOST                    = 0x3a,
    EVENT_SRC_DISCONN_NORMAL                  = 0x3b,
    EVENT_SRC_DISCONN_ROLESWAP                = 0x3d,
    EVENT_SCO_CONN_CMPL                       = 0x3e,
    EVENT_SCO_DISCONNECTED                    = 0x3f,
    EVENT_PROFILE_SDP_ATTR_INFO               = 0x40,
    EVENT_PROFILE_DID_ATTR_INFO               = 0x41,
    EVENT_PROFILE_SDP_DISCOV_CMPL             = 0x42,
    EVENT_PROFILE_CONN_SUC                    = 0x43,
    EVENT_PROFILE_DISCONN                     = 0x44,
    EVENT_PROFILE_SDP_DISCOV_STOP             = 0x45,
    EVENT_PROFILE_CONN_FAIL                   = 0x46,
    EVENT_ROLESWAP                            = 0x50,
    EVENT_ROLE_MASTER                         = 0x51,
    EVENT_ROLE_SLAVE                          = 0x52,
    EVENT_ROLESWITCH_FAIL                     = 0x53,
    EVENT_ROLESWITCH_TIMEOUT                  = 0x54,
    EVENT_CONN_SNIFF                          = 0x55,
    EVENT_CONN_ACTIVE                         = 0x56,
    EVENT_PAIRING_MODE_TIMEOUT                = 0x60,
    EVENT_DISCOVERABLE_TIMEOUT                = 0x61,
    EVENT_DEDICATED_ENTER_PAIRING_MODE        = 0x70,
    EVENT_DEDICATED_EXIT_PAIRING_MODE         = 0x71,
    EVENT_DEDICATED_CONNECT                   = 0x72,
    EVENT_DEDICATED_DISCONNECT                = 0x73,
    EVENT_DISCONNECT_ALL                      = 0x74,
    EVENT_LINKBACK_DELAY_TIMEOUT              = 0x76,
    EVENT_PAGESCAN_PARAM_TIMEOUT              = 0x77,
    EVENT_SRC_CONN_IND_TIMEOUT                = 0x78,
    EVENT_ENTER_DUT_TEST_MODE                 = 0x79,
    EVENT_START_OTA_SHAKING                   = 0x80,
    EVENT_ENTER_OTA_MODE                      = 0x81,
    EVENT_PREPARE_FOR_ROLESWAP                = 0x82,
    EVENT_RESTORE                             = 0x83,
    EVENT_SHUTDOWN_STEP_TIMEOUT               = 0x90,
    EVENT_SHUTDOWN                            = 0x91,
    EVENT_STOP                                = 0xa0,
    EVENT_B2B_AUTH_LINK_KEY_INFO              = 0xb0,
    EVENT_B2B_AUTH_LINK_KEY_REQ               = 0xb1,
} T_EVENT;

typedef enum
{
    TIMER_ID_SHUTDOWN_STEP                    = 0x00,
    TIMER_ID_FIRST_ENGAGE_CHECK               = 0x01,
    TIMER_ID_PAIRING_TIMEOUT                  = 0x03,
    TIMER_ID_DISCOVERABLE_TIMEOUT             = 0x04,
    TIMER_ID_ROLE_SWITCH                      = 0x05,
    TIMER_ID_BUD_LINKLOST_TIMEOUT             = 0x06,
    TIMER_ID_LINKBACK_TIMEOUT                 = 0x07,
    TIMER_ID_LINKBACK_DELAY                   = 0x09,
    TIMER_ID_LATER_AVRCP                      = 0x0a,
    TIMER_ID_WAIT_COUPLING                    = 0x0b,
    TIMER_ID_ENGAGE_ACTION_ADJUST             = 0x0c,
    TIMER_ID_LATER_HID                        = 0x0d,
    TIMER_ID_RECONNECT                        = 0x0e,
    TIMER_ID_PAGESCAN_PARAM_TIMEOUT           = 0x0f,
    TIMER_ID_SRC_CONN_IND_TIMEOUT             = 0x10,
    TIMER_ID_B2S_CONN_TIMER_MS                = 0x11,
} T_TIMER;

typedef enum
{
    STABLE_ENTER_MODE_NORMAL                  = 0x00,
    STABLE_ENTER_MODE_AGAIN                   = 0x01,
    STABLE_ENTER_MODE_DEDICATED_PAIRING       = 0x02,
    STABLE_ENTER_MODE_NOT_PAIRING             = 0x03,
    STABLE_ENTER_MODE_DIRECT_PAIRING          = 0x04,
} T_STABLE_ENTER_MODE;

typedef enum
{
    SHUTDOWN_STEP_BEGIN                       = 0x00,
    SHUTDOWN_STEP_START_DISCONN_B2S_PROFILE   = 0x10,
    SHUTDOWN_STEP_WAIT_DISCONN_B2S_PROFILE    = 0x11,
    SHUTDOWN_STEP_START_DISCONN_B2S_LINK      = 0x20,
    SHUTDOWN_STEP_WAIT_DISCONN_B2S_LINK       = 0x21,
    SHUTDOWN_STEP_START_DISCONN_B2B_LINK      = 0x30,
    SHUTDOWN_STEP_WAIT_DISCONN_B2B_LINK       = 0x31,
    SHUTDOWN_STEP_END                         = 0x40,
} T_SHUTDOWN_STEP;

typedef struct
{
    uint16_t link_src_conn_idx;
    T_APP_REMOTE_DEVICE_VEDDOR_ID remote_device_vendor_id;
    T_APP_ABS_VOL_STATE abs_vol_state;
    uint8_t rtk_vendor_spp_active;
    uint8_t tx_event_seqn;
    bool b2s_connected_vp_is_played;
    bool connected_by_linkback;
    uint8_t gfps_rfc_chann;
    bool    gfps_rfc_connected;
    uint8_t gfps_session_nonce[8];
    uint8_t gfps_inuse_account_key;
} T_BP_ROLESWAP_LINK;

typedef struct
{
    T_BP_ROLESWAP_LINK link;
    uint16_t last_src_conn_idx;
    T_BP_STATE bp_state;
} T_BP_ROLESWAP_INFO;

typedef enum
{
    BT_POLICY_MSG_ROLESWAP_INFO                  = 0x00,
    BT_POLICY_MSG_PRI_BP_STATE                   = 0x01,
    BT_POLICY_MSG_PRI_REQ                        = 0x02,
    BT_POLICY_MSG_SNIFF_MODE                     = 0x03,
    BT_POLICY_MSG_LINKBACK_NODE                  = 0x04,
    BT_POLICY_MSG_B2S_ACL_CONN                   = 0x05,
    BT_POLICY_MSG_SYNC_SERVICE_STATUS            = 0x06,

    BT_POLICY_MSG_MAX
} T_APP_BT_POLICY_REMOTE_MSG;

typedef struct
{
    uint8_t num_max;
    uint8_t num;
} T_B2S_CONNECTED;

typedef struct
{
    T_BP_IND_FUN ind_fun;
    bool at_once_trigger;
} T_STARTUP_PARAM;

typedef struct
{
    uint8_t *bd_addr;
    bool is_b2b;
    uint32_t prof;
    uint16_t cause;
    uint8_t bud_role;
    T_BT_LINK_KEY_TYPE key_type;
    uint8_t *link_key;
    T_BT_SDP_ATTR_INFO *sdp_info;
    bool is_special;
    T_LINKBACK_SEARCH_PARAM *search_param;
    bool check_bond_flag;
    bool not_check_addr_flag;
    bool is_visiable;
    bool is_connectable;
    bool is_force;
    bool is_suc;
    bool is_later_avrcp;
    bool is_shaking_to;
    bool is_later_hid;
} T_BT_PARAM;

typedef enum
{
    LINKBACK_SCENARIO_HF_A2DP_LAST_DEVICE,
    LINKBACK_SCENARIO_HFP_BASE,
    LINKBACK_SCENARIO_A2DP_BASE,
    LINKBACK_SCENARIO_SPP_BASE,
    LINKBACK_SCENARIO_HID_BASE,
    LINKBACK_SCENARIO_TOTAL,
} T_LINKBACK_SCENARIO;

typedef enum
{
    ROLESWITCH_EVENT_LINK_CONNECTED      = 0x00,
    ROLESWITCH_EVENT_LINK_ACTIVE         = 0x01,
    ROLESWITCH_EVENT_LINK_DISCONNECTED   = 0x02,
    ROLESWITCH_EVENT_FAIL_RETRY          = 0x03,
    ROLESWITCH_EVENT_FAIL_RETRY_MAX      = 0x04,
    ROLESWITCH_EVENT_ROLE_CHANGED        = 0x05,
} T_ROLESWITCH_EVENT;

typedef enum
{
    PRI_REQ_LET_SEC_TO_DISCONN           = 0x01,
} T_PRI_REQ;

typedef struct
{
    void *timer_handle;
    uint8_t bd_addr[6];
} T_SRC_CONN_IND;

void state_machine(T_EVENT event, void *param);
uint8_t b2s_connected_get_num_max(void);
uint32_t get_profs_by_bond_flag(uint32_t bond_flag);
void b2s_connected_set_last_conn_index(uint8_t conn_idx);
uint8_t b2s_connected_get_last_conn_index(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BT_POLICY_H_ */
