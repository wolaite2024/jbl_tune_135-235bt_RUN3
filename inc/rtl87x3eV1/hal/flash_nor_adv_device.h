/**
 *****************************************************************************************
 *     Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
 *****************************************************************************************
 * @file    flash_nor_adv_device.h
 * @brief   Nor flash advanced device API header file
 * @author  yuhungweng
 * @date    2021-05-21
 * @version v0.1
 * ***************************************************************************************
 */

#ifndef _FLASH_NOR_ADV_DEVICE_H
#define _FLASH_NOR_ADV_DEVICE_H

#include "flash_nor_adv_driver.h"

typedef enum
{
    FLASH_NOR_RET_TB_BIT_IS_OTP = 23,
} FLASH_NOR_RET_TYPE_EXT;

/****************************************************************************************
 * Nor Flash Function Prototype
 ****************************************************************************************/
/**
* @brief               task-safe nor flash set bp lv
* @param idx           specific nor flash
* @param bp_lv         bp level to be set
* @return              @ref FLASH_NOR_RET_TYPE result
*/
FLASH_NOR_RET_TYPE flash_nor_set_bp_lv_locked(FLASH_NOR_IDX_TYPE idx, uint8_t bp_lv);

/**
 * @brief               task-safe nor flash set top bottom bit
 * @param idx           specific nor flash
 * @param from_bottom   tb bit to be set
 * @return              @ref FLASH_NOR_RET_TYPE result
 */
FLASH_NOR_RET_TYPE flash_nor_set_tb_bit_locked(FLASH_NOR_IDX_TYPE idx, bool from_bottom);

FLASH_NOR_RET_TYPE flash_nor_get_uuid_locked(FLASH_NOR_IDX_TYPE idx, uint8_t *uuid, uint8_t size);
FLASH_NOR_RET_TYPE flash_nor_read_otp_locked(FLASH_NOR_IDX_TYPE idx, uint32_t addr,
                                             uint8_t *otp_data, uint8_t size);
FLASH_NOR_RET_TYPE flash_nor_erase_otp_locked(FLASH_NOR_IDX_TYPE idx, uint32_t addr);
FLASH_NOR_RET_TYPE flash_nor_write_otp_locked(FLASH_NOR_IDX_TYPE idx, uint32_t addr,
                                              uint8_t *otp_data, uint8_t size);
FLASH_NOR_RET_TYPE flash_nor_set_otp_locked(FLASH_NOR_IDX_TYPE idx, bool is_lock);
FLASH_NOR_RET_TYPE flash_nor_get_otp_locked(FLASH_NOR_IDX_TYPE idx, bool *is_lock);

#endif
