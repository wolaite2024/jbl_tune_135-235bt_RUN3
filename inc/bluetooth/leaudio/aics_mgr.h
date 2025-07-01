#ifndef _AICS_MGR_H_
#define _AICS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gatt.h"
#include "ble_audio_def.h"
#include "codec_def.h"
#include "aics_def.h"

#if LE_AUDIO_AICS_SUPPORT

typedef struct
{
    uint16_t conn_handle;
    uint8_t idx;
    uint8_t opcode;
    int8_t  gain_setting;
} T_AICS_OP;

typedef struct
{
    uint8_t   idx;
    uint8_t   *p_input_des;
    uint16_t  input_des_len;
} T_AICS_INPUT_DES;


typedef enum
{
    AICS_PARAM_INPUT_STATE       = 0x01, // T_AICS_INPUT_STATE
    AICS_PARAM_GAIN_SETTING_PROP = 0x02, // T_AICS_GAIN_SETTING_PROP
    AICS_PARAM_INPUT_TYPE        = 0x03, // 1 byte
    AICS_PARAM_INPUT_STATUS      = 0x04, // 1 byte
    AICS_PARAM_INPUT_DES         = 0x05,
} T_AICS_PARAM_TYPE;

void aics_init(uint8_t srv_num);
bool aics_set_param(uint8_t service_idx, T_AICS_PARAM_TYPE param_type, uint8_t length,
                    uint8_t *p_value, bool set_change_counter);
bool aics_get_param(uint8_t service_idx, T_AICS_PARAM_TYPE param_type, uint8_t *p_value);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
