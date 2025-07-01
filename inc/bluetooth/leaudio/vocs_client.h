#ifndef _VOCS_CLIENT_H_
#define _VOCS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "vocs_def.h"
#include "profile_client.h"

#if LE_AUDIO_VOCS_CLIENT_SUPPORT

#define VOCS_OFFSET_STATE_CCCD_FLAG           0x01
#define VOCS_AUDIO_LOCATION_CCCD_FLAG         0x02
#define VOCS_AUDIO_OUTPUT_DES_CCCD_FLAG       0x04

typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t srv_num;
} T_VOCS_CLIENT_DIS_DONE;

typedef struct
{
    uint8_t  srv_instance_id;
    uint16_t conn_handle;
    uint16_t cause;
} T_VOCS_CLIENT_CCCD;

typedef enum
{
    VOCS_CHAR_OFFSET_STATE,
    VOCS_CHAR_AUDIO_LOCATION,
    VOCS_CHAR_AUDIO_OUTPUT_DESC,
} T_VOCS_CHAR_TYPE;

typedef struct
{
    bool    counter_used;
    uint8_t change_counter;
    int16_t volume_offset;
} T_VOCS_CP_PARAM;

typedef struct
{
    uint16_t data_len;
    uint8_t  *p_data;
} T_AUDIO_OUTPUT_DESC_DATA;

typedef union
{
    T_VOCS_VOLUME_OFFSET      volume_offset;
    uint32_t                  audio_location;
    T_AUDIO_OUTPUT_DESC_DATA  audio_desc;
} T_VOCS_DATA;

typedef struct
{
    uint16_t            conn_handle;
    uint8_t             srv_instance_id;
    T_VOCS_CHAR_TYPE    type;
    T_VOCS_DATA         data;
    uint16_t            cause;
} T_VOCS_READ_RESULT;

typedef struct
{
    uint16_t            conn_handle;
    uint8_t             srv_instance_id;
    T_VOCS_CHAR_TYPE    type;
    T_VOCS_DATA         data;
} T_VOCS_NOTIFY_DATA;

typedef struct
{
    uint16_t  conn_handle;
    uint8_t  srv_instance_id;
    uint16_t cause;
    T_VOCS_CP_OP cp_op;
} T_VOCS_CP_RESULT;

typedef struct
{
    uint8_t                     idx;
    T_VOCS_VOLUME_OFFSET        volume_offset;
    uint32_t                    audio_location;
    T_AUDIO_OUTPUT_DESC_DATA    audio_desc;
} T_VOCS_SRV_DATA;


bool vocs_cfg_cccd(uint16_t conn_handle, uint8_t srv_idx, uint8_t cfg_flags, bool enable);
bool vocs_read_char_value(uint16_t conn_handle, uint8_t srv_idx, T_VOCS_CHAR_TYPE type);
bool vocs_send_cp(uint16_t conn_handle, uint8_t srv_idx, T_VOCS_CP_OP op,
                  T_VOCS_CP_PARAM *p_param);
bool vocs_write_channel_location(uint16_t conn_handle, uint8_t srv_idx, uint64_t chnl_loc);
bool vocs_write_output_des(uint16_t conn_handle, uint8_t srv_idx, uint8_t *p_output_des,
                           uint16_t len);
T_VOCS_SRV_DATA vocs_get_srv_data(uint16_t conn_handle, uint8_t srv_idx);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
