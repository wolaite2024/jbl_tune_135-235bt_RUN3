/**
************************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file     platform_utils.h
* @brief    utility helper function for user application
* @author   lory_xu
* @date     2017-02
* @version  v1.0
*************************************************************************************************************
*/

#ifndef _PLATFORM_UTILS_H_
#define _PLATFORM_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>


/** @defgroup  87x3e_PLATFORM_UTILS Platform Utilities
    * @brief Utility helper functions
    * @{
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/

extern void *RxPktFunc;
extern void *TxCompFunc;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup PLATFORM_UTILS_Exported_Functions Platform Utils Exported Functions
    * @brief
    * @{
    */

/**
 * @brief Generate random number given max number allowed
 * @param max   to specify max number that allowed
 * @return random number
 */

uint32_t platform_random(uint32_t max);

/**
 * @brief Busy delay for specified millisecond
 * @param t   to specify t milliseconds to delay
 * @return none
 */
extern volatile void (*platform_delay_ms)(uint32_t t);

/**
 * @brief Busy delay for specified micro second
 * @param t   to specify t micro seconds to delay
 * @return none
 */
extern volatile void (*platform_delay_us)(uint32_t t);

/** @} */ /* End of group PLATFORM_UTILS_Exported_Functions */

/** @} */ /* End of group 87x3e_PLATFORM_UTILS */

#ifdef __cplusplus
}
#endif

#endif

