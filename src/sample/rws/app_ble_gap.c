/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "sysm.h"
#include "ringtone.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_ext_scan.h"
#include "gap_timer.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "app_ble_gap.h"
#include "app_ble_service.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_report.h"
#include "app_cfg.h"
#include "remote.h"
#include "engage.h"
#include "app_ble_common_adv.h"
#if F_APP_TTS_SUPPORT
#include "app_ble_tts_adv.h"
#endif
#include "ble_conn.h"
#include "app_ble_device.h"
#include "app_cmd.h"
#include "ble_stream.h"
#include "ble_adv_data.h"
#include "app_audio_policy.h"
#include "app_ble_bond.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_auto_power_off.h"
#include "app_bt_sniffing.h"
#if GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#endif
#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xiaoai_transport.h"
#include "app_xm_xiaoai_ble_adv.h"
#include "app_xiaoai_device.h"
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "app_xiaowei_transport.h"
#include "app_xiaowei_ble_adv.h"
#include "app_xiaowei_device.h"
#endif
#if AMA_FEATURE_SUPPORT
#include "app_ama_ble.h"
#include "app_ama.h"
#include "app_ama_transport.h"
#include "app_ama_device.h"
#endif
#if BISTO_FEATURE_SUPPORT
#include "bisto_api.h"
#include "app_bisto_ble.h"
#endif
#if F_APP_BLE_SWIFT_PAIR_SUPPORT
#include "app_ble_swift_pair.h"
#endif
#if F_APP_LE_AUDIO_SM
#include "ble_audio.h"
#endif
#if GATTC_TBL_STORAGE_SUPPORT
#include "gattc_tbl_storage.h"
#endif
#if BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif
#if F_APP_TMAP_BMR_SUPPORT
#include "app_le_audio_scan.h"
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "asp_device_link.h"
#endif
#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_device.h"
#include "rtk_tuya_ble_adv.h"
#include "rtk_tuya_ble_app_demo.h"
#endif

#include "ble_mgr.h"
#include "app_ble_rand_addr_mgr.h"

#include "app_flags.h"
#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_ble.h"
#include "app_multilink.h"
#include "app_harman_ble_ota.h"
#include "app_harman_vendor_cmd.h"
#include "app_harman_eq.h"
#endif

typedef struct t_ble_whitelist_op_elem
{
    struct t_ble_whitelist_op_elem *p_next;
    T_GAP_WHITE_LIST_OP operation;
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    uint8_t bd_addr[6];
} T_BLE_WHITELIST_OP_ELEM;

static T_OS_QUEUE white_list_op_q;
static bool white_list_op_busy = false;
static T_GAP_DEV_STATE le_state;

uint8_t scan_rsp_data_len;
uint8_t scan_rsp_data[GAP_MAX_LEGACY_ADV_LEN];

bool app_ble_gen_scan_rsp_data(uint8_t *p_scan_len, uint8_t *p_scan_data)
{
    uint8_t device_name_len;

    if (p_scan_len == NULL || p_scan_data == NULL)
    {
        return false;
    }

    device_name_len = strlen((const char *)app_cfg_nv.device_name_le);

    if (device_name_len > GAP_MAX_LEGACY_ADV_LEN - 2)
    {
        device_name_len = GAP_MAX_LEGACY_ADV_LEN - 2;
    }

    p_scan_data[0] = device_name_len + 1;
    p_scan_data[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&p_scan_data[2], app_cfg_nv.device_name_le, device_name_len);
    *p_scan_len = device_name_len + 2;
    return true;
}

T_LE_LINK_ENCRYPTION_STATE app_ble_get_link_encryption_status(T_APP_LE_LINK *p_link)
{
    if (p_link)
    {
        return (T_LE_LINK_ENCRYPTION_STATE)p_link->encryption_status;
    }
    else
    {
        APP_PRINT_ERROR0("app_ble_get_link_encryption_status: link not exist");
        return LE_LINK_ERROR;
    }
}

