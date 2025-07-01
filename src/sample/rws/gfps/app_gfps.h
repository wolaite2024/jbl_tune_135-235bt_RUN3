/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_GFPS_H_
#define _APP_GFPS_H_

#if GFPS_FEATURE_SUPPORT
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "btm.h"
#include "gfps.h"
#include "ble_ext_adv.h"
#if GFPS_SASS_SUPPORT
#include "gfps_sass_conn_status.h"
#endif
/** @defgroup APP_RWS_GFPS App Gfps
  * @brief App Gfps
  * @{
  */

typedef enum
{
    GFPS_ACTION_IDLE,
    GFPS_ACTION_ADV_DISCOVERABLE_MODE_WITH_MODEL_ID,
    GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE,
    GFPS_ACTION_ADV_NOT_DISCOVERABLE_MODE_HIDE_UI,
} T_GFPS_ACTION;
typedef struct
{
    uint8_t                 gfps_adv_handle;
    T_BLE_EXT_ADV_MGR_STATE gfps_adv_state;
    T_GFPS_ACTION           gfps_curr_action;

    T_SERVER_ID             gfps_id;
    uint8_t                 gfps_conn_id;
    T_GAP_CONN_STATE        gfps_conn_state;

    T_GFPS_BATTERY_INFO     gfps_battery_info;
    uint8_t                 random_address[6];
} T_GFPS_DB;
void app_gfps_init(void);
void app_gfps_adv_init(void);
bool app_gfps_next_action(T_GFPS_ACTION gfps_next_action);
void app_gfps_get_random_addr(uint8_t *random_bd);
void app_gfps_handle_bt_user_confirm(T_BT_EVENT_PARAM_USER_CONFIRMATION_REQ confirm_req);

/**
 * @brief update gfps adv interval
 *
 * default interval is:
 * in pairing mdoe : extend_app_cfg_const.gfps_discov_adv_interval;
 * not in pairing mode: extend_app_cfg_const.gfps_not_discov_adv_interval;
 *
 * @param adv_interval Range:[32,400], Unit:0.625ms
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_gfps_adv_update_adv_interval(uint32_t adv_interval);

void app_gfps_handle_ble_user_confirm(uint8_t conn_id);
void app_gfps_enter_nondiscoverable_mode(void);
void app_gfps_handle_case_status(bool open);
void app_gfps_handle_b2s_connected(uint8_t *bd_addr);
void app_gfps_check_state(void);
void app_gfps_set_ble_conn_param(uint8_t conn_id);
#if GFPS_SASS_SUPPORT
bool app_gfps_gen_conn_status(T_SASS_CONN_STATUS_FIELD *conn_status);
/**
 * @brief app_gfps_le_update_conn_status
 */
void app_gfps_le_update_conn_status(void);
void app_gfps_notify_conn_status(void);
void app_gfps_update_adv(void);
#endif

bool app_gfps_adv_stop(uint8_t app_cause);
void app_gfps_update_rpa(void);
/** End of APP_RWS_GFPS
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
#endif
