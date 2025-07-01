/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_XM_ADP_H_
#define _APP_XM_ADP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_XM_ADP App XM Adp
  * @brief App XM Adp
  * @{
  */

/*****************************Chargerbox State*********************************/
#define XM_ADP_DISALLOW_ENTER_POWER_DOWN 0x01
#define XM_ADP_DISALLOW_ENTER_LPS_MODE   0x02
#define XM_ADP_ALL                       0xFF

/******************************************************************************/
/** @defgroup APP_XM_ADP_Exported_Macros App xm adp macros
  * @brief
  * @{
  */

typedef enum
{
    XM_ADP_CMD_INVALID                  = 0x00,

    XM_ADP_CMD_OPEN_CASE                = 0x01,
    XM_ADP_CMD_CLOSE_CASE               = 0x02,
    XM_ADP_CMD_FACTORY_RESET            = 0x03,
    XM_ADP_CMD_POWER_OFF                = 0x04,
    XM_ADP_CMD_ENTER_PAIRING_MODE       = 0x05,
    XM_ADP_CMD_BATT                     = 0x06,
    XM_ADP_CMD_ENTER_DUT_TEST_MODE      = 0x07,
    XM_ADP_CMD_FORCE_ENTER_PAIRING_MODE = 0x08,
    XM_ADP_CMD_RWS_FACTORY_RESET        = 0x09,
    XM_ADP_CMD_TRANSPORTATION_MODE      = 0x0A,
    XM_ADP_CMD_OTA_MODE                 = 0x0B,
} T_XM_ADP_CMD_ID;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
    * @brief  adp xm adp init
    * @param  void
    * @return void
    */
void app_xm_adp_init(void);

/**
    * @brief  stop close case power off timer
    * @param  void
    * @return void
    */
void app_xm_adp_stop_close_case_power_off_timer(void);

/**
    * @brief  check charger box status
    * @param  check bit
    * @return true shows bit exist, else false
    */
bool app_xm_adp_check_box_status(uint8_t check_bit);

/**
    * @brief  clear charger box status
    * @param  clear bit
    * @return void
    */
void app_xm_adp_clear_box_status(uint8_t clear_bit);

/**
    * @brief  set charger box status
    * @param  set bit
    * @return void
    */
void app_xm_adp_set_box_status(uint8_t set_bit);

/**
    * @brief  get charger box status
    * @param  void
    * @return charger box status
    */
uint8_t app_xm_adp_get_box_status(void);

/**
    * @brief  set accept close case cmd status
    * @param  set
    * @return void
    */
void app_xm_adp_set_receive_close_case(bool set);

/**
    * @brief  get accept close case cmd status
    * @param  void
    * @return accept closecase cmd
    */
bool app_xm_adp_get_receive_close_case(void);

/**
    * @brief  set ever pause
    * @param  ever_set
    * @return void
    */
void app_xm_adp_set_ever_pause(bool ever_set);

/**
    * @brief  check disconnect phone link
    * @param  void
    * @return true shows disconnect phone close case, else false
    */
bool app_xm_is_disconnect_phone_link(void);

/**
    * @brief  xm adp cmd received
    * @param  void
    * @return cmd
    */
uint8_t app_xm_adp_cmd_received(void);

/**
    * @brief  xm adp stop timers
    * @param  void
    * @return void
    */
void app_xm_adp_stop_timers(void);

/**
    * @brief  xm adp smart box update battery lv
    * @param  payload
    * @return void
    */
void app_xm_adp_smart_box_update_battery_lv(uint8_t payload, bool report_flag);

/** End of APP_XM_ADP
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_XM_ADP_H_ */

