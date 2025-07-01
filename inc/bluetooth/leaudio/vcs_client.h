#ifndef _VCS_CLIENT_H_
#define _VCS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "vcs_def.h"
#include "profile_client.h"

#if LE_AUDIO_VCS_CLIENT_SUPPORT

#define VCS_VOLUME_STATE_CCCD_FLAG 0x01
#define VCS_VOLUME_FLAGS_CCCD_FLAG 0x02


typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t srv_num;
} T_VCS_CLIENT_DIS_DONE;

typedef struct
{
    bool counter_used;
    uint8_t change_counter;
    uint8_t volume_setting;//used only for VCS_CP_SET_ABSOLUTE_VOLUME
} T_VCS_VOLUME_CP_PARAM;

typedef enum
{
    VCS_CHAR_VOLUME_STATE,
    VCS_CHAR_VOLUME_FLAGS,
} T_VCS_CHAR_TYPE;

typedef union
{
    T_VOLUME_STATE volume_state;
    uint8_t volume_flags;
} T_VCS_DATA;


typedef struct
{
    uint16_t        conn_handle;
    T_VCS_CHAR_TYPE type;
    T_VCS_DATA      data;
    uint16_t        cause;
} T_VCS_READ_RESULT;

typedef struct
{
    uint16_t        conn_handle;
    T_VCS_CHAR_TYPE type;
    T_VCS_DATA      data;
} T_VCS_NOTIFY_DATA;

typedef struct
{
    uint16_t    conn_handle;
    uint16_t    cause;
    T_VCS_CP_OP cp_op;
} T_VCS_CP_RESULT;

typedef struct
{
    uint16_t  conn_handle;
    uint16_t  cause;
} T_VCS_CFG_CCCD_RESULT;

bool vcs_cfg_cccd(uint16_t conn_handle, uint8_t cfg_flags, bool enable);
bool vcs_read_char_value(uint16_t conn_handle, T_VCS_CHAR_TYPE type);
bool vcs_send_volume_cp(uint16_t conn_handle, T_VCS_CP_OP op, T_VCS_VOLUME_CP_PARAM *p_param);
bool vcs_get_volume_state(uint16_t conn_handle, T_VCS_DATA *data);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
