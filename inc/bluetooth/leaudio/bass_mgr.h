#ifndef _BASS_MGR_H_
#define _BASS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if LE_AUDIO_BASS_SUPPORT
#include "ble_audio_sync.h"
#include "bass_def.h"

//LE_AUDIO_MSG_BASS_REMOTE_CP
typedef struct
{
    uint16_t conn_handle;
    T_BASS_CP_DATA *p_cp_data;
} T_BASS_REMOTE_CP;

//LE_AUDIO_MSG_BASS_BRS_MODIFY
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t source_id;
    T_BASS_BRS_DATA *p_brs_data;
} T_BASS_BRS_MODIFY;

//LE_AUDIO_MSG_BASS_BA_ADD_SOURCE
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t source_id;
} T_BASS_BA_ADD_SOURCE;

//LE_AUDIO_MSG_BASS_LOCAL_ADD_SOURCE
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t source_id;
} T_BASS_BASS_LOCAL_ADD_SOURCE;

//LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t  source_id;
    bool     is_past;
    uint16_t pa_interval;
} T_BASS_GET_PA_SYNC_PARAM;

//LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t  source_id;
} T_BASS_GET_BIG_SYNC_PARAM;

//LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t  source_id;
} T_BASS_GET_BROADCAST_CODE;

//LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC
typedef struct
{
    T_BLE_AUDIO_SYNC_HANDLE handle;
    uint8_t  source_id;
    uint8_t  num_subgroups;
    T_BASS_CP_BIS_INFO  *p_cp_bis_info;
} T_BASS_SET_PREFER_BIS_SYNC;

//LE_AUDIO_MSG_BASS_BRS_CHAR_NO_EMPTY
//APP and call ble_audio_sync_realese to delete brs data
typedef struct
{
    uint16_t conn_handle;
    T_BASS_CP_DATA *p_cp_data; //Control point: add source operation
} T_BASS_BRS_CHAR_NO_EMPTY;

typedef struct
{
    uint8_t metadata_len;
    uint8_t *p_metadata;
} T_BASS_METADATA_INFO;

//used when receive LE_AUDIO_MSG_BASS_GET_PA_SYNC_PARAM, app can call the API.
bool bass_set_pa_sync_param(uint8_t source_id, uint8_t pa_sync_options, uint16_t pa_sync_skip,
                            uint16_t pa_sync_timeout_10ms, uint16_t past_timeout_10ms);

//used when receive LE_AUDIO_MSG_BASS_GET_BIG_SYNC_PARAM, app can call the API.
bool bass_set_big_sync_param(uint8_t source_id, uint8_t big_mse, uint16_t big_sync_timeout_10ms);

//used when receive LE_AUDIO_MSG_BASS_GET_BROADCAST_CODE, app can call the API.
bool bass_set_broadcast_code(uint8_t source_id, uint8_t broadcast_code[BROADCAST_CODE_LEN]);

//used when receive LE_AUDIO_MSG_BASS_GET_PREFER_BIS_SYNC, app shall call the API.
bool bass_set_prefer_bis_sync(uint8_t source_id, uint8_t subgroups_idx, uint32_t bis_sync);

bool bass_send_broadcast_code_required(uint8_t source_id);

bool bass_get_brs_char_data(uint8_t source_id, T_BASS_BRS_DATA *p_data,
                            T_BLE_AUDIO_SYNC_HANDLE *p_handle);

bool bass_update_metadata(uint8_t source_id, uint8_t num_subgroups,
                          T_BASS_METADATA_INFO *p_metadata_tbl);


#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
