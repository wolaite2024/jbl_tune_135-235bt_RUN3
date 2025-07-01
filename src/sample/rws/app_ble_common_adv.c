/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "gap_timer.h"
#include "trace.h"
#include "gap_le_types.h"
#include "app_ble_common_adv.h"
#include "ble_ext_adv.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_ble_gap.h"
#include "app_ble_tts.h"
#include "app_relay.h"
#include "app_ble_device.h"
#include <stdlib.h>
#include "app_ble_rand_addr_mgr.h"

#include "app_flags.h"
#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_ble.h"
#include "app_harman_license.h"
#include "app_multilink.h"
#include "app_harman_ble.h"
#endif

typedef enum
{
    SYNC_BLE_ADV_START_FLAG     = 0x00,

    LE_COMMON_REMOTE_MSG_MAX    = 0x01,
} LE_COMMON_REMOTE_MSG;


typedef struct CB_ELEM CB_ELEM;

typedef struct CB_ELEM
{
    CB_ELEM             *p_next;
    LE_COMMON_ADV_CB    cb;
} CB_ELEM;



struct
{
    bool use_static_addr_type;
    uint8_t adv_handle;
    uint8_t conn_id;
    T_BLE_EXT_ADV_MGR_STATE state;
    T_OS_QUEUE cb_queue;
} le_common_adv =
{
    .use_static_addr_type = true,
    .adv_handle = 0xff,
    .conn_id = 0xff,
    .state = BLE_EXT_ADV_MGR_ADV_DISABLED,
    .cb_queue = {0}
};



static uint8_t common_adv_data[31] =
{
    /* This modification is used to solve issue: BLE name shown on Android list */
    /* TX Power Level */
    0x02,/* length */
    GAP_ADTYPE_POWER_LEVEL,
    0x00, /* Don't care now */

    /* Service */
    17,/* length */
    GAP_ADTYPE_128BIT_MORE,/* "Complete 128-bit UUIDs available"*/
    /*transmit service uuid*/
    0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFD, 0x02, 0x00,
    0x00,/*bud role:0x00 single,0x01 pri, 0x02 sec*/

    /* Manufacturer Specific Data */
    9,/* length */
    0xFF,
    0x5D, 0x00 /* Company ID */
    /*local address*/
};


static void le_common_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE2("le_common_adv_callback: adv_state %d, adv_handle %d",
                             cb_data.p_ble_state_change->state, cb_data.p_ble_state_change->adv_handle);
            le_common_adv.state = cb_data.p_ble_state_change->state;
            if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
            }
            else if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    if ((app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && le_common_adv.use_static_addr_type)
                    {
                        /*secondary ear ota need modify the random address and start advertising,
                        when advertising timeout, the random address shall be set back to le_rws_random_addr*/
                        uint8_t rand_addr[6] = {0};
                        app_ble_rand_addr_get(rand_addr);
                        le_common_adv_set_random(rand_addr);
                    }
                    break;

                default:
                    break;
                }
                APP_PRINT_TRACE2("le_common_adv_callback: stack stop adv cause 0x%x, app stop adv cause 0x%02x",
                                 cb_data.p_ble_state_change->stop_cause, cb_data.p_ble_state_change->app_cause);
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE1("le_common_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                         cb_data.p_ble_conn_info->conn_id);

        le_common_adv.conn_id = cb_data.p_ble_conn_info->conn_id;
        break;

    default:
        break;
    }

    CB_ELEM *p_cb_elem = (CB_ELEM *)le_common_adv.cb_queue.p_first;
    APP_PRINT_TRACE1("le_common_adv_callback: p_cb_elem %p", p_cb_elem);

    for (; p_cb_elem != NULL; p_cb_elem = p_cb_elem->p_next)
    {
        APP_PRINT_TRACE1("le_common_adv_callback: cb %p", p_cb_elem->cb);
        p_cb_elem->cb(cb_type, cb_data);
    }

    return;
}

#if F_APP_HARMAN_FEATURE_SUPPORT
void app_harman_adv_data_update(uint8_t *p_adv_data, uint8_t adv_data_len)
{
    APP_PRINT_TRACE1("app_harman_adv_data_update: BLE_state: %d", le_common_adv.state);
    if (adv_data_len <= 31)
    {
        memcpy(common_adv_data, p_adv_data, adv_data_len);
        ble_ext_adv_mgr_set_adv_data(le_common_adv.adv_handle, adv_data_len, common_adv_data);
    }
    else
    {
        APP_PRINT_ERROR1("app_harman_adv_data_update: adv_data_len is exceed, %d",
                         adv_data_len);
    }

}
#endif

