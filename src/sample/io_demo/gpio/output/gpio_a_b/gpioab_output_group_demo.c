/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_output_group_demo.c
* @brief    This file provides demo code of GPIOx group output mode whatever pins belong to GPIOA or B..
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
#include "trace.h"

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
const uint8_t testPinGruop[] =
{
    P0_0, P0_1, P0_2, P0_3
    , P1_4, P1_5, P1_6, P1_7,
    P2_0, P2_1, P2_2, P2_3,
    P2_4, P2_5, P2_6, P2_7,
    P4_0, P4_1,
    P4_2, P4_3,
    P4_4, P4_5, P4_6, P4_7
};

typedef struct gpio_s
{
    uint8_t pin_num;
    uint8_t gpio_num;
    uint32_t gpio_pin_bit;
    uint32_t APBPeriph;
    uint32_t APBPeriph_Clock;
    GPIO_TypeDef *gpiox;
} GPIO_S_t;

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
  * @brief  Initialize GPIO peripheral.
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
  * @brief  Initialize GPIO peripheral.
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
void gpio_group_init(uint8_t taltol_pin, uint8_t *group_pin, GPIO_S_t *gpio_stru)
{
    uint8_t i;

    for (i = 0; i < taltol_pin ; i++)
    {
        gpio_stru[i].pin_num = group_pin[i];
        gpio_stru[i].gpio_num = GPIO_GetNum(gpio_stru[i].pin_num);
        gpio_stru[i].gpio_pin_bit = GPIO_GetPin(gpio_stru[i].pin_num);
        if (gpio_stru[i].gpio_num < GPIO32)
        {
            gpio_stru[i].gpiox = GPIOA;
            gpio_stru[i].APBPeriph = APBPeriph_GPIOA;
            gpio_stru[i].APBPeriph_Clock = APBPeriph_GPIOA_CLOCK;
        }
        else
        {
            gpio_stru[i].gpiox = GPIOB;
            gpio_stru[i].APBPeriph = APBPeriph_GPIOB;
            gpio_stru[i].APBPeriph_Clock = APBPeriph_GPIOB_CLOCK;
        }
    }

}
/**
  * @brief  demo code of operation about GPIOx.
  * @param   No parameter.
  * @return  void
  */
void gpio_group_demo(void)
{
    uint8_t i = 0;
    GPIO_S_t gpio_group[sizeof(testPinGruop)];
    GPIO_S_t *p_group_gpio = 0;
    gpio_group_init(sizeof(testPinGruop), (uint8_t *)testPinGruop, gpio_group);

    for (i = 0; i < sizeof(testPinGruop); i++)
    {
        p_group_gpio = &(gpio_group[i]);
        board_gpio_init(p_group_gpio);
        /*Initialize GPIO peripheral*/
        driver_gpio_init(p_group_gpio);
    }
    /* GPIO function */
    for (i = 0; i < sizeof(testPinGruop); i++)
    {
        p_group_gpio = &(gpio_group[i]);
        gpio_test(p_group_gpio);
    }
}

/** @} */ /* End of group Gpio_Output_Exported_Functions */
/** @} */ /* End of group GPIO_OUTPUT_DEMO */

