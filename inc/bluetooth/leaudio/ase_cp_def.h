#ifndef _ASE_CP_DEF_H_
#define _ASE_CP_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_def.h"

// ASE_Error_Status field of the applicable ASE characteristic
#define ASE_CP_REASON_NONE                    0x00
#define ASE_CP_REASON_CODEC_ID                0x01
#define ASE_CP_REASON_CODEC_CFG               0x02
#define ASE_CP_REASON_SDU_INTERVAL            0x03
#define ASE_CP_REASON_FRAMING                 0x04
#define ASE_CP_REASON_PHY                     0x05
#define ASE_CP_REASON_MAX_SDU_SIZE            0x06
#define ASE_CP_REASON_RETRANS_NUM             0x07
#define ASE_CP_REASON_MAX_TRANSPORT_LATENCY   0x08
#define ASE_CP_REASON_PRESENTATION_DELAY      0x09
#define ASE_CP_REASON_INVALID_ASE_CIS_MAPPING 0x0A


typedef enum
{
    ASE_CP_RESP_SUCCESS                   = 0x00,
    ASE_CP_RESP_UNSUPPORTED_OPCODE        = 0x01,
    ASE_CP_RESP_INVALID_LENGTH            = 0x02,
    ASE_CP_RESP_INVALID_ASE_ID            = 0x03,
    ASE_CP_RESP_INVALID_ASE_SM_TRANSITION = 0x04,
    ASE_CP_RESP_INVALID_ASE_DIRECTION      = 0x05,
    ASE_CP_RESP_UNSUPPORTED_AUDIO_CAP     = 0x06,
    ASE_CP_RESP_UNSUPPORTED_PARAM         = 0x07,
    ASE_CP_RESP_REJECTED_PARAM            = 0x08,
    ASE_CP_RESP_INVALID_PARAM             = 0x09,
    ASE_CP_RESP_UNSUPPORTED_METADATA      = 0x0A,
    ASE_CP_RESP_REJECTED_METADATA         = 0x0B,
    ASE_CP_RESP_INVALID_METADATA          = 0x0C,
    ASE_CP_RESP_INSUFFICIENT_RESOURCE     = 0x0D,
    ASE_CP_RESP_UNSPECIFIED_ERROR         = 0x0E,
    ASE_CP_RESP_RFU
} T_ASE_CP_RESP;

typedef enum
{
    ASE_CP_OP_NONE            = 0x00,
    ASE_CP_OP_CONFIG_CODEC    = 0x01,//Client or Server
    ASE_CP_OP_CONFIG_QOS      = 0x02,//Client only
    ASE_CP_OP_ENABLE          = 0x03,//Client only
    ASE_CP_OP_REC_START_READY = 0x04,//Client or Server
    ASE_CP_OP_DISABLE         = 0x05,//Client or Server
    ASE_CP_OP_REC_STOP_READY  = 0x06,//Client or Server
    ASE_CP_OP_UPDATA_METADATA = 0x07,//Client or Server
    ASE_CP_OP_RELEASE         = 0x08,//Client or Server
    ASE_CP_OP_MAX,
} T_ASE_CP_OP;

typedef enum
{
    ASE_TARGET_LOWER_LATENCY      = 0x01,
    ASE_TARGET_BALANCED           = 0x02,
    ASE_TARGET_HIGHER_RELIABILITY = 0x03,
} T_ASE_TARGET_LATENCY;

typedef enum
{
    ASE_TARGET_PHY_1M    = 0x01,
    ASE_TARGET_PHY_2M    = 0x02,
    ASE_TARGET_PHY_CODED = 0x03,
} T_ASE_TARGET_PHY;

typedef struct
{
    uint8_t ase_id;
    uint8_t response_code;
    uint8_t reason;
} T_ASE_CP_NOTIFY_ARRAY_PARAM;

typedef struct
{
    uint8_t opcode;
    uint8_t number_of_ase;
    T_ASE_CP_NOTIFY_ARRAY_PARAM param[1];
} T_ASE_CP_NOTIFY_DATA;

typedef struct
{
    uint8_t ase_id;
    uint8_t target_latency;
    uint8_t target_phy;
    uint8_t codec_id[5];
    uint8_t codec_spec_cfg_len;
} T_ASE_CP_CODEC_CFG_ARRAY_PARAM;

typedef struct
{
    uint8_t ase_id;
    uint8_t cig_id;
    uint8_t cis_id;
    uint8_t sdu_interval[3];
    uint8_t framing;
    uint8_t phy;
    uint8_t max_sdu[2];
    uint8_t retransmission_number;
    uint8_t max_transport_latency[2];
    uint8_t presentation_delay[3];
} T_ASE_CP_QOS_CFG_ARRAY_PARAM;

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
