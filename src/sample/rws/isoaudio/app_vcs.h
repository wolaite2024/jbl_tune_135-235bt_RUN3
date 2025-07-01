#ifndef _APP_VCS_H_
#define _APP_VCS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

#define MAX_VCS_OUTPUT_LEVEL      15
#define MAX_VCS_VOLUME_SETTING    255

#define MIN_MIC_GAIN_SETTING     -128
#define MAX_MIC_GAIN_SETTING      127

void app_vcs_init(void);
T_APP_RESULT app_vcs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
T_APP_RESULT app_vocs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
T_APP_RESULT app_aics_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
T_APP_RESULT app_mics_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
