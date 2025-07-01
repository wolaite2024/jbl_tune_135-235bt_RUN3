#ifndef _APP_BASS_H_
#define _APP_BASS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"
#include "ble_audio_sync.h"

T_APP_RESULT app_bass_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
uint16_t app_bass_get_conn_handle(void);
uint8_t app_bass_get_source(void);
T_BLE_AUDIO_SYNC_HANDLE app_bass_get_sync_handle(uint8_t src_id);
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
