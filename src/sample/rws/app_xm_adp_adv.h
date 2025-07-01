/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_XM_ADP_ADV_H_
#define _APP_XM_ADP_ADV_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_XM_ADP_ADV App XM Adp Adv
  * @brief App XM Adp Adv
  * @{
  */

#define APP_XM_ADP_ADV_RSP_MAX_DEVICE_NAME_LENGTH   15

/******************************************************************************/
/** @defgroup APP_XM_ADP_ADV_Exported_Macros XM ADP ADV DATA Macros
  * @brief
  * @{
  */
typedef struct
{
    uint8_t charging_box_info;
    uint8_t adv_count;
    uint8_t shell_open;
    uint8_t reserved[1];
} T_APP_XM_ADP_ADV_DATA_STRUCT;
/** End of APP_XM_ADP_ADV_Exported_Macros
* @}
*/

typedef enum
{
    XM_ADP_ADV_TYPE_OPENCASE            = 0x00,
    XM_ADP_ADV_TYPE_CLOSECASE           = 0x01,
    XM_ADP_ADV_TYPE_OTHERS              = 0x02,
} T_XM_ADP_ADV_TYPE;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_XM_ADP_ADV_Exported_Functions App xm adp adv Functions
   * @{
   */
/**
    * @brief  xm adp adv detect init
    * @param  void
    * @return void
    */
void app_xm_adp_adv_init(void);

/**
    * @brief  xm adp ble adv start
    * @param  adv_type
    * @return void
    */
void app_xm_adp_adv_start(uint8_t adv_type);

/**
    * @brief  xm adp ble adv stop
    * @param  void
    * @return void
    */
void app_xm_adp_adv_stop(void);

/**
    * @brief  get xm adp ble adv state
    * @param  void
    * @return true if adp ble adv is transmitting, otherwise return false.
    */
bool app_xm_adp_get_adv_state(void);

/**
    * @brief  get xm adp opencase ble adv state
    * @param  void
    * @return true if adp opencase ble adv is transmitting, otherwise return false.
    */
bool app_xm_adp_get_opencase_adv_state(void);

/**
    * @brief  get byte[12] to byte[14] of xm adp ble adv to get battery state
    * @param  battery state
    * @return void
    */
void app_xm_adp_get_battery_state(uint8_t *p_data);
/**
    * @brief  get byte[11] of xm adp ble adv to get headset state
    * @param  void
    * @return return headset state
    */
uint8_t app_xm_adp_get_headset_state(void);

/**
    * @brief  set xm adp ble adv count
    * @param  adv_count
    * @return void
    */
void app_xm_adp_set_adv_count(uint8_t adv_count);

/**
    * @brief  get xm adp ble adv count
    * @param  void
    * @return return adv count
    */
uint8_t app_xm_adp_get_adv_count(void);

/**
    * @brief  set xm adv charging box info
    * @param  battery info
    * @return void
    */
void app_xm_adp_set_adv_charging_box_info(uint8_t battery_info);

/**
    * @brief  get xm adv charging box info
    * @param  void
    * @return return charging box info
    */
uint8_t app_xm_adp_get_adv_charging_box_info(void);

/**
    * @brief  set xm adv shell open
    * @param  shell open
    * @return void
    */
void app_xm_adp_set_adv_shell_open(uint8_t shell_open);

/**
    * @brief  stop xm delay adv timer
    * @param  shell open
    * @return void
    */
void app_xm_adp_stop_delay_adv_timer(void);

/**
    * @brief  stop xm adv timer
    * @param  shell open
    * @return void
    */
void app_xm_adp_stop_adv_timer(void);

/**
    * @brief  xm adp adv delay start
    * @param  delay time, enable delay
    * @return void
    */
void app_xm_adp_adv_delay(uint32_t delay_time, bool enable_delay);

/**
    * @brief  xm adp ble opencase adv init
    * @param  void
    * @return void
    */
void app_xm_adp_opencase_adv_init(void);

/**
    * @brief  xm adp ble set ble device name of rsp data
    * @param  device_name_le
    * @return void
    */
void app_xm_adp_set_adv_rsp_device_name(void);

/**
    * @brief  xm adp stop adv timers
    * @param  void
    * @return void
    */
void app_xm_adp_stop_adv_timers(void);
/** End of group APP_XM_ADP_ADV_Exported_Functions
* @}
*/

/** End of APP_XM_ADP_ADV
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_XM_ADP_ADV_H_ */
