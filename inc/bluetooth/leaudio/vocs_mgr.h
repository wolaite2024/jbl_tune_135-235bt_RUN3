#ifndef _VOCS_MGR_H_
#define _VOCS_MGR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "gatt.h"
#include "ble_audio_def.h"
#include "codec_def.h"
#include "vocs_def.h"

#if LE_AUDIO_VOCS_SUPPORT
#define VOCS_AUDIO_LOCATION_WRITE_MSK   0x01
#define VOCS_AUDIO_OUTPUT_DES_MSK       0x02

#define VOCS_VOLUME_OFFSET_MASK         0x01
#define VOCS_AUDIO_LOCATION_MASK        0x02
#define VOCS_OUTPUT_DES_MASK            0x04

typedef struct
{
    uint16_t conn_handle;
    uint8_t idx;
    int16_t volume_offset;
} T_VOCS_VOL_OFFSET;

typedef struct
{
    uint16_t    conn_handle;
    uint8_t     idx;
    uint32_t    chnl_loc;
} T_VOCS_CHNL_LOC;

typedef struct
{
    uint16_t            conn_handle;
    uint8_t             idx;
    T_VOCS_OUTPUT_DES   output_des;
} T_VOCS_OUTPUT_DESC;

typedef struct
{
    uint8_t             mask;
    int16_t             volume_offset;
    uint8_t             change_counter;
    uint32_t            audio_location;
    T_VOCS_OUTPUT_DES   output_des;
} T_VOCS_PARAM_SET;


typedef struct
{
    int16_t     volume_offset;
    uint8_t     change_counter;
    uint32_t    audio_location;
    uint8_t    *output_des;
    uint8_t     output_des_len;
} T_VOCS_PARAM_GET;

bool vocs_set_param(uint8_t service_idx, T_VOCS_PARAM_SET *p_param);
bool vocs_get_param(uint8_t service_idx, T_VOCS_PARAM_GET *p_param);
void vocs_init(uint8_t srv_num, uint8_t *feature);
#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
