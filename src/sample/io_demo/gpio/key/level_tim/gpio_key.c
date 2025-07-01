/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     gpio_key.c
* @brief      This file provides demo code of GPIO used as a key by level trigger mode with hw timer debounce.
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

/** @defgroup  GPIO_KEY  GPIO KEY DEMO
    * @brief  Gpio key using hw tim to debounce implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Gpio_Key_Exported_Macros Gpio Key Exported Macros
  * @brief
  * @{
  */

#define KEY_PIN                       ADC_0
#define KEY_IRQN                      GPIO0_IRQn
#define KeyIntrHandler                GPIO0_Handler

#define Switch_Button_Timer           TIM4
#define DebounceTimerIntrHandler      Timer4_Handler


#define KEY_PRESS_DEBOUNCE_TIME       (10 * 1000)            //10ms
#define KEY_RELEASE_DEBOUNCE_TIME     (20 * 1000)            //20ms

/** @} */ /* End of group Gpio_Key_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup Gpio_Key_Exported_Variables Gpio Key Exported Variables
  * @brief
  * @{
  */
uint8_t key_status = 1;
uint8_t isPress = false;

/** @} */ /* End of group Gpio_Key_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gpio_Key_Exported_Functions Gpio Key Exported Functions
  * @brief
  * @{
  */
/**
* @brief  GPIO and TIM peripheral initial function.
*
*
* @param   none.
* @return  void
*/
void key_init(void)
{
    Pinmux_Config(KEY_PIN, DWGPIO);
    Pad_Config(KEY_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    GPIO_InitTypeDef
    KeyboardButton_Param;     /* Define Mouse Button parameter structure. Mouse button is configed as GPIO. */
    GPIO_StructInit(&KeyboardButton_Param);
    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);

    KeyboardButton_Param.GPIO_PinBit  = GPIO_GetPin(KEY_PIN);
    KeyboardButton_Param.GPIO_Mode = GPIO_Mode_IN;
    KeyboardButton_Param.GPIO_ITCmd = ENABLE;
    KeyboardButton_Param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
    KeyboardButton_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    KeyboardButton_Param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_DISABLE;
    GPIO_Init(&KeyboardButton_Param);

    GPIO_INTConfig(GPIO_GetPin(KEY_PIN), ENABLE);
    GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), DISABLE);

    /*  Enable GPIO0 IRQ  */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = KEY_IRQN;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    RCC_PeriphClockCmd(APBPeriph_TIMER, APBPeriph_TIMER_CLOCK, ENABLE);
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_StructInit(&TIM_InitStruct);
    TIM_InitStruct.TIM_Period = KEY_PRESS_DEBOUNCE_TIME;
    TIM_InitStruct.TIM_Mode = 1;

    TIM_TimeBaseInit(Switch_Button_Timer, &TIM_InitStruct);

    /* enable  interrupt */
    TIM_INTConfig(Switch_Button_Timer, ENABLE);
}


/**
* @brief  GPIO interrupt trigger by button is handled in this function.
*
*
* @param
* @return  void
*/
/**
* @brief  GPIO Interrupt handler
*
*
* @return  void
*/
void KeyIntrHandler()
{
    /*  Mask GPIO interrupt */
    GPIO_INTConfig(GPIO_GetPin(KEY_PIN), DISABLE);
    GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), ENABLE);
    GPIO_ClearINTPendingBit(GPIO_GetPin(KEY_PIN));

    DBG_BUFFER(LOG_TYPE, SUBTYPE_FORMAT, MODULE_GPIO, LEVEL_INFO, "SwitchButtonIntrHandler", 0);
    key_status = GPIO_ReadInputDataBit(GPIO_GetPin(KEY_PIN));

    if (isPress == false)
    {
        TIM_ChangePeriod(Switch_Button_Timer, KEY_PRESS_DEBOUNCE_TIME);
    }
    else
    {
        TIM_ChangePeriod(Switch_Button_Timer, KEY_RELEASE_DEBOUNCE_TIME);
    }

    TIM_Cmd(Switch_Button_Timer, ENABLE);
}

/**
* @brief  When debouncing timer of key is timeout, this function shall be called.
*
*
* @param
* @return  void
*/
void DebounceTimerIntrHandler()
{

    TIM_Cmd(Switch_Button_Timer, DISABLE);
    TIM_ClearINT(Switch_Button_Timer);


    if (key_status != GPIO_ReadInputDataBit(GPIO_GetPin(KEY_PIN)))
    {
        GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), DISABLE);
        GPIO_INTConfig(GPIO_GetPin(KEY_PIN), ENABLE);
        return;
    }

    GPIO->INTPOLARITY &= ~(GPIO_GetPin(KEY_PIN));

    if (key_status)
    {
        APP_PRINT_INFO0(" ***TimerSwitchButtonIntrHandler: Key release\n");
        isPress = false;
    }
    else
    {
        APP_PRINT_INFO0(" ***TimerSwitchButtonIntrHandler: Key press\n");
        GPIO->INTPOLARITY |= GPIO_GetPin(KEY_PIN);
        isPress = true;
    }

    GPIO_MaskINTConfig(GPIO_GetPin(KEY_PIN), DISABLE);
    GPIO_INTConfig(GPIO_GetPin(KEY_PIN), ENABLE);

}

/** @} */ /* End of group Gpio_Key_Exported_Functions */
/** @} */ /* End of group GPIO_KEY */