static void app_ble_handle_dev_state_change_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_TRACE5("app_ble_handle_dev_state_change_evt: le_state.gap_adv_state %d, new_state.gap_adv_state %d, "
                     "le_state.gap_scan_state %d, new_state.gap_scan_state %d, cause 0x%04x",
                     le_state.gap_adv_state,
                     new_state.gap_adv_state,
                     le_state.gap_scan_state,
                     new_state.gap_scan_state,
                     cause);

    if (le_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO8("app_ble_handle_dev_state_change_evt: GAP stack ready role %d, tts_support %d, gfps_support %d, rtk_app_adv_support %d, ama_support %d, bisto_support %d, xiaoai_support %d, xiaowei_support %d",
                            app_cfg_nv.bud_role,
                            app_cfg_const.tts_support,
                            extend_app_cfg_const.gfps_support,
                            app_cfg_const.rtk_app_adv_support,
                            extend_app_cfg_const.ama_support,
                            extend_app_cfg_const.bisto_support,
                            extend_app_cfg_const.xiaoai_support,
                            extend_app_cfg_const.xiaowei_support);
        }
    }

    if (le_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
        {
            engage_handle_gap_scan_state(new_state, cause);
        }
    }

    if (le_state.gap_adv_state != new_state.gap_adv_state)
    {
        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
    }

    le_state = new_state;
#if (ISOC_AUDIO_SUPPORT == 1)
    uint8_t state;
    memcpy(&state, &new_state, sizeof(new_state));
    mtc_gap_handle_state_evt_callback(state, cause);
#endif
}

static void app_ble_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                              uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_TRACE3("app_ble_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);


    p_link = app_find_le_link_by_conn_id(conn_id);

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        if (p_link != NULL)
        {
            p_link->state = LE_LINK_STATE_DISCONNECTING;
        }
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        if (p_link != NULL)
        {
            uint8_t local_disc_cause = p_link->local_disc_cause;
            if (app_cfg_const.enable_data_uart)
            {
                uint8_t event_buff[3];

                event_buff[0] = p_link->id;
                event_buff[1] = (uint8_t)(disc_cause);
                event_buff[2] = (uint8_t)(disc_cause >> 8);
                app_report_event(CMD_PATH_UART, EVENT_LE_DISCONNECTED, 0, &event_buff[0], 3);
            }

            for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_peek(&p_link->disc_cb_list, i);
                if (p_entry != NULL && p_entry->disc_callback != NULL)
                {
                    p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
                }
            }

            app_transfer_queue_reset(CMD_PATH_LE);

#if (XM_XIAOAI_FEATURE_SUPPORT)
            if (extend_app_cfg_const.xiaoai_support)
            {
                app_xiaoai_handle_b2s_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                       disc_cause);
            }
#endif
#if (F_APP_XIAOWEI_FEATURE_SUPPORT)
            if (extend_app_cfg_const.xiaowei_support)
            {
                app_xiaowei_handle_b2s_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                        disc_cause);
            }
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
            T_ASP_INFO_LINK_STATE_DATA asp_link_state;
            T_ASP_DEVICE_LINK *p_asp_link = NULL;
            p_asp_link = asp_device_find_link_by_addr(p_link->bd_addr);
            if (p_asp_link && p_asp_link->link_state == ASP_DEVICE_LINK_CONNECTED_ACTIVE)
            {
                asp_link_state.asp_link_state = false;
                memcpy(asp_link_state.bd_addr, p_link->bd_addr, 6);
                asp_device_handle_asp_info(T_ASP_DEVICE_ASP_INFO_STATE, &asp_link_state, sizeof(asp_link_state));
            }
            asp_device_free_link(p_link->bd_addr);
#endif

#if F_APP_TUYA_SUPPORT
            app_tuya_handle_b2s_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                 disc_cause);
#endif

            if (conn_id == le_common_adv_get_conn_id())
            {
                le_common_adv_reset_conn_id();
                app_cmd_update_eq_ctrl(false, true);
#if F_APP_HARMAN_FEATURE_SUPPORT
                multilink_notify_deviceinfo_stop();

                if (app_harman_ble_ota_get_upgrate_status())
                {
                    app_harman_ota_exit(OTA_EXIT_REASON_BLE_DISCONN);
                }

                if (disc_cause == HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)
                {
                    app_harman_eq_reset_unsaved();
                }
#endif
            }

            app_free_le_link(p_link);

