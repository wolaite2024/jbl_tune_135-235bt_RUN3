#ifndef _BAP_UNICAST_CLIENT_H_
#define _BAP_UNICAST_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "audio_stream_session.h"
#include "codec_qos.h"
#include "ase_def.h"
#include "ase_cp_def.h"
#include "gap_msg.h"

#define BAP_UNICAST_SERVERS_MAX_NUM 2
#define AUDIO_STREAM_SRV_ASE_MAX 4

typedef enum
{
    UNICAST_AUDIO_CFG_UNKNOWN,
    UNICAST_AUDIO_CFG_1,
    UNICAST_AUDIO_CFG_2,
    UNICAST_AUDIO_CFG_3,
    UNICAST_AUDIO_CFG_4,
    UNICAST_AUDIO_CFG_5,
    UNICAST_AUDIO_CFG_6_I,
    UNICAST_AUDIO_CFG_6_II,
    UNICAST_AUDIO_CFG_7_I,
    UNICAST_AUDIO_CFG_7_II,
    UNICAST_AUDIO_CFG_8_I,
    UNICAST_AUDIO_CFG_8_II,
    UNICAST_AUDIO_CFG_9_I,
    UNICAST_AUDIO_CFG_9_II,
    UNICAST_AUDIO_CFG_10,
    UNICAST_AUDIO_CFG_11_I,
    UNICAST_AUDIO_CFG_11_II,
    UNICAST_AUDIO_CFG_MAX
} T_UNICAST_AUDIO_CFG_TYPE;

#define UNICAST_AUDIO_CFG_1_BIT     (1 << UNICAST_AUDIO_CFG_1)
#define UNICAST_AUDIO_CFG_2_BIT     (1 << UNICAST_AUDIO_CFG_2)
#define UNICAST_AUDIO_CFG_3_BIT     (1 << UNICAST_AUDIO_CFG_3)
#define UNICAST_AUDIO_CFG_4_BIT     (1 << UNICAST_AUDIO_CFG_4)
#define UNICAST_AUDIO_CFG_5_BIT     (1 << UNICAST_AUDIO_CFG_5)
#define UNICAST_AUDIO_CFG_6_I_BIT   (1 << UNICAST_AUDIO_CFG_6_I)
#define UNICAST_AUDIO_CFG_6_II_BIT  (1 << UNICAST_AUDIO_CFG_6_II)
#define UNICAST_AUDIO_CFG_7_I_BIT   (1 << UNICAST_AUDIO_CFG_7_I)
#define UNICAST_AUDIO_CFG_7_II_BIT  (1 << UNICAST_AUDIO_CFG_7_II)
#define UNICAST_AUDIO_CFG_8_I_BIT   (1 << UNICAST_AUDIO_CFG_8_I)
#define UNICAST_AUDIO_CFG_8_II_BIT  (1 << UNICAST_AUDIO_CFG_8_II)
#define UNICAST_AUDIO_CFG_9_I_BIT   (1 << UNICAST_AUDIO_CFG_9_I)
#define UNICAST_AUDIO_CFG_9_II_BIT  (1 << UNICAST_AUDIO_CFG_9_II)
#define UNICAST_AUDIO_CFG_10_BIT    (1 << UNICAST_AUDIO_CFG_10)
#define UNICAST_AUDIO_CFG_11_I_BIT  (1 << UNICAST_AUDIO_CFG_11_I)
#define UNICAST_AUDIO_CFG_11_II_BIT (1 << UNICAST_AUDIO_CFG_11_II)
#define UNICAST_AUDIO_CFG_MASK      0x1FFFE

#define AUDIO_STREAM_SRV_CIS_MAX 2

typedef enum
{
    AUDIO_STREAM_STATE_IDLE               = 0x00,      //Available API: bap_unicast_audio_cfg
    AUDIO_STREAM_STATE_IDLE_CONFIGURED    = 0x01,//Available API: bap_unicast_audio_start, bap_unicast_audio_remove_cfg
    AUDIO_STREAM_STATE_CONFIGURED         = 0x02,//Available API: bap_unicast_audio_start, bap_unicast_audio_release
    AUDIO_STREAM_STATE_STARTING           = 0x03,  //Available API: bap_unicast_audio_stop, bap_unicast_audio_release
    AUDIO_STREAM_STATE_STREAMING          = 0x04, //Available API: bap_unicast_audio_stop, bap_unicast_audio_release, bap_unicast_audio_update
    AUDIO_STREAM_STATE_PARTIAL_STREAMING  = 0x05, //Available API: bap_unicast_audio_stop, bap_unicast_audio_release, bap_unicast_audio_update
    AUDIO_STREAM_STATE_STOPPING           = 0x06,  //Available API: bap_unicast_audio_release
    AUDIO_STREAM_STATE_RELEASING          = 0x07,
} T_AUDIO_STREAM_STATE;

