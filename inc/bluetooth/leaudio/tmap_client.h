#ifndef _TMAP_CLIENT_H_
#define _TMAP_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "tmas_def.h"
#include "profile_client.h"


#if LE_AUDIO_TMAP_CLIENT_SUPPORT

typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
} T_TMAP_CLIENT_DIS_DONE;

typedef struct
{
    uint16_t conn_handle;
    uint16_t role;
    uint16_t cause;
} T_TMAP_CLIENT_READ_ROLE;
bool tmap_read_role(uint16_t conn_handle);
bool tmap_read_role_by_uuid(uint16_t conn_handle);
bool tmap_client_init(void);
#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
