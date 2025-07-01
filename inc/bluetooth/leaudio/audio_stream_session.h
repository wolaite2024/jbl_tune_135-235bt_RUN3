#ifndef _AUDIO_STREAM_SESSION_H_
#define _AUDIO_STREAM_SESSION_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio_group.h"

typedef void *T_AUDIO_STREAM_SESSION_HANDLE;

T_AUDIO_STREAM_SESSION_HANDLE audio_stream_session_allocate(T_BLE_AUDIO_GROUP_HANDLE group_handle);
T_AUDIO_STREAM_SESSION_HANDLE audio_stream_session_find_by_cig_id(uint8_t cig_id);
T_BLE_AUDIO_GROUP_HANDLE audio_stream_session_get_group(T_AUDIO_STREAM_SESSION_HANDLE handle);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
