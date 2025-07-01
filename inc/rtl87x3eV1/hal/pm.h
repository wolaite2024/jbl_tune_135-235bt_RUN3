/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    pm.h
  * @brief   This file provides apis for power manager.
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
#ifndef __PM_H_
#define __PM_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HAL_87x3e_PM
  * @brief
  * @{
  */

/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup PM_Exported_Variables PM Exported Variables
  * @{
  */

typedef enum
{
    BTPOWER_DEEP_SLEEP        = 0,   /**< Deep sleep */
    BTPOWER_ACTIVE            = 1,   /**< Active     */
} BtPowerMode;


/** @brief POWER STAGE struct */
typedef enum
{
    POWER_STAGE_STORE           = 0,
    POWER_STAGE_RESTORE         = 1,
} POWERStage;

/** @brief LPSMode struct */
typedef enum
{
    POWER_POWEROFF_MODE    = 0,
    POWER_POWERDOWN_MODE   = 1,   /**< Power down */
    POWER_DLPS_MODE        = 2,   /**< DLPS       */
    POWER_LPS_MODE         = 4,   /**< LPS        */
    POWER_ACTIVE_MODE      = 5,    /**< Active     */
    POWER_MODE_MAX         = 6
} POWERMode;

typedef enum
{
    POWER_DLPS_PFM_MODE = 0,    /**< default one */
    POWER_DLPS_RET_MODE = 1,
} POWERDlpsMode;

/* BB2 using*/
typedef enum
{
    PM_ERROR_UNKNOWN               = 0x0,
    PM_ERROR_POWER_MODE            = 0x1,
    PM_ERROR_DISABLE_DLPS_TIME     = 0x2,
    PM_ERROR_32K_CHECK_LOCK        = 0x3,
    PM_ERROR_LOG_DMA_NOT_IDLE      = 0x4,
    PM_ERROR_CALLBACK_CHECK        = 0x5,
    PM_ERROR_INTERRUPT_OCCURRED    = 0x6,
    PM_ERROR_WAKEUP_TIME           = 0x7,
    PM_ERROR_FREE_BLOCK_STORE      = 0x8,
    PM_ERROR_BUFFER_PRE_ALLOCATE   = 0x9,
} PowerModeErrorCode;

typedef enum
{
    PM_EXCLUDED_TIMER,
    PM_EXCLUDED_TASK,
    PM_EXCLUDED_TYPE_MAX,
} PowerModeExcludedHandleType;
/** @brief This CB is used for any module which needs to be checked before entering DLPS */
typedef bool (*POWERCheckFunc)();

/** @brief This CB is used for any module which needs to control the hw before entering or after exiting from DLPS */
typedef void (*POWERStageFunc)();

/** @} */ /* End of group DLPS_PLATFORM_Exported_Variables */

/*============================================================================*
 *                              Functions
*============================================================================*/
/** @defgroup DLPS_PLATFORM_Exported_Functions DLPS Platform Exported Functions
  * @{
  */

void bt_power_mode_set(BtPowerMode mode);

BtPowerMode bt_power_mode_get(void);

/**
 * @brief Register Check CB to Power module which will call it before entering Dlps.
 * @param  func  DLPSEnterCheckFunc.
 * @return  Status of Operation.
*/
int32_t power_check_cb_register(POWERCheckFunc func);


/**
 * @brief Register HW Control CB to Power module which will call it before entering power mode or after exiting from power mode (according to POWERStage) .
 * @param  func  POWERStageFunc.
 * @param  stage  tell the Power module the CB should be called when @ref POWER_STAGE_ENTER or POWER_STAGE_EXIT.
 * @return  Status of Operation.
*/
int32_t power_stage_cb_register(POWERStageFunc func, POWERStage stage);

/**
 * @brief  POWERMode Set .
 * @param  mode   POWERMode.
 * @return  none
*/
int32_t power_mode_set(POWERMode mode);

/**
 * @brief  POWERMode Get .
 * @param  none
 * @return  POWERMode
*/
POWERMode power_mode_get(void);

/**
 * @brief  POWERDLPSMode Set .
 * @param  mode   DLPSMODE,default value is DLPS_PFM.
 * @return  none
 * @note    It only takes effect when bt_power_mode_set(POWER_DLPS_MODE).Please call this function before power_mode_set(POWER_DLPS_MODE)
*/
int32_t power_dlps_mode_select(POWERDlpsMode mode);

/**
 * @brief  POWERDlpsMode Get.
 * @param  mode   none.
 * @return  POWERDlpsMode
*/
POWERDlpsMode power_dlps_mode_get(void);

/**
 * @brief  POWERMode Pause .
 * @param  none
 * @return  void
*/
int32_t power_mode_pause(void);

/**
 * @brief  POWERMode Resume .
 * @param  none
 * @return  void
*/
int32_t power_mode_resume(void);

PowerModeErrorCode power_get_error_code(void);

void power_stop_all_non_excluded_timer(void);

bool power_register_excluded_handle(void **handle, PowerModeExcludedHandleType type);

void power_get_statistics(uint32_t *wakeup_count, uint32_t *total_wakeup_time,
                          uint32_t *total_sleep_time);


typedef enum
{
    PM_SUCCESS                         = 0x0,
    PM_DVFS_BUSY                       = -0x1,
    PM_DVFS_VOLTAGE_FAIL               = -0x2,
    PM_DVFS_CONDITION_FAIL             = -0x4,
    PM_DVFS_SRAM_FAIL                  = -0x8,
    PM_DVFS_NOT_SUPPORT                = -0x10,
} PMErrorCode;

/**
 * @brief Config cpu clock freq.
 * @param  require_mhz  require cpu freqency .
 * @param  real_mhz  the freqency of current cpu.
 * @return  Status of Operation.
*/
int32_t pm_cpu_freq_set(uint32_t required_mhz, uint32_t *actual_mhz);

/**
 * @brief Config dsp1 clock freq.
 * @param  require_mhz  require dsp1 freqency .
 * @param  real_mhz  the freqency of current dsp1.
 * @return  Status of Operation.
*/
int32_t pm_dsp1_freq_set(uint32_t required_mhz, uint32_t *actual_mhz);

/**
 * @brief Get cpu clock freq.
 * @return  mhz.
*/
uint32_t pm_cpu_freq_get(void);

/**
 * @brief Get dsp1 clock freq.
 * @return  mhz.
*/
uint32_t pm_dsp1_freq_get(void);

/** @} */ /* End of group PM_Exported_Functions */

/** @} */ /* End of group PM */

/** @} End of HAL_87x3e_PM */

#ifdef __cplusplus
}
#endif

#endif
