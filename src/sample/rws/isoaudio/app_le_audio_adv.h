#ifndef _APP_LE_AUDIO_ADV_H_
#define _APP_LE_AUDIO_ADV_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define LE_EXT_ADV_PSRI 0x0001
#define LE_EXT_ADV_ASCS 0x0002
#define LE_EXT_ADV_BASS 0x0004

typedef enum
{
    LEA_ADV_STATE_DISABLED,
    LEA_ADV_STATE_ENABLED,
} T_LEA_ADV_STATE;


void app_le_audio_adv_init(void);
void app_le_audio_adv_start(bool enable_pairable, uint8_t mode);
void app_le_audio_adv_stop(bool disable_pairable);
T_LEA_ADV_STATE app_lea_adv_state(void);


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
