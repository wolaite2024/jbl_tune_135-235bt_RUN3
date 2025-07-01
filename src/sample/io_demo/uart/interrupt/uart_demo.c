/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_demo.c
* @brief    This file provides demo code of uart loop back rx data to tx by interrupt.
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
#include "vector_table.h"

/** @defgroup  UART_DEMO  UART DEMO
    * @brief  Uart implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Interrupt_Exported_Macros UART Interrupt Exported Macros
  * @brief
  * @{
  */

#define UART_TX_PIN         P2_4
#define UART_RX_PIN         P2_5

/** @} */ /* End of group UART_Interrupt_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup UART_Interrupt_Exported_Variables UART Interrupt Exported Variables
  * @brief
  * @{
  */

uint8_t RxBuffer[100];
uint8_t DemoStrBuffer[100];
uint8_t RxCount = 0;
uint8_t RxEndFlag = 0;
uint8_t line_status = 0;

/** @} */ /* End of group UART_Interrupt_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Interrupt_Exported_Functions UART Interrupt Exported Functions
  * @brief
  * @{
  */
void Data_Uart_Handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   void
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

    //enable rx interrupt and line status interrupt
    UART_INTConfig(UART, UART_INT_RD_AVA | UART_INT_LINE_STS | UART_INT_IDLE, ENABLE);

    RamVectorTableUpdate(UART0_VECTORn, (IRQ_Fun)Data_Uart_Handler);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = UART0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about UART.
  * @param   void
  * @return  void
  */
void uart_int_demo(void)
{
    uint16_t  strLen = 0;
    uint16_t  remainder = 0;
    uint16_t  blkcount = 0;
    uint16_t  i = 0;

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
        /* rx end */
        if (RxEndFlag == 1)
        {
            /* 16 is uart rx FIFO size */
            blkcount = RxCount / 16;
            remainder = RxCount % 16;

            /* send block bytes(16 bytes) */
            for (i = 0; i < blkcount; i++)
            {
                UART_SendData(UART, &RxBuffer[16 * i], 16);
                /* wait tx fifo empty */
                while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);
            }

            /* send left bytes */
            UART_SendData(UART, &RxBuffer[16 * i], remainder);
            /* wait tx fifo empty */
            while (UART_GetFlagState(UART, UART_FLAG_THR_TSR_EMPTY) != SET);

            /* return */
            UART_SendByte(UART, '\r');
            UART_SendByte(UART, '\n');
            while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);

            RxEndFlag = 0;
            RxCount = 0;
        }

        if (line_status != 0)
        {
            DBG_BUFFER(TYPE_BBPRO3, SUBTYPE_FORMAT, MODULE_UART, LEVEL_ERROR,
                       "line_status = %x", 1, line_status);

            line_status = 0;
        }
    }
}


/**
* @brief  UART interrupt handler function.
* @param   No parameter.
* @return  void
*/
void Data_Uart_Handler(void)
{
    uint32_t int_status = 0;
    uint8_t recv_len;

    if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
    {
        //clear Flag
        UART_INTConfig(UART, UART_INT_IDLE, DISABLE);
        RxEndFlag = true;
        //user code here

        UART_INTConfig(UART, UART_INT_IDLE, ENABLE);
    }

    /* read interrupt id */
    int_status = UART_GetIID(UART);

    switch (int_status)
    {
    /* tx fifo empty, not enable */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        recv_len = UART_GetRxFIFOLen(UART);
        UART_ReceiveData(UART, &RxBuffer[RxCount], recv_len);
        RxCount += recv_len;
        break;

    case UART_INT_ID_RX_TMEOUT:
        recv_len = UART_GetRxFIFOLen(UART);
        UART_ReceiveData(UART, &RxBuffer[RxCount], recv_len);
        RxCount += recv_len;

        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        line_status = UART->LSR;
        UART->RB_THR = line_status;

        break;

    default:
        break;
    }

    return;
}

/** @} */ /* End of group UART_Interrupt_Exported_Functions */
/** @} */ /* End of group UART_DEMO */
