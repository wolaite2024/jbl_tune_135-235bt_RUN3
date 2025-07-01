#ifndef _ASCS_CLIENT_H_
#define _ASCS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ascs_def.h"
#include "profile_client.h"
#include "ase_def.h"
#include "ase_cp_def.h"
#include "codec_def.h"

#if LE_AUDIO_ASCS_CLIENT_SUPPORT
//LE_AUDIO_MSG_ASCS_CLIENT_DIS_DONE
typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t snk_ase_num;
    uint8_t src_ase_num;
} T_ASCS_CLIENT_DIS_DONE;

//LE_AUDIO_MSG_ASCS_CLIENT_CCCD
typedef struct
{
    uint16_t conn_handle;
    uint16_t cause;
} T_ASCS_CLIENT_CCCD;

//LE_AUDIO_MSG_ASCS_CLIENT_CP_NOTIFY
typedef struct
{
    uint16_t conn_handle;
    uint8_t opcode;
    uint8_t number_of_ase;
    T_ASE_CP_NOTIFY_ARRAY_PARAM *p_param;
} T_ASCS_CLIENT_CP_NOTIFY;

//LE_AUDIO_MSG_ASCS_CLIENT_READ_ASE_DATA
//LE_AUDIO_MSG_ASCS_CLIENT_ASE_DATA_NOTIFY
typedef struct
{
    uint16_t conn_handle;
    uint8_t instance_id;
    bool notify;
    uint16_t cause;
    uint8_t direction;
    T_ASE_CHAR_DATA ase_data;
} T_ASCS_CLIENT_ASE_DATA;

//LE_AUDIO_MSG_ASCS_CLIENT_READ_ASE_ALL
typedef struct
{
    uint16_t conn_handle;
    uint8_t snk_ase_num;
    uint8_t src_ase_num;
    uint16_t cause;
} T_ASCS_CLIENT_READ_ASE_ALL;

//LE_AUDIO_MSG_ASCS_CLIENT_SETUP_DATA_PATH
typedef struct
{
    uint16_t conn_handle;
    uint8_t ase_id;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
    T_CODEC_CFG codec_parsed_data;
} T_ASCS_CLIENT_SETUP_DATA_PATH;

//LE_AUDIO_MSG_ASCS_CLIENT_REMOVE_DATA_PATH
typedef struct
{
    uint16_t conn_handle;
    uint8_t ase_id;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
    uint16_t cause;
} T_ASCS_CLIENT_REMOVE_DATA_PATH;

typedef struct
{
    uint8_t ase_id;
    uint8_t target_latency;
    uint8_t target_phy;
    uint8_t codec_id[5];
    uint8_t codec_spec_cfg_len;
    uint8_t codec_spec_cfg[CODEC_DATA_MAX_LEN];
} T_ASE_CP_CODEC_CFG_PARAM;

//enable, update metadata
typedef struct
{
    uint8_t ase_id;
    uint8_t metadata_len;
    uint8_t *p_data;
} T_ASE_CP_METADATA_PARAM;

bool ascs_client_init(void);
bool ascs_client_enable_cccd(uint16_t conn_handle);
bool ascs_client_read_ase(uint16_t conn_handle, uint8_t instance_id, uint8_t direction);
bool ascs_client_read_ase_all(uint16_t conn_handle);
bool ascs_client_get_ase_data(uint16_t conn_handle, uint8_t instance_id,
                              T_ASE_CHAR_DATA *p_ase_data,
                              uint8_t direction);
bool ascs_client_get_ase_data_by_id(uint16_t conn_handle, uint8_t ase_id,
                                    T_ASE_CHAR_DATA *p_ase_data);
bool ascs_client_get_codec_data(uint16_t conn_handle, uint8_t ase_id,
                                T_ASE_DATA_CODEC_CONFIGURED *p_codec_data);
bool ascs_client_get_metadata(uint16_t conn_handle, uint8_t ase_id,
                              T_ASE_DATA_WITH_METADATA *p_metadata);

bool ase_cp_config_codec(uint16_t conn_handle, uint8_t num_ases,
                         T_ASE_CP_CODEC_CFG_PARAM *p_codec);
bool ase_cp_config_qos(uint16_t conn_handle, uint8_t num_ases,
                       T_ASE_CP_QOS_CFG_ARRAY_PARAM *p_qos);
//cig range: 1-0xEF
//cis range: 1-0xEF

bool ase_cp_enable(uint16_t conn_handle, uint8_t num_ases,
                   T_ASE_CP_METADATA_PARAM *p_metadata);
bool ase_cp_update_metadata(uint16_t conn_handle, uint8_t num_ases,
                            T_ASE_CP_METADATA_PARAM *p_metadata);
bool ase_cp_disable(uint16_t conn_handle, uint8_t num_ases, uint8_t *p_ase_id);
bool ase_cp_release(uint16_t conn_handle, uint8_t num_ases, uint8_t *p_ase_id);

//Only use when the server is audio source
bool ascs_client_app_ctl_handshake(uint16_t conn_handle, uint8_t ase_id, bool app_handshake);
bool ase_cp_rec_start_ready(uint16_t conn_handle, uint8_t ase_id);
bool ase_cp_rec_stop_ready(uint16_t conn_handle, uint8_t ase_id);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
