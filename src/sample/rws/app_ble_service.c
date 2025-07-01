/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "ringtone.h"
#include "gap_conn_le.h"
#include "os_mem.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "transmit_service.h"
#include "app_ble_service.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_cfg.h"
#include "app_ble_gap.h"
#include "bas.h"
#include "dis.h"
#include "ble_stream.h"
#if GFPS_FEATURE_SUPPORT
#include "gfps.h"
#endif
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
#include "profile_server_ext.h"
#else
#include "profile_server.h"
#endif

#include "app_flags.h"

#include "app_multilink.h"
#include "app_harman_parser.h"
#include "app_harman_vendor_cmd.h"

static T_APP_RESULT audio_app_general_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    uint8_t conn_id;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    T_SERVER_EXT_APP_CB_DATA *p_para = (T_SERVER_EXT_APP_CB_DATA *)p_data;
    le_get_conn_id_by_handle(p_para->event_data.send_data_result.conn_handle, &conn_id);
#else
    T_SERVER_APP_CB_DATA *p_para = (T_SERVER_APP_CB_DATA *)p_data;
    conn_id = p_para->event_data.send_data_result.conn_id;
#endif
    APP_PRINT_INFO1("audio_app_general_srv_cb: conn_id %d", conn_id);

    switch (p_para->eventId)
    {
    case PROFILE_EVT_SRV_REG_COMPLETE:
        break;

    case PROFILE_EVT_SEND_DATA_COMPLETE:
        /*APP_PRINT_TRACE2("PROFILE_EVT_SEND_DATA_COMPLETE: cause 0x%x, attrib_idx %d",
                         p_para->event_data.send_data_result.cause,
                         p_para->event_data.send_data_result.attrib_idx);*/
        if (p_para->event_data.send_data_result.attrib_idx == TRANSMIT_SVC_TX_DATA_INDEX)
        {
            app_pop_data_transfer_queue(CMD_PATH_LE, true);
        }

        break;

    default:
        break;
    }
    return app_result;
}

static T_APP_RESULT audio_app_transmit_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_APP_LE_LINK *p_link;
    T_TRANSMIT_SRV_CALLBACK_DATA *p_callback = (T_TRANSMIT_SRV_CALLBACK_DATA *)p_data;

    APP_PRINT_INFO2("audio_app_transmit_srv_cb: conn_id %d, msg_type %d", p_callback->conn_id,
                    p_callback->msg_type);
    p_link = app_find_le_link_by_conn_id(p_callback->conn_id);
    if (p_link != NULL)
    {
        if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
        {
            app_harman_parser_process(HARMAN_PARSER_WRITE_CMD, p_data);
        }
        else if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
        {
            if (p_callback->attr_index == TRANSMIT_SVC_TX_DATA_CCCD_INDEX)
            {
                if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_ENABLE)
                {
                    p_link->transmit_srv_tx_enable_fg |= TX_ENABLE_CCCD_BIT;
                    APP_PRINT_INFO0("audio_app_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_ENABLE");
                }
                else if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_DISABLE)
                {
                    p_link->transmit_srv_tx_enable_fg &= ~TX_ENABLE_CCCD_BIT;
                    APP_PRINT_INFO0("audio_app_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_DISABLE");
                }
            }
#if HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT
            multilink_notify_deviceinfo(p_callback->conn_id, HARMAN_NOTIFY_DEVICE_INFO_TIME);
#endif
        }
        else if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE)
        {
            /* Update battery level for bas using */
            transmit_set_parameter(TRANSMIT_PARAM_BATTERY_LEVEL, 1, &app_db.local_batt_level);
            APP_PRINT_INFO1("[SD_CHECK]audio_app_transmit_srv_cb: local_batt_level %d",
                            app_db.local_batt_level);
        }
    }

    return app_result;
}

