/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_key.c
* @brief     This file provides demo code of GPIO used as a key by edge trigger mode with hw debounce.
* @details
* @author   renee
* @date     2017-3-10
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x.h"
#include "rtl876x_bitfields.h"
#include "board.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "rtl876x_tim.h"
#include "rtl876x_nvic.h"
#include "trace.h"

/** @defgroup  GPIO_EDGE_KEY  GPIO EDGE KEY DEMO
    * @brief  Gpio key by edge trigger implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Edge_Key_Exported_Macros Gpio Edge Key Exported Macros
  * @brief
  * @{
  */
#define KEY_PIN                       ADC_0
#define KEY_IRQN                      GPIO0_IRQn
#define key_handler                   GPIO0_Handler

#define KEY_DEBOUNCE_TIME             (10)            //10ms

/** @} */ /* End of group Gpio_Edge_Key_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup Gpio_Edge_Key_Exported_Variables Gpio Edge Key Exported Variables
  * @brief
  * @{
  */

uint8_t key_status = 1;
uint8_t isPress = false;

/** @} */ /* End of group Gpio_Edge_Key_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Edge_Key_Exported_Functions Gpio Edge Key Exported Functions
  * @brief
  * @{
  */

/**
 * @brief  initialization of pinmux settings and pad settings.
 * @param   No parameter.
 * @return  void  */
void board_key_init(void)
{
    Pinmux_Config(KEY_PIN, DWGPIO);

    Pad_Config(KEY_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

/**
* @brief  GPIO and TIM peripheral initial function.
* @param   none.
* @return  void
*/
void key_init(void)
{
    /* turn on GPIO clock */
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    GPIO_InitTypeDef
    KeyboardButton_Param;     /* Define Mouse Button parameter structure. Mouse button is configed as GPIO. */
    GPIO_StructInit(&KeyboardButton_Param);
    KeyboardButton_Param.GPIO_PinBit  = GPIO_GetPin(KEY_PIN);
    KeyboardButton_Param.GPIO_Mode = GPIO_Mode_IN;
    KeyboardButton_Param.GPIO_ITCmd = ENABLE;
    KeyboardButton_Param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    KeyboardButton_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    KeyboardButton_Param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    KeyboardButton_Param.GPIO_DebounceTime = KEY_DEBOUNCE_TIME;
    GPIO_Init(&KeyboardButton_Param);

    GPIO_INTConfig(GPIO_GetPin(KEY_PIN), ENABLE);
    GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), DISABLE);

    /*  Enable GPIO0 IRQ  */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = KEY_IRQN;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  demo code of operation about GPIO key.
  * @param   No parameter.
  * @return  void
  */
void key_demo(void)
{
    /* Configure PAD and pinmux firstly! */
    board_key_init();

    /* Initialize GPIO peripheral */
    key_init();

}

/**
* @brief  GPIO Interrupt handler
* @return  void
*/
void key_handler(void)
{
    /*  Mask GPIO interrupt */
    GPIO_INTConfig(GPIO_GetPin(KEY_PIN), DISABLE);
    GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), ENABLE);
    GPIO_ClearINTPendingBit(GPIO_GetPin(KEY_PIN));

    key_status = GPIO_ReadInputDataBit(GPIO_GetPin(KEY_PIN));

    if (key_status)
    {
        APP_PRINT_INFO0(" ***TimerSwitchButtonIntrHandler: Key release\n");
        GPIO->INTPOLARITY &= ~(GPIO_GetPin(KEY_PIN));
    }
    else
    {
        APP_PRINT_INFO0(" ***TimerSwitchButtonIntrHandler: Key press\n");
        GPIO->INTPOLARITY |= GPIO_GetPin(KEY_PIN);
    }

    GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), DISABLE);
}

/** @} */ /* End of group Gpio_Edge_Key_Exported_Functions */
/** @} */ /* End of group GPIO_EDGE_KEY */

