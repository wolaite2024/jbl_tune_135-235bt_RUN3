/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    system_status_api.h
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
#ifndef __SYSTEM_STATUS_API_H_
#define __SYSTEM_STATUS_API_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HAL_87x3e_SYSTEM_STATUS_API System Status Api
  * @{
  */

/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup DLPS_PLATFORM_Exported_Variables DLPS Platform Exported Variables
  * @{
  */


/** @} */ /* End of group DLPS_PLATFORM_Exported_Variables */

/*============================================================================*
 *                              Functions
*============================================================================*/
bool sys_hall_get_reset_status(void);
void sys_hall_get_dlps_aon_info(void);
bool sys_hall_adp_read_adp_level(void);
void sys_hall_set_dsp_share_memory_80k(bool is_off_ram);
void sys_hall_btaon_fast_read_safe(uint16_t *input_info, uint16_t *output_info);
void sys_hall_btaon_fast_write_safe(uint16_t offset, uint16_t *input_info);
bool sys_hall_charger_auto_enable(bool opt, uint8_t type, bool opt2);
uint8_t sys_hall_read_package_id(void);
uint8_t sys_hall_read_chip_id(void);
uint8_t sys_hall_read_rom_version(void);
uint8_t *sys_hall_get_ic_euid(void);
void sys_hall_set_rglx_auxadc(uint8_t input_pin);
void sys_hall_set_hci_download_mode(bool mode);
void sys_hall_upperstack_ini(uint8_t *upperstack_compile_stamp);
void sys_hall_auto_sleep_in_idle(bool flag);
/** End of HAL_87x3e_SYSTEM_STATUS_API
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
