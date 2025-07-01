/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     external_flash.c
* @brief    This file provides driver for flash.
* @details
* @author
* @date     2021-07-29
* @version  v1.0
*********************************************************************************************************
*/


/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "external_flash.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_gpio.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "os_queue.h"
#include "board.h"
#include "trace.h"
#include "os_mem.h"

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup External_Flash_SPI_Macros External flash data transfered by SPI Macros
    * @brief
    * @{
    */

#define SUPPORT_EXTERNAL_FLASH      1
#define FLASH_DBG                   0
#define TIME_OUT_MAX                12000000
#define TX_FIFO_SIZE                64

#define FLASH_SPI                   SPI0
#define Flash_SPI_Handler           SPI0_Handler
#define Flash_IRQChannel            SPI0_IRQn
#define FLASH_CS_Pin                GPIO_GetPin(FLASH_CS)

/** @} */ /* End of group External_Flash_SPI_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup External_Flash_SPI_Variables External flash data transfered by SPI Variables
    * @brief
    * @{
    */
EXT_FLASH_SPI_INT_DATA ext_flash_spi_int_data = {0};
T_OS_QUEUE ext_flash_msg_queue = {0};
EXT_FLASH_MSG flash_msg_elem = {0};

/** @} */ /* End of group External_Flash_SPI_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup External_Flash_SPI_Exported_Functions External flash data transfered by SPI Functions
    * @brief
    * @{
    */

#if SUPPORT_EXTERNAL_FLASH
/**
  * @brief  Clear external flash spi rx fifo.
  * @param   No parameter.
  * @return  void
  */
void ext_flash_spi_clear_rx_fifo(void)
{
    uint16_t recv_len = 0;

    recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
    while (recv_len)
    {
        SPI_ReceiveData(FLASH_SPI);
        recv_len--;
    }
}

/**
  * @brief  Read ID of flash chip.
  * @param   No parameter.
  * @return  flash id
  *
  */
uint8_t ext_flash_spi_read_id(void)
{
    uint8_t sendBuf[5] = {EXT_FLASH_READ_ID_CMD, 0, 0, 0, 0};
    uint8_t send_len = 5;
    uint8_t recvBuf[5] = {0};
    uint8_t index = 0;
    uint8_t recv_len = 0;

    /* Check SPI communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Clear receive FIFO */
    recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
    while (recv_len)
    {
        SPI_ReceiveData(FLASH_SPI);
        recv_len--;
    }
    /* Read ID of Flash */
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            recvBuf[index++] = SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }

    }
#if FLASH_DBG
    APP_PRINT_TRACE4("**** ext_flash_spi_read_id: 0x%X, 0x%X, 0x%X, 0x%X ****",
                     recvBuf[1],
                     recvBuf[2],
                     recvBuf[3],
                     recvBuf[4]
                    );
#endif
    return recvBuf[1];
}

/**
  * @brief Read status register of flash.
  * @param  none.
  * @return value of status register.
  *
  */
uint8_t ext_flash_spi_read_status(void)
{
    uint8_t sendBuf[2] = {EXT_FLASH_READ_STATUS_CMD, 0};
    uint8_t send_len = 2;
    uint8_t recvBuf[2];
    uint8_t recv_len = 0;
    uint8_t index = 0;

    /* Check SPI communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Send command of write status register */
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            recvBuf[index++] = SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    return recvBuf[1];
}

