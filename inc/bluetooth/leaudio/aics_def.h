#ifndef _AICS_DEF_H_
#define _AICS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define ATT_ERR_AICS_INVALID_CHANGE_COUNTER       0x80
#define ATT_ERR_AICS_OPCODE_NOT_SUPPORT           0x81
#define ATT_ERR_AICS_MUTE_DISABLED                0x82
#define ATT_ERR_AICS_VALUE_OUT_OF_RANGE           0x83
#define ATT_ERR_AICS_GAIN_MODE_CHANGE_NOT_ALLOWED 0x84

//audio input control service
#define GATT_UUID_AICS                           0x1843

#define AICS_UUID_CHAR_INPUT_STATE               0x2B77
#define AICS_UUID_CHAR_GAIN_SETTINGS             0x2B78
#define AICS_UUID_CHAR_INPUT_TYPE                0x2B79
#define AICS_UUID_CHAR_INPUT_STATUS              0x2B7A
#define AICS_UUID_CHAR_AUDIO_INPUT_CP            0x2B7B
#define AICS_UUID_CHAR_AUDIO_INPUT_DES           0x2B7C

#define AICS_NOT_MUTED                           0
#define AICS_MUTED                               1
#define AICS_MUTE_DISABLED                       2

#define AICS_GAIN_MODE_MANUAL_ONLY               0
#define AICS_GAIN_MODE_AUTOMATIC_ONLY            1
#define AICS_GAIN_MODE_MANUAL                    2
#define AICS_GAIN_MODE_AUTOMATIC                 3

#define AICS_INPUT_STATUS_INACTIVE               0
#define AICS_INPUT_STATUS_ACTIVE                 1

typedef enum
{
    AICS_CP_SET_GAIN_SETTING        = 0x01,
    AICS_CP_UNMUTE                  = 0x02,
    AICS_CP_MUTE                    = 0x03,
    AICS_CP_SET_MANUAL_GAIN_MODE    = 0x04,
    AICS_CP_SET_AUTOMATIC_GAIN_MODE = 0x05,
    AICS_CP_MAX
} T_AICS_CP_OP;

typedef struct
{
    int8_t  gain_setting;
    uint8_t mute;
    uint8_t gain_mode;
    uint8_t change_counter;
} T_AICS_INPUT_STATE;

typedef struct
{
    uint8_t gain_setting_units;
    int8_t  gain_setting_min;
    int8_t  gain_setting_max;
} T_AICS_GAIN_SETTING_PROP;


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