#if AMA_FEATURE_SUPPORT
            if (extend_app_cfg_const.ama_support)
            {
                T_BLE_STREAM_EVENT_PARAM ble_stream_param;
                ble_stream_param.disconnect_param.conn_id = conn_id;
                ble_stream_event_process(BLE_STREAM_DISCONNECT_EVENT, &ble_stream_param);
                le_ama_adv_start();

                app_ama_stream_delete(AMA_BLE_STREAM, NULL, conn_id);

                uint8_t bd_addr[6] = {0};
                T_GAP_REMOTE_ADDR_TYPE bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
                le_get_conn_addr(conn_id, bd_addr, (uint8_t *)&bd_type);
                if (bd_type == GAP_REMOTE_ADDR_LE_PUBLIC)
                {
                    app_ama_device_delete(COMMON_STREAM_BT_LE, bd_addr);
                }
                else
                {
                    APP_PRINT_ERROR1("app_ble_handle_new_conn_state_evt: bd_type %d, should be GAP_REMOTE_ADDR_LE_PUBLIC",
                                     bd_type);
                }

            }
#endif

            if (app_cfg_const.rtk_app_adv_support)
            {
                APP_PRINT_ERROR1("GAP_CONN_STATE_DISCONNECTED: b2s=%d", app_find_b2s_link_num());
                if (local_disc_cause != LE_LOCAL_DISC_CAUSE_POWER_OFF &&
                    local_disc_cause != LE_LOCAL_DISC_CAUSE_ROLESWAP
#if F_APP_HARMAN_FEATURE_SUPPORT
                    && app_find_b2s_link_num()
#endif
                   )
                {
                    app_ble_rtk_adv_start();
                }
            }

            if (app_get_ble_link_num() == 0)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST, app_cfg_const.timer_auto_power_off);

                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_BLE_DISC);
            }

            app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_CONN_STATE_CHANGE);
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = LE_LINK_STATE_CONNECTING;
            }
#if BISTO_FEATURE_SUPPORT
            if (extend_app_cfg_const.bisto_support)
            {
                bisto_ble_set_conn_id(bisto_ble_get_instance(), conn_id);
            }
#endif
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        if (p_link != NULL)
        {
            p_link->conn_handle = le_get_conn_handle(conn_id);
            app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_CONN_STATE_CHANGE);

#if AMA_FEATURE_SUPPORT
            if (extend_app_cfg_const.ama_support)
            {
                app_ama_stream_create(AMA_BLE_STREAM, NULL, conn_id);
            }
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
            T_ASP_DEVICE_LINK_ALLOC_PARAM param_data;
            param_data.bd_addr = p_link->bd_addr;
            param_data.conn_id = conn_id;
            asp_device_alloc_link(ASP_DEVICE_LINK_BLE, &param_data);
#endif

            if (p_link->state == LE_LINK_STATE_CONNECTING)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST);

                p_link->state = LE_LINK_STATE_CONNECTED;

                le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
                if (app_cfg_const.enable_data_uart)
                {
                    app_report_event(CMD_PATH_UART, EVENT_LE_CONNECTED, 0, &p_link->id, 1);
                }
            }

            /*secondary ear ota need modify the random address and start advertising,
             when ble link connected, the random address shall be set back to le_rws_random_addr*/
            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint8_t rand_addr[6] = {0};
                app_ble_rand_addr_get(rand_addr);
                le_common_adv_set_random(rand_addr);
            }
#if F_APP_LE_AUDIO_SM
            app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_CONNECT, NULL);
#endif

            uint16_t conn_interval_min = RWS_LE_DEFAULT_MIN_CONN_INTERVAL;
            uint16_t conn_interval_max = RWS_LE_DEFAULT_MAX_CONN_INTERVAL;
            uint16_t conn_latency = RWS_LE_DEFAULT_SLAVE_LATENCY;
            uint16_t conn_supervision_timeout = RWS_LE_DEFAULT_SUPERVISION_TIMEOUT;

            ble_set_prefer_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                                      conn_supervision_timeout);

#if F_APP_TUYA_SUPPORT
            app_tuya_handle_b2s_ble_connected(p_link->bd_addr, conn_id);
