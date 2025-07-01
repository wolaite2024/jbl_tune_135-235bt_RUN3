#ifndef _TMAS_DEF_H_
#define _TMAS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "ble_audio_def.h"

#define GATT_UUID_TMAS                           0x8FD0

#define TMAS_UUID_CHAR_ROLE                      0x8FC9

#define TMAS_CG_ROLE_SUP                0x01
#define TMAS_CT_ROLE_SUP                0x02
#define TMAS_UNS_ROLE_SUP               0x04
#define TMAS_UMR_ROLE_SUP               0x08
#define TMAS_BMS_ROLE_SUP               0x10
#define TMAS_BMR_ROLE_SUP               0x20
#define TMAS_ROLE_RFU                   0xC0
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
