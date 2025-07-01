/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_input_demo.c
* @brief      This file provides demo code of GPIO input mode.
* @details
* @author   renee
* @date     2017-06-23
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


/** @defgroup  GPIO_INPUT_DEMO  GPIO INPUT DEMO
    * @brief  Gpio read input data implementation demo code
    * @{
    */


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup GPIO_Input_Exported_Macros Gpio Input Exported Macros
  * @brief
  * @{
  */
#define TEST_Pin            ADC_3
#define GPIO_Test_Pin       GPIO_GetPin(TEST_Pin)

/** @} */ /* End of group GPIO_Input_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GPIO_Input_Exported_Functions Gpio Input Exported Functions
  * @brief
  * @{
  */

/**
 * @brief  initialization of pinmux settings and pad settings.
 * @param   No parameter.
 * @return  void  */
void board_gpio_init(void)
{
    Pad_Config(TEST_Pin, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pinmux_Config(TEST_Pin, DWGPIO);
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
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
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
    GPIO_ReadInputData();
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
/** @} */ /* End of group GPIO_Input_Exported_Functions */
/** @} */ /* End of group GPIO_INPUT_DEMO */