static T_APP_RESULT audio_app_ota_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_OTA_CALLBACK_DATA *p_ota_cb_data = (T_OTA_CALLBACK_DATA *)p_data;
    APP_PRINT_INFO2("audio_app_ota_srv_cb: service_id %d, msg_type %d",
                    service_id, p_ota_cb_data->msg_type);
    switch (p_ota_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        break;
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        if (OTA_WRITE_CHAR_VAL == p_ota_cb_data->msg_data.write.opcode &&
            OTA_VALUE_ENTER == p_ota_cb_data->msg_data.write.value)
        {
            /* Check battery level first */
            if (app_db.local_batt_level >= 30)
            {
                T_APP_LE_LINK *p_link;
                p_link = app_find_le_link_by_conn_id(p_ota_cb_data->conn_id);
                /* Battery level is greater than or equal to 30 percent */
                if (p_link != NULL)
                {
                    app_ble_disconnect(p_link, LE_LOCAL_DISC_CAUSE_SWITCH_TO_OTA);
                }
                APP_PRINT_INFO1("audio_app_ota_srv_cb: Preparing switch into OTA mode conn_id %d",
                                p_ota_cb_data->conn_id);
            }
            else
            {
                /* Battery level is less than 30 percent */
                APP_PRINT_WARN1("audio_app_ota_srv_cb: Battery level is not enough to support OTA, local_batt_level %d",
                                app_db.local_batt_level);
            }
        }
        break;

    default:

        break;
    }

    return app_result;
}

static T_APP_RESULT audio_app_bas_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
    switch (p_bas_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_bas_cb_data->msg_data.notification_indification_index == BAS_NOTIFY_BATTERY_LEVEL_ENABLE)
            {
                APP_PRINT_INFO0("audio_app_bas_srv_cb: BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
            }
            else if (p_bas_cb_data->msg_data.notification_indification_index ==
                     BAS_NOTIFY_BATTERY_LEVEL_DISABLE)
            {
                APP_PRINT_INFO0("audio_app_bas_srv_cb: BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
            }
        }
        break;

    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            /* Update battery level for bas using */
            bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &app_db.local_batt_level);
            APP_PRINT_INFO1("audio_app_bas_srv_cb: local_batt_level %d", app_db.local_batt_level);
        }
        break;

    default:
        break;
    }

    return app_result;
}

static T_APP_RESULT audio_app_dis_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_DIS_CALLBACK_DATA *p_dis_cb_data = (T_DIS_CALLBACK_DATA *)p_data;
    switch (p_dis_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            if (p_dis_cb_data->msg_data.read_value_index == DIS_READ_FIRMWARE_REV_INDEX)
            {
#if GFPS_FEATURE_SUPPORT
                const uint8_t DISFirmwareRev[] = GFPS_FIRMWARE_VERSION;
#else
                const uint8_t DISFirmwareRev[] = "1.0.0";
#endif
                dis_set_parameter(DIS_PARAM_FIRMWARE_REVISION,
                                  sizeof(DISFirmwareRev),
                                  (void *)DISFirmwareRev);
            }
            else if (p_dis_cb_data->msg_data.read_value_index == DIS_READ_PNP_ID_INDEX)
            {

            }
        }
        break;

    default:
        break;
    }

    return app_result;
}

void app_ble_service_init(void)
{
    /** NOTES: 4 includes transimit service, ota service, bas, dis service.
     *  if more ble service are added, you need to modify this value.
     * */

    uint8_t server_num = 4;

#if GFPS_FEATURE_SUPPORT
    server_num = server_num + 1;
#if GFPS_FINDER_SUPPORT
    server_num = server_num + 1;
#endif
#endif

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    server_cfg_use_ext_api(true);
    server_init(server_num);
    APP_PRINT_INFO0("app_ble_service_init: server_cfg_use_ext_api true");
    server_ext_register_app_cb(audio_app_general_srv_cb);
#else

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
    server_init(server_num + MAX_BLE_SRV_NUM);
#else
    server_init(server_num);
#endif
    server_register_app_cb(audio_app_general_srv_cb);
#endif

    /** Harman BLE_RX_TX Service */
    transmit_srv_add(audio_app_transmit_srv_cb);

    if (app_cfg_const.rtk_app_adv_support)
    {
        ota_add_service(audio_app_ota_srv_cb);
    }
    //bas_add_service(audio_app_bas_srv_cb);

    dis_add_service(audio_app_dis_srv_cb);//Add for GFPS
}
