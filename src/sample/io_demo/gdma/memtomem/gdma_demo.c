/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gdma_demo.c
* @brief    This file provides demo code of meomory to memory transfer by GDMA.
* @details
* @author   renee
* @date     2017-01-23
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"

/** @defgroup  GDMA_DEMO  GDMA Demo
    * @brief  Gdma implementation demo code
    * @{
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup GDMA_Demo_Exported_Variables Gdma transfer Exported Variables
    * @brief
    * @{
    */

uint8_t GDMA_SendBuffer[100];
uint8_t GDMA_RecvBuffer[100];
uint8_t mem_to_mem_dma_ch_num = 0xa5;

#define DEMO_DMA_CHANNEL_NUM     mem_to_mem_dma_ch_num
#define DEMO_DMA_CHANNEL         DMA_CH_BASE(mem_to_mem_dma_ch_num)
#define DEMO_DMA_IRQ             DMA_CH_IRQ(mem_to_mem_dma_ch_num)

/** @} */ /* End of group GDMA_Demo_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GDMA_Demo_Exported_Functions Gdma transfer Exported Functions
    * @brief
    * @{
    */
void demo_dma_handler(void);
/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_mem_to_mem_dma_init(void)
{
    uint16_t i = 0;
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&mem_to_mem_dma_ch_num, demo_dma_handler, true))
    {
        return;
    }

    /*--------------initialize test buffer---------------------*/
    for (i = 0; i < 100; i++)
    {
        GDMA_SendBuffer[i] = (i & 0xff);
    }
    for (i = 0; i < 100; i++)
    {
        GDMA_RecvBuffer[i] = 0;
    }

    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = DEMO_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = 100;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)GDMA_SendBuffer;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)GDMA_RecvBuffer;
    GDMA_Init(DEMO_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = DEMO_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(DEMO_DMA_CHANNEL_NUM, GDMA_INT_Block, ENABLE);
    GDMA_Cmd(DEMO_DMA_CHANNEL_NUM, ENABLE);
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void demo_dma_handler(void)
{
    GDMA_ClearAllTypeINT(DEMO_DMA_CHANNEL_NUM);

    GDMA_Cmd(DEMO_DMA_CHANNEL_NUM, ENABLE);
}
/** @} */ /* End of group GDMA_Demo_Exported_Functions */
/** @} */ /* End of group GDMA_DEMO */

