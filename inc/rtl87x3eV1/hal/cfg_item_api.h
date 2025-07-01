/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     cfg_item_api.h
* @brief    This file provides api for cfg item operation.
* @details
* @author
* @date     2021-12-9
* @version  v1.0
*****************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __CFG_ITEM_API_H_
#define __CFG_ITEM_API_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "patch_header_check.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HAL_87x3e_CFG_ITEM_API System Status Api
  * @{
  */

/*============================================================================*
 *                              Variables
*============================================================================*/
typedef enum
{
    CFG_SERACH_ENTRY_SUCCESS = 0,
    CFG_SERACH_ENTRY_NOT_EXIST = 1,
    CFG_SERACH_ENTRY_SIZE_MISMATCH = 2,
    CFG_SERACH_MODULE_SIZE_MISMATCH = 3,
    CFG_SERACH_MODULE_SIG_MISMATCH = 4,
    CFG_SEARCH_ENTRY_SIZE_INVALID = 5,
} CFG_SERACH_RESULT_E;

typedef enum
{
    CFG_UPDATE_SUCCESS = 0,
    CFG_UPDATE_UNLOCK_BP_FAIL = 1,
    CFG_UPDATE_ERASE_FAIL = 2,
    CFG_UPDATE_WRITE_FAIL = 3,
    CFG_UPDATE_LOCK_BP_FAIL = 4,
} CFG_UPDATE_RESULT_E;
/*============================================================================*
 *                              Functions
*============================================================================*/
/**
 * Implementation of cfg_get_size function.
 *
 * \param p_cfg_payload   NULL: get the cfg module total length on flash OCCD_ADDRESS.
 *                        ram address: pointer to the occd cfg payload data backup on ram
 *
 * @return total cfg module length
 */
uint32_t cfg_get_size(void *p_cfg_payload);
/**
 * Implementation of cfg_backup function.
 *
 * \param address   specify the OCCD read address,default set as OCCD_ADDRESS
 * \param backup_len  specify the backup cfg data length
 *
 * @return the cfg buffer backup pointer on heap
 */
void *cfg_backup(uint32_t address, uint32_t backup_len);
/**
 * Implementation of cfg_write_to_flash function.
 *
 * \param p_new_cfg_buf   pointer to the occd cfg payload data backup on ram
 * \param backup_len      specify the cfg buffer length
 *
 * @return the cfg buffer written result
 */
bool cfg_write_to_flash(void *p_new_cfg_buf, uint32_t backup_len);
/**
 * Implementation of cfg_write_to_flash function.
 *
 * \param p_new_cfg_buf   pointer to the occd cfg payload data backup on ram
 * \param module_id       specify the cfg item module id
 * \param p_cfg_item      pointer to the cfg entry item to update
 * @return the result of updating the cfg item in p_new_cfg_buf
 */
bool cfg_update_item_in_store(uint8_t *p_new_cfg_buf, uint16_t module_id,
                              ConfigEntry_T *p_cfg_item);

/**
 * Implementation of cfg_add_item function.
 * update the cfg entry item on the occd flash
 * \param module_id       specify the cfg item module id
 * \param p_cfg_item      pointer to the cfg entry item to update
 * @return the result of updating the cfg item on the flash of OCCD
 */
bool cfg_add_item(uint16_t module_id, ConfigEntry_T *p_cfg_item);

/**
  * @brief Write MAC address to config, this is mainly used on production line.
  * @param[in] p_mac_addr  The buffer hold MAC address (6 bytes).
  * @return Write MAC to config fail or success.
  *      @retval true    Write MAC to config success.
  *      @retval false   Write MAC to config fails or not write existed MAC.
 */
bool cfg_update_mac(uint8_t *p_mac_addr);
/**
  * @brief Write 40M XTAL calibration data to config sc_xi_40m and sc_xo_40m,
  *        this is mainly used on production line.
  * @param[in] xtal               The value of 40M XTAL calibration data
  * @return Write calibration data to config fail or success.
  *     @retval true              Success.
  *     @retval false             Fail.
  */
bool cfg_update_xtal(uint8_t xtal);
/** End of HAL_87x3e_CFG_ITEM_API
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
