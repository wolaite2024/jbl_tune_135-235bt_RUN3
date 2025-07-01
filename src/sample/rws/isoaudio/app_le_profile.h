#ifndef _APP_PROFILE_INIT_H_
#define _APP_PROFILE_INIT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define APP_MAX_PA_ADV_SET_NUM 4
#define APP_MAX_SYNC_HANDLE_NUM 4

#define APP_ISOC_CIG_MAX_NUM                      2
#define APP_ISOC_CIS_MAX_NUM                      4

#define APP_ISOC_BROADCASTER_MAX_BIG_HANDLE_NUM  2
#define APP_ISOC_BROADCASTER_MAX_BIS_NUM  4

#define APP_SYNC_RECEIVER_MAX_BIG_HANDLE_NUM  2
#define APP_SYNC_RECEIVER_MAX_BIS_NUM  4

void app_le_profile_init(void);
void app_le_audio_role_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
