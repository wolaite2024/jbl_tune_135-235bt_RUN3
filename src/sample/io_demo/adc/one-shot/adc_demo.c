/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file     adc_demo.c
   * @brief    This file provides demo code of adc in one-shot mode.
   * @details
   * @author   renee
   * @date     2017-01-23
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>

#include "os_timer.h"
#include "trace.h"

#include "rtl876x_adc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"

/** @defgroup  ADC_DEMO ADC Demo
    * @brief  adc implementation demo code
    * @{
    */

#define ADC_TIMER_PERIOD  1000  /*unit  ms*/

void *adc_test_timer_handle = NULL;
void adc_timer_finish_handler(void *xTimer);

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

    /* High bypass resistance mode config, Please notice that the input voltage of
      adc channel using high bypass mode should not be over 0.9V */
    ADC_HighBypassCmd(1, ENABLE);
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
    adcInitStruct.schIndex[0]         = EXT_SINGLE_ENDED(0);;
    adcInitStruct.schIndex[1]         = EXT_SINGLE_ENDED(1);
    adcInitStruct.schIndex[2]         = INTERNAL_VBAT_MODE;
    adcInitStruct.schIndex[3]         = INTERNAL_VADPIN_MODE;
    adcInitStruct.bitmap              = 0x0f;
    ADC_Init(ADC, &adcInitStruct);

    ADC_INTConfig(ADC, ADC_INT_ONE_SHOT_DONE, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);
}
void adc_test_timer_init(void)
{
    /* Create adc_test_timer */
    if (adc_test_timer_handle == NULL)
    {
        if (!os_timer_create(&adc_test_timer_handle, "adc_test_timer", 1,
                             ADC_TIMER_PERIOD, true, adc_timer_finish_handler))
        {
            IO_PRINT_ERROR0("Error: adc_test_timer create Fail.");
        }
    }
}
/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
void adc_oneshot_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_adc_init();
    /* Initialize ADC timer */
    adc_test_timer_init();
    /* Initialize ADC peripheral */
    driver_adc_init();
    /* Start adc_test_timer */
    if (!os_timer_start(&adc_test_timer_handle))
    {
        IO_PRINT_ERROR0("Error: adc_test_timer Start Fail.");
    }
}

/**
* @brief  ADC interrupt handler function.
* @param   No parameter.
* @return  void
*/
void ADC_Handler(void)
{
    int32_t data[4];
    int32_t res[4];

    if (ADC_GetIntFlagStatus(ADC, ADC_INT_ONE_SHOT_DONE) == SET)
    {
        ADC_ClearINTPendingBit(ADC, ADC_INT_ONE_SHOT_DONE);
        data[0] = ADC_Read(ADC, 0);
        data[1] = ADC_Read(ADC, 1);
        data[2] = ADC_Read(ADC, 2);
        data[3] = ADC_Read(ADC, 3);
        res[0] = ADC_GetRes(data[0], EXT_SINGLE_ENDED(0));
        res[1] = ADC_GetHighBypassRes(data[1], EXT_SINGLE_ENDED(1));
        res[2] = ADC_GetRes(data[2], INTERNAL_VBAT_MODE);
        res[3] = ADC_GetRes(data[3], INTERNAL_VADPIN_MODE);

        IO_PRINT_INFO4("ADC Result[0] = %d, ADC Result[1] =%d Result[2] =%d Result[3] =%d", res[0], res[1],
                       res[2], res[3]);
    }
}


/**
 * @brief  adc timer finish handler
 * @param  xTimer: timer
 * @retval None
 */
void adc_timer_finish_handler(void *xTimer)
{
    IO_PRINT_ERROR0("adc_timer_finish_handler.");

    ADC_Cmd(ADC, ADC_One_Shot_Mode, ENABLE);
}

/** @} */ /* End of group ADC_DEMO_Exported_Functions */
/** @} */ /* End of group ADC_DEMO */
