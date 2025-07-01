/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_tx_gdma.c
* @brief    This file provides demo code of UART send data by GDMA.
* @details
* @author   renee
* @date     2017-06-11
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_uart.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"

/** @defgroup  UART_TX_DMA  UART TX GDMA DEMO
    * @brief  Uart send data by gdma implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Send_ByGDMA_Exported_Macros Uart Send by Gdma Exported Macros
  * @brief
  * @{
  */
#define UART_TX_PIN         P2_4
#define UART_RX_PIN         P2_5

/** @} */ /* End of group UART_Send_ByGDMA_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/

/** @defgroup UART_Send_ByGDMA_Exported_Variables Uart send by gdma Exported Variables
  * @brief
  * @{
  */
uint8_t uart_tx_dma_ch_num = 0xa5;

#define UART_TX_DMA_CHANNEL_NUM     uart_tx_dma_ch_num
#define UART_TX_DMA_CHANNEL         DMA_CH_BASE(uart_tx_dma_ch_num)
#define UART_TX_DMA_IRQ             DMA_CH_IRQ(uart_tx_dma_ch_num)
/** @} */ /* End of group UART_Send_ByGDMA_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Send_ByGDMA_Exported_Functions Uart Send by Gdma Exported Functions
  * @brief
  * @{
  */
void uart_tx_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_uart_init(void)
{
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(UART_TX_PIN, DATA_UART_TX);
    Pinmux_Config(UART_RX_PIN, DATA_UART_RX);
}

/**
  * @brief  Initialize UART peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_uart_init(void)
{
    /* turn on UART clock */
    RCC_PeriphClockCmd(APBPeriph_UART, APBPeriph_UART_CLOCK, ENABLE);

    /* uart init */
    UART_InitTypeDef uartInitStruct;

    UART_StructInit(&uartInitStruct);

    /* enable dma function */
    uartInitStruct.dmaEn = UART_DMA_ENABLE;
    uartInitStruct.TxDmaEn = ENABLE;
    uartInitStruct.TxWaterlevel = 15;

    UART_Init(UART, &uartInitStruct);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */

void uart_gdma_init(void)
{
    uint8_t GDMA_SendBuffer[256];
    uint8_t i = 0;
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&uart_tx_dma_ch_num, uart_tx_handler, true))
    {
        return;
    }


    /*--------------initialize test buffer which for sending data to UART---------------------*/
    for (i = 0; i < 255; i++)
    {
        GDMA_SendBuffer[i] = 0;
    }

    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = UART_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = 200;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)(GDMA_SendBuffer);
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(&(UART->RB_THR));
    GDMA_InitStruct.GDMA_DestHandshake = GDMA_Handshake_UART0_TX;

    GDMA_Init(UART_TX_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(UART_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    /*-----------------start to receive data-----------*/
    GDMA_Cmd(UART_TX_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  demo code of operation about uart send data by dma.
  * @param   No parameter.
  * @return  void
  */
void uart_dma_democode(void)
{
    board_uart_init();

    driver_uart_init();

    uart_gdma_init();
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void uart_tx_handler(void)
{
    GDMA_ClearAllTypeINT(UART_TX_DMA_CHANNEL_NUM);
    GDMA_Cmd(UART_TX_DMA_CHANNEL_NUM, ENABLE);
}

/** @} */ /* End of group UART_Send_ByGDMA_Exported_Functions */
/** @} */ /* End of group UART_TX_DMA */
