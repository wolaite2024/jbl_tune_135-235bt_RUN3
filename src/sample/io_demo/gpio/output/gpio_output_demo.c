/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_output_demo.c
* @brief    This file provides demo code of GPIO output mode.
* @details
* @author   elliot chen
* @date     2015-06-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"

/** @defgroup  GPIO_OUTPUT_DEMO  GPIO OUTPUT DEMO
    * @brief  Gpio output data implementation demo code
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Output_Exported_Macros Gpio Output Exported Macros
  * @brief
  * @{
  */

#define TEST_Pin            ADC_1
#define GPIO_Test_Pin       GPIO_GetPin(TEST_Pin)

/** @} */ /* End of group Gpio_Output_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Output_Exported_Functions Gpio Output Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_gpio_init(void)
{
    Pinmux_Config(TEST_Pin, DWGPIO);

    Pad_Config(TEST_Pin, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize GPIO peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_gpio_init(void)
{
    /* turn on GPIO clock */
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_PinBit  = GPIO_Test_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd = DISABLE;

    GPIO_Init(&GPIO_InitStruct);
}

/**
  * @brief  Initialize GPIO peripheral.
  * @param   No parameter.
  * @return  void
  */
void gpio_test(void)
{
    GPIO_SetBits(GPIO_Test_Pin);
    GPIO_ResetBits(GPIO_Test_Pin);
}

/**
  * @brief  demo code of operation about GPIO.
  * @param   No parameter.
  * @return  void
  */
void gpio_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_gpio_init();

    /* Initialize GPIO peripheral */
    driver_gpio_init();

    /* GPIO function */
    gpio_test();

}

/** @} */ /* End of group Gpio_Output_Exported_Functions */
/** @} */ /* End of group GPIO_OUTPUT_DEMO */

