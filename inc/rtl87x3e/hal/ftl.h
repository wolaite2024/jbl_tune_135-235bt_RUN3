/**
****************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
****************************************************************************************************
* @file      ftl.h
* @brief     flash transport layer is used as abstraction layer for user application to save read/write
*            parameters in flash.
* @note      ftl is dedicate block in flash, only used for save read/write value, and offset here is
*            logical offset which is defined for whole ftl section.If value is only for one time read,
*            refer to fs_load_app_data_8 or other APIs in flash_device.h
* @author    Brenda_li
* @date      2016-12-27
* @version   v1.0
* **************************************************************************************************
*/

#ifndef _FTL_H_
#define _FTL_H_

#include <stdint.h>
#include <stdbool.h>
#include "errno.h"

#ifdef  __cplusplus
extern  "C" {
#endif  // __cplusplus


/** @defgroup  87x3e_FTL    Flash Transport Layer
    * @brief simple implementation of file system for flash
    * @{
    */

/*============================================================================*
  *                                   Macros
  *============================================================================*/
/** @defgroup 87x3e_FTL_Exported_Macros Flash Transport Layer Exported Macros
    * @brief
    * @{
    */

/** @defgroup 87x3e_FTL_IO_CTL_CODE Flash Transport Layer ioctrl code
 * @{
 */

/**
  * @}
  */


/** End of FTL_Exported_Macros
    * @}
    */
/*============================================================================*
  *                                   Types
  *============================================================================*/


/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup 87x3e_FTL_Exported_Functions Flash Transport Layer Exported Functions
    * @brief
    * @{
    */
/**
    * @brief    Save specified value to specified ftl offset
    * @param    pdata  specify data buffer
    * @param    offset specify FTL offset to store
    * @param    size   size to store
    * @return   status
    * @retval   0  status successful
    * @retval   otherwise fail
    * @note     FTL offset is pre-defined and no confict with ROM
    */
extern uint32_t(*ftl_save_to_storage)(void *pdata_tmp, uint16_t offset, uint16_t size);

/**
    * @brief    Load specified ftl offset parameter to specified buffer
    * @param    pdata  specify data buffer
    * @param    offset specify FTL offset to load
    * @param    size   size to load
    * @return   status
    * @retval   0  status successful
    * @retval   otherwise fail
    * @note     FTL offset is pre-defined and no confict with ROM
    */
extern uint32_t(*ftl_load_from_storage)(void *pdata_tmp, uint16_t offset, uint16_t size);

/**
    * @brief    Save specified value to specified ftl offset
    * @note     FTL offset is pre-defined and no conflict. @ref FTL_Page
    * @param    pdata  specify data buffer
    * @param    offset specify FTL offset to store
    * @param    size   size to store
    * @return   status
    * @retval   0  status successful
    * @retval   otherwise fail
    */

uint32_t ftl_save(void *pdata, uint16_t offset, uint16_t size);



/**
    * @brief    Load specified ftl offset parameter to specified buffer
    * @note     FTL offset is pre-defined and no conflict. @ref FTL_Page
    * @param    pdata  specify data buffer
    * @param    offset specify FTL offset to load
    * @param    size   size to load
    * @return   status
    * @retval   0  status successful
    * @retval   otherwise fail
    */
uint32_t ftl_load(void *pdata, uint16_t offset, uint16_t size);

uint32_t ftl_init(void);

extern uint32_t ftl_partition_init(uint32_t u32PageStartAddr, uint8_t pagenum, uint32_t value_size,
                                   uint16_t *offset_min, uint16_t *offset_max);

/** @} */ /* End of group 87x3e_FTL_Exported_Functions */

/** @} */ /* End of group 87x3e_FTL */

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // _FTL_H_