#endif

#if GFPS_FEATURE_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                app_gfps_set_ble_conn_param(conn_id);
            }
#endif
        }
        break;

    default:
        break;
    }
#if BT_GATT_CLIENT_SUPPORT
    gatt_client_handle_conn_state_evt(conn_id, new_state);
#endif
}

#if BT_GATT_CLIENT_SUPPORT
T_APP_RESULT app_bt_client_discov_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                     void *p_data)
{
    if (type == GATT_CLIENT_EVENT_DIS_ALL_STATE)
    {
        T_GATT_CLIENT_DIS_ALL_DONE *p_disc = (T_GATT_CLIENT_DIS_ALL_DONE *)p_data;
        APP_PRINT_INFO2("app_bt_client_discov_cb:is_success %d, load_form_ftl %d",
                        p_disc->state, p_disc->load_form_ftl);
    }
    return APP_RESULT_SUCCESS;
}
#endif

void app_ble_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO3("app_ble_handle_authen_state_evt:conn_id %d, state %d, cause 0x%x",
                    conn_id, new_state, cause);

    if (new_state == GAP_AUTHEN_STATE_COMPLETE)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_find_le_link_by_conn_id(conn_id);
        if (p_link != NULL)
        {
#if BISTO_FEATURE_SUPPORT
            if (extend_app_cfg_const.bisto_support)
            {
                bisto_ble_set_bonded(bisto_ble_get_instance(), true);
                app_bisto_gatt_client_start_discovery(p_link->conn_id);
            }
#endif
            if (app_cfg_const.enable_data_uart)
            {
                uint8_t event_buff[3];
                event_buff[0] = p_link->id;
                event_buff[1] = cause;
                event_buff[2] = cause >> 8;
                app_report_event(CMD_PATH_UART, EVENT_LE_PAIR_STATUS, 0, &event_buff[0], 3);
            }

            if (cause == GAP_SUCCESS)
            {
                uint8_t addr[6];
                T_GAP_REMOTE_ADDR_TYPE bd_type;
                uint8_t resolved_addr[6];
                T_GAP_IDENT_ADDR_TYPE resolved_bd_type;

                p_link->transmit_srv_tx_enable_fg |= TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_ENCRYPTIONED;
                app_audio_tone_type_play(TONE_LE_PAIR_COMPLETE, false, false);
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_ENCRYPT_STATE_CHANGE);

                le_get_conn_addr(conn_id, addr, &bd_type);
                if (bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
                {
                    if (le_resolve_random_address(addr, resolved_addr, &resolved_bd_type) == true)
                    {
                        if (resolved_bd_type == GAP_IDENT_ADDR_PUBLIC)
                        {
#if F_APP_BLE_BOND_SYNC_SUPPORT
                            app_ble_bond_add_send_to_sec(resolved_addr);
#endif

                            memcpy(p_link->bd_addr, resolved_addr, 6);
#if AMA_FEATURE_SUPPORT | BISTO_FEATURE_SUPPORT
                            ble_stream_copy_pub_addr(conn_id, resolved_addr);
#endif

#if AMA_FEATURE_SUPPORT
                            //app_ama_device_create should after copy addr for update ble_stream addr
                            app_ama_device_create(COMMON_STREAM_BT_LE, resolved_addr);
#endif

                        }
                    }
                }
                else if (bd_type == GAP_REMOTE_ADDR_LE_PUBLIC)
                {
                    memcpy(p_link->bd_addr, addr, 6);
                }

#if XM_XIAOAI_FEATURE_SUPPORT
                if (extend_app_cfg_const.xiaoai_support)
                {
                    app_xiaoai_handle_b2s_ble_connected(p_link->bd_addr, conn_id);
                    app_xiaoai_device_set_mtu(T_XM_CMD_COMMUNICATION_WAY_TYPE_BLE, p_link->bd_addr, conn_id,
                                              p_link->mtu_size - 3);
                }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
                if (extend_app_cfg_const.xiaowei_support)
                {
                    app_xiaowei_handle_b2s_ble_connected(p_link->bd_addr, conn_id);
                    app_xiaowei_device_set_mtu(XIAOWEI_CONNECTION_TYPE_BLE, p_link->bd_addr, conn_id,
                                               p_link->mtu_size - 3);
                }
#endif
#if F_APP_LE_AUDIO_SM
                app_le_audio_device_sm(LE_AUDIO_ADV_STOP, NULL);
#endif

#if HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT
                multilink_notify_deviceinfo(conn_id, HARMAN_NOTIFY_DEVICE_INFO_TIME);
#endif
                // LE supervision timeout 5sec
                if (conn_id == le_common_adv_get_conn_id())
                {
                    uint16_t conn_interval_min = RWS_LE_DEFAULT_MIN_CONN_INTERVAL;
                    uint16_t conn_interval_max = RWS_LE_DEFAULT_MAX_CONN_INTERVAL;
                    uint16_t conn_latency = RWS_LE_DEFAULT_SLAVE_LATENCY;
                    uint16_t conn_supervision_timeout = RWS_LE_DEFAULT_SUPERVISION_TIMEOUT;

                    ble_set_prefer_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                                              conn_supervision_timeout);
                }
            }
            else
            {
                p_link->transmit_srv_tx_enable_fg &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_UNENCRYPTIONED;
#if !HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT
                app_ble_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);
