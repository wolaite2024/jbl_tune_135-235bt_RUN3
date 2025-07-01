/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      multitopology_bw_prio.c
   * @brief     This file handles bandwidth setting process routines.
   * @author    mj.mengjie.han
   * @date      2021-12-20
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2021 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
*                              Header Files
*============================================================================*/
#include <string.h>
#include "trace.h"
#include "gap_lib_le.h"
#include "gap_vendor.h"
#include "gap_msg.h"
#include "app_mmi.h"
#include "app_ble_gap.h"
#include "multitopology_pro.h"
#include "multitopology_ctrl.h"
extern T_GAP_CAUSE le_vendor_set_priority(T_GAP_VENDOR_PRIORITY_PARAM *p_priority_param);
/*============================================================================*
 *                              Constants
 *============================================================================*/



/*============================================================================*
 *                              Variables
 *============================================================================*/

T_GAP_VENDOR_PRIORITY_LEVEL link_pri;
T_GAP_VENDOR_PRIORITY_LEVEL adv_pri;
T_GAP_VENDOR_PRIORITY_LEVEL initiate_pri;
T_GAP_VENDOR_PRIORITY_LEVEL scan_pri;
bool initial_set = false;
T_MTC_GAP_VENDOR_MODE pendingmode = MTC_GAP_VENDOR_NONE;

/*============================================================================*
 *                              Functions
 *============================================================================*/

uint8_t mtc_gap_set_pri(T_MTC_GAP_VENDOR_MODE para)
{
    T_GAP_CAUSE cause = GAP_CAUSE_ERROR_UNKNOWN;
#if (VENDOR_PRIORITY == 1)
    uint8_t prio_mode = 0;
    T_GAP_VENDOR_PRIORITY_PARAM p_priority_param;
    APP_PRINT_INFO2("mtc_gap_set_pri: initial_set %d, %d",
                    initial_set, para);
    if (!initial_set)
    {
        p_priority_param.set_priority_mode = GAP_VENDOR_RESET_PRIORITY;
        pendingmode = para;
        prio_mode = MTC_GAP_VENDOR_INIT;
    }
    else
    {
        pendingmode = MTC_GAP_VENDOR_NONE;
        prio_mode = para;
    }

    switch (prio_mode)
    {
    case MTC_GAP_VENDOR_INIT:
        {
            p_priority_param.link_priority_mode = GAP_VENDOR_SET_ALL_LINK_PRIORITY;
            p_priority_param.link_priority_level = GAP_VENDOR_PRIORITY_LEVEL_2;
            p_priority_param.adv_priority.set_priority_flag = true;
            p_priority_param.adv_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_2;
            p_priority_param.initiate_priority.set_priority_flag = true;
            p_priority_param.initiate_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_1;
            p_priority_param.scan_priority.set_priority_flag = true;
            p_priority_param.scan_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_1;
        }
        break;
    case MTC_GAP_VENDOR_ADV:
        {
            p_priority_param.link_priority_mode = GAP_VENDOR_NOT_SET_LINK_PRIORITY;
            p_priority_param.link_priority_level = GAP_VENDOR_PRIORITY_LEVEL_2;
            p_priority_param.adv_priority.set_priority_flag = true;
            p_priority_param.adv_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_3;
            p_priority_param.initiate_priority.set_priority_flag = false;
            p_priority_param.initiate_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_1;
            p_priority_param.scan_priority.set_priority_flag = false;
            p_priority_param.scan_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_1;
        }
        break;

    default:
        {

        }
        break;
    }
    cause = le_vendor_set_priority(&p_priority_param);
#endif
    return (uint8_t)cause;
}
/**
  * @brief Callback for gap le to notify app
  * @param[in] new_state gsp msf from le device state change msy type @ref T_GAP_DEV_STATE.
  * @param[in] cause.
  * @retval result @ref T_APP_RESULT
  */
void mtc_gap_handle_state_evt_callback(uint8_t new_state, uint16_t cause)
{
    T_GAP_DEV_STATE state = {0, 0, 0, 0, 0};
    memcpy(&state, &new_state, sizeof(new_state));
    APP_PRINT_TRACE4("mtc_gap_handle_state_evt_callback: state.gap_adv_state %d, state.gap_scan_state %d, state.gap_init_state %d, cause 0x%04x",
                     state.gap_adv_state,
                     state.gap_scan_state,
                     state.gap_init_state,
                     cause);
}

/**
  * @brief Callback for gap le to notify app gap state
  * @retval result @ref bool
  */
bool mtc_gap_is_ready(void)
{
    if (!app_ble_gap_stack_info())
    {
        return false;
    }
    else
    {
        return true;
    }
}

/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
uint8_t mtc_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CAUSE cause = GAP_CAUSE_ERROR_UNKNOWN;
#if (VENDOR_PRIORITY == 1)
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
    APP_PRINT_INFO1("mtc_gap_callback: conn_id 0x%x", cb_type);
    switch (cb_type)
    {
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        {
            //APP_PRINT_INFO3("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x",
            //                 p_data->p_le_data_len_change_info->conn_id,
            //                 p_data->p_le_data_len_change_info->max_tx_octets,
            //                 p_data->p_le_data_len_change_info->max_tx_time);
            cause = GAP_CAUSE_SUCCESS;
        } break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            // APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
            //                 p_data->p_le_modify_white_list_rsp->operation,
            //                 p_data->p_le_modify_white_list_rsp->cause);
            cause = GAP_CAUSE_SUCCESS;
        } break;

    case GAP_MSG_LE_VENDOR_SET_PRIORITY:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_VENDOR_SET_PRIORITY: cause 0x%x",
                            p_data->le_cause.cause);
            if (!p_data->le_cause.cause)
            {
                if (!initial_set)
                {
                    initial_set = true;
                    cause = (T_GAP_CAUSE)mtc_gap_set_pri(pendingmode);
                }
            }
        } break;

    default:
        cause = GAP_CAUSE_ERROR_UNKNOWN;
        break;
    }
#endif
    return (uint8_t)cause;
}


