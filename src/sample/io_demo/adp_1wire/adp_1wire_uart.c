/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adp_1wire_uart.c
* @brief    This file provides demo code of adapter 1wire uart function.
* @details
* @author   colin
* @date     2021-12-20
* @version  v1.0
*********************************************************************************************************
*/



/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "stdlib.h"
#include "vector_table.h"
#include "hal_adp.h"
#include "trace.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_uart.h"


/** @defgroup  ADP_1WIRE_DEMO  ADP 1WIRE DEMO
    * @brief  ADP 1wire uart demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup ADP_1WIRE_Exported_Macros ADP 1wire Exported Macros
  * @brief
  * @{
  */
#define ADP_PIN         P10_0   //for rtl87x3e and rtl87x3g,rtl87x3d use LOUT_P

/** @} */ /* End of group ADP_1WIRE_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup ADP_1WIRE_Exported_Variables ADP 1WIRE Exported Variables
  * @brief
  * @{
  */
uint8_t RxBuffer[100];
uint8_t line_status = 0;

/** @} */ /* End of group ADP_1WIRE_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup ADP_1WIRE_Exported_Functions
  * @brief
  * @{
  */

/**
* @brief  UART interrupt handler function.
* @param   No parameter.
* @return  void
*/
void adp_1wire_uart_handler(void)
{
    uint32_t int_status = 0;
    uint8_t recv_len;

    if (UART_GetFlagState(UART0, UART_FLAG_RX_IDLE) == SET)
    {
        //clear Flag
        UART_INTConfig(UART0, UART_INT_IDLE, DISABLE);
        //user code here
        APP_PRINT_TRACE0("uart recv idle");
        UART_INTConfig(UART0, UART_INT_IDLE, ENABLE);
    }

    /* read interrupt id */
    int_status = UART_GetIID(UART0);

    switch (int_status)
    {
    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        recv_len = UART_GetRxFIFOLen(UART0);
        UART_ReceiveData(UART, &RxBuffer[0], recv_len);
        for (uint8_t i = 0; i < recv_len; i++)
        {
            APP_PRINT_TRACE2("uart recv reach %d %d", i, RxBuffer[i]);
        }
        APP_PRINT_TRACE0("uart recv reach");
        break;

    case UART_INT_ID_RX_TMEOUT:
        recv_len = UART_GetRxFIFOLen(UART0);
        UART_ReceiveData(UART, &RxBuffer[0], recv_len);
        for (uint8_t i = 0; i < recv_len; i++)
        {
            APP_PRINT_TRACE2("uart recv time out %d %d", i, RxBuffer[i]);
        }
        APP_PRINT_TRACE0("uart recv time out");
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        line_status = UART_GetLineStatus(UART0);
        if (UART_CheckLineStatus(line_status, UART_FLAG_BREAK_ERR))
        {
            /* adp 1wire switch to adp io function */
            UART_INTConfig(UART0, UART_INT_IDLE, DISABLE);
            UART_INTConfig(UART0, UART_INT_RD_AVA, DISABLE);
            UART_INTConfig(UART0, UART_INT_LINE_STS, DISABLE);
            adp_open(ADP_DETECT_IO);
        }
        break;

    default:
        break;
    }

    return;
}

/**
  * @brief  user adp level change callback
  * @param  event: indicate adp in or out
  * @param  user_data: user data
  * @return  void
  */
void adp_io_in_out(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_IN)
    {
        APP_PRINT_TRACE0("adp io in");
        /* adp 1wire switch to uart function */
        adp_close(ADP_DETECT_IO);

        /* must deinit first */
        UART_DeInit(UART0);

        Pinmux_Deinit(ADP_PIN);

        Pad_Config(ADP_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        Pinmux_Config(ADP_PIN, UART0_RX);

        /* turn on UART clock */
        RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);

        /* uart init */
        UART_InitTypeDef uartInitStruct;

        UART_StructInit(&uartInitStruct);

        /* 9600 */
        uartInitStruct.div = 271;
        uartInitStruct.ovsr = 10;
        uartInitStruct.ovsr_adj = 0x24A;
        uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;

        UART_Init(UART0, &uartInitStruct);

        //enable rx interrupt and line status interrupt
        UART_INTConfig(UART0, UART_INT_RD_AVA | UART_INT_LINE_STS | UART_INT_IDLE, ENABLE);

        RamVectorTableUpdate(UART0_VECTORn, (IRQ_Fun)adp_1wire_uart_handler);

        NVIC_InitTypeDef NVIC_InitStruct;
        NVIC_InitStruct.NVIC_IRQChannel = UART0_IRQn;
        NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStruct);
    }
    else
    {
        APP_PRINT_TRACE0("adp io out");
    }
}

void adp_1wire_uart_demo(void)
{

    APP_PRINT_TRACE0("adp_1wire_uart_demo");

    /* set adp io debounce time */
    adp_set_debounce_time(ADP_DETECT_IO, IO_DEBOUNCE_TIME_500MS, IO_DEBOUNCE_TIME_100MS);

    /* register user adp level change callback */
    if (!adp_register_state_change_cb(ADP_DETECT_IO, adp_io_in_out, NULL))
    {
        APP_PRINT_TRACE0("adp isr update error");
    }
}
/** @} */ /* End of group ADP_1WIRE_Exported_Functions */
/** @} */ /* End of group ADP_1WIRE_DEMO */
