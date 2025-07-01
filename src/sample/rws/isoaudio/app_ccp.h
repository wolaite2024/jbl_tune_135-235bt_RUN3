#ifndef _APP_CCP_H_
#define _APP_CCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

#define CCP_CALL_STATE_CHARA_LEN   3

T_APP_RESULT app_ccp_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t app_ccp_get_active_conn_handle(void);
bool app_ccp_set_active_conn_handle(uint16_t conn_handle);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
