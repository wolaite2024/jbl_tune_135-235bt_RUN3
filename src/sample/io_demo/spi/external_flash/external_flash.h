/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      external_flash.h
* @brief
* @details
* @author    elliot chen
* @date      2021-07-29
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __EXTERNAL_FLASH_H
#define __EXTERNAL_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "rtl876x.h"
#include "rtl876x_spi.h"

/** @addtogroup SPI external flash
  * @brief External flash SPI module
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/

/**
  * @brief  External flash operation cmd definition
  */

typedef enum
{
    EXT_FLASH_WRITE_STATUS_CMD      = 0x01,
    EXT_FLASH_PROGRAM_CMD           = 0x02,
    EXT_FLASH_READ_CMD              = 0x03,
    EXT_FLASH_WRITE_DISABLE_CMD     = 0x04,
    EXT_FLASH_READ_STATUS_CMD       = 0x05,
    EXT_FLASH_WRITE_ENABLE_CMD      = 0x06,
    EXT_FLASH_SECTOR_ERASE_CMD      = 0x20,
    EXT_FLASH_BLOCK_ERASE_32_CMD    = 0x52,
    EXT_FLASH_CHIP_ERASE_CMD        = 0x60,
    EXT_FLASH_BLOCK_ERASE_64_CMD    = 0xd8,
    EXT_FLASH_READ_ID_CMD           = 0x9F,
} EXT_FLASH_OPERATION_CMD;

/**
  * @brief  External flash operation status definition
  */

typedef enum
{
    FLASH_OPERATION_SUCCESS                 = 0,
    FLASH_OPERATION_ERASE_FAILED            = 1,
    FLASH_OPERATION_WRITE_FAILED            = 2,
    FLASH_OPERATION_READ_FAILED             = 3,
    FLASH_OPERATION_CHECK_STATUS_FAILED     = 4,
    FLASH_OPERATION_WRITE_CMD_FAILED        = 5,
} EXT_FLASH_SPI_OPERATION_STATUS;

/**
  * @brief  External flash interrupt operation struct definition
  */

typedef struct
{
    uint8_t read_index              ;
    uint8_t write_index             ;
    uint8_t read_count              ;
    uint8_t write_count             ;
    uint8_t *program_int_buffer     ;
    uint8_t *read_int_buffer        ;
} EXT_FLASH_SPI_INT_DATA;

/**
  * @brief  External flash msg type definition
  */

typedef enum
{
    EXT_FLASH_MSG_INT_READ_FINISH,
    EXT_FLSAH_MSG_INT_PROGRAM_FINISH,
} EXT_FLASH_MSG_TYPE;

/**
  * @brief  External flash msg definition
  */

typedef struct ext_flash_msg
{
    struct ext_flash_msg *next;
    EXT_FLASH_MSG_TYPE msg;
} EXT_FLASH_MSG;

/*============================================================================*
 *                         Constants
 *============================================================================*/

/** @defgroup FLASH ID
  * @{
  */

#define GD25Q32_FLASH_ID        0xC8
#define ATMLS05154_FLASH_ID     0x1F

#define EXT_FLASH_ID            GD25Q32_FLASH_ID
/** End of group FLASH ID
  * @}
  */

/** @defgroup External flash status register bits
  * @{
  */

#define EXT_FLASH_STATUS_WIP              BIT(0)
#define EXT_FLASH_STATUS_WEL              BIT(1)
#define EXT_FLASH_STATUS_BP0              BIT(2)
#define EXT_FLASH_STATUS_BP1              BIT(3)
#define EXT_FLASH_STATUS_BP2              BIT(4)
#define EXT_FLASH_STATUS_BP3              BIT(5)
#define EXT_FLASH_STATUS_BP4              BIT(6)
#define EXT_FLASH_STATUS_SRP0             BIT(7)
#define EXT_FLASH_SECTION_PROTECT         (EXT_FLASH_STATUS_BP0 | EXT_FLASH_STATUS_BP1 |\
                                           EXT_FLASH_STATUS_BP2 | EXT_FLASH_STATUS_BP3 |\
                                           EXT_FLASH_STATUS_BP4)
/** End of group External flash status register bits
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/**
  * @brief  Read ID of flash chip.
  * @param   No parameter.
  * @return  flash id
  *
  */
