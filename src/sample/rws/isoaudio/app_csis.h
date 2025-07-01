#ifndef _APP_CSIS_H_
#define _APP_CSIS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "csis_def.h"

extern uint8_t csis_sirk[CSI_SIRK_LEN];

void app_csis_init(void);
T_APP_RESULT app_csis_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