bool le_common_ota_adv_data_update(void)
{
    uint8_t mac_addr[6];

    common_adv_data[20] = app_cfg_nv.bud_role;
    common_adv_data[21] = 3 + sizeof(mac_addr);
    common_adv_data[22] = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
    common_adv_data[23] = 0x5D;
    common_adv_data[24] = 0x00;

    for (int i = 0; i < 6; i++)
    {
        mac_addr[i] = app_cfg_nv.bud_peer_addr[5 - i];
    }
    memcpy(&common_adv_data[25], mac_addr, sizeof(mac_addr));

    /*Ensure that the advertising address of the secondary ear and the primary ear are different.
      If the advertising addresses on both sides are the same, the phone may not know which ear to be upgrade.
      secondary ear ota need modify the random address and start advertising,
      when advertising timeout, the random address shall be set back to le_rws_random_addr,
      when secondary ble link connected,the random address shall be set back to le_rws_random_addr */
    if (le_common_adv.use_static_addr_type)
    {
        uint8_t ota_random_address[6] = {0};
        app_ble_rand_addr_get(ota_random_address);
        ota_random_address[0]++;
        le_common_adv_set_random(ota_random_address);
    }

    if (ble_ext_adv_mgr_set_adv_data(le_common_adv.adv_handle, sizeof(common_adv_data),
                                     common_adv_data) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void rws_le_common_adv_update_bud_role(T_REMOTE_SESSION_ROLE role)
{
    /*common_adv_data update bud role*/
    if (common_adv_data[20] != role)
    {
        common_adv_data[20] = role;
        ble_ext_adv_mgr_set_adv_data(le_common_adv.adv_handle, sizeof(common_adv_data), common_adv_data);
    }
    else
    {
        //bud role not change
    }
}

void harman_le_common_disc(void)
{
    APP_PRINT_INFO0("harman_le_common_disc");
    //ble link disconnect
    if (le_common_adv.conn_id != 0xFF)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_find_le_link_by_conn_id(le_common_adv.conn_id);

        if (p_link != NULL)
        {
            app_ble_disconnect(p_link, LE_LOCAL_DISC_CAUSE_HARMAN_DEVICE_DISC);
        }
    }
}

void rws_le_common_secondary_bud_stop_adv_and_link()
{
    APP_PRINT_INFO0("rws_le_common_secondary_bud_stop_adv_and_link");
    //le_common_adv_stop
    if (le_common_adv_get_state() != BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        le_common_adv_stop(APP_STOP_ADV_CAUSE_ROLE_SECONDARY);
    }

    //ble link disconnect
    if (le_common_adv.conn_id != 0xFF)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_find_le_link_by_conn_id(le_common_adv.conn_id);

        if (p_link != NULL)
        {
            app_ble_disconnect(p_link, LE_LOCAL_DISC_CAUSE_SECONDARY);
        }
    }
}

void rws_le_common_adv_handle_roleswap(T_REMOTE_SESSION_ROLE role)
{
    /*common_adv_data update bud role*/
    rws_le_common_adv_update_bud_role(role);
    APP_PRINT_INFO3("rws_le_common_adv_handle_roleswap: common_adv_data %b,conn_id %d %d",
                    TRACE_BINARY(sizeof(common_adv_data), common_adv_data), le_common_adv.conn_id,
                    app_db.ble_common_adv_after_roleswap);

    if (role == REMOTE_SESSION_ROLE_PRIMARY && app_db.ble_common_adv_after_roleswap)
    {
        rws_le_common_adv_start(app_cfg_const.timer_ota_adv_timeout * 100);
    }
    else if (role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        rws_le_common_secondary_bud_stop_adv_and_link();
    }
    app_db.ble_common_adv_after_roleswap = false;
}

void rws_le_common_adv_handle_role_swap_fail(T_REMOTE_SESSION_ROLE device_role)
{
    if (device_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        rws_le_common_adv_start(app_cfg_const.timer_ota_adv_timeout * 100);
    }
}

