/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CHARGER_H_
#define _APP_CHARGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_CHARGER App Charger
  * @brief App Charger
  * @{
  */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_CHARGER_Exported_Macros App charger Macros
   * @{
   */

#define CHARGERBOX_ADAPTOR_DETECT_TIMER          500
#define SMART_CHARGERBOX_ADAPTOR_DETECT_TIMER    200

#define POWER_OFF_ENTER_DUT_MODE_INTERVAL 500//ms

/* Battery Status */
#define BATTERY_STATUS_EMPTY    0
#define BATTERY_STATUS_LOW      1
#define BATTERY_STATUS_NORMAL   2
#define BATTERY_STATUS_FULL     3

#if HARMAN_POWER_DISPLAY_ACCURACY_1_PERCENTAGE_SUPPORT
#define MULTIPLE_OF_TEN(x) (x/1)
#else
#define MULTIPLE_OF_TEN(x) ((x + 9) / 10 * 10)
#endif

#define LOW_BAT_WARNING_TO_MULTIPLIER_MINUTE   60
#define LOW_BAT_WARNING_TO_MULTIPLIER_SECOND   1

/** End of APP_CHARGER_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CHARGER_Exported_Types App Charger Types
   * @{
   */
/**  @brief  Charger battery capacity,  percentage form */
typedef enum t_bat_capacity
{
    BAT_CAPACITY_0 = 0,
    BAT_CAPACITY_10 = 10,
    BAT_CAPACITY_20 = 20,
    BAT_CAPACITY_30 = 30,
    BAT_CAPACITY_40 = 40,
    BAT_CAPACITY_50 = 50,
    BAT_CAPACITY_60 = 60,
    BAT_CAPACITY_70 = 70,
    BAT_CAPACITY_80 = 80,
    BAT_CAPACITY_90 = 90,
    BAT_CAPACITY_100 = 100,

    BAT_CAPACITY_TOTAL
} T_BAT_CAPACITY;

typedef enum
{
    ADAPTOR_UNPLUG,
    ADAPTOR_PLUG
} T_ADAPTOR_STATUS;

/**  @brief  Charger control type. */
typedef enum
{
    CHARGER_CONTROL_BY_BOX_BATTERY,
    CHARGER_CONTROL_BY_ADP_VOLTAGE, /* charger auto enable must be false */
} T_CHARGER_CONTROL_TYPE;


typedef enum
{
    /*for discharger*/
    APP_IO_TIMER_LOW_BAT_WARNING         = 0,
    APP_IO_TIMER_LOW_BAT_LED             = 1,
    APP_IO_TIMER_RTK_UNPLUG_DEBOUNCE     = 4,
    APP_IO_TIMER_SMOOTH_BAT_REPORT       = 5,
    APP_IO_TIMER_DELAY_LOW_BATT_POWEROFF = 6,
} APP_CHARGER_TIMER_MSG_TYPE;


typedef enum
{
    APP_CHARGER_STATE_NO_CHARGE,
    APP_CHARGER_STATE_CHARGING,
    APP_CHARGER_STATE_CHARGE_FINISH,
    APP_CHARGER_STATE_ERROR,
} APP_CHARGER_STATE;


/* App handle charger state or state of charge change message */
typedef enum t_chg_msg
{
    CHG_MSG_CHARGER_STATE_CHANGE,           /* charger state message*/
    CHG_MSG_STATE_OF_CHARGE_CHANGE,         /* Battery percentage message*/

    CHG_MSG_TOTAL
} T_CHG_MSG;

typedef enum
{
    APP_CHARGER_EVENT_BATT_CHANGE           = 0x00,
    APP_CHARGER_EVENT_BATT_STATE_CHANGE     = 0x01,
    APP_CHARGER_EVENT_BATT_ROLESWAP_FLAG    = 0x02,
    APP_CHARGER_EVENT_BATT_CASE_LEVEL       = 0x03,
    APP_CHARGER_EVENT_BATT_REQ_LEVEL        = 0x04,

    APP_CHARGER_MSG_TOTAL                   = 0x05,
} T_APP_CHARGER_REMOTE_MSG;

#if (F_APP_AVP_INIT_SUPPORT == 1)
extern void (*app_charger_box_trig_hook)(void);
extern void (*app_charger_box_stop_hook)(void);
extern void (*app_charger_box_reset_hook)(void);;
#endif
/** End of APP_CHARGER_Exported_Types
    * @}
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CHARGER_Exported_Functions App Charger Functions
    * @{
    */
/**
   * @brief  App charger init. App will register charger state callback and state of charge callback.
   *         And init charger three led type.
   *         Charger state include charger start, fast charge, charger finish etc.
   *         Charging status display charger battery current capacity, such as 10%, 50% etc.
   * @param  void
   * @return void
   */
void app_charger_init(void);
/**
    * @brief  discharger module init.
    * @param  void
    * @return void
    */
void app_discharger_init(void);


APP_CHARGER_STATE app_charger_get_charge_state(void);

uint8_t app_charger_get_soc(void);

void app_battery_evt_handle(uint8_t event, bool from_remote, uint8_t para);

/**
    * @brief  check whether local battery level is low or not.
    * @param  void
    * @return true if is low battery level, otherwise return false.
    */
bool app_charger_local_battery_status_is_low(void);

/**
    * @brief  check whether remote battery level is low or not.
    * @param  void
    * @return true if is low battery level, otherwise return false.
    */
bool app_charger_remote_battery_status_is_low(void);


/**
    * @brief  adaptor plug msg process
    * @param  void
    * @return none
    */
void app_charger_plug_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  adaptor unplug msg process
    * @param  void
    * @return none
    */
void app_charger_unplug_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  handle charger state and battery level msg
    * @param  void
    * @return none
    */
void app_charger_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  Get all the battery info (battery level and charging state of left/right/case).
    * @param  left_charging : left bud charging or not
    * @param  left_battery : left bud's battery level
    * @param  right_charging : right bud charging or not
    * @param  right_battery : right bud's battery level
    * @param  case_charging : case charging or not
    * @param  case_battery : case's battery level
    * @return void
    */
void app_charger_get_battery_info(bool *left_charging, uint8_t *left_battery,
                                  bool *right_charging, uint8_t *right_battery,
                                  bool *case_charging, uint8_t *case_battery);

uint8_t app_charger_get_low_bat_state(void);
void app_charger_update(void);
void app_discharger_init(void);
void common_charge_state_send_msg(APP_CHARGER_STATE app_charger_state);
void app_charger_harman_low_bat_warning_vp(uint8_t state_of_charge, const char *func_name,
                                           const uint32_t line_no);
#if F_APP_TEAMS_CUSTOMIZED_CMD_SUPPORT
int32_t app_charger_get_bat_curr(void);

uint32_t app_charger_get_bat_vol(void);
#endif

/** @} */ /* End of group APP_CHARGER_Exported_Functions */
/** End of APP_CHARGER
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CHARGER_H_ */
