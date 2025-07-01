/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     srv_util.h
  * @brief    Head file for server structure.
  * @details  Common data struct definition.
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef SRV_UTIL_H
#define SRV_UTIL_H

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdbool.h>
#include "gatt.h"

typedef struct
{
    uint16_t index;
    uint8_t  uuid_size;
    union
    {
        uint16_t char_uuid16;
        uint8_t char_uuid128[16];
    } uu;
} T_CHAR_UUID;

T_CHAR_UUID srv_find_char_uuid_by_index(const T_ATTRIB_APPL *p_srv, uint16_t index);
uint8_t srv_get_char_num_by_uuid(const T_ATTRIB_APPL *p_srv, T_CHAR_UUID char_uuid,
                                 uint16_t attr_num);
bool srv_find_char_index_by_uuid(const T_ATTRIB_APPL *p_srv, T_CHAR_UUID char_uuid,
                                 uint16_t attr_num, uint16_t *index);
uint8_t srv_find_char_index_all_by_uuid(const T_ATTRIB_APPL *p_srv, T_CHAR_UUID char_uuid,
                                        uint16_t attr_num, uint16_t *index_array, uint8_t array_num);


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif /* SRV_UTIL_H */
