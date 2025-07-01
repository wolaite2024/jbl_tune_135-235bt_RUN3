/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    fmc_api_ext.h
* @brief   This file provides fmc ext api wrapper for sdk customer.
* @author  zola_zhang
* @date    2021-4-29
* @version v1.0
* *************************************************************************************
*/

#ifndef __FMC_API_EXT_H_
#define __FMC_API_EXT_H_

typedef enum
{
    FMC_FLASH_NOR_1_BIT_MODE,
    FMC_FLASH_NOR_2_BIT_MODE,
    FMC_FLASH_NOR_4_BIT_MODE,
    FMC_FLASH_NOR_8_BIT_MODE,
} FMC_FLASH_NOR_BIT_MODE;

typedef enum
{
    FMC_PSRAM_IDX0,
    FMC_PSRAM_IDX1,
    FMC_PSRAM_IDX2,
    FMC_PSRAM_IDX3
} FMC_PSRAM_IDX_TYPE;

typedef union _FMC_SPIC_ACCESS_INFO
{
    uint32_t d32[3];
    struct
    {
        uint32_t cmd: 16;
        uint32_t cmd_len: 2;
        uint32_t rsvd0: 14;
        uint32_t addr;
        uint32_t addr_len: 3;
        uint32_t cmd_ch: 2;
        uint32_t addr_ch: 2;
        uint32_t data_ch: 2;
        uint32_t dummy_len: 15;
        uint32_t rsvd1: 8;
    };
} FMC_SPIC_ACCESS_INFO;

typedef enum
{
    FMC_SPIC_SING_CH,
    FMC_SPIC_DUAL_CH,
    FMC_SPIC_QUAD_CH,
    FMC_SPIC_OCTAL_CH,
} FMC_SPIC_CFG_CH;

typedef enum
{
    FMC_SPIC_DEVICE_NOR_FLASH,
    FMC_SPIC_DEVICE_NAND_FLASH,
    FMC_SPIC_DEVICE_QSPI_PSRAM,
} FMC_SPIC_DEVICE_TYPE;


typedef enum
{
    FMC_SPIC_DATA_BYTE,
    FMC_SPIC_DATA_HALF,
    FMC_SPIC_DATA_WORD,
} FMC_SPIC_CFG_DATA_LEN;

bool fmc_psram_set_4bit_mode(void);
bool fmc_psram_set_seq_trans(bool enable);

bool fmc_flash_try_high_speed_mode(FMC_FLASH_NOR_IDX_TYPE idx, FMC_FLASH_NOR_BIT_MODE bit_mode);
bool fmc_flash_set_4_byte_address_mode(FMC_FLASH_NOR_IDX_TYPE idx, bool enable);
bool fmc_flash_set_seq_trans(FMC_FLASH_NOR_IDX_TYPE idx, bool enable);
bool fmc_flash_set_auto_mode(FMC_FLASH_NOR_IDX_TYPE idx, FMC_FLASH_NOR_BIT_MODE bit_mode);
uint32_t fmc_flash_nor_get_rdid(FMC_FLASH_NOR_IDX_TYPE idx);

bool fmc_psram_enter_lpm(FMC_PSRAM_IDX_TYPE idx);
bool fmc_psram_exit_lpm(FMC_PSRAM_IDX_TYPE idx);

void fmc_spic2_init(void);
bool fmc_spic2_set_max_retry(uint32_t max_retry);
uint32_t fmc_spic2_get_max_retry(void);
bool fmc_spic2_in_busy(void);
bool fmc_spic2_lock(uint8_t *lock_flag);
bool fmc_spic2_unlock(uint8_t *lock_flag);
void fmc_spic2_set_device(FMC_SPIC_DEVICE_TYPE dev);
void fmc_spic2_enable(bool enable);
void fmc_spic2_disable_interrupt(void);
void fmc_spic2_set_sipol(uint8_t sipol, bool enable);
void fmc_spic2_set_rx_mode(void);
void fmc_spic2_set_tx_mode(void);
void fmc_spic2_set_auto_mode(void);
void fmc_spic2_set_user_mode(void);
void fmc_spic2_set_rxndf(uint32_t ndf);
void fmc_spic2_set_txndf(uint32_t ndf);
void fmc_spic2_set_dr(uint32_t data, FMC_SPIC_CFG_DATA_LEN len);
uint32_t fmc_spic2_get_dr(FMC_SPIC_CFG_DATA_LEN len);
void fmc_spic2_set_cmd_len(uint8_t len);
void fmc_spic2_set_user_addr_len(uint8_t len);
void fmc_spic2_set_auto_addr_len(uint8_t len);
void fmc_spic2_set_delay_len(uint8_t delay_len);
void fmc_spic2_set_user_dummy_len(uint8_t dummy_len);
void fmc_spic2_set_auto_dummy_len(uint8_t dummy_len);
void fmc_spic2_set_baud(uint16_t baud);
uint16_t fmc_spic2_get_baud(void);
void fmc_spic2_set_multi_ch(FMC_SPIC_CFG_CH cmd, FMC_SPIC_CFG_CH addr, FMC_SPIC_CFG_CH data);
void fmc_spic2_set_seq_trans(bool enable);
void fmc_spic2_clean_valid_cmd(void);

void fmc_spic2_enable_dum_byte(bool enable);
bool fmc_spic2_cmd_rx(FMC_SPIC_ACCESS_INFO *info, uint8_t *buf, uint8_t buf_len);
bool fmc_spic2_cmd_tx(FMC_SPIC_ACCESS_INFO *info, uint8_t *buf, uint8_t buf_len);
#endif

