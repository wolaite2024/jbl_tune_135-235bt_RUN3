/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_switchcs_demo.c
* @brief    This file provides demo code of SPI comunication when Dynamic switching cs signal.
            Attention: only SPI1 have this function.
* @details
* @author   elliot chen
* @date     2015-10-08
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"

/** @defgroup  SPI_SWITCH_CS_DEMO  SPI DYNAMIC SWITCH CS DEMO
    * @brief  Spi switch cs implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Switch_CS_Demo_Exported_Macros SPI Switch CS Demo Exported Macros
  * @brief
  * @{
  */

#define SPI1_SCK                P1_0
#define SPI1_MOSI               P1_1
#define SPI1_MISO               P1_2
#define SPI1_CS0                P1_3
#define SPI1_CS1                P1_4
#define SPI1_CS2                P1_5

/** @} */ /* End of group SPI_Switch_CS_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SPI_Switch_CS_Demo_Exported_Functions SPI Switch CS Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_spi_init(void)
{
    Pinmux_Config(SPI1_SCK, SPI1_CLK_MASTER);
    Pinmux_Config(SPI1_MOSI, SPI1_MO_MASTER);
    Pinmux_Config(SPI1_MISO, SPI1_MI_MASTER);
    Pinmux_Config(SPI1_CS0, SPI1_SS_N_0_MASTER);
    Pinmux_Config(SPI1_CS1, SPI1_SS_N_1_MASTER);
    Pinmux_Config(SPI1_CS2, SPI1_SS_N_2_MASTER);

    Pad_Config(SPI1_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_CS0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_CS1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_CS2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_spi_init(void)
{
    /* turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_RxThresholdLevel  = 0;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}

/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi_swcs_demo(void)
{
    uint8_t SPI_WriteBuf[3] = {0x01, 0x02, 0x03};

    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize ADC peripheral */
    driver_spi_init();

    SPI_SendBuffer(SPI1, SPI_WriteBuf, 3);
    /* wait Tx FIFO empty */
    while (SPI_GetFlagState(SPI1, SPI_FLAG_TFE) == RESET);
    while (SPI_GetFlagState(SPI1, SPI_FLAG_BUSY));

    /* switch to CS1 signal */
    SPI_SetCSNumber(SPI1, 1);

    SPI_SendBuffer(SPI1, SPI_WriteBuf, 3);
    /* wait Tx FIFO empty */
    while (SPI_GetFlagState(SPI1, SPI_FLAG_TFE) == RESET);
    while (SPI_GetFlagState(SPI1, SPI_FLAG_BUSY));

    /* switch to CS2 signal */
    SPI_SetCSNumber(SPI1, 2);

    SPI_SendBuffer(SPI1, SPI_WriteBuf, 3);
    /* wait Tx FIFO empty */
    while (SPI_GetFlagState(SPI1, SPI_FLAG_TFE) == RESET);
    while (SPI_GetFlagState(SPI1, SPI_FLAG_BUSY));
}

/** @} */ /* End of group SPI_Switch_CS_Demo_Exported_Functions */
/** @} */ /* End of group SPI_SWITCH_CS_DEMO */

