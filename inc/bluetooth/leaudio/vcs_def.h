#ifndef _VCS_DEF_H_
#define _VCS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define ATT_ERR_VCS_INVALID_CHANGE_COUNTER       0x80
#define ATT_ERR_VCS_OPCODE_NOT_SUPPORT           0x81

//volume control service
#define GATT_UUID_VCS                            0x1844

#define VCS_UUID_CHAR_VOLUME_STATE               0x2B7D
#define VCS_UUID_CHAR_VOLUME_CP                  0x2B7E
#define VCS_UUID_CHAR_VOLUME_FLAGS               0x2B7F

#define VCS_VOLUME_SETTING_MIN                   0
#define VCS_VOLUME_SETTING_MAX                   255

#define VCS_MUTED                                1
#define VCS_NOT_MUTED                            0

#define VCS_VOLUME_SETTING_PERSISTED_FLAG        0x01
#define VCS_RESET_VOLUME_SETTING                 0x00
#define VCS_USER_SET_VOLUME_SETTING              0x01


typedef enum
{
    VCS_CP_RELATIVE_VOLUME_DOMN        = 0x00,
    VCS_CP_RELATIVE_VOLUME_UP          = 0x01,
    VCS_CP_UNMUTE_RELATIVE_VOLUME_DOMN = 0x02,
    VCS_CP_UNMUTE_RELATIVE_VOLUME_UP   = 0x03,
    VCS_CP_SET_ABSOLUTE_VOLUME         = 0x04,
    VCS_CP_UNMUTE                      = 0x05,
    VCS_CP_MUTE                        = 0x06,
    VCS_CP_MAX
} T_VCS_CP_OP;

typedef struct
{
    uint8_t volume_setting;
    uint8_t mute;
    uint8_t change_counter;
} T_VOLUME_STATE;

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
