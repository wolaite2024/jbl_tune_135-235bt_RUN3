#ifndef _MICS_CLIENT_H_
#define _MICS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "mics_def.h"
#include "profile_client.h"

#if LE_AUDIO_MICS_CLIENT_SUPPORT
typedef enum
{
    MICS_CHAR_MUTE_VALUE,
} T_MICS_CHAR_TYPE;

typedef struct
{
    uint16_t      conn_handle;
    bool          is_found;
    bool          load_form_ftl;
    uint8_t       srv_num;
    uint8_t       aics_num;
    uint8_t       *p_aics_ids;
} T_MICS_CLIENT_DIS_DONE;

typedef struct
{
    uint16_t  conn_handle;
    uint16_t  cause;
} T_MICS_CFG_CCCD_RESULT;


typedef struct
{
    uint16_t          conn_handle;
    T_MICS_CHAR_TYPE  type;
    uint8_t           mute_value;
    uint16_t          cause;
} T_MICS_READ_RESULT;

typedef struct
{
    uint16_t        conn_handle;
    uint8_t         srv_instance_id;
    uint16_t        cause;
} T_MICS_WRITE_RESULT;

typedef struct
{
    uint16_t            conn_handle;
    T_MICS_CHAR_TYPE    type;
    uint8_t             mute_value;
} T_MICS_NOTIFY_DATA;

uint8_t mics_get_all_aics_ids(uint16_t conn_handle, uint8_t **p);
bool mics_cfg_cccd(uint16_t conn_handle, uint8_t cfg_flags, bool enable);
bool mics_read_char_value(uint16_t conn_handle, T_MICS_CHAR_TYPE type);
bool mics_send_mute_value(uint16_t conn_handle, uint8_t mute_value);
bool mics_get_mute_value(uint16_t conn_handle, uint8_t *p_value);
bool mics_client_init(void);
#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