uint8_t ext_flash_spi_read_id(void);

/**
  * @brief Read status register of flash.
  * @param  none.
  * @return value of status register.
  *
  */
static uint8_t ext_flash_spi_read_status(void);

/**
  * @brief Check flash is in progress or not.
  * @param  none.
  * @return Check write status
  * @retval FLASH_OPERATION_CHECK_STATUS_FAILED     Flash is always in progress
  * @retval FLASH_OPERATION_SUCCESS                 Flash is in standby mode
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_check_write_status(void);

/**
  * @brief  Send write enable command before every page program(PP),
  * sector earse(SE), block earse(BE), chip earse(CE) and write status register(WRSR) command.
  * @param  NewState                             enable or disable flash write
  * @return Check write status
  * @retval FLASH_OPERATION_WRITE_CMD_FAILED     Write timeout
  * @retval FLASH_OPERATION_SUCCESS              Write success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_write_enable_cmd(FunctionalState NewState);

/**
  * @brief  Send write status register command.
  * @param  status: data whch want to write to status register.
  * @return Check write status
  * @retval FLASH_OPERATION_WRITE_CMD_FAILED     Write timeout
  * @retval FLASH_OPERATION_SUCCESS              Write success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_write_status_register(uint8_t status);

/**
  * @brief Earse flash.
  * @param  address: address which begin to earse.
  * @param  mode: select earse mode.
  * @return Check erase status
  * @retval FLASH_OPERATION_ERASE_FAILED        Erase timeout
  * @retval FLASH_OPERATION_SUCCESS             Erase success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_erase(uint32_t address, EXT_FLASH_OPERATION_CMD mode);

/**
  * @brief Earse chip.
  * @param  none.
  * @return Check chip erase status
  * @retval FLASH_OPERATION_ERASE_FAILED        Erase chip timeout
  * @retval FLASH_OPERATION_SUCCESS             Erase chip success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_erase_chip(void);

/**
  * @brief Program flash.
  * @param  address: address which begin to write.
  * @param  psendBuf: address of data buffer which want to write.
  * @param  len: length of data buffer which want to write.
  * @return Check program status
  * @retval FLASH_OPERATION_WRITE_FAILED        Program timeout
  * @retval FLASH_OPERATION_SUCCESS             Program success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_program(uint32_t address, uint8_t *psendBuf,
                                                     uint16_t len);
/**
  * @brief Write flash by interrupt.
  * @param  address: address which begin to write.
  * @param  psendBuf: address of data buffer which want to write.
  * @param  len: length of data buffer which want to write.
  * @return Check interrupt program status
  * @retval FLASH_OPERATION_WRITE_FAILED        Last program or read not finish
  * @retval FLASH_OPERATION_SUCCESS             Program success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_program_by_interrupt(uint32_t address,
                                                                  uint8_t *psendBuf,
                                                                  uint16_t len);
/**
  * @brief Read flash.
  * @param  address: address which begin to read.
  * @param  pStoreBuf: address of data buffer which want to read.
  * @param  len: length of data buffer which want to read.
  * @return Check read status
  * @retval FLASH_OPERATION_READ_FAILED         Read timeout
  * @retval FLASH_OPERATION_SUCCESS             Read success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_read(uint32_t address, uint8_t *pStoreBuf,
                                                  uint16_t len);

/**
  * @brief Read flash by interrupt.
  * @param  address: address which begin to read.
  * @param  pStoreBuf: address of data buffer which want to read.
  * @param  len: length of data buffer which want to read.
  * @return Check read status
  * @retval FLASH_OPERATION_READ_FAILED         Last read or write not finish
  * @retval FLASH_OPERATION_SUCCESS             Read success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_read_by_interrupt(uint32_t address, uint8_t *pStoreBuf,
                                                               uint16_t len);

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_ext_flash_spi_init(void);

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_ext_flash_spi_init(void);

/**
  * @brief  Initialize Flash chip.
  * @param   No parameter.
  * @return  void
  */
void ext_flash_spi_init(void);

/**
  * @brief  demo code of operation about Flash.
  * @param  No parameter.
  * @return  void
  */
void ext_flash_spi_test_code(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__EXTERNAL_FLASH_H*/


/******************* (C) COPYRIGHT 2021 Realtek Semiconductor Corporation *****END OF FILE****/

