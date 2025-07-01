/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     uart_dma_demo.c
* @brief    uart + gdma rx demo--interrupt, save data in a loop buffer
* @details
* @author   renee
* @date     2017-02-17
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "string.h"
#include "trace.h"
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_uart.h"
#include "ring_buffer.h"
#include "vector_table.h"

/** @defgroup  UART_RX_DMA  UART RX GDMA DEMO
    * @brief  Uart receive data by gdma implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup UART_Receive_ByGDMA_Exported_Macros Uart receive by gdma Exported Macros
  * @brief
  * @{
  */

#define UART_BUFFER_SIZE        1024
#define UART_TX_PIN             P2_4
#define UART_RX_PIN             P2_5

#define UART_RX_DMA_CHANNEL_NUM              uart_rx_dma_ch_num
#define UART_RX_DMA_CHANNEL                  DMA_CH_BASE(uart_rx_dma_ch_num)
#define UART_RX_DMA_IRQ                      DMA_CH_IRQ(uart_rx_dma_ch_num)

/** @} */ /* End of group UART_Receive_ByGDMA_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/

/** @defgroup UART_Receive_ByGDMA_Exported_Variables Uart receive by gdma Exported Variables
  * @brief
  * @{
  */
/* uart received data */
uint8_t uart_receive_buf[UART_BUFFER_SIZE];
uint16_t uart_receive_count = 0;

/* uart dma channel */
uint8_t uart_rx_dma_ch_num = 0xa5;

/** @} */ /* End of group UART_Receive_ByGDMA_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup UART_Receive_ByGDMA_Exported_Functions Uart receive by gdma Exported Functions
  * @brief
  * @{
  */
void Data_Uart_Handler(void);
void uart_rx_dma_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_dma_uart_init(void)
{
    Pad_Config(UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(UART_TX_PIN, UART0_TX);
    Pinmux_Config(UART_RX_PIN, UART0_RX);
}

/**
  * @brief  Initialize UART peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_uart_init(void)
{
    UART_DeInit(UART0);
    RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
    /* uart init */
    UART_InitTypeDef uartInitStruct;
    UART_StructInit(&uartInitStruct);

    /* change default rx trigger level */
    uartInitStruct.rxTriggerLevel = 29;
    /* Config the baudrate is 3M */
    uartInitStruct.div            = 1;
    uartInitStruct.ovsr           = 8;
    uartInitStruct.ovsr_adj       = 0x492;
    uartInitStruct.dmaEn = UART_DMA_ENABLE;
    uartInitStruct.RxWaterlevel = 1;
    uartInitStruct.RxDmaEn        = ENABLE;
    UART_Init(UART0, &uartInitStruct);

    //enable rx interrupt and line status interrupt
    UART_INTConfig(UART0, UART_INT_LINE_STS | UART_INT_IDLE, ENABLE);

    RamVectorTableUpdate(UART0_VECTORn, (IRQ_Fun)Data_Uart_Handler);

    /*  Enable UART IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART0_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_gdma_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    GDMA_InitTypeDef GDMA_InitStruct;

    /* get free gdma channel */
    if (!GDMA_channel_request(&uart_rx_dma_ch_num, uart_rx_dma_handler, true))
    {
        return;
    }

    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = UART_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = 128;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)(&(UART0->RB_THR));
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(uart_receive_buf);
    GDMA_InitStruct.GDMA_SourceHandshake    = GDMA_Handshake_UART0_RX;

    GDMA_Init(UART_RX_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(UART_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------start to receive data-----------*/
    GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  uart send string buffer.
  * @param   No parameter.
  * @return  void
  */
static void uart_send_buf(uint8_t *str, uint16_t str_len)
{
    uint8_t blk, remain, i;
    blk = str_len / UART_TX_FIFO_SIZE;
    remain = str_len % UART_TX_FIFO_SIZE;

    //send through uart
    for (i = 0; i < blk; i++)
    {
        UART_SendData(UART, (uint8_t *)&str[16 * i], 16);
        while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
    }

    UART_SendData(UART, (uint8_t *)&str[16 * i], remain);
    while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
}

/**
  * @brief  demo code of operation about PWM.
  * @param   No parameter.
  * @return  void
  */
void uart_rx_gdma_demo(void)
{
    char *demo_str = "### Welcome to use RealTek Bumblebee ###\r\n";

    /* Pinmux & Pad Config */
    board_dma_uart_init();

    /* Initialize UART peripheral */
    driver_uart_init();

    /* Initialize GDMA peripheral */
    driver_gdma_init();

    /* Send Demo string buffer */
    uart_send_buf((uint8_t *)demo_str, strlen(demo_str));

    /* Send back recieved data */
    while (1)
    {
        if (uart_receive_count)
        {
            uart_send_buf(uart_receive_buf, uart_receive_count);
            uart_receive_count = 0;
        }
    }
}

/**
* @brief  Data Uart interrupt handler function.
* @param   No parameter.
* @return  void
*/
void Data_Uart_Handler(void)
{
    uint32_t int_status = UART_GetIID(UART0);

    /* disable interrupt */
    if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
    {
        //clear Flag
        UART_INTConfig(UART, UART_INT_IDLE, DISABLE);

        if (GDMA_GetChannelStatus(UART_RX_DMA_CHANNEL_NUM))
            /*suspend at first to let DMA pop the fifo data*/
        {
            GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, ENABLE);
        }
        /*should waiting or fifo_count will not right when rx one byte or three byte*/
        while (GDMA_GetFIFOStatus(UART_RX_DMA_CHANNEL) != SET);

        /*read DMA total transfer count*/
        uart_receive_count = GDMA_GetTransferLen(UART_RX_DMA_CHANNEL);

        /*abort DMA */
        GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, DISABLE);

        /* restart dma receive */
        GDMA_SetDestinationAddress(UART_RX_DMA_CHANNEL, (uint32_t)uart_receive_buf);
        GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, DISABLE);
        GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, ENABLE);

        UART_INTConfig(UART0, UART_INT_IDLE, ENABLE);
    }

    switch (int_status)
    {

    /* tx fifo empty, not enable */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        break;

    case UART_INT_ID_RX_TMEOUT:
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        {
            UART_INTConfig(UART0, UART_INT_LINE_STS, DISABLE);
            APP_PRINT_ERROR1("Line status error!!!!0x%x\n", UART->LSR);
            UART_INTConfig(UART0, UART_INT_LINE_STS, ENABLE);
        }
        break;

    default:
        break;
    }

    return;
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void uart_rx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(UART_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    GDMA_SetDestinationAddress(UART_RX_DMA_CHANNEL, (uint32_t)uart_receive_buf);
    GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, ENABLE);
}

/** @} */ /* End of group UART_Receive_ByGDMA_Exported_Functions */
/** @} */ /* End of group UART_RX_DMA */