#endif
            }
#if BT_GATT_CLIENT_SUPPORT
            if (cause == GAP_SUCCESS)
            {
                p_link->auth_cmpl = true;
                if (p_link->mtu_received)
                {
                    gatt_client_start_discovery_all(p_link->conn_handle, app_bt_client_discov_cb);
                }
            }
            else
            {
                p_link->auth_cmpl = false;
            }
#endif
        }
    }
#if F_APP_LE_AUDIO_SM
    else if (new_state == GAP_AUTHEN_STATE_STARTED)
    {
        APP_PRINT_INFO0("app_ble_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
    }
#endif
}

T_GAP_CAUSE app_ble_modify_white_list(T_GAP_WHITE_LIST_OP  operation, uint8_t *bd_addr,
                                      T_GAP_REMOTE_ADDR_TYPE bd_type)
{
    T_GAP_CAUSE ret;
    if (white_list_op_busy == false)
    {
        white_list_op_busy = true;
        ret = le_modify_white_list(operation, bd_addr, bd_type);
    }
    else
    {
        T_BLE_WHITELIST_OP_ELEM *op = malloc(sizeof(T_BLE_WHITELIST_OP_ELEM));
        if (!op)
        {
            ret = GAP_CAUSE_NO_RESOURCE;
            return ret;
        }
        op->operation = operation;
        op->bd_type = bd_type;
        memcpy(op->bd_addr, bd_addr, 6);
        os_queue_in(&white_list_op_q, op);
        ret = GAP_CAUSE_SUCCESS;
    }
    return ret;
}

