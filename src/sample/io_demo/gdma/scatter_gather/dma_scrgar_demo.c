/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dma_scrgar_demo.c
* @brief    This file provides demo code of meomory to memory transfer by Scatter/Gather GDMA.
* @details
* @author   renee
* @date     2017-02-17
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

/** @defgroup  GDMA_SCRGAR_DEMO  GDMA SARGAR Demo
    * @brief  Gdma scatter/gather transfer implementation demo code
    * @{
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup GDMA_SCRGAR_Exported_Variables Gdma Scatter/Gather transfer Variables
  * @brief
  * @{
  */

uint8_t GDMA_SendBuffer_5[256];
uint8_t GDMA_RecvBuffer_5[256];
uint8_t scatter_dma_ch_num = 0xa5;

#define SCATTER_DMA_CHANNEL_NUM     scatter_dma_ch_num
#define SCATTER_DMA_CHANNEL         DMA_CH_BASE(scatter_dma_ch_num)
#define SCATTER_DMA_IRQ             DMA_CH_IRQ(scatter_dma_ch_num)
/** @} */ /* End of group GDMA_SCRGAR_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GDMA_SCRGAR_Exported_Functions Gdma Scatter/Gather transfer Functions
  * @brief
  * @{
  */
void scatter_dma_handler(void);

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_dma_scr_init(void)
{
    uint16_t i = 0;
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&scatter_dma_ch_num, scatter_dma_handler, true))
    {
        return;
    }

    for (i = 0; i < 256; i++)
    {
        GDMA_SendBuffer_5[i] = i;
    }
    for (i = 0; i < 256; i++)
    {
        GDMA_RecvBuffer_5[i] = 0;
    }

    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SCATTER_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = 100;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)GDMA_SendBuffer_5;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)GDMA_RecvBuffer_5;
    GDMA_InitStruct.GDMA_Scatter_En = ENABLE;
    GDMA_InitStruct.GDMA_ScatterCount = 4;
    GDMA_InitStruct.GDMA_ScatterInterval = 4;
    GDMA_InitStruct.GDMA_Gather_En = DISABLE;
    GDMA_InitStruct.GDMA_GatherCount = 4;
    GDMA_InitStruct.GDMA_GatherInterval = 4;

    /*  Enable GDMA IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SCATTER_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_Init(SCATTER_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SCATTER_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_Cmd(SCATTER_DMA_CHANNEL_NUM, ENABLE);
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void scatter_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SCATTER_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    GDMA_Cmd(SCATTER_DMA_CHANNEL_NUM, DISABLE);
    for (uint8_t i = 0; i < 128; i++)
    {
        GDMA_SendBuffer_5[i] = i;
        GDMA_RecvBuffer_5[i] = 0;
    }
    GDMA_SetSourceAddress(SCATTER_DMA_CHANNEL, (uint32_t)GDMA_SendBuffer_5);
    GDMA_SetDestinationAddress(SCATTER_DMA_CHANNEL, (uint32_t)GDMA_RecvBuffer_5);
    GDMA_Cmd(SCATTER_DMA_CHANNEL_NUM, ENABLE);
}
/** @} */ /* End of group GDMA_SCRGAR_Exported_Functions */
/** @} */ /* End of group GDMA_SCRGAR_DEMO */
