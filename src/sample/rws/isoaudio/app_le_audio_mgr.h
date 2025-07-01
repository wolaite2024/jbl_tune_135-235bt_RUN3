#ifndef _APP_LE_AUDIO_MGR_H_
#define _APP_LE_AUDIO_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "audio.h"
#include "codec_def.h"
#include "multitopology_ctrl.h"

extern bool big_passive_flag;
typedef struct
{
    bool    used;
    uint8_t conn_id;
    uint8_t source_id;
    uint8_t bis_idx;
    uint16_t bis_conn_handle;
    T_CODEC_CFG     bis_codec_cfg;
    uint8_t frame_num;
    T_AUDIO_TRACK_HANDLE audio_track_handle;
} T_BIS_CB;


typedef enum
{
    LEA_ADV_TO_CIS      = 0x00,
    LEA_ADV_TO_DELEGATOR,

    LEA_ADV_TO_TOTAL,
} T_LEA_ADV_TO;


typedef enum
{
    LEA_ADV_MODE_PAIRING      = 0x00,
    LEA_ADV_MODE_LOSSBACK,
    LEA_ADV_MODE_DELEGATOR,

    LEA_ADV_MODE_TOTAL,
} T_LEA_ADV_MODE;


typedef enum
{
    LE_AUDIO_DEVICE_IDLE    = 0x00,
    LE_AUDIO_DEVICE_ADV     = 0x01,

    LE_AUDIO_DEVICE_TOTAL,
} T_LE_AUDIO_DEVICE_SM;

typedef enum
{
    LE_AUDIO_LINK_IDLE      = 0x00,
    LE_AUDIO_LINK_CONNECTED = 0x01,
    LE_AUDIO_LINK_STREAMING = 0x02,

    LE_AUDIO_LINK_TOTAL,
} T_LE_AUDIO_LINK_SM;




typedef enum
{
    LE_AUDIO_BIS_STATE_IDLE      = 0x00,
    LE_AUDIO_BIS_STATE_PRE_ADV   = 0x01,
    LE_AUDIO_BIS_STATE_ADV       = 0x02,
    LE_AUDIO_BIS_STATE_PRE_SCAN  = 0x03,
    LE_AUDIO_BIS_STATE_SCAN      = 0x04,
    LE_AUDIO_BIS_STATE_CONN      = 0x05,
    LE_AUDIO_BIS_STATE_STREAMING = 0x06,

    LE_AUDIO_BIS_SM_TOTAL,
} T_LE_AUDIO_BIS_SM;

typedef enum
{
    LE_AUDIO_POWER_OFF          = 0x01,
    LE_AUDIO_ENTER_PAIRING      = 0x02,
    LE_AUDIO_EXIT_PAIRING       = 0x03,
    LE_AUDIO_FE_SHAKING_DONE    = 0x04,
    LE_AUDIO_AFE_SHAKING_DONE   = 0x05,
    LE_AUDIO_MMI                = 0x06,
    LE_AUDIO_ADV_START          = 0x07,
    LE_AUDIO_ADV_STOP           = 0x08,
    LE_AUDIO_ADV_TIMEOUT        = 0x09,
    //LE_AUDIO_ADD_DEVICE         = 0x0A,
    //LE_AUDIO_PASYNC             = 0x0B,  //no using
    LE_AUDIO_BIGSYNC            = 0x0C,
    LE_AUDIO_BIGTERMINATE       = 0x0D,
    LE_AUDIO_SCAN_TIMEOUT       = 0x0E,
    LE_AUDIO_SCAN_STOP          = 0x0F,
    LE_AUDIO_SCAN_DELEGATOR     = 0x10,
    LE_AUDIO_CIG_START          = 0x11,

    LE_AUDIO_DEVICE_EVENT_TOTAL,
} T_LE_AUDIO_DEVICE_EVENT;