void app_ble_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    APP_PRINT_TRACE1("app_ble_handle_gap_msg: subtype %d", p_io_msg->subtype);
    T_LE_GAP_MSG stack_msg;
    T_APP_LE_LINK *p_link;

    memcpy(&stack_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    ble_audio_handle_gap_msg(p_io_msg->subtype, stack_msg);
#endif

    ble_mgr_handle_gap_msg(p_io_msg->subtype, &stack_msg);

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_ble_handle_dev_state_change_evt(stack_msg.msg_data.gap_dev_state_change.new_state,
                                                stack_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_ble_handle_new_conn_state_evt(stack_msg.msg_data.gap_conn_state_change.conn_id,
                                              (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                                              stack_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            p_link = app_find_le_link_by_conn_id(stack_msg.msg_data.gap_conn_mtu_info.conn_id);
            if (p_link != NULL)
            {
                p_link->mtu_size = stack_msg.msg_data.gap_conn_mtu_info.mtu_size;

#if AMA_FEATURE_SUPPORT
                if (extend_app_cfg_const.ama_support && app_ama_transport_supported(AMA_BLE_STREAM))
                {
                    T_BLE_STREAM_EVENT_PARAM ble_stream_param;
                    ble_stream_param.update_mut_param.conn_id = stack_msg.msg_data.gap_conn_mtu_info.conn_id;
                    ble_stream_param.update_mut_param.mtu     = stack_msg.msg_data.gap_conn_mtu_info.mtu_size;
                    ble_stream_event_process(BLE_STREAM_UPDATE_MTU_EVENT, &ble_stream_param);
                }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
                app_xiaoai_device_set_mtu(T_XM_CMD_COMMUNICATION_WAY_TYPE_BLE, p_link->bd_addr, p_link->conn_id,
                                          p_link->mtu_size - 3);
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
                app_xiaowei_device_set_mtu(XIAOWEI_CONNECTION_TYPE_BLE, p_link->bd_addr, p_link->conn_id,
                                           p_link->mtu_size - 3);
#endif
//#if F_APP_LE_AUDIO_SM
                //TODO:  to avoid service discovery taking long time, change to 40ms
//                ble_set_prefer_conn_param(p_link->conn_id, 0x20, 0x20, 0, 500);
//#endif

#if BT_GATT_CLIENT_SUPPORT
                p_link->mtu_received = true;
                if (p_link->auth_cmpl)
                {
                    gatt_client_start_discovery_all(p_link->conn_handle,
                                                    app_bt_client_discov_cb);
                }
#endif
            }
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_ble_handle_authen_state_evt(stack_msg.msg_data.gap_authen_state.conn_id,
                                            stack_msg.msg_data.gap_authen_state.new_state,
                                            stack_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            uint8_t conn_id = stack_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("app_ble_handle_gap_msg: GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
#if GFPS_FEATURE_SUPPORT
            if (extend_app_cfg_const.gfps_support)
            {
                app_gfps_handle_ble_user_confirm(stack_msg.msg_data.gap_bond_user_conf.conn_id);
            }
            else
#endif
            {
                le_bond_user_confirm(stack_msg.msg_data.gap_bond_user_conf.conn_id, GAP_CFM_CAUSE_ACCEPT);
            }
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        break;

    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE:
        {
            if ((T_GAP_EXT_ADV_STATE)stack_msg.msg_data.gap_ext_adv_state_change.new_state ==
                EXT_ADV_STATE_ADVERTISING)
            {
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
            }
            else if (((T_GAP_EXT_ADV_STATE)stack_msg.msg_data.gap_ext_adv_state_change.new_state ==
                      EXT_ADV_STATE_IDLE) && (stack_msg.msg_data.gap_ext_adv_state_change.cause != 0))
            {
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
            }
        }
        break;

    default:
        break;
    }
}

static T_APP_RESULT app_ble_gap_cb(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    ble_audio_handle_gap_cb(cb_type, p_cb_data);
#endif

    ble_mgr_handle_gap_cb(cb_type, p_cb_data);

    switch (cb_type)
    {
    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE_INFO:
        {
            if ((T_GAP_EXT_ADV_STATE)cb_data.p_le_ext_adv_state_change_info->state ==
                EXT_ADV_STATE_ADVERTISING)
            {
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
            }
            else if (((T_GAP_EXT_ADV_STATE)cb_data.p_le_ext_adv_state_change_info->state ==
                      EXT_ADV_STATE_IDLE) && (cb_data.p_le_ext_adv_state_change_info->cause != 0))
            {
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
            }
        }
        break;
    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
        APP_PRINT_INFO6("GAP_MSG_LE_EXT_ADV_REPORT_INFO:event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d,data %b",
                        cb_data.p_le_ext_adv_report_info->event_type,
                        TRACE_BDADDR(cb_data.p_le_ext_adv_report_info->bd_addr),
                        cb_data.p_le_ext_adv_report_info->addr_type,
                        cb_data.p_le_ext_adv_report_info->rssi,
                        cb_data.p_le_ext_adv_report_info->data_len,
                        TRACE_BINARY(cb_data.p_le_ext_adv_report_info->data_len, cb_data.p_le_ext_adv_report_info->p_data));

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_nv.one_wire_start_force_engage)
        {
            if (!memcmp(cb_data.p_le_ext_adv_report_info->bd_addr, app_cfg_nv.bud_peer_factory_addr, 6))
            {
                APP_PRINT_TRACE1("GAP_MSG_LE_EXT_ADV_REPORT_INFO: scan peer factory addr %s",
                                 TRACE_BDADDR(app_cfg_nv.bud_peer_factory_addr));
            }
            else
            {
                break;
            }
        }
#endif

#if F_APP_TMAP_BMR_SUPPORT
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE && app_bt_policy_get_b2b_connected() == false)
#else
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
#endif
        {
            engage_handle_le_scan_info(cb_data.p_le_ext_adv_report_info);
        }
#if F_APP_TMAP_BMR_SUPPORT
        app_le_audio_scan_handle_report(cb_data.p_le_ext_adv_report_info);
#endif
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        result = APP_RESULT_ACCEPT;
        break;

    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation = 0x%x cause=0x%x",
                            cb_data.p_le_modify_white_list_rsp->operation,
                            cb_data.p_le_modify_white_list_rsp->cause);

            white_list_op_busy = false;
            if (white_list_op_q.count != 0)
            {
                T_BLE_WHITELIST_OP_ELEM *op = os_queue_out(&white_list_op_q);
                if (op != NULL)
                {
                    app_ble_modify_white_list(op->operation, op->bd_addr, op->bd_type);
                    free(op);
                    op = NULL;
                }
            }
        }
        break;
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                            cb_data.p_le_bond_modify_info->type);
#if GATTC_TBL_STORAGE_SUPPORT
            gattc_tbl_storage_handle_bond_modify(cb_data.p_le_bond_modify_info);
#endif
        }
        break;

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    case GAP_MSG_LE_EXT_SCAN_STATE_CHANGE_INFO:
        {
            if (cb_data.p_le_ext_scan_state_change_info->state == GAP_SCAN_STATE_IDLE)
            {
                APP_PRINT_INFO0("GAP ext scan stop");
            }
            else if (cb_data.p_le_ext_scan_state_change_info->state == GAP_SCAN_STATE_SCANNING)
            {
                APP_PRINT_INFO0("GAP ext scan start");
            }
        }
        break;