typedef enum
{
    BAP_UNICAST_ACTION_IDLE             = 0x00,
    BAP_UNICAST_ACTION_START            = 0x01,
    BAP_UNICAST_ACTION_STOP             = 0x02,
    BAP_UNICAST_ACTION_RELEASE          = 0x03,
    BAP_UNICAST_ACTION_UPDATE           = 0x04,
    BAP_UNICAST_ACTION_REMOVE_CFG       = 0x05,
    BAP_UNICAST_ACTION_REMOVE_SESSION   = 0x06,
    BAP_UNICAST_ACTION_SERVER_STOP      = 0x07,
    BAP_UNICAST_ACTION_SERVER_RELEASE   = 0x08,
} T_BAP_UNICAST_ACTION;

typedef struct
{
    uint8_t     cis_id;
    uint16_t    cis_conn_handle;
    uint8_t     data_path_flags;
    uint8_t     sink_ase_id;
    T_ASE_STATE sink_ase_state;
    uint8_t     source_ase_id;
    T_ASE_STATE source_ase_state;
} T_AUDIO_CIS_INFO;

typedef struct
{
    uint8_t                cis_num;
    T_AUDIO_CIS_INFO       cis_info[AUDIO_STREAM_SRV_CIS_MAX];
} T_AUDIO_STREAM_CIS_INFO;

typedef struct
{
    uint8_t ase_id;
    uint8_t cfg_idx;
    T_ASE_STATE ase_state;
    uint8_t direction;
    uint8_t channel_num;
} T_BAP_UNICAST_ASE_INFO;

typedef struct
{
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    T_GAP_CONN_STATE       conn_state;
    uint8_t                ase_num;
    T_BAP_UNICAST_ASE_INFO ase_info[AUDIO_STREAM_SRV_ASE_MAX];
} T_BAP_UNICAST_DEV_INFO;

typedef struct
{
    T_AUDIO_STREAM_STATE          state;
    T_UNICAST_AUDIO_CFG_TYPE      cfg_type;
    uint8_t                       conn_dev_num;
    uint8_t                       dev_num;
    T_BAP_UNICAST_DEV_INFO        dev_info[BAP_UNICAST_SERVERS_MAX_NUM];
} T_BAP_UNICAST_SESSION_INFO;

typedef struct
{
    uint8_t              codec_id[CODEC_ID_LEN];
    T_CODEC_CFG          codec_cfg;
    T_ASE_TARGET_LATENCY target_latency;
    T_ASE_TARGET_PHY     target_phy;
} T_AUDIO_ASE_CODEC_CFG;

typedef struct
{
    uint8_t  phy;
    uint8_t  retransmission_number;
    uint16_t max_sdu;
} T_AUDIO_ASE_QOS_CFG;

typedef struct
{
    uint8_t  sca;
    uint8_t  packing;
    uint8_t  framing;
    uint32_t sdu_interval_m_s;/*3bytes*/
    uint32_t sdu_interval_s_m;/*3bytes*/
    uint16_t latency_m_s;
    uint16_t latency_s_m;
    uint32_t sink_presentation_delay;
    uint32_t source_presentation_delay;
} T_AUDIO_SESSION_QOS_CFG;

//group msg
typedef enum
{
    BAP_UNICAST_RESULT_SUCCESS,
    BAP_UNICAST_RESULT_FAILED,
    BAP_UNICAST_RESULT_CIG_SET_ERR,
    BAP_UNICAST_RESULT_CIS_ERR,
    BAP_UNICAST_RESULT_ASE_CP_ERR,
    BAP_UNICAST_RESULT_DATA_PATH_ERR,
    BAP_UNICAST_RESULT_ASE_TIMEOUT,
    BAP_UNICAST_RESULT_ACL_DISCONN,
} T_BAP_UNICAST_RESULT;

//AUDIO_GROUP_MSG_BAP_STATE
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    T_BAP_UNICAST_ACTION curr_action;
    T_AUDIO_STREAM_STATE state;
    T_BAP_UNICAST_RESULT result;
    uint16_t             cause;
} T_AUDIO_GROUP_BAP_STATE;

//AUDIO_GROUP_MSG_BAP_SESSION_REMOVE
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
} T_AUDIO_GROUP_BAP_SESSION_REMOVE;

