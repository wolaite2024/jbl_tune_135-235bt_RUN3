/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     main.c
* @brief    This file provides demo code of codec line in.
* @details
* @author   renee
* @date     2017-04-19
* @version  v1.0
*********************************************************************************************************
*/


/* Includes ------------------------------------------------------------------*/
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_i2s.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_audio_codec.h"
#include "rtl876x_audio_eq.h"

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_line_in_init(void)
{
    Pad_Config(AUX_R, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(AUX_L, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

/**
  * @brief  Initialize SPI3WIRE peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_i2s_init(void)
{
    RCC_PeriphCodecClockCmd(APB_SPORT0_Clock, DISABLE);
    I2S_CtrlbyMCUCmd(ENABLE);
    RCC_PeriphCodecClockCmd(APB_SPORT0_Clock, ENABLE);

    I2S_InitTypeDef I2S_InitStruct;
    I2S_InitStruct.I2S_SClock_Mi = 625;
    I2S_InitStruct.I2S_SClock_Ni = 48;
    I2S_InitStruct.I2S_mono = I2S_stereo;
    I2S_InitStruct.I2S_DataFormat = I2S_Mode;
    I2S_StructInit(&I2S_InitStruct);
    I2S_Init(I2S0, &I2S_InitStruct);

    I2S_RxCmd(I2S0, ENABLE);
}

/**
  * @brief  Initialize SPI3WIRE peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_codecinif_init(void)
{
    RCC_CodecCmd(ENABLE);

    /* Enable ASRC */
    Codec_AsrcInit();
    Codec_AsrcCmd(ENABLE);

    CodecInIf_InitTypeDef CodecInIf_InitStruct;
    CodecInIf_InitStruct.InputMode = LineIn_Mode;
    CodecInIf_InitStruct.InIf_LChannelEn = ENABLE;
    CodecInIf_InitStruct.InIf_RChannelEn = ENABLE;
    CodecInIf_InitStruct.ADC_2nd_ADJHPF_En = ENABLE;
    CodecInIf_StructInit(&CodecInIf_InitStruct);
    CodecInIf_Init(&CodecInIf_InitStruct);

    Codec_Cmd(ENABLE);
}



/**
  * @brief  Initialize GDMA peripheral for sending data.
  * @param   No parameter.
  * @return  void
  */
/* GDMA defines */
#define GDMA_CODEC_NUM                  3
#define GDMA_CODEC_CHANNEL              GDMA_Channel3
#define GDMA_CODEC_IRQn                 GDMA0_Channel3_IRQn
#define GDMA_CODEC_Handler              GDMA0_Channel3_Handler
void driver_dma_init(void)
{
    /* Enable GDMA clock */
    GDMA_DeInit();
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);

    GDMA_InitStruct.GDMA_ChannelNum          = GDMA_CODEC_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    /* Configure total transmission size */
    GDMA_InitStruct.GDMA_BufferSize          = 128;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_16;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_64;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)(&(I2S0->RX_DR));
    GDMA_InitStruct.GDMA_SourceHandshake       = GDMA_Handshake_SPORT0_RX;

    GDMA_Init(GDMA_CODEC_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(GDMA_CODEC_NUM, GDMA_INT_Transfer, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = GDMA_CODEC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
* @brief GDMA interrupt handler function for sending data.
* @param   No parameter.
* @return  void
*/
void GDMA_CODEC_Handler(void)
{
    /* Clear GDMA transfer interrupt */
    GDMA_ClearINTPendingBit(GDMA_CODEC_NUM, GDMA_INT_Transfer);
}

/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/
