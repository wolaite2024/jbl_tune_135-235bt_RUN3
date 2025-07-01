/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_fc_demo.c
* @brief    uart demo-- auto hardware flow control
* @details
* @author   renee
* @date     2017-07-10
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

/** @defgroup  UART_FC_DEMO  UART FLOW CONTROL DEMO
    * @brief  Uart work in hw flow control mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_FlowControl_Demo_Exported_Macros UART FlowControl Demo Exported Macros
  * @brief
  * @{
  */

#define UART_TX_PIN         P0_2
#define UART_RX_PIN         P0_3
#define UART_CTS_PIN        P0_4
#define UART_RTS_PIN        P0_5

/** @} */ /* End of group UART_FlowControl_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_FlowControl_Demo_Exported_Functions UART FlowControl Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_uart_init(void)
{
    Pinmux_Config(UART_TX_PIN, DATA_UART_TX);
    Pinmux_Config(UART_RX_PIN, DATA_UART_RX);
    Pinmux_Config(UART_CTS_PIN, DATA_UART_CTS);
    Pinmux_Config(UART_RTS_PIN, DATA_UART_RTS);
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_CTS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(UART_RTS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

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

    uartInitStruct.autoFlowCtrl = UART_AUTO_FLOW_CTRL_EN;
    UART_Init(UART, &uartInitStruct);
}

/**
  * @brief  demo code of operation about UART.
  * @param   No parameter.
  * @return  void
  */
void uart_flow_ctrl(void)
{
    uint16_t  strLen = 0;
    uint16_t  remainder = 0;
    uint16_t  blkcount = 0;
    uint16_t  i = 0;
    uint8_t   DemoStrBuffer[100];
    uint8_t   rxByte = 0;

    char *demoStr = "### Uart demo--Auto Hardware Flow Contrl ###\r\n";
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

/** @} */ /* End of group UART_FlowControl_Demo_Exported_Functions */
/** @} */ /* End of group UART_FC_DEMO */


