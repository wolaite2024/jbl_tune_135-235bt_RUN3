/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_BLE_TTS_OTA_H_
#define _APP_BLE_TTS_OTA_H_

#include <stdint.h>
#include <stdbool.h>
#include "ble_ext_adv.h"
#include "remote.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_BLE_TTS_OTA App BLE TTS OTA
  * @brief App BLE Device
  * @{
  */


typedef void (*LE_COMMON_ADV_CB)(uint8_t cb_type, T_BLE_EXT_ADV_CB_DATA cb_data);


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_BLE_TTS_OTA_Exported_Macros App Ble TTS OTA Macros
   * @{
   */

/** End of APP_BLE_TTS_OTA_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_BLE_TTS_OTA_Exported_Functions App Ble TTS OTA Functions
    * @{
    */
/**
    * @brief  get ble common advertising state
    * @param  void
    * @return ble advertising state
    */
T_BLE_EXT_ADV_MGR_STATE le_common_adv_get_state(void);

/**
    * @brief  get ble common advertising conn id
    * @param  void
    * @return ble advertising conn id
    */
uint8_t le_common_adv_get_conn_id(void);

/**
    * @brief  reset ble common advertising conn id
    * @param  void
    * @return void
    */
void le_common_adv_reset_conn_id(void);

/**
    * @brief  start rws ble common advertising
    * @param  duration_ms advertising duration time
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool rws_le_common_adv_start(uint16_t duration_10ms);

/**
    * @brief  Update OTA advertising data
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool le_common_ota_adv_data_update(void);

/**
    * @brief  update bud role in rws ble common advertising data when role swap and start advertising
    * @param  T_REMOTE_SESSION_ROLE
    * @return void
    */
void rws_le_common_adv_handle_roleswap(T_REMOTE_SESSION_ROLE role);

/**
    * @brief  role swap fail, primary start advertising
    * @param  T_REMOTE_SESSION_ROLE
    * @return void
    */
void rws_le_common_adv_handle_role_swap_fail(T_REMOTE_SESSION_ROLE device_role);
/**
    * @brief  sync le adv start flag
    * @return void
    */
void le_adv_sync_start_flag(void);

/**
    * @brief  update bud role in rws ble common advertising data when engage role decided and start advertising
    * @param  T_REMOTE_SESSION_ROLE
    * @return void
    */
void rws_le_common_adv_handle_engage_role_decided(void);

/**
    * @brief  start ble common advertising
    * @param  duration_ms advertising duration time
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool le_common_adv_start(uint16_t duration_10ms);

/**
    * @brief  stop ble common advertising
    * @param  app_cause cause
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool le_common_adv_stop(int8_t app_cause);

/**
    * @brief  set ble common advertising random address
    * @param  random_address random address
    * @return void
    */
void le_common_adv_set_random(uint8_t *random_address);

/**
    * @brief  init ble common advertising parameters
    * @param  void
    * @return void
    */
void le_common_adv_init(void);

void harman_le_common_disc(void);

void le_common_adv_cb_reg(LE_COMMON_ADV_CB cb);

/**
    * @brief  update ble common advertising scan response data
    * @param  void
    * @return true update success
    */
bool le_common_adv_update_scan_rsp_data(void);
/** @} */ /* End of group APP_BLE_TTS_OTA_Exported_Functions */

/** End of APP_BLE_TTS_OTA
* @}
*/
void le_common_adv_callback(uint8_t cb_type, void *p_cb_data);
void app_harman_adv_data_update(uint8_t *p_adv_data, uint8_t adv_data_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_TTS_H_ */
