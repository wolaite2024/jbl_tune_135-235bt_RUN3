#ifndef _VOCS_DEF_H_
#define _VOCS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define ATT_ERR_VOCS_INVALID_CHANGE_COUNTER      0x80
#define ATT_ERR_VOCS_OPCODE_NOT_SUPPORT          0x81
#define ATT_ERR_VOCS_VALUE_OUT_OF_RANGE          0x82

//volume offset control service
#define GATT_UUID_VOCS                           0x1845

#define VOCS_UUID_CHAR_OFFSET_STATE              0x2B80
#define VOCS_UUID_CHAR_AUDIO_LOCATION            0x2B81
#define VOCS_UUID_CHAR_VOLUME_OFFSET_CP          0x2B82
#define VOCS_UUID_CHAR_AUDIO_OUTPUT_DES          0x2B83

typedef enum
{
    VOCS_CP_SET_VOLUME_OFFSET = 0x01,
    VOCS_CP_MAX
} T_VOCS_CP_OP;

typedef struct
{
    int16_t volume_offset;
    uint8_t change_counter;
} T_VOCS_VOLUME_OFFSET;

typedef struct
{
    uint8_t    *p_output_des;
    uint8_t     output_des_len;
} T_VOCS_OUTPUT_DES;
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
