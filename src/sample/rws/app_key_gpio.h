/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_KEY_GPIO_H_
#define _APP_KEY_GPIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Extern variable */
#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
#else
extern void *timer_handle_key0_debounce;
extern uint8_t key_gpio_timer_queue_id;
#endif

/** @defgroup APP_KEY_GPIO App Key GPIO
  * @brief App Key GPIO
  * @{
  */

/**max key timer crash count*/
#define GPIO_RESET_COUNT 3

/* App define KEY_GPIO timer type */
typedef enum
{
    APP_IO_TIMER_KEY0_DEBOUNCE,
} T_APP_KEY_GPIO_TIMER;

/**  @brief  Define key GPIO data for saving key press or release and key status information */
typedef struct
{
    uint8_t key_press[MAX_KEY_NUM];         /**< key press or key release */
    uint8_t key_status[MAX_KEY_NUM];            /**< key GPIO get value of  specified input port pin */
} T_KEY_GPIO_DATA;

/**
    * @brief  Key GPIO initial.
    *         Include APB peripheral clock config, key GPIO parameter config and
    *         key specific GPIO interrupt mark config. Enable key GPIO interrupt.
    * @param  void
    * @return void
    */
void key_gpio_initial(void);

/**
    * @brief  Key1~Key5 GPIO interrupt will be handle in this function. And they are all edge trigger.
    *         First disable app enter dlps and read current specific key GPIO input data bit.
    *         Disable key GPIO interrupt and send IO_MSG_TYPE_GPIO message to app task.
    *         Then enable key GPIO interrupt.
    * @param  key_mask key GPIO mask
    * @param  gpio_index  key GPIO pinmux
    * @param  key_index key number, such as key1, key2 etc.
    * @return void
    */
void key_gpio_intr_handler(uint8_t key_mask, uint8_t gpio_index, uint8_t key_index);

/**
    * @brief  Key0 GPIO interrupt will be handle in this function. And it is level trigger.
    *         Key0 GPIO can wake up the system from powerdown mode.
    *         First disable app enter dlps mode and read current key0 GPIO input data bit.
    *         Then disable key0 GPIO interrupt and start HW timer to read the key0 GPIO status.
    * @param  void
    * @return void
    */
void key0_gpio_intr_handler(void);

/**
    * @brief  mfb key init
    * @param  none
    * @return none
        */
void key_mfb_init(void);

bool key_get_press_state(uint8_t key_index);

/**
    * @brief app_key_check_power_button_pressed
    * @return ture pressed
    */
bool app_key_check_power_button_pressed(void);

/** End of APP_KEY_GPIO
* @}
*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_KEY_GPIO_H_ */