typedef enum
{
    LE_AUDIO_CONNECT            = 0x01,
    LE_AUDIO_DISCONNECT         = 0x02,
    LE_AUDIO_CONFIG_CODEC       = 0x03,
    LE_AUDIO_CONFIG_QOS         = 0x04,
    LE_AUDIO_ENABLE             = 0x05,
    LE_AUDIO_SETUP_DATA_PATH    = 0x06,
    LE_AUDIO_REMOVE_DATA_PATH   = 0x07,
    LE_AUDIO_STREAMING          = 0x08,
    LE_AUDIO_PAUSE              = 0x09,
    LE_AUDIO_A2DP_START         = 0x0A,
    LE_AUDIO_A2DP_SUSPEND       = 0x0B,
    LE_AUDIO_AVRCP_PLAYING      = 0x0C,
    LE_AUDIO_AVRCP_PAUSE        = 0x0D,
    LE_AUDIO_MCP_STATE          = 0x0E,
    LE_AUDIO_HFP_CALL_STATE     = 0x0F,
    LE_AUDIO_CCP_CALL_STATE     = 0x10,
    LE_AUDIO_CCP_READ_RESULT    = 0x11,
    LE_AUDIO_CCP_TERM_NOTIFY    = 0x12,
    LE_AUDIO_SNIFFING_STOP      = 0x13,
    LE_AUDIO_VCS_VOL_CHANGE     = 0x14,

    LE_AUDIO_LINK_EVENT_TOTAL,
} T_LE_AUDIO_LINK_EVENT;

typedef enum t_bt_ccp_call_status
{
    BT_CCP_CALL_IDLE                              = 0x00,
    BT_CCP_VOICE_ACTIVATION_ONGOING               = 0x01,
    BT_CCP_INCOMING_CALL_ONGOING                  = 0x02,
    BT_CCP_OUTGOING_CALL_ONGOING                  = 0x03,
    BT_CCP_CALL_ACTIVE                            = 0x04,
    BT_CCP_CALL_ACTIVE_WITH_CALL_WAITING          = 0x05,
    BT_CCP_CALL_ACTIVE_WITH_CALL_HOLD             = 0x06,
    BT_CCP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT   = 0x07,
    BT_CCP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD   = 0x08,
} T_BT_CCP_CALL_STATUS;

typedef enum
{
    LEA_REMOTE_MMI_SWITCH_SYNC     = 0x01,

    LEA_REMOTE_MSG_TOTAL
} T_LEA_REMOTE_MSG;

typedef enum
{
    T_LEA_DEV_CRL_SET_IDLE = 0,
    T_LEA_DEV_CRL_DISCOVERABLE = 1,
    T_LEA_DEV_CRL_GET_LEGACY = 2,
    T_LEA_DEV_CRL_SET_RADIO = 3,
    T_LEA_DEV_CRL_GET_CIS_POLICY = 4,
} T_LEA_BUD_DEV_CRL;

typedef enum
{
    T_LEA_CIS_AUTO = 0,
    T_LEA_CIS_MMI = 1,
} T_LEA_CIS_LINK_POLICY;

typedef enum
{
    BIS_POLICY_RANDOM             = 0x00,
    BIS_POLICY_SPECIFIC           = 0x01,
    //BIS_POLICY_RANDOM_AND_LAST  = 0x02,  wait implement

} T_BIS_POLICY;

typedef enum
{
    T_LEA_BROADCAST_NONE = 0,
    T_LEA_BROADCAST_DELEGATOR = 1,
    T_LEA_BROADCAST_SINK = 2,
} T_LEA_BROADCAST_ROLE;

typedef enum
{
    CIS_PROFILE_MCP                 = 0x01,
    CIS_PROFILE_CCP                  = 0x02,
    CIS_PROFILE_CSIS                 = 0x04,
} T_CIS_PROFILE;

typedef enum
{
    LEA_LINKBACK_POLICY_SAME             = 0x00,
    LEA_LINKBACK_POLICY_CIS_H             = 0x01,
    LEA_LINKBACK_POLICY_LEGACY_H     = 0x02,
} T_LEA_LINKBACK_POLICY;

