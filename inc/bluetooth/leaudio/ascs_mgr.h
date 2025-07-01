#ifndef _ASCS_MGR_H_
#define _ASCS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ase_def.h"
#include "ase_cp_def.h"
#include "codec_def.h"

#if LE_AUDIO_ASCS_SUPPORT
typedef struct
{
    uint8_t ase_id;
    uint8_t metadata_length;
    uint8_t *p_metadata;
} T_ASE_CP_METADATA_DATA;

typedef struct
{
    T_ASE_CP_CODEC_CFG_ARRAY_PARAM data;
    uint8_t *p_codec_spec_cfg;
    T_CODEC_CFG codec_parsed_data;
} T_ASE_CP_CONFIG_CODEC_DATA;

//LE_AUDIO_MSG_ASCS_ASE_STATE
typedef struct
{
    uint16_t conn_handle;
    T_ASE_CHAR_DATA ase_data;
} T_ASCS_ASE_STATE;

//LE_AUDIO_MSG_ASCS_SETUP_DATA_PATH
typedef struct
{
    uint16_t conn_handle;
    uint8_t ase_id;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
    T_CODEC_CFG codec_parsed_data;
} T_ASCS_SETUP_DATA_PATH;

//LE_AUDIO_MSG_ASCS_REMOVE_DATA_PATH
typedef struct
{
    uint16_t conn_handle;
    uint8_t ase_id;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
} T_ASCS_REMOVE_DATA_PATH;

//LE_AUDIO_MSG_ASCS_CP_CONFIG_CODEC
typedef struct
{
    uint16_t conn_handle;
    uint8_t number_of_ase;
    T_ASE_CP_CONFIG_CODEC_DATA param[ASCS_AES_CHAR_MAX_NUM];
} T_ASCS_CP_CONFIG_CODEC;

//LE_AUDIO_MSG_ASCS_CP_CONFIG_QOS
typedef struct
{
    uint16_t conn_handle;
    uint8_t number_of_ase;
    T_ASE_CP_QOS_CFG_ARRAY_PARAM param[ASCS_AES_CHAR_MAX_NUM];
} T_ASCS_CP_CONFIG_QOS;

//LE_AUDIO_MSG_ASCS_CP_ENABLE
typedef struct
{
    uint16_t conn_handle;
    uint8_t number_of_ase;
    T_ASE_CP_METADATA_DATA param[ASCS_AES_CHAR_MAX_NUM];
} T_ASCS_CP_ENABLE;

//LE_AUDIO_MSG_ASCS_CP_DISABLE
typedef struct
{
    uint16_t conn_handle;
    uint8_t number_of_ase;
    uint8_t ase_id[ASCS_AES_CHAR_MAX_NUM];
} T_ASCS_CP_DISABLE;

//LE_AUDIO_MSG_ASCS_CP_UPDATE_METADATA
typedef struct
{
    uint16_t conn_handle;
    uint8_t number_of_ase;
    T_ASE_CP_METADATA_DATA param[ASCS_AES_CHAR_MAX_NUM];
} T_ASCS_CP_UPDATE_METADATA;

//LE_AUDIO_MSG_ASCS_PREFER_QOS
typedef struct
{
    uint16_t             conn_handle;
    uint8_t              ase_id;
    uint8_t              direction;
    T_CODEC_CFG          codec_cfg;
    T_ASE_TARGET_LATENCY target_latency;
    T_ASE_TARGET_PHY     target_phy;
} T_ASCS_PREFER_QOS;

//LE_AUDIO_MSG_ASCS_CIS_REQUEST_IND
typedef struct
{
    uint16_t conn_handle;
    uint16_t cis_conn_handle;
    uint8_t  snk_ase_id;
    uint8_t  snk_ase_state;
    uint8_t  src_ase_id;
    uint8_t  src_ase_state;
} T_ASCS_CIS_REQUEST_IND;

typedef struct
{
    uint8_t ase_id;
    uint8_t ase_state;
} T_ASE_INFO;

typedef struct
{
    uint8_t snk_ase_num;
    uint8_t src_ase_num;
    T_ASE_INFO snk_info[ASCS_AES_CHAR_MAX_NUM];
    T_ASE_INFO src_info[ASCS_AES_CHAR_MAX_NUM];
} T_ASCS_INFO;

typedef struct
{
    uint8_t  supported_framing;
    uint8_t  preferred_phy;
    uint8_t  preferred_retrans_number;
    uint16_t max_transport_latency;
    uint32_t presentation_delay_min;
    uint32_t presentation_delay_max;
    uint32_t preferred_presentation_delay_min;
    uint32_t preferred_presentation_delay_max;
} T_ASCS_PREFER_QOS_DATA;

void ascs_init(uint8_t snk_ase_num, uint8_t src_ase_num);
bool ascs_get_ase_data(uint16_t conn_handle, uint8_t ase_id, T_ASE_CHAR_DATA *p_ase_data);
bool ascs_get_codec_cfg(uint16_t conn_handle, uint8_t ase_id, T_ASE_DATA_CODEC_CONFIGURED *p_cfg);
bool ascs_get_qos_cfg(uint16_t conn_handle, uint8_t ase_id, T_ASE_QOS_CFG_STATE_DATA *p_cfg);
bool ascs_get_ase_info(uint16_t conn_handle, T_ASCS_INFO *p_info);
bool ascs_set_ase_prefer_qos(uint16_t conn_handle, uint8_t ase_id, T_ASCS_PREFER_QOS_DATA *p_qos);

bool ascs_action_cfg_codec(uint16_t conn_handle, uint8_t ase_id,
                           T_ASE_CP_CODEC_CFG_ARRAY_PARAM *p_param, uint8_t *p_codec_data);
bool ascs_action_disable(uint16_t conn_handle, uint8_t ase_id);
bool ascs_action_updata_metadata(uint16_t conn_handle, uint8_t ase_id,
                                 uint8_t metadata_len, uint8_t *p_metadata);
bool ascs_action_release(uint16_t conn_handle, uint8_t ase_id);
bool ascs_action_release_by_cig(uint16_t conn_handle, uint8_t cig_id);

bool ascs_cis_request_cfm(uint16_t cis_conn_handle, bool accept, uint8_t reject_reason);

//Only use when the server is audio sink
bool ascs_app_ctl_handshake(uint16_t conn_handle, uint8_t ase_id, bool app_handshake);
bool ascs_action_rec_start_ready(uint16_t conn_handle, uint8_t ase_id);
bool ascs_action_rec_stop_ready(uint16_t conn_handle, uint8_t ase_id);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
