#include <string.h>
#include "trace.h"
#include "ascs_def.h"
#include "bass_def.h"
#include "ble_ext_adv.h"
#include "csis_rsi.h"
#include "gap_timer.h"
#include "gatt.h"
#include "pacs_mgr.h"
#include "app_auto_power_off.h"
#include "app_cfg.h"
#include "app_csis.h"
#include "app_device.h"
#include "app_le_audio_mgr.h"
#include "app_le_audio_adv.h"
#include "app_link_util.h"
#include "app_pacs.h"
#include "app_audio_policy.h"
#include "app_bt_policy_api.h"

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT)
static T_BLE_EXT_ADV_MGR_STATE le_audio_adv_state = BLE_EXT_ADV_MGR_ADV_DISABLED;
uint8_t le_audio_adv_handle = 0xff;


/** @brief  GAP - Advertisement data (max size = 31 bytes, best kept short to conserve power) */
uint8_t audio_adv_data[31] =
{
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
};

uint8_t audio_scan_rsp_data[] =
{
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};

/*Unicast Server that sent the PDU is available for a general audio use case.
In this specification, a general audio use case means the transmission or reception of Audio Data
that has not been initiated by a higher-layer specification.*/
void ble_ext_adv_data_general_audio(uint16_t audio_adv_flag, uint16_t *audio_adv_len)
{
    uint16_t idx = *audio_adv_len;

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    if (audio_adv_flag & LE_EXT_ADV_ASCS)
    {
        audio_adv_data[idx] = 0x09;
        idx++;
        audio_adv_data[idx] = GAP_ADTYPE_SERVICE_DATA;
        idx++;
        audio_adv_data[idx] = LO_WORD(GATT_UUID_ASCS);
        idx++;
        audio_adv_data[idx] = HI_WORD(GATT_UUID_ASCS);
        idx++;

        audio_adv_data[idx] = ASCS_ADV_TARGETED_ANNOUNCEMENT;
        idx++;

        LE_UINT16_TO_ARRAY(audio_adv_data + idx,
                           sink_available_contexts);
        idx += 2;
        LE_UINT16_TO_ARRAY(audio_adv_data + idx,
                           source_available_contexts);
        idx += 2;
        audio_adv_data[idx] = 0; // metadata length
        idx++;
    }
#endif
#if F_APP_TMAP_BMR_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_BASS)
    {
        audio_adv_data[idx] = 0x03;
        idx++;
        audio_adv_data[idx] = GAP_ADTYPE_SERVICE_DATA;
        idx++;
        audio_adv_data[idx] = LO_WORD(GATT_UUID_BASS);
        idx++;
        audio_adv_data[idx] = HI_WORD(GATT_UUID_BASS);
        idx++;
    }
#endif
    *audio_adv_len = idx;
#if F_APP_CSIS_SUPPORT
    if (audio_adv_flag & LE_EXT_ADV_PSRI)
    {
        uint8_t psri_data[CSI_RSI_LEN];
        if (csis_gen_rsi(csis_sirk, psri_data))
        {
            uint16_t idx = *audio_adv_len;
            audio_adv_data[idx] = CSI_RSI_LEN + 1;
            idx++;
            audio_adv_data[idx] = GAP_ADTYPE_RSI;
            idx++;
            memcpy(audio_adv_data + idx, psri_data, CSI_RSI_LEN);
            idx += CSI_RSI_LEN;
            *audio_adv_len = idx;
        }
    }
#endif
}

void app_le_audio_adv_disconn_cback(uint8_t conn_id, uint8_t local_disc_cause, uint16_t disc_cause)
{
    APP_PRINT_TRACE2("app_le_audio_adv_disconn_cback %x, %x", local_disc_cause, disc_cause);
    T_APP_LE_LINK *p_link;
    p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link)
    {
        app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_DISCONNECT, &disc_cause);
    }

    if ((disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) ||
        (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)))
    {
        // app_audio_tone_type_play(TONE_CIS_START, false, false);
        app_le_audio_device_sm(LE_AUDIO_CIG_START, &disc_cause);
    }
}

