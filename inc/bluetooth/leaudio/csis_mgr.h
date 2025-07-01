#ifndef _CSIS_MGR_H_
#define _CSIS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gap.h"
#include "gatt.h"
#include "profile_server_def.h"
#include "csis_rsi.h"
#include "csis_def.h"

//LE_AUDIO_MSG_CSIS_READ_SIRK
typedef struct
{
    uint16_t conn_handle;
    T_SERVER_ID service_id;
    T_CSIS_SIRK_TYPE sirk_type;
} T_CSIS_READ_SIRK;

//LE_AUDIO_MSG_CSIS_LOCK_STATE
typedef struct
{
    T_SERVER_ID service_id;
    T_CSIS_LOCK lock_state;
    uint8_t lock_address[GAP_BD_ADDR_LEN];
    uint8_t lock_address_type;
} T_CSIS_LOCK_STATE;

void csis_init(uint8_t set_mem_num);
T_SERVER_ID csis_reg_srv(uint8_t *sirk, T_CSIS_SIRK_TYPE sirk_type, uint8_t size, uint8_t rank,
                         uint8_t feature);
bool csis_get_srv_tbl(T_SERVER_ID service_id, T_ATTRIB_APPL **pp_srv_tbl);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
