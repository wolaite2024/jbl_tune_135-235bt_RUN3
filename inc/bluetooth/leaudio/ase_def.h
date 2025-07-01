#ifndef _ASE_DEF_H_
#define _ASE_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_def.h"

#define ASE_INVALID_ID 0x00

#define QOS_SUPPORTED_UNFRAMED_BIT         0x01   //bit0
#define QOS_SUPPORTED_FRAMED_BIT           0x02   //bit1

#define QOS_SDU_INTERVAL_MIN               0x0000FF
#define QOS_SDU_INTERVAL_MAX               0xFFFFFF
#define QOS_MAX_SDU_MIN                    0x0000
#define QOS_MAX_SDU_MAX                    0x0FFF
#define QOS_RETRANSMISSION_NUMBER_MIN      0x00
#define QOS_RETRANSMISSION_NUMBER_MAX      0xFF
#define QOS_TRANSPORT_LATENCY_MIN          0x0005
#define QOS_TRANSPORT_LATENCY_MAX          0x0FA0

//Param: Preferred_PHY
#define ASE_LE_1M_PHY_BIT                  0x01
#define ASE_LE_2M_PHY_BIT                  0x02
#define ASE_LE_CODED_PHY_BIT               0x04
#define ASE_LE_PHY_CFG_MASK                0x07

typedef enum
{
    AUDIO_UNFRAMED_SUPPORTED      = 0x00,
    AUDIO_UNFRAMED_NOT_SUPPORTED  = 0x01,
} T_AUDIO_UNFRAMED_SUPPORTED;

typedef enum
{
    AUDIO_UNFRAMED = 0x00,
    AUDIO_FRAMED   = 0x01,
} T_AUDIO_FRAMING;

typedef enum
{
    ASE_STATE_IDLE             = 0x00,
    ASE_STATE_CODEC_CONFIGURED = 0x01,
    ASE_STATE_QOS_CONFIGURED   = 0x02,
    ASE_STATE_ENABLING         = 0x03,
    ASE_STATE_STREAMING        = 0x04,
    ASE_STATE_DISABLING        = 0x05,
    ASE_STATE_RELEASING        = 0x06,

    ASE_STATE_UNKOWN           = 0xff,
} T_ASE_STATE;

/* The format of the Additional_ASE_Parameters field
when ASE_State = 0x01 (Codec Configured)*/
typedef struct
{
    uint8_t supported_framing;
    uint8_t preferred_phy;
    uint8_t preferred_retrans_number;
    uint8_t max_transport_latency[2];
    uint8_t presentation_delay_min[3];
    uint8_t presentation_delay_max[3];
    uint8_t preferred_presentation_delay_min[3];
    uint8_t preferred_presentation_delay_max[3];
    uint8_t codec_id[5];
    uint8_t codec_spec_cfg_len;
} T_ASE_CODEC_CFG_STATE_DATA;

/* The format of the Additional_ASE_Parameters field
when ASE_State = 0x02 (QoS Configured)*/
typedef struct
{
    uint8_t cig_id;
    uint8_t cis_id;
    uint8_t sdu_interval[3];
    uint8_t framing;
    uint8_t phy;
    uint8_t max_sdu[2];
    uint8_t retransmission_number;
    uint8_t max_transport_latency[2];
    uint8_t presentation_delay[3];
} T_ASE_QOS_CFG_STATE_DATA;

typedef struct
{
    T_ASE_CODEC_CFG_STATE_DATA data;
    uint8_t *p_codec_spec_cfg;
} T_ASE_DATA_CODEC_CONFIGURED;

typedef struct
{
    uint8_t cig_id;
    uint8_t cis_id;
    uint8_t metadata_length;
    uint8_t *p_metadata;
} T_ASE_DATA_WITH_METADATA;

typedef union
{
    T_ASE_DATA_CODEC_CONFIGURED codec_configured;
    T_ASE_QOS_CFG_STATE_DATA    qos_configured;
    T_ASE_DATA_WITH_METADATA    enabling;
    T_ASE_DATA_WITH_METADATA    streaming;
    T_ASE_DATA_WITH_METADATA    disabling;
} T_ASE_DATA;

typedef struct
{
    uint8_t ase_id;
    uint8_t direction;
    uint8_t ase_state;
    T_ASE_DATA param;
} T_ASE_CHAR_DATA;


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