void app_le_audio_adv_cback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            le_audio_adv_state = cb_data.p_ble_state_change->state;
            if (le_audio_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                APP_PRINT_TRACE1("app_le_audio_adv_cback: BLE_EXT_ADV_MGR_ADV_ENABLED, adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
            }
            else if (le_audio_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                APP_PRINT_TRACE1("app_le_audio_adv_cback: BLE_EXT_ADV_MGR_ADV_DISABLED, adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    APP_PRINT_TRACE1("app_le_audio_adv_cback: BLE_EXT_ADV_STOP_CAUSE_APP app_cause 0x%02x",
                                     cb_data.p_ble_state_change->app_cause);
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    APP_PRINT_TRACE0("app_le_audio_adv_cback: BLE_EXT_ADV_STOP_CAUSE_CONN");
                    break;
                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    APP_PRINT_TRACE0("app_le_audio_adv_cback: BLE_EXT_ADV_STOP_CAUSE_TIMEOUT");
                    break;
                default:
                    APP_PRINT_TRACE1("app_le_audio_adv_cback: stop_cause %d",
                                     cb_data.p_ble_state_change->stop_cause);
                    break;
                }
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE1("app_le_audio_adv_cback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                         cb_data.p_ble_conn_info->conn_id);
        app_reg_le_link_disc_cb(cb_data.p_ble_conn_info->conn_id, app_le_audio_adv_disconn_cback);
        break;

    default:
        break;
    }
    return;
}

void app_le_audio_adv_start(bool enable_pairable, uint8_t mode)
{
    APP_PRINT_INFO0("app_le_audio_adv_start");

    if (le_audio_adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(le_audio_adv_handle, 0) == GAP_CAUSE_SUCCESS)
        {
            if ((T_LEA_ADV_MODE)mode == LEA_ADV_MODE_LOSSBACK)

            {
                mtc_start_timer(MTC_TMR_CIS, MTC_CIS_TMR_LINKLOSS_BACK,
                                app_cfg_const.cis_linkloss_resync_to * 1000);
            }
            else  if ((T_LEA_ADV_MODE)mode == LEA_ADV_MODE_PAIRING)
            {
                mtc_start_timer(MTC_TMR_CIS, MTC_CIS_TMR_PAIRING, app_cfg_const.cis_pairing_to * 1000);
            }
            else  if ((T_LEA_ADV_MODE)mode == LEA_ADV_MODE_DELEGATOR)
            {
                mtc_start_timer(MTC_TMR_BIS, MTC_BIS_TMR_DELEGATOR, app_cfg_const.scan_to * 1000);
            }

            if (enable_pairable == true)
            {
                uint8_t pairable = GAP_PAIRING_MODE_PAIRABLE;
                gap_set_param(GAP_PARAM_BOND_LE_PAIRING_MODE, sizeof(uint8_t), &pairable);
            }
        }
    }
    else
    {
        APP_PRINT_TRACE0("app_le_audio_adv_start: Already started");
    }
}

void app_le_audio_adv_stop(bool disable_pairable)
{
    mtc_stop_timer(MTC_TMR_CIS);

    if (le_audio_adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
    {
        if (ble_ext_adv_mgr_disable(le_audio_adv_handle, 0) == GAP_CAUSE_SUCCESS)
        {
            if (disable_pairable == true)
            {
                uint8_t pairable = GAP_PAIRING_MODE_NO_PAIRING;
                gap_set_param(GAP_PARAM_BOND_LE_PAIRING_MODE, sizeof(uint8_t), &pairable);
            }
        }
    }
    else
    {
        APP_PRINT_TRACE0("app_le_audio_adv_stop: Already stoped");
    }
}

void app_le_audio_adv_set(uint16_t audio_adv_flag)
{
    uint16_t audio_adv_len = 3;
    T_LE_EXT_ADV_EXTENDED_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0x40;
    uint16_t adv_interval_max = 0x50;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    ble_ext_adv_data_general_audio(audio_adv_flag, &audio_adv_len);
    ble_ext_adv_mgr_init_adv_params(&le_audio_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, audio_adv_len, audio_adv_data,
                                    sizeof(audio_scan_rsp_data), audio_scan_rsp_data, NULL);

    ble_ext_adv_mgr_register_callback(app_le_audio_adv_cback, le_audio_adv_handle);
}

T_LEA_ADV_STATE app_lea_adv_state(void)
{
    return (T_LEA_ADV_STATE)le_audio_adv_state;
}

void app_le_audio_adv_init(void)
{
    uint16_t audio_adv_flag = 0;
#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    audio_adv_flag |= LE_EXT_ADV_ASCS;
#endif
#if F_APP_CSIS_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_PSRI;
#endif
#if F_APP_TMAP_BMR_SUPPORT
    audio_adv_flag |= LE_EXT_ADV_BASS;
#endif

    app_le_audio_adv_set(audio_adv_flag);
}
#endif
