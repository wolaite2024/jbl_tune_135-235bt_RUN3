#ifndef _MICS_DEF_H_
#define _MICS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define MICS_MUTE_DISABLED_ERROR                      0x80

#define GATT_UUID_MICS                            0x184D

#define MICS_UUID_CHAR_MUTE                       0x2BC3

typedef enum
{
    MICS_NOT_MUTE   = 0,
    MICS_MUTED      = 1,
    MICS_DISABLED   = 2,
    MICS_RFU,
} T_MICS_MUTE_CHAR_VALUE;


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
