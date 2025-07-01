#ifndef _VCS_MGR_H_
#define _VCS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gatt.h"
#include "ble_audio_def.h"
#include "codec_def.h"
#include "vcs_def.h"
#if LE_AUDIO_VCS_SUPPORT
typedef struct
{
    uint8_t volume_setting;
    uint8_t mute;
    uint8_t change_counter;
    uint8_t volume_flags;
    uint8_t step_size;
} T_VCS_PARAM;

typedef struct
{
    uint16_t conn_handle;
    uint8_t volume_setting;
    uint8_t mute;
} T_VCS_VOL_STATE;

void vcs_init(uint8_t vocs_num, uint8_t aics_num);
bool vcs_set_param(T_VCS_PARAM *p_param);
bool vcs_get_param(T_VCS_PARAM *p_param);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
