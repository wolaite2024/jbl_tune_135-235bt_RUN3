/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      led.h
* @brief     Modulize LED driver for more simple led implementation
* @details   Led module is to group led_driver to provide more simple and easy usage for LED module.
* @author    brenda_li
* @date      2016-12-15
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __LED_DEMO_H_
#define __LED_DEMO_H_

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdbool.h>
#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup MODULE_LED  LED
  * @brief modulization for sleep led module for simple application usage.
  * @{
  */

/*============================================================================*
  *                                   Types
  *============================================================================*/
/** @defgroup LED_MODULE_Exported_Types Led Module Exported Types
  * @{
  */

extern void led_demo_init(void);

extern void led_demo_blink(void);

extern void led_demo_breath_init(void);

extern void led_demo_breath_timer_handler(void);
/** @} */ /* End of group LED_MODULE_Exported_Functions */

/** @} */ /* End of group MODULE_LED */

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __LED_H_ */

