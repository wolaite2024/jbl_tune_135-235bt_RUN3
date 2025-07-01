/**
 *****************************************************************************************
 *     Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
 *****************************************************************************************
 * @file    flash_nor_adv_driver.h
 * @brief   Nor flash advanced driver API header file
 * @author  yuhungweng
 * @date    2021-05-21
 * @version v0.1
 * ***************************************************************************************
 */

#ifndef _FLASH_NOR_ADV_DRIVER_H
#define _FLASH_NOR_ADV_DRIVER_H

#include "flash_nor_driver.h"

/****************************************************************************************
 * Nor Flash Enumeration
 ****************************************************************************************/
typedef enum
{
    FLASH_NOR_REQ_SET_BP            = 0x080,
    FLASH_NOR_REQ_SET_TB            = 0x100,
    FLASH_NOR_REQ_SET_BP_TB_MASK    = 0x180,
} FLASH_NOR_REQ_TYPE_EXT;

typedef struct
{
    uint8_t cmd_rd_uuid;
    uint8_t cmd_erase;
    uint8_t cmd_read;
    uint8_t cmd_write;
    uint8_t cmd_rd_status;
    uint8_t cmd_wr_status;
    uint8_t cmd_exit_otp;
    uint8_t cmd_enter_otp;
    uint8_t uuid_addr_size;
    uint8_t otp_addr_size;
    uint8_t otp_bit;
} FLASH_NOR_OTP_CMD;

/****************************************************************************************
 * Nor Flash Function Prototype
 ****************************************************************************************/
uint32_t flash_nor_lock_set_bp_tb_operation(FLASH_NOR_REQ_TYPE req, uint32_t addr, uint32_t len,
                                            FLASH_NOR_CMD_NODE_TYPE **cmd_node);
void flash_nor_unlock_set_bp_tb_operation(FLASH_NOR_REQ_TYPE req, uint32_t lock_flag,
                                          FLASH_NOR_CMD_NODE_TYPE **req_cmd_node);
bool flash_nor_check_is_erasing(FLASH_NOR_IDX_TYPE idx, FLASH_NOR_CMD_NODE_TYPE **cmd_node);
bool flash_nor_erase_suspend_check(uint8_t primask, FLASH_NOR_CMD_NODE_TYPE **node);
void flash_nor_erase_resume_check(uint8_t primask, FLASH_NOR_CMD_NODE_TYPE *node, bool ret);
FLASH_NOR_RET_TYPE flash_nor_get_uuid(FLASH_NOR_IDX_TYPE idx, uint8_t *uuid, uint8_t size);
FLASH_NOR_RET_TYPE flash_nor_otp_init(FLASH_NOR_IDX_TYPE idx);
FLASH_NOR_RET_TYPE flash_nor_read_otp(FLASH_NOR_IDX_TYPE idx, uint32_t addr, uint8_t *otp_data,
                                      uint8_t size);
FLASH_NOR_RET_TYPE flash_nor_erase_otp(FLASH_NOR_IDX_TYPE idx, uint32_t addr);
FLASH_NOR_RET_TYPE flash_nor_write_otp(FLASH_NOR_IDX_TYPE idx, uint32_t addr, uint8_t *otp_data,
                                       uint8_t size);
FLASH_NOR_RET_TYPE flash_nor_set_otp(FLASH_NOR_IDX_TYPE idx, bool is_lock);
FLASH_NOR_RET_TYPE flash_nor_get_otp(FLASH_NOR_IDX_TYPE idx, bool *is_lock);


#endif