#endif
    default:
        break;
    }
#if (ISOC_AUDIO_SUPPORT == 1)
    mtc_gap_callback(cb_type, p_cb_data);
#endif

    return result;
}

bool app_ble_disconnect(T_APP_LE_LINK *p_link, T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    if (p_link != NULL)
    {
        APP_PRINT_TRACE2("app_ble_disconnect: conn_id %d, disc_cause %d",
                         p_link->conn_id, disc_cause);
        if (le_disconnect(p_link->conn_id) == GAP_CAUSE_SUCCESS)
        {
            p_link->local_disc_cause = disc_cause;
            return true;
        }
    }
    return false;
}

void app_ble_disconnect_all(T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        p_link = app_find_le_link_by_conn_id(i);
        if (p_link != NULL && p_link->used)
        {
            if (p_link->state == LE_LINK_STATE_CONNECTING
                || p_link->state == LE_LINK_STATE_CONNECTED)
            {
                app_ble_disconnect(p_link, disc_cause);
            }
        }
    }
}

static uint32_t get_ext_adv_num(void)
{
    uint8_t adv_handle_number = 0;

#if F_APP_AVP_INIT_SUPPORT
    {
        adv_handle_number++;//for durian adv
    }
#endif

#if F_APP_ERWS_SUPPORT
    {
        adv_handle_number++;//for engage adv
    }
#endif

    if (extend_app_cfg_const.ama_support || extend_app_cfg_const.xiaowei_support ||
        extend_app_cfg_const.bisto_support ||
        (extend_app_cfg_const.xiaoai_support &&
         !app_cfg_const.enable_rtk_charging_box))
    {
        adv_handle_number++;//for public address adv,those adv has a single adv handle.
    }

    if (app_cfg_const.swift_pair_support)
    {
        adv_handle_number++;//for swift pair adv
    }

    if (app_cfg_const.rtk_app_adv_support)
    {
        adv_handle_number++;//for common adv
    }

#if F_APP_TTS_SUPPORT
    adv_handle_number++;//for tts ibeacon
#endif

    if (app_cfg_nv.ota_tooling_start)
    {
        adv_handle_number++;//for ota tooling adv
    }

    if (app_cfg_const.enable_rtk_fast_pair_adv)
    {
        adv_handle_number++;//for rtk fast pair adv
    }

    if (app_cfg_const.enable_rtk_charging_box)
    {
        adv_handle_number++;//for xiaomi chargerbox adv
    }

    if (extend_app_cfg_const.gfps_support)
    {
        adv_handle_number++;//for gfps adv
#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            adv_handle_number++;//for gfps finder beacon adv
        }
#endif
    }

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    adv_handle_number++;
#endif
#if F_APP_CSIS_SUPPORT
    adv_handle_number++;
