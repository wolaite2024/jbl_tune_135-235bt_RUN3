/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_pinmux.h
* @brief
* @details
* @author
* @date      2021-4-29
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _IO_DEBUG_
#define _IO_DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl876x.h"

/** @addtogroup IO_DEBUG IO_DEBUG
  * @brief IO debug function module
  * @{
  */
typedef struct
{
    uint8_t mode: 1;
    uint8_t in_value: 1;
    uint8_t out_value: 1;
    uint8_t int_en: 1;
    uint8_t int_en_mask: 1;
    uint8_t int_polarty: 1;
    uint8_t int_type: 1;
    uint8_t int_type_edg_both: 1;
    uint8_t int_status: 1;
    uint8_t debounce: 1;

} T_GPIO_SETTING;

/**
  * @brief  Set all the pad to shut down mode, for power saving bubug only
  * @param  None.
  * @retval  None.
  */
void pad_set_all_shut_down(void);

/**
  * @brief  Dump the pad setting of the specific pin, for io pad bubug only
  * @param  pin_num: the pin function to dump.
  * @retval  None.
  */
int32_t pad_print_setting(uint8_t pin_num);

/**
  * @brief  dump all the pad setting
  * @param  None.
  * @retval  None.
  */
void pad_print_all_pin_setting(void);

/**
  * @brief  Get the key name by the key_mask
  * @param  key_mask: the specific key mask.
  * @retval  None.
  */
const char *key_get_name(uint8_t key_mask);

/**
  * @brief  Convert the key status string
  * @param  active: the polarity of key active
  * @param  key_status: the current key status
  * @retval  None.
  */
const char *key_get_stat_str(uint32_t active, uint8_t key_status);

/**
  * @brief  Dump the gpio setting of the specific pin, for io pad bubug only
  * @param  pin_num: the pin function to dump.
  * @retval  None.
  */
void gpio_print_pin_setting(uint8_t pin_num);

#ifdef __cplusplus
}
#endif

#endif /* _IO_DEBUG_ */

/** @} */ /* End of group IO_DEBUG */


/******************* (C) COPYRIGHT 2021 Realtek Semiconductor *****END OF FILE****/

