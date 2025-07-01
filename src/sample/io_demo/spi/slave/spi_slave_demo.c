/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_slave_demo.c
* @brief    This file provides demo code of SPI comunication in interrupt mode as salve.
* @details
* @author   elliot chen
* @date     2015-07-03
* @version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "trace.h"

/** @defgroup  SPI_SLAVE_DEMO  SPI SLAVE DEMO
    * @brief  Spi work in slave mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Slave_Demo_Exported_Macros SPI Slave Demo Exported Macros
  * @brief
  * @{
  */

#define SPI0_SCK                MIC1_P
#define SPI0_MOSI               MIC2_P
#define SPI0_MISO               MIC1_N
#define SPI0_CS                 MIC2_N

/** @} */ /* End of group SPI_Slave_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup SPI_Slave_Demo_Exported_Functions SPI Slave Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_spi_slave_init(void)
{
    Pinmux_Config(SPI0_SCK, SPI0_CLK_SLAVE);
    Pinmux_Config(SPI0_MOSI, SPI0_MO_SLAVE);
    Pinmux_Config(SPI0_MISO, SPI0_MI_SLAVE);
    Pinmux_Config(SPI0_CS, SPI0_SS_N_0_SLAVE);

    Pad_Config(SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_spi_slave_init(void)
{
    /* turn on SPI clock */
    SPI_DeInit(SPI0);
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;

    SPI_InitStructure.SPI_RxThresholdLevel  =
        0;/* cause SPI_INT_RXF interrupt if data length in receive FIFO  >= SPI_RxThresholdLevel + 1*/
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI0, &SPI_InitStructure);
    SPI_Cmd(SPI0, ENABLE);

    /* detect receive data */
    SPI_INTConfig(SPI0, SPI_INT_RXF, ENABLE);
    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
void spi_slave_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_spi_slave_init();

    /* Initialize ADC peripheral */
    driver_spi_slave_init();
}


void SPI0_Handler(void)
{
    uint8_t len = 0;
    uint8_t idx = 0;
    uint8_t SPI_ReadINTBuf[16] = {0, 0, 0, 0};

    if (SPI_GetINTStatus(SPI0, SPI_INT_RXF) == SET)
    {
        len = SPI_GetRxFIFOLen(SPI0);
        for (idx = 0; idx < len; idx++)
        {
            /* must read all data in receive FIFO , otherwise cause SPI_INT_RXF interrupt again */
            SPI_ReadINTBuf[idx] = SPI_ReceiveData(SPI0);
            APP_PRINT_TRACE1("%x", SPI_ReadINTBuf[idx]);
        }
    }
}

/** @} */ /* End of group SPI_Slave_Demo_Exported_Functions */
/** @} */ /* End of group SPI_SLAVE_DEMO */

