/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi2spi_dma_demo.c
* @brief    This file provides demo code of SPI0 read data and SPI1 send data by GDMA.
* @details
* @author   renee
* @date     2017-03-14
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "dma_channel.h"
#include "trace.h"
#include "rtl876x_gdma.h"
#include "rtl876x_gpio.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"

/*============================================================================*
 *                              Macros
 *============================================================================*/
#define EN_DBG                  1
#define SPI0_SCK                P1_4
#define SPI0_MOSI               P1_5
#define SPI0_MISO               P1_6
#define SPI0_CS                 P1_7

#define SPI1_SCK                P2_2
#define SPI1_MOSI               P2_3
#define SPI1_MISO               P2_4
#define SPI1_CS                 P2_5

/*============================================================================*
 *                              Variables
 *============================================================================*/
uint8_t spi2spi_dma_ch_num = 0xa5;

#define SPI2SPI_DMA_CHANNEL_NUM     spi2spi_dma_ch_num
#define SPI2SPI_DMA_CHANNEL         DMA_CH_BASE(spi2spi_dma_ch_num)
#define SPI2SPI_DMA_IRQ             DMA_CH_IRQ(spi2spi_dma_ch_num)

/*============================================================================*
 *                              Functions
 *============================================================================*/
void spi2spi_dma_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_spi_init(void)
{
    Pinmux_Config(SPI0_SCK,  SPI0_CLK_MASTER);
    Pinmux_Config(SPI0_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(SPI0_MISO, SPI0_MI_MASTER);
    Pinmux_Config(SPI0_CS,   SPI0_SS_N_0_MASTER);

    Pinmux_Config(SPI1_SCK,  SPI1_CLK_MASTER);
    Pinmux_Config(SPI1_MOSI, SPI1_MO_MASTER);
    Pinmux_Config(SPI1_MISO, SPI1_MI_MASTER);
    Pinmux_Config(SPI1_CS,   SPI1_SS_N_0_MASTER);

    Pad_Config(SPI0_SCK,  PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_CS,   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Config(SPI1_SCK,  PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_CS,   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_spi_init(void)
{
    /* turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;

    /*----------------------SPI0 init---------------------------------*/
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction           = SPI_Direction_EEPROM;
    SPI_InitStructure.SPI_Mode                = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize            = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL                = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA                = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler   = 100;
    SPI_InitStructure.SPI_FrameFormat         = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_NDF                 = 255;    //Read FrameSize + 1
    SPI_InitStructure.SPI_RxDmaEn             = ENABLE;
    SPI_InitStructure.SPI_RxWaterlevel        = 8;
    /* SPI0 init */
    SPI_Init(SPI0, &SPI_InitStructure);
    SPI_Cmd(SPI0, ENABLE);

    /*----------------------SPI1 init---------------------------------*/
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction           = SPI_Direction_TxOnly;
    SPI_InitStructure.SPI_Mode                = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize            = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL                = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA                = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_FrameFormat         = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_TxDmaEn             = ENABLE;
    SPI_InitStructure.SPI_TxWaterlevel        = 35;
    /*SPI1 init */
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_gdma_init(void)
{
    /* turn on GDMA clock */
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&spi2spi_dma_ch_num, spi2spi_dma_handler, true))
    {
        return;
    }

    /*---------------------GDMA init----------------------------------*/
    GDMA_InitStruct.GDMA_ChannelNum          = SPI2SPI_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize          = 256;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;//GDMA_DataSize_Word
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;//GDMA_DataSize_Word
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;//GDMA_Msize_8
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;//GDMA_Msize_8

    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)SPI0->DR;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)SPI1->DR;
    GDMA_InitStruct.GDMA_SourceHandshake     = GDMA_Handshake_SPI0_RX;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPI1_TX;

    GDMA_Init(SPI2SPI_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(SPI2SPI_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI2SPI_DMA_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi_gdma_demo(void)
{
    board_spi_init();

    driver_spi_init();

    driver_gdma_init();
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
void spi2spi_dma_handler(void)
{
    GDMA_ClearAllTypeINT(SPI2SPI_DMA_CHANNEL_NUM);
    GDMA_channel_release(SPI2SPI_DMA_CHANNEL_NUM);
    //GDMA_Cmd(SPI2SPI_DMA_CHANNEL_NUM, ENABLE);
}
