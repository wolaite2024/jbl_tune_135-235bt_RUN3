#include "trace.h"
#include "gap_timer.h"
#include "ble_audio_def.h"
#include "ble_audio_scan.h"
#include "app_le_audio_mgr.h"
#include "app_le_audio_scan.h"
#include "app_broadcast_sync.h"
#include "app_bt_policy_api.h"
#include "app_cfg.h"

#if F_APP_TMAP_BMR_SUPPORT
bool app_le_audio_scan_handle_report(T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    uint8_t *p_service_data;
    uint16_t service_data_len;
    APP_PRINT_INFO6("GAP_MSG_LE_EXT_ADV_REPORT_INFO:connectable %d, scannable %d, direct %d, scan response %d, legacy %d, data status 0x%x",
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_CONNECTABLE_ADV,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCANNABLE_ADV,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_DIRECTED_ADV,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCAN_RESPONSE,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV,
                    p_report->data_status);
    APP_PRINT_INFO5("GAP_MSG_LE_EXT_ADV_REPORT_INFO:event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d",
                    p_report->event_type,
                    TRACE_BDADDR(p_report->bd_addr),
                    p_report->addr_type,
                    p_report->rssi,
                    p_report->data_len);
    APP_PRINT_INFO5("GAP_MSG_LE_EXT_ADV_REPORT_INFO:primary_phy %d, secondary_phy %d, adv_sid %d, tx_power %d, peri_adv_interval %d",
                    p_report->primary_phy,
                    p_report->secondary_phy,
                    p_report->adv_sid,
                    p_report->tx_power,
                    p_report->peri_adv_interval);
    APP_PRINT_INFO2("GAP_MSG_LE_EXT_ADV_REPORT_INFO:direct_addr_type 0x%x, direct_addr %s",
                    p_report->direct_addr_type,
                    TRACE_BDADDR(p_report->direct_addr));

    uint8_t ret = ble_audio_adv_filter_service_data(p_report->data_len,
                                                    p_report->p_data,
                                                    BROADCAST_AUDIO_ANNOUNCEMENT_SRV_UUID, &p_service_data, &service_data_len);
    APP_PRINT_INFO3("ret: ble_audio_adv_filter_service_data 0x%x, state 0x%x, bis state 0x%x", ret,
                    app_bt_policy_get_state(), app_lea_bis_get_state());

    if (ret && ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) ||
                (app_bt_policy_get_state() != BP_STATE_FIRST_ENGAGE &&
                 app_bt_policy_get_state() != BP_STATE_ENGAGE &&
                 app_lea_bis_get_state() != LE_AUDIO_BIS_STATE_IDLE)))
    {
        app_bc_sync_add_device(p_report->bd_addr,
                               p_report->addr_type,
                               p_report->adv_sid);
    }
    return true;
}

void app_le_audio_scan_start(uint16_t timeout)
{
    APP_PRINT_INFO0("app_le_audio_scan_start");
    T_GAP_LOCAL_ADDR_TYPE  own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_SCAN_FILTER_POLICY  ext_scan_filter_policy = GAP_SCAN_FILTER_ANY;
    T_GAP_SCAN_FILTER_DUPLICATE  ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT;

    T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];
    extended_scan_param[0].scan_type = GAP_SCAN_MODE_PASSIVE;
    extended_scan_param[0].scan_interval =  0x140;//0x00C8;//400;
    extended_scan_param[0].scan_window = 0xD0; // 0x005A;//

    extended_scan_param[1].scan_type = GAP_SCAN_MODE_PASSIVE;
    extended_scan_param[1].scan_interval = 0x0050;//440;
    extended_scan_param[1].scan_window = 0x0025;//220;

    /* Initialize extended scan parameters */
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_LOCAL_ADDR_TYPE, sizeof(own_address_type),
                          &own_address_type);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                          &ext_scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                          &ext_scan_filter_duplicate);

    /* Initialize extended scan PHY parameters */
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_CODED, &extended_scan_param[1]);

    //le_ext_scan_report_filter(true, GAP_EXT_ADV_REPORT_BIT_CONNECTABLE_ADV, false);

    /* Enable extended scan */
    le_ext_scan_start();

    APP_PRINT_INFO8("app_le_audio_scan_start, %d, %d, %d, %d, %d, %d, %d ,%d",
                    app_cfg_const.iso_mode,
                    app_cfg_const.subgroup,
                    app_cfg_const.cis_autolink,
                    app_cfg_const.legacy_enable,
                    app_cfg_const.bstsrc[5],
                    app_cfg_const.bis_policy,
                    app_cfg_const.legacy_cis_linkback_prio,
                    app_cfg_const.cis_profile);

    APP_PRINT_INFO8("app_le_audio_scan_start %d, %d, %d, %d, %d, %d, %d, %d",
                    app_cfg_const.bis_mode,
                    app_cfg_const.active_prio_connected_device,
                    app_cfg_const.power_off_cis_to,
                    app_cfg_const.power_off_bis_to,
                    app_cfg_const.active_prio_connected_device_after_bis,
                    app_cfg_const.a2dp_interrupt_lea,
                    app_cfg_const.keep_cur_act_device_after_source,
                    app_cfg_const.cis_pairing_to);
    uint32_t legacy_cis_linkback_total = app_cfg_const.legacy_cis_linkback_total_h << 16 +
                                         app_cfg_const.legacy_cis_linkback_total_l;
    APP_PRINT_INFO5("app_le_audio_scan_start %d, %d, %d, %d, %d",
                    app_cfg_const.cis_linkback_to,
                    legacy_cis_linkback_total,
                    app_cfg_const.scan_to,
                    app_cfg_const.bis_resync_to,
                    app_cfg_const.cis_linkloss_resync_to);
    if (!mtc_exist_handler(MTC_TMR_BIS))
    {
        mtc_start_timer(MTC_TMR_BIS, MTC_BIS_TMR_SCAN, (app_cfg_const.scan_to * 1000));
    }
}

void app_le_audio_scan_stop()
{
    APP_PRINT_INFO0("app_le_audio_scan_stop");

    mtc_stop_timer(MTC_TMR_BIS);
    le_ext_scan_stop();
}
#endif
