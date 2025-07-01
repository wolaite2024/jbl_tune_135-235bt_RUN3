/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hal_gpio.h
* @brief
* @details
* @author
* @date
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup 87x3e_HAL_GPIO_H_
  * @brief HAL GPIO device driver module
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
typedef enum
{
    GPIO_STATUS_ERROR              = -3,        /**< The GPIO function failed to execute.*/
    GPIO_STATUS_ERROR_PIN          = -2,        /**< Invalid input pin number. */
    GPIO_STATUS_INVALID_PARAMETER  = -1,        /**< Invalid input parameter. */
    GPIO_STATUS_OK                 = 0,         /**< The GPIO function executed successfully. */
} T_GPIO_STATUS;

typedef enum
{
    GPIO_TYPE_AUTO      = 0,    /**< The GPIO function switch automatically, use core GPIO in active mode,
                                    use aon GPIO in low power mode.*/
    GPIO_TYPE_CORE      = 1,    /**< The GPIO function in core domain, only works when active mode.
                                    Support all the features. */
    GPIO_TYPE_AON       = 2,    /**< The GPIO function in aon domain, works in dlps/power down,
                                    much slower than CORE mode,
                                    not support read level and debounce.*/
} T_GPIO_TYPE;

typedef enum
{
    GPIO_DIR_INPUT     = 0,
    GPIO_DIR_OUTPUT    = 1,
} T_GPIO_DIRECTION;

typedef enum
{
    GPIO_LEVEL_LOW     = 0,
    GPIO_LEVEL_HIGH    = 1,
    GPIO_LEVEL_UNKNOWN = 2,
} T_GPIO_LEVEL;

typedef enum
{
    GPIO_IRQ_EDGE,
    GPIO_IRQ_LEVEL,
} T_GPIO_IRQ_MODE;

typedef enum
{
    GPIO_IRQ_ACTIVE_LOW,
    GPIO_IRQ_ACTIVE_HIGH,
} T_GPIO_IRQ_POLARITY;

typedef void (*P_GPIO_CBACK)(uint32_t context);

/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * hal_gpio.h
 *
 * \brief   Initialize a pin to gpio mode.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h "Pin_Number" part.
 *
 * \param[in]   type        The TYPE of GPIO mode to be used.
 *
 * \param[in]   direction   The direction GPIO is set to, could be output mode or input mode.
 *
 * \return                   The status of the gpio pin initialization.
 * \retval GPIO_STATUS_OK        The GPIO pin was initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to initialized due to invalid pin number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT);
 *     gpio_init_pin(TEST_PIN_2, GPIO_TYPE_CORE, GPIO_DIR_INPUT);
 * }
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_init_pin(uint8_t pin_index, T_GPIO_TYPE type, T_GPIO_DIRECTION direction);

/**
 * hal_gpio.h
 *
 * \brief   De-Initialize the pin to shutdown.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h "Pin_Number" part.
 *
 *
 * \return                   The status of the gpio pin initialization.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     gpio_deinit(TEST_PIN);
 *     gpio_deinit(TEST_PIN_2);
 * }
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_deinit_pin(uint8_t pin_index);

/**
 * hal_gpio.h
 *
 * \brief   Set the pin output level.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h "Pin_Number" part.
 *
 * \param[in]  level        The level for the specific pin to output
 *
 *
 * \return                   The status of the gpio pin initialization.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT);
 *     gpio_set_level(TEST_PIN_2, GPIO_LEVEL_HIGH);
 * }
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_set_level(uint8_t pin_index, T_GPIO_LEVEL level);

/**
 * hal_gpio.h
 *
 * \brief   Get the pin current level.
 *
 * \param[in]  pin_index    The pin index, please refer to rtl876x.h "Pin_Number" part. *
 *
 * \return                   The current pin level.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * int test(void)
 * {
 *     gpio_init_pin(TEST_PIN, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT);
 *     gpio_set_level(TEST_PIN_2, GPIO_LEVEL_HIGH);
 * }
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_LEVEL gpio_get_level(uint8_t pin_index);

/**
 * hal_gpio.h
 *
 * \brief   Get the pin current level.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h "Pin_Number" part. *
 * \param[in]  mode               The interrupt mode to be set to.
 * \param[in]  polarity           The polarity for the interrupt to be set to.
 * \param[in]  debounce_enable    Enable or Disable the pin hardware debounce feature.
 * \param[in]  callback           The callback to be called when the specific interrupt happened.
 * \param[in]  context            The user data when callback is called.
 *
 * \return                   The current pin level.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_set_up_irq(uint8_t pin_index, T_GPIO_IRQ_MODE mode, T_GPIO_IRQ_POLARITY polarity,
                              bool debounce_enable, P_GPIO_CBACK callback, uint32_t context);

/**
 * hal_gpio.h
 *
 * \brief   Enable the interrupt of the pin.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h "Pin_Number" part. *
 *
 * \return                   The status of enable interrupt.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_irq_enable(uint8_t pin_index);


/**
 * hal_gpio.h
 *
 * \brief   Disable the interrupt of the pin.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h "Pin_Number" part. *
 *
 * \return                   The status of disable interrupt.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_irq_disable(uint8_t pin_index);

/**
 * hal_gpio.h
 *
 * \brief   Set the hardware debounce time. the interrupt could be triggered after 1 to 2 of debounce time.
 *
 * \param[in]  ms          The pin index, please refer to rtl876x.h "Pin_Number" part. *
 *
 * \return                   The status of setting debounce time.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_set_debounce_time(uint8_t ms);

/**
 * hal_gpio.h
 *
 * \brief   Change the interrupt polarity of GPIO current mode.
 *
 * \param[in]  pin_index          The pin index, please refer to rtl876x.h "Pin_Number" part. *
 *
 * \return                   The status of change polarity.
 * \retval GPIO_STATUS_OK        The GPIO pin was de-initialized successfully.
 * \retval GPIO_STATUS_ERROR_PIN     The GPIO pin was failed to de-initialized due to invalid pin number.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
T_GPIO_STATUS gpio_irq_change_polarity(uint8_t pin_index, T_GPIO_IRQ_POLARITY polarity);


/**
 * hal_gpio.h
 *
 * \brief   Init the hal gpio module, gpio clock would be enabled, and hal gpio function could be used after this function called.
 *
 * \param   None
 * \return  None.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
void gpio_init(void);

/**
 * hal_gpio.h
 *
 * \brief   De-Init the hal gpio module, gpio clock would be enabled, and hal gpio function could not be used after this function called.
 *
 * \param   None
 * \return  None.
 *
 * \endcode
 *
 * \ingroup  GPIO
 */
void gpio_deinit(void);


#ifdef __cplusplus
}
#endif

#endif

/** @} */ /* End of group 87x3e_HAL_GPIO_H_ */

/******************* (C) COPYRIGHT 2021 Realtek Semiconductor *****END OF FILE****/
