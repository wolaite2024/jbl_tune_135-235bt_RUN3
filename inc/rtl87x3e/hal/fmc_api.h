/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    fmc_api.h
* @brief   This file provides fmc api wrapper for sdk customer.
* @author  yuhungweng
* @date    2020-11-26
* @version v1.0
* *************************************************************************************
*/

#ifndef __FMC_API_H_
#define __FMC_API_H_

#include <stdint.h>
#include "storage.h"

typedef enum
{
    FMC_FLASH_NOR_IDX0,
    FMC_FLASH_NOR_IDX1,
    FMC_FLASH_NOR_IDX2,
    FMC_FLASH_NOR_IDX3
} FMC_FLASH_NOR_IDX_TYPE;

typedef enum
{
    FMC_FLASH_NOR_ERASE_CHIP   = 1,
    FMC_FLASH_NOR_ERASE_SECTOR = 2,
    FMC_FLASH_NOR_ERASE_BLOCK  = 4,
} FMC_FLASH_NOR_ERASE_MODE;


typedef enum
{
    PARTITION_FLASH_OCCD,
    PARTITION_FLASH_OTA_BANK_0,
    PARTITION_FLASH_OTA_BANK_1,
    PARTITION_FLASH_OTA_TMP,
    PARTITION_FLASH_BKP_DATA1,
    PARTITION_FLASH_BKP_DATA2,
    PARTITION_FLASH_FTL,
    PARTITION_FLASH_HARDFAULT_RECORD,

    PARTITION_FLASH_TOTAL,

} T_FLASH_PARTITION_NAME;

typedef enum _FLASH_IMG_ID
{
    FLASH_IMG_OTA         = 0, /* OTA header */
    FLASH_IMG_SBL         = 1,
    FLASH_IMG_MCUPATCH    = 2,
    FLASH_IMG_MCUAPP      = 3,
    FLASH_IMG_DSPPATCH    = 4,
    FLASH_IMG_DSPAPP      = 5,
    FLASH_IMG_MCUDATA     = 6,
    FLASH_IMG_DSPDATA     = 7,
    FLASH_IMG_ANC         = 8,
    FLASH_IMG_MAX,
} FLASH_IMG_ID;

#define FMC_SEC_SECTION_LEN 0x1000

typedef void (*FMC_FLASH_NOR_ASYNC_CB)(void);

/**
 * @brief           task-safe of @ref flash_nor_read
 * @param addr      the ram address mapping of nor flash going to be read
 * @param data      data buffer to be read into
 * @param len       read data length
 * @return          true if read successful, otherwise false
 */
bool fmc_flash_nor_read(uint32_t addr, void *data, uint32_t len);

/**
 * @brief           task-safe of @ref flash_nor_write
 * @param addr      the ram address mapping of nor flash going to be written
 * @param data      data buffer to be write into
 * @param len       write data length
 * @return          true if write successful, otherwise false
 */
bool fmc_flash_nor_write(uint32_t addr, void *data, uint32_t len);

/**
 * @brief           task-safe of @ref flash_nor_erase
 * @param addr      the ram address mapping of nor flash going to be erased
 * @param mode      erase mode defined as @ref FMC_FLASH_NOR_ERASE_MODE
 * @return          true if erase successful, otherwise false
 */
bool fmc_flash_nor_erase(uint32_t addr, FMC_FLASH_NOR_ERASE_MODE mode);

/**
 * @brief           task-safe nor flash auto dma read
 * @param src       the ram address mapping of nor flash going to be read from
 * @param dst       the ram address going to be written to
 * @param len       dma data length
 * @param cb        call back function which is to be executed when dma finishing @ref FMC_FLASH_NOR_ASYNC_CB
 * @return          true if trigger auto dma read successful, otherwise false
 */
bool fmc_flash_nor_auto_dma_read(uint32_t src, uint32_t dst, uint32_t len,
                                 FMC_FLASH_NOR_ASYNC_CB cb);


uint32_t flash_partition_addr_get(T_FLASH_PARTITION_NAME name);

uint32_t flash_partition_size_get(T_FLASH_PARTITION_NAME name);

uint32_t flash_cur_bank_img_payload_addr_get(FLASH_IMG_ID id);

uint32_t flash_cur_bank_img_header_addr_get(FLASH_IMG_ID id);

bool fmc_flash_nor_set_bp_lv(uint32_t addr, uint8_t bp_lv);

bool fmc_flash_nor_get_bp_lv(uint32_t addr, uint8_t *bp_lv);

/**
 * @brief           init winbond opi psram (W955D8MBYA)
 * @return          true if init successful, otherwise false
 */
bool fmc_psram_winbond_opi_init(void);

/**
 * @brief           init APMemory qpi psram (APS1604M-SQR) only supports quad mode
 * @return          true if init successful, otherwise false
 */
bool fmc_psram_ap_memory_qpi_init(void);

/**
 * @brief           init APMemory opi psram (APS6408L-OBx)
 * @return          true if init successful, otherwise false
 */
bool fmc_psram_ap_memory_opi_init(void);
bool fmc_psram_mode_switch_quad(void);

bool fmc_flash_nor_get_otp(bool *is_lock);
bool fmc_flash_nor_set_otp(bool is_lock);
bool fmc_flash_nor_write_otp(uint32_t addr, uint8_t *otp_data, uint32_t size);
bool fmc_flash_nor_erase_otp(uint32_t addr);
bool fmc_flash_nor_read_otp(uint32_t addr, uint8_t *otp_data, uint32_t size);
bool fmc_flash_nor_get_uuid(uint8_t *uuid, uint8_t size);
bool fmc_flash_nor_otp_init(void);


//bool fmc_psram_clock_switch(CLK_FREQ_TYPE clk);

#endif