#endif
#if F_APP_TMAP_BMR_SUPPORT
    adv_handle_number++;
#endif

#if F_APP_TUYA_SUPPORT
    adv_handle_number++;//for tuya adv
#endif

    return adv_handle_number;
}


void app_ble_mgr_init(void)
{
    BLE_MGR_PARAMS param = {0};

    param.ble_ext_adv.enable = true;
    param.ble_ext_adv.adv_num = get_ext_adv_num();

    param.ble_conn.enable = true;
    uint8_t supported_max_le_link_num = le_get_max_link_num();
    APP_PRINT_TRACE1("app_ble_mgr_init: le_get_max_link_num %d",
                     supported_max_le_link_num);
    param.ble_conn.link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                               supported_max_le_link_num);

    if (extend_app_cfg_const.ama_support || extend_app_cfg_const.xiaowei_support ||
        extend_app_cfg_const.bisto_support ||
        (extend_app_cfg_const.xiaoai_support &&
         !app_cfg_const.enable_rtk_charging_box))
    {
        param.ble_adv_data.enable = true;
        param.ble_adv_data.update_scan_data = true;
        param.ble_adv_data.adv_interval = (extend_app_cfg_const.multi_adv_interval * 8) / 5;
        app_ble_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);
        param.ble_adv_data.scan_rsp_len = scan_rsp_data_len;
        param.ble_adv_data.scan_rsp_data = scan_rsp_data;
    }

    ble_mgr_init(&param);
}


void app_ble_gap_init(void)
{
    le_register_app_cb(app_ble_gap_cb);
    app_ble_rand_addr_init();
    app_ble_mgr_init();

    os_queue_init(&white_list_op_q);

#if F_APP_TTS_SUPPORT
    if (app_cfg_const.tts_support)
    {
        tts_ibeacon_adv_init();
    }
#endif
}

void app_ble_gap_param_init(void)
{
    //device name and device appearance
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    T_GAP_SCAN_MODE  scan_mode = GAP_SCAN_MODE_PASSIVE;
    uint8_t scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    //GAP Bond Manager parameters
    uint32_t passkey = 0; // passkey "000000"
    uint8_t use_fixed_passkey = false;
    uint8_t sec_req_enable = false;
    uint16_t sec_req_requirement = GAP_AUTHEN_BIT_SC_FLAG;
    /* Initialize extended scan parameters */
    T_GAP_LOCAL_ADDR_TYPE  own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];
    extended_scan_param[0].scan_type = scan_mode;
    extended_scan_param[0].scan_interval = 0x10;
    extended_scan_param[0].scan_window = 0x10;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;

    uint8_t supported_max_le_link_num = le_get_max_link_num();
    APP_PRINT_TRACE1("app_ble_gap_param_init: le_get_max_link_num %d",
                     supported_max_le_link_num);
    uint8_t link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                        supported_max_le_link_num);
    le_gap_init(link_num);
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    bool cis_flag = true;
    le_set_gap_param(GAP_PARAM_CIS_HOST_SUPPORT, sizeof(cis_flag), &cis_flag);
#endif
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_le);

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_set_ble_name();
    }
#endif
    //Set device appearance
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

    // Setup the GAP Bond Manager
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(uint32_t), &passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(uint8_t), &use_fixed_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_requirement);

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_LOCAL_ADDR_TYPE, sizeof(own_address_type),
                          &own_address_type);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                          &scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                          &scan_filter_duplicate);
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);

    uint8_t slave_init_gatt_mtu_req = true;
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_gatt_mtu_req),
                     &slave_init_gatt_mtu_req);
}

#if (ISOC_AUDIO_SUPPORT == 1)
bool app_ble_gap_stack_info(void)
{
    if (le_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif

