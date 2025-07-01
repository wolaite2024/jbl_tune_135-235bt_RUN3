#ifndef _TBS_DEF_H_
#define _TBS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#define TBS_BEARER_SIGNAL_STRENGTH_SUP          0x01
#define TBS_IN_CALL_TARGET_BEARER_URI_SUP       0x02
#define TBS_CALL_FRIENDLY_NAME_SUP              0x04

#define TBS_BEAR_TECH_3G                      0x01
#define TBS_BEAR_TECH_4G                      0x02
#define TBS_BEAR_TECH_LTE                     0x03
#define TBS_BEAR_TECH_WIFI                    0x04
#define TBS_BEAR_TECH_5G                      0x05
#define TBS_BEAR_TECH_GSM                     0x06
#define TBS_BEAR_TECH_CDMA                    0x07
#define TBS_BEAR_TECH_2G                      0x08
#define TBS_BEAR_TECH_WCDMA                   0x09
#define TBS_BEAR_TECH_IP                      0x0A


//telephone bear service
#define GATT_UUID_TBS                         0x184B
#define GATT_UUID_GTBS                        0x184C

#define TBS_UUID_CHAR_BPN                     0x2BB3
#define TBS_UUID_CHAR_UCI                     0x2BB4
#define TBS_UUID_CHAR_TECH                    0x2BB5
#define TBS_UUID_CHAR_URI_SUP_LIST            0x2BB6
#define TBS_UUID_CHAR_SIG_STREN               0x2BB7
#define TBS_UUID_CHAR_SIG_STREN_RPT_INT       0x2BB8
#define TBS_UUID_CHAR_LIST_CUR_CALLS          0x2BB9
#define TBS_UUID_CHAR_CCID                    0x2BBA
#define TBS_UUID_CHAR_STATUS_FLAGS            0x2BBB
#define TBS_UUID_CHAR_IN_CALL_TG_URI          0x2BBC
#define TBS_UUID_CHAR_CALL_STATE              0x2BBD
#define TBS_UUID_CHAR_CCP                     0x2BBE
#define TBS_UUID_CHAR_CCP_OPT_OPCODES         0x2BBF
#define TBS_UUID_CHAR_TERM_REASON             0x2BC0
#define TBS_UUID_CHAR_INCOMING_CALL           0x2BC1
#define TBS_UUID_CHAR_CALL_FRIENDLY_NAME      0x2BC2


#define TBS_LOCAL_HOLD_RETR_OPCODE_SUP        0x0001
#define TBS_JOIN_OPCODE_SUP                   0x0002

#define INCOMING_CALL_FLAGS                   0x00
#define OUTGOING_CALL_FLAGS                   0x01
#define INFO_HELD_BY_SERV_CALL_FLAGS          0x02
#define INFO_HELD_BY_NETWORK_CALL_FLAGS       0x04

#define TBS_READ_LONG_ERR_CODE            0x80

typedef enum
{
    TBS_INCOMING_CALL_TYPE = 0x01,
    TBS_OUTGOING_CALL_TYPE = 0x02
} T_TBS_CALL_TYPE;

typedef enum
{
    TBS_CCP_OP_ACCEPT             = 0x00,
    TBS_CCP_OP_TERMINATE          = 0x01,
    TBS_CCP_OP_LOCAL_HOLD         = 0x02,
    TBS_CCP_OP_LOCAL_RETRIEVE     = 0x03,
    TBS_CCP_OP_ORIGINATE          = 0x04,
    TBS_CCP_OP_JOIN               = 0x05,
    TBS_CCP_OP_RFU
} T_TBS_CCP_OP;

typedef enum
{
    TBS_CALL_STATE_INCOMING       = 0x00,
    TBS_CALL_STATE_DIALING        = 0x01,
    TBS_CALL_STATE_ALERTING       = 0x02,
    TBS_CALL_STATE_ACTIVE         = 0x03,
    TBS_CALL_STATE_LOC_HELD       = 0x04,
    TBS_CALL_STATE_REM_HELD       = 0x05,
    TBS_CALL_STATE_LOC_REM_HELD   = 0x06,
    TBS_CALL_STATE_RFU
} T_TBS_CALL_STATE;


typedef enum
{
    TBS_CCP_SUCCESS               = 0x00,
    TBS_CCP_OP_NOT_SUP            = 0x01,
    TBS_CCP_OPER_NOT_POSSIBLE     = 0x02,
    TBS_CCP_INV_CALL_INDEX        = 0x03,
    TBS_CCP_STATE_MISMATCH        = 0x04,
    TBS_CCP_LACK_OF_RES           = 0x05,
    TBS_CCP_INV_OUTGOING_URI      = 0x06,
    TBS_CCP_RES_RFU
} T_TBS_CCP_NOTIFY_RESULT;

typedef enum
{
    TBS_TERM_URI_IMPROPER           = 0x00,
    TBS_TERM_CALL_FAILED            = 0x01,
    TBS_TERM_REMOTE_END_CALL        = 0x02,
    TBS_TERM_SERVER_END_CALL        = 0x03,
    TBS_TERM_LINE_BUSY              = 0x04,
    TBS_TERM_NETWORK_CONGEST        = 0x05,
    TBS_TERM_CLI_TERM_CALL          = 0x06,
    TBS_TERM_NO_SERVICE             = 0x07,
    TBS_TERM_NO_ANSWER              = 0x08,
    TBS_TERM_UNSPECIFIED            = 0x09,
    TBS_TERM_RESON_RFU
} T_TBS_TERM_REASON_CODE;

typedef struct
{
    uint8_t     *bp_name;
    uint16_t    len;
} T_TBS_BPN;

typedef struct
{
    uint8_t     *p_uci;
    uint16_t    len;
} T_TBS_UCI;

typedef struct
{
    uint8_t     *p_bearer_tech;
    uint16_t    len;
} T_TBS_TECH;

typedef struct
{
    uint8_t     *p_uri_list;
    uint16_t    len;
} T_TBS_URI_SUP_LIST;

typedef struct
{
    uint8_t     sig_stren;
    uint8_t     interval;
} T_TBS_SIG_STREN;

typedef struct
{
    uint8_t    *p_cur_call_list;
    uint16_t    len;
} T_TBS_CUR_CALL_LIST;

typedef struct
{
    uint8_t     call_index;
    uint8_t    *tg_uri;
    uint16_t    uri_len;
} T_TBS_IN_CALL_TG_URI;

typedef struct
{
    uint8_t                    call_index;
    T_TBS_TERM_REASON_CODE     reason_code;
} T_TBS_TERM_REASON;


typedef struct
{
    uint8_t     call_index;
    uint8_t    *p_uri;
    uint16_t    uri_len;
} T_TBS_INCOMING_CALL;

typedef struct
{
    uint8_t     call_index;
    uint8_t    *p_name;
    uint16_t    name_len;
} T_TBS_CALL_FRI_NAME;
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
