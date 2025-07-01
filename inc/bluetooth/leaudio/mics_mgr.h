#ifndef _MICS_MGR_H_
#define _MICS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gatt.h"
#include "ble_audio_def.h"
#include "codec_def.h"
#include "mics_def.h"

#if LE_AUDIO_MICS_SUPPORT
typedef struct
{
    uint8_t mute_value;
} T_MICS_PARAM;

void mics_init(uint8_t aics_num, uint8_t *id_array);
void mics_set_param(T_MICS_PARAM *p_param);
bool mics_get_param(T_MICS_PARAM *p_param);
#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