void rws_le_common_adv_handle_engage_role_decided(void)
{
    /*common_adv_data update bud role*/
    rws_le_common_adv_update_bud_role((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    APP_PRINT_INFO2("rws_le_common_adv_handle_engage_role_decided: common_adv_data %b,conn_id %d",
                    TRACE_BINARY(sizeof(common_adv_data), common_adv_data), le_common_adv.conn_id);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_ble_rtk_adv_start();
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        rws_le_common_secondary_bud_stop_adv_and_link();
    }
}

bool rws_le_common_adv_start(uint16_t duration_10ms)
{
    /*rtk adv only support one ble link, if rtk ble link already exist, shall not start rtk adv*/
    if (le_common_adv.conn_id != 0xFF)
    {
        APP_PRINT_ERROR0("rws_le_common_adv_start: rtk link already exist");
        return false;
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        APP_PRINT_TRACE0("rws_le_common_adv_start: bud_role 2, should not start adv");
        return false;
    }
    bool ret = le_common_adv_start(duration_10ms);
    return ret;
}


#if F_APP_ERWS_SUPPORT

uint16_t app_le_common_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    APP_PRINT_INFO1("app_le_common_relay_cback: msg_type %d", msg_type);

    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case SYNC_BLE_ADV_START_FLAG:
        {
            payload_len = 1;
            msg_ptr = &app_db.ble_common_adv_after_roleswap;
        }
        break;

    default:
        break;

    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_BLE_COMMON_ADV, payload_len, msg_ptr, skip,
                              total);
}


static void le_common_parse(LE_COMMON_REMOTE_MSG msg_type, uint8_t *buf, uint16_t len,
                            T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("le_common_parse: msg = 0x%x, status = %x", msg_type, status);

    switch (msg_type)
    {
    case SYNC_BLE_ADV_START_FLAG:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t *p_info = (uint8_t *)buf;
                    app_db.ble_common_adv_after_roleswap = p_info[0];
                }
            }
        }
        break;

    default:
        break;
    }

}
#endif


bool le_common_adv_start(uint16_t duration_10ms)
{
    if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(le_common_adv.adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        APP_PRINT_TRACE0("le_common_adv_start: Already started");
        return true;
    }
}

