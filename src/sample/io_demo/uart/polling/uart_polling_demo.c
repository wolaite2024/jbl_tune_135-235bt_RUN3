/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_polling_demo.c
* @brief    This file provides demo code of uart loop back rx data to tx by polling.
* @details
* @author   renee
* @date     2017-05-25
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_uart.h"
#include "rtl876x_rcc.h"
#include <string.h>
#include "string.h"
#include "board.h"
#include "trace.h"

/** @defgroup  UART_POLLING_DEMO  UART POLLING DEMO
    * @brief  Uart work in polling mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Polling_Exported_Macros UART Polling Exported Macros
  * @brief
  * @{
  */

#define UART_TX_PIN         P2_4
#define UART_RX_PIN         P2_5

/** @} */ /* End of group UART_Polling_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup UART_Polling_Exported_Variables UART Polling Exported Variables
  * @brief
  * @{
  */

uint8_t DemoStrBuffer[100];

/** @} */ /* End of group UART_Polling_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Polling_Exported_Functions UART Polling Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   void
  * @return  void
  */
void board_uart_init(void)
{
    Pinmux_Config(UART_TX_PIN, DATA_UART_TX);
    Pinmux_Config(UART_RX_PIN, DATA_UART_RX);

    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

/**
  * @brief  Initialize UART peripheral.
  * @param   void
  * @return  void
  */
void driver_uart_init(void)
{
    /* turn on UART clock */
    RCC_PeriphClockCmd(APBPeriph_UART, APBPeriph_UART_CLOCK, ENABLE);

    /* uart init */
    UART_InitTypeDef uartInitStruct;

    UART_StructInit(&uartInitStruct);

    /* change default rx trigger level */
    uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;

    UART_Init(UART, &uartInitStruct);
}

/**
  * @brief  demo code of operation about UART.
  * @param   void
  * @return  void
  */
void uart_polling_demo(void)
{
    uint16_t  strLen = 0;
    uint16_t  remainder = 0;
    uint16_t  blkcount = 0;
    uint16_t  i = 0;
    uint8_t   rxByte = 0;

    /* Configure PAD and pinmux firstly! */
    board_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    char *demoStr = "### Welcome to use RealTek Bumblebee ###\r\n";
    strLen = strlen(demoStr);
    memcpy(DemoStrBuffer, demoStr, strLen);

    /* send demo tips */
    blkcount = strLen / UART_TX_FIFO_SIZE;
    remainder = strLen % UART_TX_FIFO_SIZE;

    /* send block bytes(16 bytes) */
    for (i = 0; i < blkcount; i++)
    {
        UART_SendData(UART, &DemoStrBuffer[16 * i], 16);
        /* wait tx fifo empty */
        while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);
    }

    /* send left bytes */
    UART_SendData(UART, &DemoStrBuffer[16 * i], remainder);
    /* wait tx fifo empty */
    while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);

    /* loop rx and tx */
    while (1)
    {
        if (UART_GetFlagState(UART, UART_FLAG_RX_DATA_RDY) == SET)
        {
            rxByte = UART_ReceiveByte(UART);
            UART_SendByte(UART, rxByte);
        }
    }
}

/** @} */ /* End of group UART_Polling_Exported_Functions */
/** @} */ /* End of group UART_POLLING_DEMO */

