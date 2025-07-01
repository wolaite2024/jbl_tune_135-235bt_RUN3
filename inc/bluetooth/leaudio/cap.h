#ifndef _CAP_H_
#define _CAP_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "cas_def.h"
#include "ble_audio_group.h"

#if LE_AUDIO_CAP_SUPPORT
typedef struct
{
    uint8_t default_volume_settings;
    uint8_t default_mute;
    uint8_t default_mic_mute;
} T_CAP_DEFAULT_PARAM;

typedef struct
{
    uint16_t conn_handle;
    bool    is_found;
    bool    load_form_ftl;
    uint8_t srv_num;
} T_CAP_DIS_DONE;

bool cap_change_volume_by_address(uint8_t *bd_addr, uint8_t addr_type, uint8_t volume_setting);
bool cap_change_volume(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint8_t volume_setting);
bool cap_change_mute_by_address(uint8_t *bd_addr, uint8_t addr_type, uint8_t mute);
bool cap_change_mute(T_BLE_AUDIO_GROUP_HANDLE group_handle, uint8_t mute);
bool cap_change_mic_mute_by_address(uint8_t *bd_addr, uint8_t addr_type, uint8_t mute);
bool cap_change_input_gain_by_address(uint8_t *bd_addr, uint8_t addr_type, int8_t gain);
void cap_update_default_param(T_CAP_DEFAULT_PARAM *p_param);
void cap_init(T_CAP_DEFAULT_PARAM *p_param);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
