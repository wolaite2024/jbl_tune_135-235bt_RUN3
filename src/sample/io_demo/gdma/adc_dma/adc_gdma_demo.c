/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adc_gdma_demo.c
* @brief        This file provides demo code of adc with gdma function in continuous mode.
* @details
* @author   renee
* @date     2017-02-22
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "string.h"
#include "trace.h"
#include "dma_channel.h"
#include "rtl876x_adc.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_uart.h"

/** @defgroup  ADC_GDMA_DEMO ADC GDMA Demo
    * @brief  Adc data read by gdma implmentaion demo code
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup ADC_GDMA_Exported_Variables Adc data transfered by gdma Variables
    * @brief
    * @{
    */
uint16_t ADC_Buffer[200];
uint8_t adc_dma_ch_num = 0xa5;

#define ADC_DMA_CHANNEL_NUM     adc_dma_ch_num
#define ADC_DMA_CHANNEL         DMA_CH_BASE(adc_dma_ch_num)
#define ADC_DMA_IRQ             DMA_CH_IRQ(adc_dma_ch_num)
/** @} */ /* End of group ADC_GDMA_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup ADC_GDMA_Exported_Functions Adc data transfered by gdma Functions
    * @brief
    * @{
    */
void adc_dma_handler(void);
/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_adc_init(void)
{
    Pad_Config(ADC_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(ADC_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(ADC_0, IDLE_MODE);
    Pinmux_Config(ADC_1, IDLE_MODE);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_adc_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

    ADC_InitTypeDef adcInitStruct;
    ADC_StructInit(&adcInitStruct);
    adcInitStruct.schIndex[0]         = EXT_SINGLE_ENDED(0);
    adcInitStruct.schIndex[1]         = EXT_SINGLE_ENDED(1);
    adcInitStruct.bitmap              = 0x03;
    adcInitStruct.adcFifoThd          = 5;
    adcInitStruct.adcClock            = ADC_CLK_1_22K;
    adcInitStruct.adcBurstSize        = 1;
    ADC_Init(ADC, &adcInitStruct);

    ADC_INTConfig(ADC, ADC_INT_FIFO_RD_ERR, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void adc_gdma_init(void)
{

    uint8_t i = 0;
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&adc_dma_ch_num, adc_dma_handler, true))
    {
        return;
    }

    /*--------------initialize test buffer which for storing data from UART---------------------*/
    for (i = 0; i < 200; i++)
    {
        ADC_Buffer[i] = 0;
    }

    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = ADC_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;//GDMA_DIR_PeripheralToMemory
    GDMA_InitStruct.GDMA_BufferSize      = 200;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)(&(ADC->FIFO));
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)ADC_Buffer;
    GDMA_InitStruct.GDMA_SourceHandshake    = GDMA_Handshake_ADC;

    GDMA_Init(ADC_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = ADC_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(ADC_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------start to receive data-----------*/
    GDMA_Cmd(ADC_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void adc_gdma_demo(void)
{
    board_adc_init();

    driver_adc_init();

    adc_gdma_init();

    ADC_Cmd(ADC, ADC_Continuous_Mode, ENABLE);
}

/**
* @brief  ADC interrupt handler function.
* @param   No parameter.
* @return  void
*/
void ADC_Handler(void)
{
    if (ADC_GetIntFlagStatus(ADC, ADC_INT_FIFO_RD_ERR) == SET)
    {
        APP_PRINT_INFO0("adc read error interrupt");
        ADC_ClearINTPendingBit(ADC, ADC_INT_FIFO_RD_ERR);
    }
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void adc_dma_handler(void)
{
    GDMA_ClearINTPendingBit(ADC_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    ADC_Cmd(ADC, ADC_Continuous_Mode, DISABLE);
    GDMA_channel_release(ADC_DMA_CHANNEL_NUM);
}

/** @} */ /* End of group ADC_GDMA_Exported_Functions */
/** @} */ /* End of group ADC_GDMA_DEMO */

