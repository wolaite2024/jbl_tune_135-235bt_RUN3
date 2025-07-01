/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      dlps_util.h
* @brief
* @details
* @author
* @date      2021-9-1
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _DLPS_DEBUG_
#define _DLPS_DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "rtl876x.h"


/** @addtogroup 87x3e_DLPS_DEBUG DLPS_DEBUG
  * @brief DLPS debug function module
  * @{
  */
/** @brief DLPS wake up reason */

/** @brief Power down wake up reason */
#define POWER_DOWN_WAKEUP_RTC       BIT0          /* must open 32k in power down */
#define POWER_DOWN_WAKEUP_PAD       BIT1
#define POWER_DOWN_WAKEUP_MFB       BIT2          /* must open 32k in power down */
#define POWER_DOWN_WAKEUP_ADP       BIT3
#define POWER_DOWN_WAKEUP_CTC       BIT4          /* must open 32k in power down */

/**
 * @brief  Check current dlps wake up timer.
 * @param  none
 * @retval true: Find dlps wake up timer success
 * @retval false: Find dlps wake up timer failed
*/
bool dlps_check_os_timer(void);

/**
 * @brief  Check power down wake up reason, must open 32k in power down except PAD and ADP.
 * @param  none
 * @retval Return power down wake up reason.
*/
uint8_t power_down_check_wake_up_reason(void);

/**
 * @brief  Print the dlps wake up status.
 * @param  None
 * @retval None
*/
void dlps_utils_print_wake_up_info(void);

/**
 * dlps_util.h
 *
 * \brief   start monitor cpu sleep time.
 *
 * \param[in]  period_ms               The interval for print in miliseconds.
 *
 * \return           The current pin level.
 * \retval true      The monitor timer is started successfully.
 * \retval false     The monitor timer is failed to start.
 *
 * \endcode
 *
 * \ingroup  DLPS_UTIL
 */
bool dlps_util_cpu_sleep_monitor_start(uint32_t period_ms);


/**
 * dlps_util.h
 *
 * \brief   stop monitor cpu sleep time.
 *
 * \endcode
 *
 * \ingroup  DLPS_UTIL
 */
void dlps_util_cpu_sleep_monitor_stop(void);


/**
 * dlps_util.h
 *
 * \brief   Init dlps debug timer for monitor.
 *
 * \endcode
 *
 * \ingroup  DLPS_UTIL
 */
void dlps_util_debug_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _DLPS_DEBUG_ */

/** @} */ /* End of group 87x3e_DLPS_DEBUG */


/******************* (C) COPYRIGHT 2021 Realtek Semiconductor *****END OF FILE****/