//AUDIO_GROUP_MSG_BAP_START_QOS_CFG
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    uint32_t sink_presentation_delay_min;
    uint32_t sink_presentation_delay_max;
    uint32_t source_presentation_delay_min;
    uint32_t source_presentation_delay_max;
    uint16_t sink_transport_latency_max;
    uint16_t source_transport_latency_max;
} T_AUDIO_GROUP_BAP_START_QOS_CFG;

//AUDIO_GROUP_MSG_BAP_CREATE_CIS
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    uint8_t                       cig_id;
    uint16_t                      dev_num;
    uint16_t                      conn_handle_tbl[4];
} T_AUDIO_GROUP_BAP_CREATE_CIS;


//AUDIO_GROUP_MSG_BAP_START_METADATA_CFG
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    uint8_t ase_id;
} T_AUDIO_GROUP_BAP_START_METADATA_CFG;

//AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    uint8_t ase_id;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
    T_CODEC_CFG codec_parsed_data;
} T_AUDIO_GROUP_MSG_BAP_SETUP_DATA_PATH;

//AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    uint8_t ase_id;
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
    uint16_t cause;
} T_AUDIO_GROUP_MSG_BAP_REMOVE_DATA_PATH;

//AUDIO_GROUP_MSG_BAP_CIS_DISCONN
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    uint16_t                      cause;
    uint16_t                      cis_conn_handle;
    uint16_t                      conn_handle;
    uint8_t                       cig_id;
    uint8_t                       cis_id;
} T_AUDIO_GROUP_BAP_CIS_DISCONN;


//AUDIO_GROUP_MSG_BAP_METADATA_UPDATE
typedef struct
{
    T_AUDIO_STREAM_SESSION_HANDLE handle;
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    uint8_t ase_id;
    uint8_t metadata_length;
    uint8_t *p_metadata;
} T_AUDIO_GROUP_MSG_BAP_METADATA_UPDATE;

uint32_t bap_unicast_get_audio_cfg_mask(T_AUDIO_STREAM_SESSION_HANDLE handle, uint32_t prefer_cfg,
                                        uint8_t dev_num, T_BLE_AUDIO_DEV_HANDLE *p_dev_tbl);
bool bap_unicast_audio_get_cis_info(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                    T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                    T_AUDIO_STREAM_CIS_INFO *p_info);
bool bap_unicast_audio_get_session_info(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                        T_BAP_UNICAST_SESSION_INFO *p_info);

bool bap_unicast_audio_cfg(T_AUDIO_STREAM_SESSION_HANDLE handle, T_UNICAST_AUDIO_CFG_TYPE cfg_type,
                           uint8_t dev_num, T_BLE_AUDIO_DEV_HANDLE *p_dev_tbl);
// used before bap_unicast_audio_start, use after bap_unicast_audio_cfg
bool bap_unicast_audio_cfg_ase_codec(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                     T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                     uint8_t cfg_idx, T_AUDIO_ASE_CODEC_CFG *p_cfg);
// used when AUDIO_GROUP_MSG_BAP_START_QOS_CFG
bool bap_unicast_audio_cfg_session_qos(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                       T_AUDIO_SESSION_QOS_CFG *p_cfg);
bool bap_unicast_audio_cfg_ase_qos(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                   T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                   uint8_t ase_id, T_AUDIO_ASE_QOS_CFG *p_cfg);
bool bap_unicast_audio_get_session_qos(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                       T_AUDIO_SESSION_QOS_CFG *p_cfg);
bool bap_unicast_audio_get_ase_qos(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                   T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                   uint8_t ase_id, T_AUDIO_ASE_QOS_CFG *p_cfg);
// used when AUDIO_GROUP_MSG_BAP_START_METADATA_CFG
bool bap_unicast_audio_cfg_ase_metadata(T_AUDIO_STREAM_SESSION_HANDLE handle,
                                        T_BLE_AUDIO_DEV_HANDLE dev_handle,
                                        uint8_t ase_id, uint8_t metadata_len, uint8_t *p_metadata);

bool bap_unicast_audio_remove_cfg(T_AUDIO_STREAM_SESSION_HANDLE handle);
bool bap_unicast_audio_remove_session(T_AUDIO_STREAM_SESSION_HANDLE handle);

bool bap_unicast_audio_start(T_AUDIO_STREAM_SESSION_HANDLE handle);
bool bap_unicast_audio_stop(T_AUDIO_STREAM_SESSION_HANDLE handle, uint32_t cis_timeout_ms);
bool bap_unicast_audio_release(T_AUDIO_STREAM_SESSION_HANDLE handle);
bool bap_unicast_audio_update(T_AUDIO_STREAM_SESSION_HANDLE handle);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
