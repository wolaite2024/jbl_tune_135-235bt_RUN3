/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "os_mem.h"
#include "gap_timer.h"
#include "trace.h"
#include "gap_le_types.h"
#include "ble_ext_adv.h"
#include "app_cfg.h"
#include "app_main.h"

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
#include "string.h"
#include "bt_types.h"
#include "app_ble_swift_pair.h"
#include "app_adv_stop_cause.h"
#include "app_ble_rand_addr_mgr.h"
uint8_t swift_pair_adv_handle = 0xff;
static T_BLE_EXT_ADV_MGR_STATE swift_pair_adv_state = BLE_EXT_ADV_MGR_ADV_DISABLED;
static uint32_t appearance_major = MAJOR_DEVICE_CLASS_AUDIO;
static uint32_t appearance_minor = MINOR_DEVICE_CLASS_HEADSET;
static uint8_t swift_pair_adv_len = 7;

static uint8_t swift_pair_adv_data[31] =
{
    /* Flags */
    0x00,       /* length, when get all data, count length */
    GAP_ADTYPE_MANUFACTURER_SPECIFIC,
    0x06,/*mocrosoftVendor ID*/
    0x00,/*mocrosoftVendor ID*/
    0x03,/*mocrosoftVendor ID*/

    0x01,/*microsoft beacon sub scenario*/
    0x80,/*reserved rssi byte*/
};

static void swift_pair_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            swift_pair_adv_state = cb_data.p_ble_state_change->state;
            if (swift_pair_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                APP_PRINT_TRACE1("swift_pair_adv_callback: BLE_EXT_ADV_MGR_ADV_ENABLED, adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
            }
            else if (swift_pair_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                APP_PRINT_TRACE1("swift_pair_adv_callback: BLE_EXT_ADV_MGR_ADV_DISABLED, adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    APP_PRINT_TRACE1("swift_pair_adv_callback: BLE_EXT_ADV_STOP_CAUSE_APP app_cause 0x%02x",
                                     cb_data.p_ble_state_change->app_cause);
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    APP_PRINT_TRACE0("swift_pair_adv_callback: BLE_EXT_ADV_STOP_CAUSE_CONN");
                    break;
                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    APP_PRINT_TRACE0("swift_pair_adv_callback: BLE_EXT_ADV_STOP_CAUSE_TIMEOUT");
                    break;
                default:
                    APP_PRINT_TRACE1("swift_pair_adv_callback: stop_cause %d",
                                     cb_data.p_ble_state_change->stop_cause);
                    break;
                }
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE1("swift_pair_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                         cb_data.p_ble_conn_info->conn_id);
        swift_pair_adv_state = ble_ext_adv_mgr_get_adv_state(swift_pair_adv_handle);
        break;

    default:
        break;
    }
    return;
}

//start action must be after app_swift_pair_handle_power_on
bool swift_pair_adv_start(uint16_t duration_10ms)
{
    if (swift_pair_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(swift_pair_adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
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
        APP_PRINT_TRACE0("swift_pair_adv_start: Already started");
        return true;
    }
}

bool swift_pair_adv_stop(int8_t app_cause)
{
    if (ble_ext_adv_mgr_disable(swift_pair_adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void swift_pair_adv_init(void)
{
    /* set adv parameter */
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop =
        LE_EXT_ADV_LEGACY_ADV_NON_SCAN_NON_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0x20;
    uint16_t adv_interval_max = 0x20;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    uint8_t name_len;
    uint32_t icon = (appearance_major | appearance_minor);

    /* set adv data*/
    /* put bd_addr in adv data*/
    //for (uint8_t i = 0; i<6;i++)
    //{
    //   swift_pair_adv_data[7+i] = app_cfg_nv.bud_local_addr[5-i];
    //}
    memcpy(&swift_pair_adv_data[7], app_cfg_nv.bud_local_addr, 6);
    swift_pair_adv_len += 6;

    /* put appearance in adv data*/
    memcpy(&swift_pair_adv_data[13], &icon, 3);
    swift_pair_adv_len += 3;

    /* put device legacy name in adv data*/
    const char *device_legacy_name = (const char *)app_cfg_nv.device_name_legacy;
#if HARMAN_PACE_SUPPORT
    device_legacy_name = "JBL ENDU Pace";
#elif HARMAN_RUN3_SUPPORT
    device_legacy_name = "JBL ENDU Run 3";
#else
    device_legacy_name = (const char *)app_cfg_nv.device_name_legacy;
#endif
    name_len = strlen(device_legacy_name);
    if (name_len > 15)
    {
        name_len = 15;
    }
    memcpy(&swift_pair_adv_data[16], device_legacy_name, name_len);

    swift_pair_adv_len += name_len;
    swift_pair_adv_data[0] = swift_pair_adv_len - 1;
    /* modify length field of adv data*/

    uint8_t random_addr[6] = {0};
    app_ble_rand_addr_get(random_addr);

    /* build new adv*/
    ble_ext_adv_mgr_init_adv_params(&swift_pair_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, swift_pair_adv_len, swift_pair_adv_data,
                                    0, NULL, random_addr);

    /* set adv event handle callback*/
    ble_ext_adv_mgr_register_callback(swift_pair_adv_callback, swift_pair_adv_handle);
}

void app_swift_pair_handle_power_on(int16_t duration_10ms)
{
    swift_pair_adv_init();
}

void app_swift_pair_handle_power_off(void)
{
    swift_pair_adv_stop(APP_STOP_ADV_CAUSE_POWER_OFF);
}

#endif