bool le_common_adv_stop(int8_t app_cause)
{
    APP_PRINT_INFO0("le_common_adv_stop");
    if (ble_ext_adv_mgr_disable(le_common_adv.adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

T_BLE_EXT_ADV_MGR_STATE le_common_adv_get_state(void)
{
    return ble_ext_adv_mgr_get_adv_state(le_common_adv.adv_handle);
}

uint8_t le_common_adv_get_conn_id(void)
{
    APP_PRINT_INFO1("le_common_adv_get_conn_id: conn_id %d", le_common_adv.conn_id);
    return le_common_adv.conn_id;
}

void le_common_adv_reset_conn_id(void)
{
    APP_PRINT_INFO1("le_common_adv_reset_conn_id: conn_id %d", le_common_adv.conn_id);
    le_common_adv.conn_id = 0xff;
}

void le_common_adv_set_random(uint8_t *random_address)
{
    ble_ext_adv_mgr_set_random(le_common_adv.adv_handle, random_address);
}

#if F_APP_ERWS_SUPPORT
void le_adv_sync_start_flag(void)
{
    app_relay_async_single(APP_MODULE_TYPE_BLE_COMMON_ADV, SYNC_BLE_ADV_START_FLAG);
}
#endif


void le_common_adv_cb_reg(LE_COMMON_ADV_CB cb)
{
    CB_ELEM *p_cb_elem = calloc(1, sizeof(CB_ELEM));

    p_cb_elem->cb = cb;

    APP_PRINT_TRACE2("le_common_adv_cb_reg: p_cb_elem %p, cb %p", p_cb_elem, cb);

    os_queue_in(&le_common_adv.cb_queue, p_cb_elem);
}



static void rand_addr_cb(RANDOM_ADDR_MGR_EVT_DATA *evt_data)
{
    APP_PRINT_TRACE1("app_ble_tts_ota rand_addr_cb: evt %d", evt_data->evt);

    uint8_t *rand_addr = NULL;

    if (le_common_adv.use_static_addr_type == false)
    {
        return;
    }

    if (evt_data->evt == RANDOM_ADDR_TWS_ADDR_UPD)
    {
        rand_addr = evt_data->upd_tws_addr.addr;
    }
    else if (evt_data->evt == RANDOM_ADDR_TWS_ADDR_RECVED)
    {
        rand_addr = evt_data->recv_tws_addr.addr;
    }

    le_common_adv_set_random(rand_addr);
}

bool le_common_adv_update_scan_rsp_data(void)
{
    /*scan_rsp_data update ble device name*/
    app_ble_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);
    T_GAP_CAUSE ret = ble_ext_adv_mgr_set_scan_response_data(le_common_adv.adv_handle,
                                                             scan_rsp_data_len, scan_rsp_data);

    if (ret == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void le_common_adv_init(void)
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;

    if (le_common_adv.use_static_addr_type)
    {
        own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    }
    else
    {
        own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    }

    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    uint8_t data_len = 0;

#if F_APP_SPECIFIC_UUID_SUPPORT
    if (app_cfg_const.enable_specific_service_uuid)
    {
        memcpy(&common_adv_data[5], app_cfg_const.specific_service_uuid, 15);
    }
#endif

    /* company id */
    common_adv_data[20] = app_cfg_nv.bud_role;

#if F_APP_AVP_INIT_SUPPORT
    common_adv_data[23] = 0x5D;//app_cfg_const.company_id not exist
    common_adv_data[24] = 0x00;
#else
    common_adv_data[23] = app_cfg_const.company_id[0];
    common_adv_data[24] = app_cfg_const.company_id[1];
#endif

    uint8_t *spk1_mac = NULL;

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        spk1_mac = app_cfg_nv.bud_local_addr;
    }
    else
    {
        spk1_mac = app_cfg_nv.bud_peer_addr;
    }

    for (int i = 0; i < 6; i++)
    {
        common_adv_data[25 + i] = spk1_mac[5 - i];
    }
#if F_APP_AVP_INIT_SUPPORT
    data_len = sizeof(app_cfg_nv.bud_local_addr) + 2;//2:sizeof(app_cfg_const.company_id)
#else
    data_len = sizeof(app_cfg_nv.bud_local_addr) + sizeof(app_cfg_const.company_id);
#endif

    app_ble_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);

    uint8_t le_random_addr[6] = {0};
    app_ble_rand_addr_get(le_random_addr);

    if (F_APP_HARMAN_FEATURE_SUPPORT)
    {
        uint16_t pid = 0;
        uint8_t cid = 0;
        uint8_t *p_harman_adv_data = NULL;
        uint8_t harman_adv_data_len = 0;
        uint8_t *p_bt_mac_crc = NULL;

        p_harman_adv_data = app_harman_ble_data_get();
        harman_adv_data_len = app_harman_ble_data_len_get();
        memcpy(&common_adv_data[0], p_harman_adv_data, harman_adv_data_len);

        pid = app_harman_license_pid_get();
        memcpy(&common_adv_data[11], (uint8_t *)&pid, 2);

        common_adv_data[13] = 0x00;
        common_adv_data[14] = 0x90;

        cid = app_harman_license_cid_get();
        common_adv_data[15] = cid;

        p_bt_mac_crc = app_harman_ble_bt_mac_crc_get();
        memcpy(&common_adv_data[16], p_bt_mac_crc, 2);

        data_len = harman_adv_data_len;

        ble_ext_adv_mgr_init_adv_params(&le_common_adv.adv_handle, adv_event_prop, adv_interval_min,
                                        adv_interval_max, own_address_type, peer_address_type, peer_address,
                                        filter_policy, data_len, common_adv_data,
                                        scan_rsp_data_len, scan_rsp_data, le_random_addr);
    }
    else
    {
        ble_ext_adv_mgr_init_adv_params(&le_common_adv.adv_handle, adv_event_prop, adv_interval_min,
                                        adv_interval_max, own_address_type, peer_address_type, peer_address,
                                        filter_policy, 23 + data_len, common_adv_data,
                                        scan_rsp_data_len, scan_rsp_data, le_random_addr);
    }

    APP_PRINT_TRACE2("le_common_adv_init: common_adv_data %b, le_random_addr %s",
                     TRACE_BINARY(sizeof(common_adv_data), common_adv_data), TRACE_BDADDR(le_random_addr));

    ble_ext_adv_mgr_register_callback(le_common_adv_callback, le_common_adv.adv_handle);

#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_le_common_relay_cback, (P_APP_PARSE_CBACK)le_common_parse,
                             APP_MODULE_TYPE_BLE_COMMON_ADV, LE_COMMON_REMOTE_MSG_MAX);
#endif
    os_queue_init(&le_common_adv.cb_queue);

    app_ble_rand_addr_cb_reg(rand_addr_cb);
}