/**
  * @brief Check flash is in progress or not.
  * @param  none.
  * @return Check write status
  * @retval FLASH_OPERATION_CHECK_STATUS_FAILED     Flash is always in progress
  * @retval FLASH_OPERATION_SUCCESS                 Flash is in standby mode
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_check_write_status(void)
{
    uint8_t reg_value = 0;
    uint32_t time_out = TIME_OUT_MAX;

    do
    {
        /* Time out control */
        time_out--;
        if (time_out == 0)
        {
            return FLASH_OPERATION_CHECK_STATUS_FAILED;
        }
        reg_value = ext_flash_spi_read_status();
    }
    while (reg_value & EXT_FLASH_STATUS_WIP); /* Check flash is communicating or not */

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief  Send write enable command before every page program(PP),
  * sector earse(SE), block earse(BE), chip earse(CE) and write status register(WRSR) command.
  * @param  NewState                             enable or disable flash write
  * @return Check write status
  * @retval FLASH_OPERATION_WRITE_CMD_FAILED     Write timeout
  * @retval FLASH_OPERATION_SUCCESS              Write success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_write_enable_cmd(FunctionalState NewState)
{
    uint8_t send_len = 1;
    uint8_t recv_len = 0;

#if FLASH_DBG
    APP_PRINT_TRACE1("**** ext_flash_spi_write_enable_cmd: 0x%X ****", NewState);
#endif

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_CMD_FAILED;
    }

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Send write command */
    if (NewState == ENABLE)
    {
        /* Send write enable command */
        SPI_SendData(FLASH_SPI, EXT_FLASH_WRITE_ENABLE_CMD);
    }
    else
    {
        /* Send write disable command */
        SPI_SendData(FLASH_SPI, EXT_FLASH_WRITE_DISABLE_CMD);
    }

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_CMD_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief  Send write status register command.
  * @param  status: data whch want to write to status register.
  * @return Check write status
  * @retval FLASH_OPERATION_WRITE_CMD_FAILED     Write timeout
  * @retval FLASH_OPERATION_SUCCESS              Write success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_write_status_register(uint8_t status)
{
    uint8_t sendBuf[2] = {EXT_FLASH_WRITE_STATUS_CMD, 0};
    uint8_t send_len = 2;
    uint8_t recv_len = 0;

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Write status register */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    sendBuf[1] = status;
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Earse flash.
  * @param  address: address which begin to earse.
  * @param  mode: select earse mode.
  * @return Check erase status
  * @retval FLASH_OPERATION_ERASE_FAILED        Erase timeout
  * @retval FLASH_OPERATION_SUCCESS             Erase success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_erase(uint32_t address, EXT_FLASH_OPERATION_CMD mode)
{
    uint8_t sendBuf[4] = {0, 0, 0, 0};
    uint8_t send_len = 4;
    uint16_t recv_len = 0;

#if FLASH_DBG
    APP_PRINT_TRACE1("**** ext_flash_spi_erase: Mode - 0x%X ****", mode);
#endif
    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Write data */
    sendBuf[0] = mode;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;

    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data no matter it is useful or not */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_ERASE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Earse chip.
  * @param  none.
  * @return Check chip erase status
  * @retval FLASH_OPERATION_ERASE_FAILED        Erase chip timeout
  * @retval FLASH_OPERATION_SUCCESS             Erase chip success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_erase_chip(void)
{
    uint8_t sendBuf[4] = {0, 0, 0, 0};
    uint8_t send_len = 1;
    uint16_t recv_len = 0;

#if FLASH_DBG
    APP_PRINT_TRACE0("**** ext_flash_spi_erase_chip ****");
#endif

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Write data */
    sendBuf[0] = EXT_FLASH_CHIP_ERASE_CMD;
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data no matter it is useful or not */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_ERASE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

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
                                                     uint16_t len)
{
    uint8_t sendBuf[4];

#if FLASH_DBG
    APP_PRINT_TRACE2("**** ext_flash_spi_program: address - 0x%X, len - %d ****", address, len);
#endif

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to Tx only mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_TxOnly);

    /* Send write command */
    sendBuf[0] = EXT_FLASH_PROGRAM_CMD;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    /* Write data */
    SPI_SendBuffer(FLASH_SPI, psendBuf, len);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to full duplex mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);

    /* Clear rx fifo */
    ext_flash_spi_clear_rx_fifo();

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

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
                                                                  uint16_t len)
{
    uint8_t sendBuf[4];
    NVIC_InitTypeDef NVIC_InitStruct;

#if FLASH_DBG
    APP_PRINT_TRACE2("**** ext_flash_spi_program_by_interrupt: address - 0x%X, len - %d ****", address,
                     len);
#endif

    /* SPI will change to EEPROM mode when in reading process */
    if (ext_flash_spi_int_data.read_count || ext_flash_spi_int_data.write_count)
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to Tx only mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_TxOnly);

    /* Use GPIO as CS to prevent CS pull high when trigger TXE interrupt */
    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
    Pinmux_Config(FLASH_CS, DWGPIO);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_PinBit  = FLASH_CS_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd = DISABLE;
    GPIOx_Init(FLASH_CS_GPIO_GROUP, &GPIO_InitStruct);

    GPIOx_ResetBits(FLASH_CS_GPIO_GROUP, FLASH_CS_Pin);

    /* Send write command */
    sendBuf[0] = EXT_FLASH_PROGRAM_CMD;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    ext_flash_spi_int_data.write_count = len;
    ext_flash_spi_int_data.write_index = 0;
    ext_flash_spi_int_data.program_int_buffer = psendBuf;
    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    /* Write data */
    if (len >= TX_FIFO_SIZE)
    {
        SPI_SendBuffer(FLASH_SPI, psendBuf, TX_FIFO_SIZE);
        ext_flash_spi_int_data.write_index += TX_FIFO_SIZE;
    }
    else
    {
        SPI_SendBuffer(FLASH_SPI, psendBuf, len);
        ext_flash_spi_int_data.write_index += len;
    }

    SPI_INTConfig(FLASH_SPI, SPI_INT_TXE, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = Flash_IRQChannel;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    return FLASH_OPERATION_SUCCESS;
}

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
                                                  uint16_t len)
{
    uint32_t recvlen = 0;
    uint8_t sendBuf[4] = {EXT_FLASH_READ_CMD, 0, 0, 0};
    uint32_t time_out = TIME_OUT_MAX;

#if FLASH_DBG
    APP_PRINT_TRACE2("**** ext_flash_spi_read: address - 0x%X, len - %d ****", address, len);
#endif

    /* Read flash status register */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to EEPROM mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_EEPROM);

    /* Clear RX FIFO */
    ext_flash_spi_clear_rx_fifo();

    /* Configure length of data which you want to read */
    SPI_SetReadLen(FLASH_SPI, len);

    /* Send read command and address */
    sendBuf[0] = 0x03;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    /* Wait RX FIFO not empty flag */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_RFNE) == RESET);

    /* Read data */
    while (len)
    {
        recvlen = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recvlen)
        {
            *pStoreBuf++ = (uint8_t)SPI_ReceiveData(FLASH_SPI);
            recvlen--;
            len--;
        }
        if (recvlen == 0)
        {
            if ((--time_out) == 0)
            {
                return FLASH_OPERATION_READ_FAILED;
            }
        }
    }
    /* Switch to full duplex mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);

    return FLASH_OPERATION_SUCCESS;
}

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
                                                               uint16_t len)
{
    uint8_t sendBuf[4] = {EXT_FLASH_READ_CMD, 0, 0, 0};
    NVIC_InitTypeDef NVIC_InitStruct;

#if FLASH_DBG
    APP_PRINT_TRACE2("**** ext_flash_spi_read_by_interrupt: address - 0x%X, len - %d ****", address,
                     len);
#endif

    /* Last read finish */
    if (ext_flash_spi_int_data.read_count)
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Read flash status register */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to EEPROM mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_EEPROM);

    /* Clear RX FIFO */
    ext_flash_spi_clear_rx_fifo();

    /* Configure length of data which you want to read */
    SPI_SetReadLen(FLASH_SPI, len);

    /* Send read command and address */
    sendBuf[0] = 0x03;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    ext_flash_spi_int_data.read_int_buffer = pStoreBuf;
    ext_flash_spi_int_data.read_count = len;
    ext_flash_spi_int_data.read_index = 0;
    SPI_INTConfig(FLASH_SPI, SPI_INT_RXF, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = Flash_IRQChannel;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_ext_flash_spi_init(void)
{
    Pinmux_Config(FLASH_SCK, SPI0_CLK_MASTER);
    Pinmux_Config(FLASH_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(FLASH_MISO, SPI0_MI_MASTER);
    Pinmux_Config(FLASH_CS, SPI0_SS_N_0_MASTER);

    Pad_Config(FLASH_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(FLASH_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(FLASH_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(FLASH_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_ext_flash_spi_init(void)
{
    SPI_InitTypeDef  SPI_InitStructure;

    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_TxThresholdLevel  = 64;
    SPI_InitStructure.SPI_RxThresholdLevel  = 0;
    SPI_Init(FLASH_SPI, &SPI_InitStructure);
    SPI_Cmd(FLASH_SPI, ENABLE);
}

/**
  * @brief  Initialize Flash chip.
  * @param   No parameter.
  * @return  void
  */
void ext_flash_spi_init(void)
{
    board_ext_flash_spi_init();
    driver_ext_flash_spi_init();
    os_queue_init(&ext_flash_msg_queue);

    /* Check Flash ID */
    if (EXT_FLASH_ID != ext_flash_spi_read_id())
    {
        //Add error handle code here
        APP_PRINT_ERROR0("!!!!!!! Initialize Flash chip failed !!!!!!!");
    }
}

/**
  * @brief  demo code of operation about Flash.
  * @param  No parameter.
  * @return  void
  */
void ext_flash_spi_test_code(void)
{
    uint8_t sendBuf[256];
    uint8_t recvBuf[256];
    uint16_t index = 0;
    uint8_t status = 0;
    bool test_result = true;
    EXT_FLASH_MSG *flash_msg = NULL;


    for (index = 0; index < 200; index++)
    {
        sendBuf[index] = index + 2;
    }

    for (index = 0; index < 256; index++)
    {
        recvBuf[index] = 0;
    }

    ext_flash_spi_init();

    status = ext_flash_spi_read_status();

    if (status & EXT_FLASH_SECTION_PROTECT)
    {
        APP_PRINT_TRACE1("flash some section protect 0x%X", status);
    }

    test_result = true;
    APP_PRINT_TRACE0("==== Test Case 1 ====");
    ext_flash_spi_erase(0, EXT_FLASH_BLOCK_ERASE_32_CMD);
    ext_flash_spi_program(0, sendBuf, 200);
    ext_flash_spi_read(0, recvBuf, 200);

    for (uint8_t dbg_index = 0; dbg_index < 200; dbg_index++)
    {
        if (sendBuf[dbg_index] != recvBuf[dbg_index])
        {
            APP_PRINT_ERROR1("!!! External_Flash_TestCode Failed, dbg_index = %d!!!!", dbg_index);
            test_result = false;
            break;
        }
    }
    if (test_result)
    {
        APP_PRINT_TRACE0("==== Test Result: Pass ====");
    }
    else
    {
        APP_PRINT_TRACE0("==== Test Result: Fail ====");
    }

    test_result = true;
    APP_PRINT_TRACE0("==== Test Case 2 ====");
    for (index = 0; index < 256; index++)
    {
        sendBuf[index] = 0xAA;
    }

    for (index = 0; index < 256; index++)
    {
        recvBuf[index] = 0;
    }

    ext_flash_spi_erase(0, EXT_FLASH_SECTOR_ERASE_CMD);
    ext_flash_spi_program(0, sendBuf, 200);
    ext_flash_spi_read(0, recvBuf, 200);
    for (uint8_t dbg_index = 0; dbg_index < 200; dbg_index++)
    {
        if (sendBuf[dbg_index] != recvBuf[dbg_index])
        {
            APP_PRINT_ERROR1("!!! External_Flash_TestCode Failed, dbg_index = %d!!!!", dbg_index);
            test_result = false;
            break;
        }
    }

    if (test_result)
    {
        APP_PRINT_TRACE0("==== Test Result: Pass ====");
    }
    else
    {
        APP_PRINT_TRACE0("==== Test Result: Fail ====");
    }

    test_result = true;
    APP_PRINT_TRACE0("==== Test Case 3 ====");
    for (index = 0; index < 200; index++)
    {
        sendBuf[index] = index + 3;
    }

    for (index = 0; index < 256; index++)
    {
        recvBuf[index] = 0;
    }

    ext_flash_spi_erase(0, EXT_FLASH_BLOCK_ERASE_32_CMD);
    ext_flash_spi_program_by_interrupt(0, sendBuf, 200);
    while (!ext_flash_msg_queue.count);
    flash_msg = os_queue_out(&ext_flash_msg_queue);
    if (flash_msg->msg == EXT_FLSAH_MSG_INT_PROGRAM_FINISH)
    {
        APP_PRINT_TRACE0("External_Flash_INT_Program finish");
    }

    ext_flash_spi_read_by_interrupt(0, recvBuf, 200);
    while (!ext_flash_msg_queue.count);
    flash_msg = os_queue_out(&ext_flash_msg_queue);
    if (flash_msg->msg == EXT_FLASH_MSG_INT_READ_FINISH)
    {
        APP_PRINT_TRACE0("External_Flash_INT_Read finish");
    }

    for (uint8_t dbg_index = 0; dbg_index < 200; dbg_index++)
    {
        if (sendBuf[dbg_index] != recvBuf[dbg_index])
        {
            APP_PRINT_ERROR1("!!! External_Flash_TestCode Failed, dbg_index = %d!!!!", dbg_index);
            test_result = false;
            break;
        }
    }
    if (test_result)
    {
        APP_PRINT_TRACE0("==== Test Result: Pass ====");
    }
    else
    {
        APP_PRINT_TRACE0("==== Test Result: Fail ====");
    }

    test_result = true;
    APP_PRINT_TRACE0("==== Test Case 4 ====");
    ext_flash_spi_erase_chip();
    ext_flash_spi_read(0, recvBuf, 200);

    for (uint8_t dbg_index = 0; dbg_index < 200; dbg_index++)
    {
        if (0xFF != recvBuf[dbg_index])
        {
            APP_PRINT_ERROR1("!!! External_Flash_Chip erase Failed, dbg_index = %d!!!!", dbg_index);
            test_result = false;
            break;
        }
    }

    if (test_result)
    {
        APP_PRINT_TRACE0("==== Test Result: Pass ====");
    }
    else
    {
        APP_PRINT_TRACE0("==== Test Result: Fail ====");
    }

}

/**
* @brief  SPI interrupt handler function.
* @param   No parameter.
* @return  void
*/
void Flash_SPI_Handler(void)
{
    uint8_t idx = 0;
    uint16_t recv_len = 0;

    if (SPI_GetINTStatus(FLASH_SPI, SPI_INT_RXF) == SET)
    {
        SPI_INTConfig(FLASH_SPI, SPI_INT_RXF, DISABLE);
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        for (idx = 0; idx < recv_len; idx++)
        {
            /* must read all data in receive FIFO , otherwise cause SPI_INT_RXF interrupt again */
            if (ext_flash_spi_int_data.read_index < ext_flash_spi_int_data.read_count)
            {
                ext_flash_spi_int_data.read_int_buffer[ext_flash_spi_int_data.read_index] = SPI_ReceiveData(
                                                                                                FLASH_SPI);
                ext_flash_spi_int_data.read_index++;
            }
            else
            {
                SPI_ReceiveData(FLASH_SPI);
            }
        }
        if (ext_flash_spi_int_data.read_index == ext_flash_spi_int_data.read_count)
        {
            /* Send read finish msg */
            flash_msg_elem.msg = EXT_FLASH_MSG_INT_READ_FINISH;
            os_queue_in(&ext_flash_msg_queue, &flash_msg_elem);
            ext_flash_spi_int_data.read_count = 0;
            /* Switch to full duplex mode */
            SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);
        }
        else
        {
            SPI_INTConfig(FLASH_SPI, SPI_INT_RXF, ENABLE);
        }
    }

    if (SPI_GetINTStatus(FLASH_SPI, SPI_INT_TXE) == SET)
    {
        SPI_INTConfig(FLASH_SPI, SPI_INT_TXE, DISABLE);

        if (ext_flash_spi_int_data.write_index >= ext_flash_spi_int_data.write_count)
        {
            /* Send program finish msg */
            flash_msg_elem.msg = EXT_FLSAH_MSG_INT_PROGRAM_FINISH;
            os_queue_in(&ext_flash_msg_queue, &flash_msg_elem);
            ext_flash_spi_int_data.write_count = 0;

            /* Must wait for SPI free before change CS pin */
            while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
            Pinmux_Config(FLASH_CS, SPI0_SS_N_0_MASTER);
            SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);
            /* Clear rx fifo */
            ext_flash_spi_clear_rx_fifo();
        }
        else
        {
            if ((ext_flash_spi_int_data.write_count - ext_flash_spi_int_data.write_index) >= TX_FIFO_SIZE)
            {
                SPI_SendBuffer(FLASH_SPI, &
                               (ext_flash_spi_int_data.program_int_buffer[ext_flash_spi_int_data.write_index]), TX_FIFO_SIZE);
                ext_flash_spi_int_data.write_index += TX_FIFO_SIZE;
            }
            else
            {
                SPI_SendBuffer(FLASH_SPI, &
                               (ext_flash_spi_int_data.program_int_buffer[ext_flash_spi_int_data.write_index]),
                               ext_flash_spi_int_data.write_count - ext_flash_spi_int_data.write_index);
                ext_flash_spi_int_data.write_index = ext_flash_spi_int_data.write_count;
            }
            SPI_INTConfig(FLASH_SPI, SPI_INT_TXE, ENABLE);
        }
    }
}
/** @} */ /* End of group External_Flash_SPI_Exported_Functions */
#endif

/******************* (C) COPYRIGHT 2021 Realtek Semiconductor Corporation *****END OF FILE****/

