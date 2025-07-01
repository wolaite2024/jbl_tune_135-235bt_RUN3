/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_output_demo.c
* @brief    This file provides demo code of GPIOx  output mode whatever pin belongs to GPIOA or B.
* @details
* @author   justin kang
* @date     2020-10-10
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
#include "os_mem.h"
#include "trace.h"

typedef struct GPIO_S
{

    uint8_t pin_num;
    uint8_t gpio_num;
    uint32_t gpio_pin_bit;
    uint32_t APBPeriph;
    uint32_t APBPeriph_Clock;
    GPIO_TypeDef *gpiox;
} GPIO_S_t;

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

#define TEST_Pin            P4_3

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
void board_gpio_init(GPIO_S_t *gpio_stru)
{

    Pinmux_Config(gpio_stru->pin_num, DWGPIO);
    Pad_Config(gpio_stru->pin_num, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
}

/**
  * @brief  Initialize GPIOx peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_gpio_init(GPIO_S_t *gpio_stru)
{
    /* turn on GPIO clock */
    RCC_PeriphClockCmd(gpio_stru->APBPeriph, gpio_stru->APBPeriph_Clock, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_PinBit  = gpio_stru->gpio_pin_bit;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd = DISABLE;
    GPIOx_Init(gpio_stru->gpiox, &GPIO_InitStruct);
}

/**
  * @brief  Initialize GPIOx peripheral.
  * @param   No parameter.
  * @return  void
  */
void gpio_test(GPIO_S_t *gpio_stru)
{
    uint16_t i;
    for (i = 0; i < 10; i++)
    {
        GPIOx_SetBits(gpio_stru->gpiox, gpio_stru->gpio_pin_bit);
        GPIOx_ResetBits(gpio_stru->gpiox, gpio_stru->gpio_pin_bit);
    }
}

/**
  * @brief  demo code of operation about GPIOx.
  * @param   No parameter.
  * @return  void
  */
GPIO_S_t *gpio_stru;
void gpio_demo(void)
{
    APP_PRINT_INFO1(" ***gpio_demo sizeof(GPIO_S_t) =%d\n", sizeof(GPIO_S_t));
    gpio_stru = (GPIO_S_t *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(GPIO_S_t));
    gpio_stru->pin_num = TEST_Pin;
    gpio_stru->gpio_num = GPIO_GetNum(gpio_stru->pin_num);
    gpio_stru->gpio_pin_bit = GPIO_GetPin(gpio_stru->pin_num);
    if (gpio_stru->gpio_num < GPIO32)
    {
        gpio_stru->gpiox = GPIOA;
        gpio_stru->APBPeriph = APBPeriph_GPIOA;
        gpio_stru->APBPeriph_Clock = APBPeriph_GPIOA_CLOCK;
    }
    else
    {
        gpio_stru->gpiox = GPIOB;
        gpio_stru->APBPeriph = APBPeriph_GPIOB;
        gpio_stru->APBPeriph_Clock = APBPeriph_GPIOB_CLOCK;
    }
    /* Configure PAD and pinmux firstly! */
    board_gpio_init(gpio_stru);
    /* Initialize GPIO peripheral */
    driver_gpio_init(gpio_stru);
    /* GPIO function */
    gpio_test(gpio_stru);
}

/** @} */ /* End of group Gpio_Output_Exported_Functions */
/** @} */ /* End of group GPIO_OUTPUT_DEMO */

