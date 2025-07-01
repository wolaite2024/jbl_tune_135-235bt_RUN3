#ifndef _APP_MCP_H_
#define _APP_MCP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

T_APP_RESULT app_mcp_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t app_mcp_get_active_conn_handle(void);
bool app_mcp_set_active_conn_handle(uint16_t conn_handle);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
