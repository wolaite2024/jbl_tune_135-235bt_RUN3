#ifndef _APP_AUDIO_LIB_MSG_H_
#define _APP_AUDIO_LIB_MSG_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

T_APP_RESULT app_le_audio_msg_cback(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
