#ifndef _APP_ASCS_H_
#define _APP_ASCS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

void app_ascs_init(void);
T_APP_RESULT app_ascs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
