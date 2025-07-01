#ifndef _AICS_CLIENT_H_
#define _AICS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "aics_def.h"
#include "profile_client.h"

#if LE_AUDIO_AICS_CLIENT_SUPPORT

#define AICS_INPUT_STATE_CCCD_FLAG      0x01
#define AICS_INPUT_STATUS_CCCD_FLAG     0x02
#define AICS_AUDIO_INPUT_DESC_CCCD_FLAG 0x04

typedef struct
{
    uint16_t data_len;
    uint8_t  *p_data;
} T_AUDIO_INPUT_DESC_DATA;

typedef struct
{
    T_AICS_INPUT_STATE       input_state;
    T_AICS_GAIN_SETTING_PROP setting_prop;
    uint8_t                  input_type;
    uint8_t                  input_status;
    T_AUDIO_INPUT_DESC_DATA  audio_desc;
} T_AICS_DATA;

typedef enum
{
    AICS_CHAR_INPUT_STATE,
    AICS_CHAR_GAIN_SETTING_PROP,
    AICS_CHAR_INPUT_TYPE,
    AICS_CHAR_INPUT_STATUS,
    AICS_CHAR_AUDIO_INPUT_DESC,
} T_AICS_CHAR_TYPE;

typedef struct
{
    uint16_t            conn_handle;
    uint8_t             srv_instance_id;
    T_AICS_CHAR_TYPE    type;
    T_AICS_DATA         data;
    uint16_t            cause;
} T_AICS_READ_RESULT;

typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t srv_num;
} T_AICS_CLIENT_DIS_DONE;

typedef struct
{
    bool counter_used;
    uint8_t change_counter;
    int8_t gaining_setting;
} T_AICS_CP_PARAM;


typedef struct
{
    uint16_t            conn_handle;
    uint8_t             srv_instance_id;
    T_AICS_CHAR_TYPE    type;
    T_AICS_DATA         data;
} T_AICS_NOTIFY_DATA;

typedef struct
{
    uint16_t        conn_handle;
    uint8_t         srv_instance_id;
    uint16_t        cause;
    T_AICS_CP_OP    cp_op;
} T_AICS_CP_RESULT;


typedef struct
{
    uint8_t srv_instance_id;
    uint16_t conn_handle;
    uint16_t cause;
} T_AICS_CLIENT_CCCD;

T_AICS_DATA *aics_get_srv_data(uint16_t conn_handle, uint8_t srv_idx);
bool aics_cfg_cccd(uint16_t conn_handle, uint8_t srv_idx, uint8_t cfg_flags, bool enable);
bool aics_read_char_value(uint16_t conn_handle, uint8_t srv_idx, T_AICS_CHAR_TYPE type);
bool aics_send_cp(uint16_t conn_handle, uint8_t srv_idx, T_AICS_CP_OP op,
                  T_AICS_CP_PARAM *p_param);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
