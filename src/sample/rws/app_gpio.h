/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_GPIO_H_
#define _APP_GPIO_H_

#include <stdbool.h>
#include "app_msg.h"
#include "rtl876x.h"
#include "rtl876x_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_GPIO App Gpio
  * @brief App GPIO
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_GPIO_Exported_Macros App Gpio Macros
   * @{
   */
#define TOTAL_PINMUX_NUM        (P4_1 + 1) //39

#define GPIO_INIT_DEBOUNCE_TIME     10
#define GPIO_DETECT_DEBOUNCE_TIME   10
#define GPIO_RELEASE_DEBOUNCE_TIME  10
/** End of APP_GPIO_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_GPIO_Exported_Functions App Gpio Functions
    * @{
    */

/**
    * @brief  Initializes the GPIO A and B peripheral according to the specified
    *         parameters in the GPIO_InitStruct.
    * @param  Pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @param  GPIO_InitStruct: pointer to a GPIO_InitTypeDef structure that
    *         contains the configuration information for the specified GPIO peripheral.
    * @return void
    */
void gpio_param_config(uint8_t pin_num, GPIO_InitTypeDef *GPIO_InitStruct);

/**
    * @brief  Specified GPIO APB peripheral clock.
    * @param  Pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @param  NewState: Enable or disable APB peripheral clock`.
    * @return void
    */
void gpio_periphclk_config(uint8_t pin_num, FunctionalState NewState);

/**
    * @brief  Specified GPIO interrupt.
    * @param  Pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @param  NewState: Enable or disable interrupt.
    * @return void
    */
void gpio_int_config(uint8_t pin_num, FunctionalState NewState);

/**
    * @brief  mask the specified GPIO interrupt.
    * @param  Pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @param  NewState: Enable or disable mask interrupt.
    * @return void
    */
void gpio_mask_int_config(uint8_t pin_num, FunctionalState NewState);

/**
    * @brief  clear the specified GPIO int pending.
    * @param  Pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @return void
    */
void gpio_clear_int_pending(uint8_t pin_num);

/**
    * @brief  change the specified int polarity.
    * @param  Pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @param  NewState: Set or reset int polarity.
    * @return void
    */
void gpio_intpolarity_config(uint8_t pin_num, GPIOIT_PolarityType NewState);

/**
    * @brief  All the GPIO pinmux should init NVIC in this function.
    * @param  gpio IO message communications between tasks
    * @return void
    */
void gpio_init_irq(uint8_t gpio);

/**
    * @brief  Set the GPIO pinmux output level.
    * @param  pin_num assigned pinmux index
    * @param  level output level, 0 or 1.
    * @return void
    */
void gpio_write_output_level(uint8_t pin_num, uint8_t level);

/**
    * @brief  Read the GPIO pinmux input level.
    * @param  pin_num: This parameter is from ADC_0 to P4_1, please refer to rtl876x.h "Pin_Number" part.
    * @return input level,0 or 1
    */
uint8_t gpio_read_input_level(uint8_t pin_num);

/**
    * @brief  All the GPIO interrupt will be handled in this function.
    *         Include key, line in, uart rx wake up GPIO pinmux.
    * @param  gpio_num assigned GPIO number
    * @return void
    */
void gpio_handler(uint8_t gpio_num);

/** @} */ /* End of group APP_GPIO_Exported_Functions */
/** End of APP_GPIO
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_GPIO_H_ */