typedef enum
{
    LEA_ACTIVE_PRIO_CIS_H                      = 0x00,
    LEA_ACTIVE_PRIO_LEGACY_H              = 0x01,
    LEA_ACTIVE_PRIO_CIS_ONLY               = 0x02,
    LEA_ACTIVE_PRIO_LEGACU_ONLY      = 0x03,
    LEA_ACTIVE_PRIO_LASTONE                = 0x04,
} T_LEA_ACTIVE_PRIO;

typedef enum
{
    LEA_BIS_TO_POWEROFF_DIS                = 0x00,
    LEA_BIS_TO_POWEROFF_EN                 = 0x01
} T_LEA_BIS_TO_POWEROFF;

typedef enum
{
    LEA_CIS_TO_POWEROFF_DIS                = 0x00,
    LEA_CIS_TO_POWEROFF_EN                 = 0x01
} T_LEA_CIS_TO_POWEROFF;

typedef enum
{
    LEA_BIS_STOP_ACTIVE_CIS_H                       = 0x00,
    LEA_BIS_STOP_ACTIVE_LEGACY_H              = 0x01,
    LEA_BIS_STOP_ACTIVE_CIS_ONLY               = 0x02,
    LEA_BIS_STOP_ACTIVE_LEGACY_ONLY       = 0x03,
    LEA_BIS_STOP_ACTIVE_LASTONE                = 0x04,
    LEA_BIS_STOP_ACTIVE_NONE            = 0x05,
} T_LEA_BIS_STOP_ACTIVE;

typedef enum
{
    LEA_A2DP_INTERRUPT_DIS                           = 0x00,
    LEA_A2DP_INTERRUPT_ALLOW                = 0x01,
} T_LEA_A2DP_INTERRUPT;

typedef enum
{
    LEA_KEEP_CUR_ACT_DIS                           = 0x00,
    LEA_KEEP_CUR_ACT_EN                 = 0x01,
} T_LEA_KEEP_CUR_ACT;


typedef enum
{

    LEA_MMI_AV_PAUSE = 0xF8,
    LEA_MMI_AV_RESUME = 0xF9,

    LEA_MMI_TOTAL
} T_LEA_MMI_ACTION;

bool app_le_audio_dev_ctrl(uint8_t para, uint8_t *data);

void app_le_audio_init(void);
void app_le_audio_device_sm(uint8_t event, void *p_data);
T_LE_AUDIO_DEVICE_SM app_lea_audio_get_device_state(void);
void app_le_audio_link_sm(uint16_t conn_handle, uint8_t event, void *p_data);
void app_lea_direct_cback(uint8_t cb_type, void *p_cb_data);
void app_le_audio_track_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len);
bool app_le_audio_switch_remote_sync(void);
T_BT_CCP_CALL_STATUS app_le_audio_get_call_status(void);

void app_le_audio_mmi_handle_action(uint8_t action);


//bis
void app_big_handle_switch(uint8_t event, void *p_data);
bool app_le_audio_bis_cb_allocate(uint8_t bis_idx, T_CODEC_CFG *bis_codec_cfg);
T_BIS_CB *app_le_audio_find_bis_by_conn_handle(uint16_t conn_handle);
T_BIS_CB *app_le_audio_find_bis_by_bis_idx(uint8_t bis_idx);
T_BIS_CB *app_le_audio_find_bis_by_source_id(uint8_t source_id);
void app_le_audio_track_create_for_bis(uint16_t bis_conn_handle, uint8_t source_id,
                                       uint8_t bis_idx);
void app_le_audio_track_release_for_bis(uint16_t bis_conn_handle);
bool app_le_audio_free_bis(T_BIS_CB *p_bis_cb);
void app_lea_check_bis_cb(uint8_t *index);
void app_le_audio_bis_state_machine(uint8_t event, void *p_data);

void app_le_audio_a2dp_pause(void);
void app_lea_bis_state_change(T_LE_AUDIO_BIS_SM state);
T_LE_AUDIO_BIS_SM app_lea_bis_get_state(void);
void app_lea_bis_data_path_reset(T_BIS_CB *p_bis_cb);


void app_lea_trigger_mmi_handle_action(uint8_t action, bool inter);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
