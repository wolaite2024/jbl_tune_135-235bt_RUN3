/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adc_hw_average_demo.c
* @brief    This file provides demo code of adc turn on hardware average function in one-shot mode.
* @details
* @author   arthur
* @date     2021-11-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_adc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "trace.h"

/** @defgroup  ADC_HW_AVERAGE_DEMO ADC Demo
    * @brief  adc implementation demo code
    * @{
    */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_adc_init(void)
{
    Pad_Config(ADC_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
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

    ADC_InitTypeDef adc_init_struct;
    ADC_StructInit(&adc_init_struct);
    adc_init_struct.schIndex[0] = EXT_SINGLE_ENDED(1);
    adc_init_struct.bitmap = 0x01;
    ADC_Init(ADC, &adc_init_struct);

    /* ADC hardware average function can only be used in one-shot mode
      and only one schedule can be used. */
    ADC_HwEvgEn(ADC, ENABLE);
    ADC_HwEvgSel(ADC, ADC_DTAT_AVG_SEL_BY32);

    ADC_INTConfig(ADC, ADC_INT_ONE_SHOT_DONE, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);
}

/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
void adc_hw_average_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_adc_init();

    /* Initialize ADC peripheral */
    driver_adc_init();
}

/**
* @brief  ADC interrupt handler function.
* @param   No parameter.
* @return  void
*/
void ADC_Handler(void)
{
    int32_t data;
    int32_t result;
    if (ADC_GetIntFlagStatus(ADC, ADC_INT_ONE_SHOT_DONE) == SET)
    {
        ADC_ClearINTPendingBit(ADC, ADC_INT_ONE_SHOT_DONE);
        data = ADC_HwEvgRead(ADC);
        result = ADC_GetRes(data, EXT_SINGLE_ENDED(1));
        APP_PRINT_INFO1("ADC Result = %d", result);
    }
}

/** @} */ /* End of group ADC_HW_AVERAGE_DEMO */

