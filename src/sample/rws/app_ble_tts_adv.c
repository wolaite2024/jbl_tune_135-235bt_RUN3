#if F_APP_TTS_SUPPORT

#include "app_ble_tts_adv.h"
#include <string.h>
#include "trace.h"
#include "gap_timer.h"
#include "app_cfg.h"
#include "app_ble_rand_addr_mgr.h"

uint8_t tts_ibeacon_adv_handle = 0xff;
static T_BLE_EXT_ADV_MGR_STATE tts_ibeacon_adv_state = BLE_EXT_ADV_MGR_ADV_DISABLED;

static void *p_ibeacon_timer_handle = NULL;
static uint32_t ibeacon_data_timer_ms = 1000;
static uint8_t ibeacon_timer_queue_id = 0;

static uint8_t ibeacon_adv_data[30] =
{
    0x02, /* 1:length*/
    0x01, /* 2:type Flag */
    0x06, /* 3:Dicoverable mode & not support BR/EDR */
    0x1A, /* 4:length */
    0xFF, /* 5:type="GAP_ADTYPE_MANUFACTURER_SPECIFIC" */
    0x4C, 0x00, /* 6-7: Apple company Id*/
    0x02, 0x15, /* 8-9:For all proximity beacon,specify data type & remaining data length*/
    //0xFD, 0xA5, 0x06, 0x93, 0xA4, 0xE2, 0x4F, 0xB1, 0xAF, 0xCF, 0xC6, 0xEB, 0x07, 0x64, 0x78, 0x25,
    0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFD, 0x02, 0x00, 0x00,
    0x00, 0x0A, /* 26-27:major id*/
    0x00, 0x07, /* 28-29:minor id*/
    0xC6 /* 30:mesured power*/
};

T_BLE_EXT_ADV_MGR_STATE tts_ibeacon_adv_get_state(void)
{
    return ble_ext_adv_mgr_get_adv_state(tts_ibeacon_adv_handle);;
}

void tts_ibeacon_adv_set_random(uint8_t *random_address)
{
    ble_ext_adv_mgr_set_random(tts_ibeacon_adv_handle, random_address);
}

void tts_ibeacon_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            T_BLE_EXT_ADV_MGR_STATE ext_adv_state = cb_data.p_ble_state_change->state;
            if (ext_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                APP_PRINT_TRACE1("tts_ibeacon_adv_callback: BLE_EXT_ADV_MGR_ADV_ENABLED, adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
            }
            else if (ext_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                APP_PRINT_TRACE1("tts_ibeacon_adv_callback: BLE_EXT_ADV_MGR_ADV_DISABLED, adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
                gap_stop_timer(&p_ibeacon_timer_handle);

                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    APP_PRINT_TRACE1("tts_ibeacon_adv_callback: BLE_EXT_ADV_STOP_CAUSE_APP app_cause 0x%02x",
                                     cb_data.p_ble_state_change->app_cause);
                    break;
                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    APP_PRINT_TRACE0("tts_ibeacon_adv_callback: BLE_EXT_ADV_STOP_CAUSE_TIMEOUT");
                    break;

                default:
                    APP_PRINT_TRACE1("tts_ibeacon_adv_callback: stop_cause %d",
                                     cb_data.p_ble_state_change->stop_cause);
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
    return;
}

void tts_ibeacon_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE0("tts_ibeacon_timer_callback");
    if (p_ibeacon_timer_handle != NULL)
    {
        tts_ibeacon_adv_state = ble_ext_adv_mgr_get_adv_state(tts_ibeacon_adv_handle);
        if (tts_ibeacon_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
        {
            if (ibeacon_adv_data[24] < 0x09)
            {
                ibeacon_adv_data[24]++;
            }
            else
            {
                ibeacon_adv_data[24] = 0x00;
            }
            ble_ext_adv_mgr_set_adv_data(tts_ibeacon_adv_handle, sizeof(ibeacon_adv_data), ibeacon_adv_data);

            gap_start_timer(&p_ibeacon_timer_handle, "timer", ibeacon_timer_queue_id, 0, 0,
                            ibeacon_data_timer_ms);
        }
    }
}

bool tts_ibeacon_config_timer(uint32_t ibeacon_data_ms)
{
    if (ibeacon_data_timer_ms == ibeacon_data_ms && p_ibeacon_timer_handle != NULL)
    {
        return true;
    }

    ibeacon_data_timer_ms = ibeacon_data_ms;
    gap_stop_timer(&p_ibeacon_timer_handle);

    return true;
}

bool tts_ibeacon_adv_start(uint16_t duration_10ms)
{
    if (ble_ext_adv_mgr_enable(tts_ibeacon_adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
    {
        gap_start_timer(&p_ibeacon_timer_handle, "timer", ibeacon_timer_queue_id, 0, 0,
                        ibeacon_data_timer_ms);
        return true;
    }
    return false;
}

bool tts_ibeacon_adv_stop(uint8_t app_cause)
{
    tts_ibeacon_adv_state = ble_ext_adv_mgr_get_adv_state(tts_ibeacon_adv_handle);
    if (tts_ibeacon_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
    {
        if (ble_ext_adv_mgr_disable(tts_ibeacon_adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        return false;
    }
    else
    {
        APP_PRINT_ERROR1("tts_ibeacon_adv_stop: Invalid state, tts_ibeacon_adv_state %d",
                         tts_ibeacon_adv_state);
        return false;
    }
}

void tts_ibeacon_adv_init(void)
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop =
        LE_EXT_ADV_LEGACY_ADV_NON_SCAN_NON_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 300;
    uint16_t adv_interval_max = 300;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    uint8_t rand_addr[6] = {6};
    app_ble_rand_addr_get(rand_addr);

    ble_ext_adv_mgr_init_adv_params(&tts_ibeacon_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ibeacon_adv_data), ibeacon_adv_data,
                                    0, NULL, rand_addr);
    ble_ext_adv_mgr_register_callback(tts_ibeacon_adv_callback, tts_ibeacon_adv_handle);
    gap_reg_timer_cb(tts_ibeacon_timer_callback, &ibeacon_timer_queue_id);
    tts_ibeacon_config_timer(1000);
}



#endif

















