/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi0_hs_demo.c
* @brief    This file provides demo code of spi0 master high speed mode for rtl87x3d.
* @details
* @author   colin_lu
* @date     2021-12-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "clock_manager.h"
#include "pm.h"
/** @defgroup  SPI0_HS_DEMO  SPI0 HS Demo
    * @brief  SPI0 hs implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Macros SPI Demo Exported Macros
  * @brief
  * @{
  */

/* If use SPI0_HS, recommend to use these pin */
#define SPI0_HS_SCK                P1_3
#define SPI0_HS_MOSI               P1_4
#define SPI0_HS_MISO               P1_5
#define SPI0_HS_CS                 P1_2

/** @} */ /* End of group SPI_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup SPI_Demo_Exported_Functions SPI transfer Exported Functions
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
    /* SPI0 */
    Pinmux_Config(SPI0_HS_SCK, SPI0_CLK_MASTER);
    Pinmux_Config(SPI0_HS_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(SPI0_HS_MISO, SPI0_MI_MASTER);
    Pinmux_Config(SPI0_HS_CS, SPI0_SS_N_0_MASTER);

    Pad_Config(SPI0_HS_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(SPI0_HS_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(SPI0_HS_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(SPI0_HS_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);

}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_spi_init(void)
{
    SPI_InitTypeDef SPI_InitStruct;
    uint32_t actual_mhz;
    int32_t ret;

    /*
        If spi0_hs clock set 40M, cpu freq must greater than 120M.
        If spi0_hs clock set 20M, cpu freq must greater than 80M.
    */
    ret = pm_cpu_freq_set(120, &actual_mhz);
    APP_PRINT_TRACE2("spi hs improve cpu freq %d %d", ret, actual_mhz);

    /* If choose pll as spi0 clock source, config pll */
    ActiveMux_InitType mux_config = {.mux_enable = true};
    mux_config.mux_sel = CKO1_PLL4;
    mux_config.mux_src = CKO1_PLL4_SRC;
    mux_config.mux_output_freq = CLK_80MHZ;
    set_active_clock_mux_output(mux_config);

    /* SPI0 clock source switch to pll clock */
    RCC_SPIClkSourceSwitch(SPI0, SPI_CLOCK_SOURCE_PLL);

    /* Open SPI0 clock */
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    /* SPI0_HS only support master*/
    SPI_StructInit(&SPI_InitStruct);
    SPI_InitStruct.SPI_Direction = SPI_Direction_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_BaudRatePrescaler = 2;       /* 80 / 2 = 40M */
    SPI_InitStruct.SPI_RxThresholdLevel = 0;
    SPI_InitStruct.SPI_FrameFormat = SPI_Frame_Motorola;
    /* If spi0 clock greater than 10M, SPI_RxSampleDly need to be set 1*/
    SPI_InitStruct.SPI_RxSampleDly = 1;

    SPI_Init(SPI0_HS, &SPI_InitStruct);

    SPI_Cmd(SPI0_HS, ENABLE);

}

/**
  * @brief  demo code of operation about SPI.
  * @param   No parameter.
  * @return  void
  */
void spi0_hs_demo(void)
{
    uint8_t SPI_WriteBuf[30] = {0};
    uint8_t SPI_ReadBuf[30] = {0};
    uint8_t recv_len = 0;
    uint8_t index = 0;

    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize SPI peripheral */
    driver_spi_init();

    /*---------------send demo buffer--------------*/
    for (uint8_t i = 0; i < 30; i++)
    {
        SPI_WriteBuf[i] = i;
    }

    SPI_SendBuffer(SPI0_HS, SPI_WriteBuf, 20);

    while (SPI_GetFlagState(SPI0_HS, SPI_FLAG_RFNE) == RESET);

    while (1)
    {
        recv_len = SPI_GetRxFIFOLen(SPI0_HS);
        while (recv_len)
        {
            SPI_ReadBuf[index++] = SPI_ReceiveData(SPI0_HS);
            recv_len--;
            DBG_DIRECT("spi0 hs recv %d %d", index, SPI_ReadBuf[index - 1]);
        }
    }
}

/** @} */ /* End of group SPI_Demo_Exported_Functions */
/** @} */ /* End of group SPI_DEMO */

