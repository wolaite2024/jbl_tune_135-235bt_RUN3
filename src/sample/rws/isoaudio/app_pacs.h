#ifndef _APP_PACS_H_
#define _APP_PACS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

extern uint16_t sink_available_contexts;
extern uint16_t source_available_contexts;

void app_pacs_init(void);
T_APP_RESULT app_pacs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
