/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    bblite_wrapper.h
  * @brief   This file provides api wrapper for bbpro compatibility..
  * @author  sandy_jiang
  * @date    2018-11-29
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __WDG_H_
#define __WDG_H_

typedef enum _WDG_MODE
{
    INTERRUPT_CPU = 0,          /**< Interrupt CPU only */
    RESET_ALL_EXCEPT_AON = 1,   /**< Reset all except RTC and some AON register */
    RESET_CORE_DOMAIN = 2,      /**< Reset core domain */
    RESET_ALL = 3               /**< Reset all */
} T_WDG_MODE;

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   * @brief  Watch Dog Timer Enable / Disable / Restart
   * @param  type @ref T_WDG_TYPE
   */

extern bool (*wdg_start)(uint32_t seconds, T_WDG_MODE  wdg_mode);
#define WDG_Start(seconds, wdg_mode) wdg_start(seconds, wdg_mode);

extern void (*wdg_disable)(void);
#define WDG_Disable() wdg_disable();

extern void (*wdg_kick)(void);
#define WDG_Kick() wdg_kick();

extern void (*chip_reset)(T_WDG_MODE  wdg_mode);

#ifdef __cplusplus
}
#endif

#endif
