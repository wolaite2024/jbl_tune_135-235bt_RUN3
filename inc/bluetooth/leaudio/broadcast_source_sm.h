#ifndef _BROADCAST_SOURCE_SM_H_
#define _BROADCAST_SOURCE_SM_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if LE_AUDIO_BROADCAST_SOURCE_ROLE
#include "gap.h"
#include "gap_pa_adv.h"
#if LE_AUDIO_USE_BLE_EXT_ADV
#include "ble_ext_adv.h"
#else
#include "gap_ext_adv.h"
#endif
#include "gap_big_mgr.h"
#include "ble_audio_def.h"

typedef void *T_BROADCAST_SOURCE_HANDLE;

#define MSG_BROADCAST_SOURCE_STATE_CHANGE         0x01
#define MSG_BROADCAST_SOURCE_METAUPDATE           0x02
#define MSG_BROADCAST_SOURCE_RECONFIG             0x03
#define MSG_BROADCAST_SOURCE_SETUP_DATA_PATH      0x04
#define MSG_BROADCAST_SOURCE_REMOVE_DATA_PATH     0x05
#define MSG_BROADCAST_SOURCE_BIG_SYNC_MODE        0x06


typedef enum
{
    BROADCAST_SOURCE_STATE_IDLE                = 0x00,
    BROADCAST_SOURCE_STATE_CONFIGURED_STARTING = 0x01,
    BROADCAST_SOURCE_STATE_CONFIGURED          = 0x02,
    BROADCAST_SOURCE_STATE_CONFIGURED_STOPPING = 0x03,
    BROADCAST_SOURCE_STATE_STREAMING_STARTING  = 0x04,
    BROADCAST_SOURCE_STATE_STREAMING           = 0x05,
    BROADCAST_SOURCE_STATE_STREAMING_STOPPING  = 0x06,
} T_BROADCAST_SOURCE_STATE;

typedef struct
{
    T_BROADCAST_SOURCE_STATE state;
    uint16_t cause;
} T_BROADCAST_SOURCE_STATE_CHANGE;

typedef struct
{
    bool big_sync_mode;
    uint16_t cause;
} T_BROADCAST_SOURCE_BIG_SYNC_MODE;

typedef struct
{
    uint8_t bis_idx;
    uint16_t bis_conn_handle;
    uint16_t cause;
} T_BROADCAST_SOURCE_SETUP_DATA_PATH;

typedef struct
{
    uint16_t bis_conn_handle;
    uint16_t cause;
} T_BROADCAST_SOURCE_REMOVE_DATA_PATH;

typedef union
{
    uint16_t cause;
    T_BROADCAST_SOURCE_STATE_CHANGE *p_state_change;
    T_BROADCAST_SOURCE_SETUP_DATA_PATH *p_setup_data_path;
    T_BROADCAST_SOURCE_REMOVE_DATA_PATH *p_remove_data_path;
    T_BROADCAST_SOURCE_BIG_SYNC_MODE *p_big_sync_mode;
} T_BROADCAST_SOURCE_SM_CB_DATA;

typedef struct
{
    T_BROADCAST_SOURCE_STATE state;
    uint8_t adv_sid;
    uint8_t broadcast_id[BROADCAST_ID_LEN];
    T_GAP_EXT_ADV_STATE ext_adv_state;
    T_GAP_PA_ADV_STATE pa_state;
    T_GAP_BIG_ISOC_BROADCAST_STATE big_state;
    uint8_t adv_handle;
    uint8_t big_handle;
} T_BROADCAST_SOURCE_INFO;


typedef T_APP_RESULT(*P_FUN_BROADCAST_SOURCE_SM_CB)(T_BROADCAST_SOURCE_HANDLE handle,
                                                    uint8_t cb_type,
                                                    void *p_cb_data);

T_BROADCAST_SOURCE_HANDLE broadcast_source_add(P_FUN_BROADCAST_SOURCE_SM_CB cb_pfn);
T_BROADCAST_SOURCE_HANDLE broadcast_source_find(uint8_t adv_sid,
                                                uint8_t broadcast_id[BROADCAST_ID_LEN]);
bool broadcast_source_set_eadv_param(T_BROADCAST_SOURCE_HANDLE handle,
                                     uint32_t primary_adv_interval_min, uint32_t primary_adv_interval_max,
                                     uint8_t primary_adv_channel_map,
                                     T_GAP_LOCAL_ADDR_TYPE own_address_type, uint8_t *p_local_rand_addr,
                                     T_GAP_ADV_FILTER_POLICY filter_policy, uint8_t tx_power,
                                     T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy, uint8_t secondary_adv_max_skip,
                                     T_GAP_PHYS_TYPE secondary_adv_phy,
                                     uint8_t broadcast_data_len, uint8_t *p_broadcast_data);
bool broadcast_source_set_pa_param(T_BROADCAST_SOURCE_HANDLE handle,
                                   uint16_t periodic_adv_interval_min,
                                   uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                                   uint8_t basic_data_len, uint8_t *p_basic_data);
bool broadcast_source_get_info(T_BROADCAST_SOURCE_HANDLE handle, T_BROADCAST_SOURCE_INFO *p_info);
T_BIG_MGR_BIS_CONN_HANDLE_INFO *broadcast_source_get_bis_info(T_BROADCAST_SOURCE_HANDLE handle,
                                                              uint8_t *p_bis_num);
bool broadcast_source_get_bis_conn_handle(T_BROADCAST_SOURCE_HANDLE handle, uint8_t bis_idx,
                                          uint16_t *p_bis_conn_handle);

uint8_t broadcast_source_gen_ea_broadcast_data(T_BROADCAST_SOURCE_HANDLE handle, uint8_t *p_data);
//action
bool broadcast_source_config(T_BROADCAST_SOURCE_HANDLE handle);
bool broadcast_source_reconfig(T_BROADCAST_SOURCE_HANDLE handle, uint8_t basic_data_len,
                               uint8_t *p_basic_data);
bool broadcast_source_metaupdate(T_BROADCAST_SOURCE_HANDLE handle, uint8_t basic_data_len,
                                 uint8_t *p_basic_data);
bool broadcast_source_establish(T_BROADCAST_SOURCE_HANDLE handle,
                                T_BIG_MGR_ISOC_BROADCASTER_CREATE_BIG_PARAM create_big_param);
bool broadcast_source_disable(T_BROADCAST_SOURCE_HANDLE handle, uint8_t reason);
bool broadcast_source_release(T_BROADCAST_SOURCE_HANDLE handle);
bool broadcast_source_big_sync_mode(T_BROADCAST_SOURCE_HANDLE handle, bool big_sync_mode);

bool broadcast_source_setup_data_path(T_BROADCAST_SOURCE_HANDLE handle, uint8_t bis_idx,
                                      uint8_t codec_id[5], uint32_t controller_delay,
                                      uint8_t codec_len, uint8_t *p_codec_data);
bool broadcast_source_remove_data_path(T_BROADCAST_SOURCE_HANDLE handle, uint8_t bis_idx);


#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
