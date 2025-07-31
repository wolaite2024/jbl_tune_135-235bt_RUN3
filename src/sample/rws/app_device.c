/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "string.h"
#include "wdg.h"
#include "os_timer.h"
#include "trace.h"
#include "gap_timer.h"
#include "gap_le.h"
#include "io_dlps.h"
#include "app_charger.h"
#if F_APP_APT_SUPPORT
#include "audio_passthrough.h"
#endif
#include "sysm.h"
#include "btm.h"
#include "ble_mgr.h"
#include "app_device.h"
#include "app_ble_device.h"
#include "app_bt_sniffing.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "app_report.h"
#include "app_key_process.h"
#include "app_key_gpio.h"
#include "app_charger.h"
#include "app_led.h"
#include "bt_hfp.h"
#include "bt_avrcp.h"
#include "bt_bond.h"
#include "remote.h"
#include "ringtone.h"
#include "voice_prompt.h"
#include "app_mmi.h"
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
#include "engage.h"
#include "app_bt_policy_api.h"
#include "app_roleswap.h"
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#if F_APP_BUZZER_SUPPORT
#include "app_buzzer.h"
#endif
#if AMA_FEATURE_SUPPORT
#include "app_ama_device.h"
#endif
#include "app_relay.h"
#include "app_sensor.h"
#include "app_multilink.h"
#include "app_audio_policy.h"
#include "app_hfp.h"
#include "app_bond.h"
#include "app_iap_rtk.h"
#include "eq_utils.h"
#include "pm.h"
#include "app_dlps.h"
#include "app_adp.h"
#include "app_ota.h"
#include "app_auto_power_off.h"
#include "app_bt_policy_int.h"
#include "app_bud_loc.h"
#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif
#include "test_mode.h"
#include "app_cmd.h"
#include "app_ota_tooling.h"
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#include "anc.h"
#include "anc_tuning.h"
#endif
#if (F_APP_DUAL_AUDIO_EFFECT == 1)
#include "app_dual_audio_effect.h"
#endif
#include "app_bt_policy_api.h"
#include "app_wdg.h"
#if F_APP_ADC_SUPPORT
#include "app_adc.h"
#endif
#include "app_audio_passthrough.h"
#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif
#include "app_sniff_mode.h"

#if (F_APP_AVP_INIT_SUPPORT == 1)
#include "app_avp.h"
#endif


#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_multilink.h"
#include "app_dongle_spp.h"
#include "app_dongle_record.h"
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_asp_device.h"
#include "app_teams_audio_policy.h"
#include "app_teams_bt_policy.h"
#include "app_teams_cmd.h"
#include "app_teams_extend_led.h"
#endif

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#endif

#if (F_APP_VENDOR_SENSOR_INTERFACE_SUPPORT == 1)
#include "app_vendor_driver_interface.h"
#endif

#if (F_APP_USB_AUDIO_SUPPORT == 1)
#include "app_usb_audio.h"
#endif

#include "single_tone.h"

#if (F_APP_SPP_CAPTURE_DSP_DATA == 1)
#include "app_cmd.h"
#endif

#if GFPS_FEATURE_SUPPORT
#include "app_gfps_device.h"
#include "app_gfps.h"
#if GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#if GFPS_FINDER_DULT_SUPPORT
#include "app_dult_device.h"
#include "app_dult.h"
#endif
#endif
#endif

#if F_APP_LE_AUDIO_SM
#include "app_le_audio_mgr.h"
#endif

#include "app_ble_common_adv.h"

#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
#include "rtl876x_rtc.h"
#include "adapter.h"
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_ble.h"
#include "app_harman_vendor_cmd.h"
#include "app_harman_eq.h"
#include "app_harman_ble_ota.h"
#include "app_harman_license.h"
#include "app_harman_adc.h"
#include "fmc_api.h"
#include "ota_ext.h"
#include "app_cfg.h"
#include "ftl.h"
#endif

#if GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#endif

#if F_APP_LOSS_BATTERY_PROTECT
#include "fmc_api.h"
#include "fmc_api_ext.h"
#include "hal_gpio.h"
#include "app_io_msg.h"

#define LOSS_BATTERY_IO_DETECT_PIN      P1_0
#endif

#if HARMAN_REQ_REMOTE_DEVICE_NAME_TIME
#include "app_ble_gap.h"
#endif

static bool linkback_disable_power_off_fg = false;
static bool pairing_disable_power_off_fg = false;
static bool enable_pairing_complete_led = false;

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_power_on_hook)(void) = NULL;
void (*app_power_off_hook)(void) = NULL;
void (*app_src_connect_hook)(void) = NULL;
void (*app_src_disconnect_hook)(void) = NULL;
void (*app_profile_connect_hook)(void) = NULL;
void (*app_pairing_finish_hook)(void) = NULL;
void (*app_pairing_timeout_hook)(void) = NULL;
void (*app_bud_connect_hook)(void) = NULL;
void (*app_bud_disconnect_hook)(void) = NULL;
void (*app_roleswap_src_connect_hook)(void) = NULL;
#endif


static uint8_t device_timer_queue_id = 0;
static void *timer_handle_device_reboot = NULL;
static void *timer_handle_dut_mode_wait_link_disc = NULL;
static void *timer_handle_play_bud_role_tone = NULL;

#if F_APP_ERWS_SUPPORT
static void new_pri_apply_app_db_info_when_roleswap_suc(void);
static T_ROLESWAP_APP_DB roleswap_app_db_temp = {0};
#endif

#if F_APP_TEAMS_BT_POLICY
uint8_t temp_master_device_name[40] = {0};
uint8_t temp_master_device_name_len = 0;
#endif

typedef enum
{
    APP_DEVICE_TIMER_REBOOT,
    APP_DEVICE_TIMER_DUT_MODE_WAIT_LINK_DISC,
    APP_DEVICE_TIMER_PLAY_BUD_ROLE_TONE,
    APP_DEVICE_TIMER_REQ_REMOTE_DEVICE_NAME,
} T_DEVICE_TIMER;

#if HARMAN_REQ_REMOTE_DEVICE_NAME_TIME
#define REQ_REMOTE_DEVICE_NAME_TIME     5000

static void *timer_handle_req_remote_device_name = NULL;
static bool need_wait_remote_name_rsp = false;
static uint8_t remote_name_req_idx = 0;

static uint8_t app_harman_req_bt_remote_name(uint8_t app_idx)
{
    uint8_t cur_id = 0;
    uint8_t another_idx = MAX_BR_LINK_NUM;
    uint8_t link_num = app_find_b2s_link_num();

    cur_id = app_idx;

    APP_PRINT_TRACE3("app_harman_req_bt_remote_name before: link_num: 0x%x, app_idx: 0x%x, need_wait_remote_name_rsp: %d",
                     link_num, app_idx, need_wait_remote_name_rsp);
    if (link_num == 1)
    {
        if (!need_wait_remote_name_rsp)
        {
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].used)
                {
                    app_idx = i;
                    break;
                }
            }

            if (app_idx != MAX_BR_LINK_NUM)
            {
                bt_remote_name_req(&app_db.br_link[app_idx].bd_addr[0]);
                need_wait_remote_name_rsp = true;
            }
        }
    }
    else if (link_num == 2)
    {
        if (!need_wait_remote_name_rsp)
        {
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (i != app_idx && app_db.br_link[i].used)
                {
                    another_idx = i;
                    break;
                }
            }

            if (another_idx != MAX_BR_LINK_NUM)
            {
                bt_remote_name_req(&app_db.br_link[another_idx].bd_addr[0]);
                need_wait_remote_name_rsp = true;

                cur_id = another_idx;
            }
        }
    }
    APP_PRINT_TRACE4("app_harman_req_bt_remote_name after: app_idx: 0x%x, another_idx: 0x%x, cur_id: 0x%x, need_wait_remote_name_rsp: %d",
                     app_idx, another_idx, cur_id, need_wait_remote_name_rsp);

    return  cur_id;
}

void app_harman_req_remote_device_name_timer_start(void)
{
    if (timer_handle_req_remote_device_name == NULL)
    {
        gap_start_timer(&timer_handle_req_remote_device_name, "req_remote_device_name",
                        device_timer_queue_id, APP_DEVICE_TIMER_REQ_REMOTE_DEVICE_NAME, 0,
                        REQ_REMOTE_DEVICE_NAME_TIME);
        power_register_excluded_handle(&timer_handle_req_remote_device_name, PM_EXCLUDED_TIMER);
    }
    else
    {
        gap_start_timer(&timer_handle_req_remote_device_name, "req_remote_device_name",
                        device_timer_queue_id, APP_DEVICE_TIMER_REQ_REMOTE_DEVICE_NAME, 0,
                        REQ_REMOTE_DEVICE_NAME_TIME);
    }
}
#endif

static void app_device_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_device_timer_callback: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_DEVICE_TIMER_REBOOT:
        {
            gap_stop_timer(&timer_handle_device_reboot);
            app_wdg_reset((T_WDG_MODE)timer_chann);
        }
        break;

    case APP_DEVICE_TIMER_DUT_MODE_WAIT_LINK_DISC:
        {
            gap_stop_timer(&timer_handle_dut_mode_wait_link_disc);
            if ((!app_bt_policy_get_b2b_connected()) && (!app_bt_policy_get_b2b_connected()))
            {
                switch_into_single_tone_test_mode();
            }
            else
            {
                app_bt_policy_disconnect_all_link();
                gap_start_timer(&timer_handle_dut_mode_wait_link_disc, "dut_mode_wait_link_disc",
                                device_timer_queue_id,
                                APP_DEVICE_TIMER_DUT_MODE_WAIT_LINK_DISC, 0, 2000);
            }
        }
        break;

    case APP_DEVICE_TIMER_PLAY_BUD_ROLE_TONE:
        {
            gap_stop_timer(&timer_handle_play_bud_role_tone);

            if (ringtone_remaining_count_get() == 0 &&
                voice_prompt_remaining_count_get() == 0 &&
                tts_remaining_count_get() == 0)
            {
                app_relay_async_single(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_REMOTE_SPK2_PLAY_SYNC);
            }
            else
            {
                gap_start_timer(&timer_handle_play_bud_role_tone, "play_bud_role_tone",
                                device_timer_queue_id,
                                APP_DEVICE_TIMER_PLAY_BUD_ROLE_TONE, 0, 500);
            }
        }
        break;

#if HARMAN_REQ_REMOTE_DEVICE_NAME_TIME
    case APP_DEVICE_TIMER_REQ_REMOTE_DEVICE_NAME:
        {
            T_APP_LE_LINK *p_le_link;

            p_le_link = app_find_le_link_by_conn_id(le_common_adv_get_conn_id());
            if ((app_find_b2s_link_num()) &&
                ((le_common_adv_get_conn_id() == 0xFF) ||
                 (p_le_link != NULL) && (p_le_link->state != LE_LINK_STATE_CONNECTED)))
            {
                remote_name_req_idx = app_harman_req_bt_remote_name(remote_name_req_idx);

                app_harman_req_remote_device_name_timer_start();
            }
        }
        break;
#endif

    default:
        break;
    }
}

void app_device_reboot(uint32_t timeout_ms, T_WDG_MODE wdg_mode)
{
    gap_start_timer(&timer_handle_device_reboot, "device_reboot",
                    device_timer_queue_id,
                    APP_DEVICE_TIMER_REBOOT, wdg_mode, timeout_ms);
    power_register_excluded_handle(&timer_handle_device_reboot, PM_EXCLUDED_TIMER);
}

static bool app_device_disconn_need_play(uint8_t *bd_addr, uint16_t cause)
{
    bool need_play = true;

#if (F_APP_AVP_INIT_SUPPORT == 1)
    if ((app_db.local_loc == BUD_LOC_IN_CASE) && (app_db.remote_loc == BUD_LOC_IN_CASE))
    {
        need_play = false;
    }
#endif

    if ((app_db.disconnect_inactive_link_actively == true) &&
        !(memcmp(app_db.resume_addr, bd_addr, 6)) &&
        (cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        //If link disconnection is initiated by roleswap module, it needn't play disconnect tone.
        need_play = false;
    }

    return need_play;
}

void app_device_factory_reset(void)
{
    uint8_t temp_first_engaged;
    uint8_t temp_case_battery;
    uint16_t temp_magic;
    uint8_t  temp_need_set_lps_mode;
    uint8_t  temp_ota_parking_lps_mode;

    app_ble_handle_factory_reset();

    if (app_cfg_const.enable_rtk_charging_box)
    {
        app_adp_case_battery_check(&app_db.case_battery, &app_cfg_nv.case_battery);
    }

    temp_first_engaged = app_cfg_nv.first_engaged;
    temp_case_battery = app_cfg_nv.case_battery;
    temp_magic = app_cfg_nv.peer_valid_magic;
    temp_need_set_lps_mode = app_cfg_nv.need_set_lps_mode;
    temp_ota_parking_lps_mode = app_cfg_nv.ota_parking_lps_mode;

    app_cfg_reset();

    app_cfg_nv.need_set_lps_mode = temp_need_set_lps_mode;
    app_cfg_nv.ota_parking_lps_mode = temp_ota_parking_lps_mode;

#if F_APP_TEAMS_BT_POLICY
    app_teams_cmd_handle_factory_reset();
#endif

    if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_ALL)
    {
        bt_bond_clear();
    }
    else if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_NORMAL)
    {
        app_bond_clear_non_rws_keys();
        app_cfg_nv.peer_valid_magic = temp_magic;
    }
    else if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_PHONE)
    {
        app_bond_clear_non_rws_keys();
        app_cfg_nv.peer_valid_magic = temp_magic;
        app_cfg_nv.first_engaged = temp_first_engaged;
    }
    else if (app_db.factory_reset_clear_mode == FACTORY_RESET_CLEAR_CFG)
    {
        app_cfg_nv.peer_valid_magic = temp_magic;
        app_cfg_nv.first_engaged = temp_first_engaged;
    }
    else
    {
        /* no action */
    }

    if (app_cfg_const.enable_rtk_charging_box)
    {
        if (((temp_case_battery & 0x7F) > 100) || ((temp_case_battery & 0x7F) == 0))//invalid battery level
        {
            app_cfg_nv.case_battery = (temp_case_battery & 0x80) | 0x50;
        }
        else
        {
            app_cfg_nv.case_battery = temp_case_battery;
        }
    }

    remote_session_role_set((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_MPTEST);
#if HARMAN_OPEN_LR_FEATURE
    harman_set_vp_ringtone_balance(__func__, __LINE__);
#endif
    voice_prompt_volume_set(app_cfg_const.voice_prompt_volume_default);
    ringtone_volume_set(app_cfg_const.ringtone_volume_default);

    app_cfg_nv.factory_reset_done = 1;
    if (app_cfg_nv.adp_factory_reset_power_on)
    {
        app_cfg_nv.power_off_cause_cmd = 0;
        app_cfg_nv.auto_power_on_after_factory_reset = 0;
    }
    else if (app_db.ota_tooling_start)
    {
        app_cfg_nv.auto_power_on_after_factory_reset = 0;
    }
    else if (app_cfg_const.auto_power_on_after_factory_reset)
    {
        app_cfg_nv.auto_power_on_after_factory_reset = 1;
    }

    if (app_cfg_store_all() != 0)
    {
        APP_PRINT_ERROR0("app_device_dm_cback: save nv cfg data error");
    }

#if F_APP_TEAMS_CUSTOMIZED_CMD_SUPPORT
    app_teams_cmd_info_reset();
#endif

#if F_APP_KEY_EXTEND_FEATURE
    app_key_reset_key_customized_parameter();
#if F_APP_RWS_KEY_SUPPORT
    app_key_reset_rws_key_customized_parameter(true);
#endif
#endif
}

static void app_device_link_policy_ind(T_BP_EVENT event, T_BP_EVENT_PARAM *event_param)
{
    APP_PRINT_TRACE1("app_device_link_policy_ind: event 0x%02x", event);

    switch (event)
    {
    case BP_EVENT_STATE_CHANGED:
        {
            if (event_param->is_shut_down)
            {
                if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
                {
                }
            }
            else
            {
                if (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
                {
                    enable_pairing_complete_led = true;

                    pairing_disable_power_off_fg = true;
#if F_APP_LE_AUDIO_SM
                    uint8_t isLegacy = 0, isCis = 0;
                    isLegacy = app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_LEGACY, NULL);
                    isCis = app_le_audio_dev_ctrl(T_LEA_DEV_CRL_GET_CIS_POLICY, NULL);
                    if (isLegacy)
                    {
                        app_auto_power_off_disable(AUTO_POWER_OFF_MASK_PAIRING_MODE);
                    }

                    if (!app_key_is_enter_pairing() && !event_param->is_ignore && isLegacy)
                    {
                        if (isCis)
                        {
                            app_audio_tone_type_play(TONE_CIS_SIMU, false, false);
                        }
                        else
                        {
                            app_audio_tone_type_play(TONE_PAIRING, false, false);
                        }

                    }
                    else
                    {
                        if (isCis)
                        {
                            app_audio_tone_type_play(TONE_CIS_START, false, false);
                        }
                    }
#else
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_PAIRING_MODE);

                    /*If press key0 from power off --> power on -->enter pairing mode, then release, will not play tone here*/
                    if (!app_key_is_enter_pairing() && !event_param->is_ignore)
                    {
#if F_APP_HARMAN_FEATURE_SUPPORT
                        if (app_cfg_nv.language_status == 0)
                        {
                            app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_60, false, false);
                        }
                        else
#endif
                        {
                            app_audio_tone_type_play(TONE_PAIRING, false, false);
                        }
                    }
#endif
                    app_key_reset_enter_pairing();
                }
                else
                {
                    if (pairing_disable_power_off_fg)
                    {
                        pairing_disable_power_off_fg = false;

                        if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off &&
                            (app_bt_policy_get_b2s_connected_num() != 0))
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_PAIRING_MODE,
                                                      app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
                        }
                        else
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_PAIRING_MODE, app_cfg_const.timer_auto_power_off);
                        }
                    }

                    if (app_bt_policy_get_state() != BP_STATE_CONNECTED)
                    {
                        enable_pairing_complete_led = false;
                    }
                }

                if (app_bt_policy_get_state() == BP_STATE_LINKBACK)
                {
                    linkback_disable_power_off_fg = true;
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_LINKBACK);
#if F_APP_ERWS_SUPPORT
                    app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_LEGACY_LINK_CHANGE);
                    app_bt_sniffing_judge_dynamic_set_gaming_mode();
                    if (app_bt_policy_get_linkback_device())
                    {
                        app_hfp_adjust_sco_window(app_bt_policy_get_linkback_device(), APP_SCO_ADJUST_LINKBACK_EVT);
                    }
#endif
                }
                else
                {
                    if (linkback_disable_power_off_fg)
                    {
                        linkback_disable_power_off_fg = false;

                        if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off &&
                            (app_bt_policy_get_b2s_connected_num() != 0))
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_LINKBACK,
                                                      app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
                        }
                        else
                        {
                            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_LINKBACK, app_cfg_const.timer_auto_power_off);
                        }

#if F_APP_ERWS_SUPPORT
                        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_LEGACY_LINK_CHANGE);
                        app_bt_sniffing_judge_dynamic_set_gaming_mode();
#endif
                    }

                    if (app_bt_policy_get_b2s_connected_num() > 1)
                    {
                        app_hfp_adjust_sco_window(NULL, APP_SCO_ADJUST_LINKBACK_END_EVT);
                    }
                }
            }
        }
        break;

    case BP_EVENT_RADIO_MODE_CHANGED:
        {
            T_BT_DEVICE_MODE radio_mode = app_bt_policy_get_radio_mode();
            app_ble_handle_radio_mode(radio_mode);
        }
        break;

    case BP_EVENT_ROLE_DECIDED:
        {
            T_BT_DEVICE_MODE radio_mode = app_bt_policy_get_radio_mode();
            app_ble_handle_engage_role_decided(radio_mode);

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_bud_connect_hook && (app_cfg_const.enable_broadcasting_headset != 0))
            {
                app_bud_connect_hook();
            }
#endif
        }
        break;

    case BP_EVENT_BUD_CONN_START:
        {
            app_db.bud_couple_state = BUD_COUPLE_STATE_START;
#if (BROADCAST_BY_RTK_CHARGER_BOX && F_APP_AVP_INIT_SUPPORT)
            if (app_cfg_const.enable_rtk_charging_box)
            {
                if (app_charger_box_trig_hook && (app_cfg_const.enable_broadcasting_headset == 0))
                {
                    app_charger_box_trig_hook();
                }
            }
#endif
        }
        break;

    case BP_EVENT_BUD_AUTH_SUC:
        {
            app_db.bud_couple_state = BUD_COUPLE_STATE_CONNECTED;

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BUD_COUPLING, app_cfg_const.timer_auto_power_off);
            }
            else
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BUD_COUPLING);
            }

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (app_db.ota_tooling_start)
            {
                if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
                    (app_bt_policy_get_b2s_connected_num() == 0))
                {
                    app_bt_policy_enter_ota_mode(true);
                    app_ota_adv_start(app_db.jig_dongle_id, 30);
                }
                else
                {
                    app_bt_policy_enter_ota_mode(false);
                }
            }
#endif


#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_bud_connect_hook && (app_cfg_const.enable_broadcasting_headset != 0))
            {
                app_bud_connect_hook();
            }
#endif
        }
        break;

    case BP_EVENT_BUD_CONN_FAIL:
    case BP_EVENT_BUD_AUTH_FAIL:
        {
            app_db.bud_couple_state = BUD_COUPLE_STATE_IDLE;
        }
        break;

    case BP_EVENT_BUD_DISCONN_NORMAL:
    case BP_EVENT_BUD_DISCONN_LOST:
        {
            if (event == BP_EVENT_BUD_DISCONN_NORMAL)
            {
                app_audio_tone_type_play(TONE_REMOTE_DISCONNECT, false, false);
            }
            else if (event == BP_EVENT_BUD_DISCONN_LOST)
            {
                app_audio_tone_type_play(TONE_REMOTE_LOSS, false, false);
            }

            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BUD_COUPLING, app_cfg_const.timer_auto_power_off);

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_bud_disconnect_hook)
            {
                app_bud_disconnect_hook();
            }
#endif

            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].connected_profile & HFP_PROFILE_MASK)
                {
                    app_hfp_batt_level_report(app_db.br_link[i].bd_addr);
                }
            }

            app_report_rws_bud_info();

#if F_APP_LISTENING_MODE_SUPPORT
            if (app_cfg_const.disallow_listening_mode_before_bud_connected)
            {
                app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_BUD_DISCONNECTED);
            }
#endif

        }
        break;

    case BP_EVENT_BUD_REMOTE_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_audio_tone_type_play(TONE_REMOTE_CONNECTED, false, true);

                if (ringtone_remaining_count_get() == 0 &&
                    voice_prompt_remaining_count_get() == 0 &&
                    tts_remaining_count_get() == 0)
                {
                    app_relay_async_single(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_REMOTE_SPK2_PLAY_SYNC);
                }
                else
                {
                    gap_start_timer(&timer_handle_play_bud_role_tone, "play_bud_role_tone",
                                    device_timer_queue_id,
                                    APP_DEVICE_TIMER_PLAY_BUD_ROLE_TONE, 0, 500);
                }
            }

            app_ble_handle_remote_conn_cmpl();

#if F_APP_LISTENING_MODE_SUPPORT
            if (app_cfg_const.disallow_listening_mode_before_bud_connected)
            {
                app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_BUD_CONNECTED);
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
#if (F_APP_DUAL_AUDIO_EFFECT == 1)
                if (app_db.gaming_mode)
                {
                    dual_audio_set_effect((DUAL_EFFECT_TYPE)app_cfg_nv.dual_audio_effect_gaming_type, false);
                }
                else
                {
                    dual_audio_restore();
                }
                dual_audio_sync_info();
                dual_audio_sync_all_app_key_val(false);
                dual_audio_effect_restart_track();
#endif
                //app_listening_anc_apt_relay();
            }
            else
            {
            }
        }
        break;

    case BP_EVENT_BUD_REMOTE_DISCONN_CMPL:
        {
            app_ble_handle_remote_disconn_cmpl();

            if ((app_connected_profiles() & HFP_PROFILE_MASK) == 0)
            {
                app_hfp_set_call_status(BT_HFP_CALL_IDLE);
            }

            if ((app_connected_profiles() & A2DP_PROFILE_MASK) == 0)
            {
                app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
            }
        }
        break;

    case BP_EVENT_BUD_LINKLOST_TIMEOUT:
        {
            app_db.power_off_cause = POWER_OFF_CAUSE_SEC_LINKLOST_TIMEOUT;
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
        break;

    case BP_EVENT_SRC_AUTH_SUC:
        {
#if F_APP_IAP_RTK_SUPPORT
            app_iap_rtk_create(event_param->bd_addr);
#endif
            app_ble_handle_b2s_bt_connected(event_param->bd_addr);

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            T_APP_BR_LINK *p_teams_link = app_find_br_link(event_param->bd_addr);
            if (p_teams_link)
            {
                app_teams_multilink_handle_link_connected(p_teams_link->id);
            }
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            app_db.remote_is_8753bau = false;
            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_REMOTE_IS_8753BAU);
#endif

            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_SOURCE_LINK);

            if (app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }

            app_relay_async_single(APP_MODULE_TYPE_BT_POLICY, BT_POLICY_MSG_B2S_ACL_CONN);
            app_bt_policy_b2s_conn_start_timer();
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_roleswap_src_connect_hook)
                {
                    app_roleswap_src_connect_hook();
                }
#endif
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
#if (F_APP_ERWS_SUPPORT == 0)
            if (app_src_connect_hook)
            {
                app_src_connect_hook();
            }
#endif
#endif

            if (app_cfg_const.enable_rtk_charging_box)
            {

#if (BROADCAST_BY_RTK_CHARGER_BOX && F_APP_AVP_INIT_SUPPORT)
                if (app_charger_box_trig_hook && (app_cfg_const.enable_broadcasting_headset == 0))
                {
                    app_charger_box_trig_hook();
                }
#endif
            }

#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                if (app_cfg_const.rws_sniff_negotiation)
                {
                    app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ANC_APT);
                }
            }
            else
            {
#if F_APP_APT_SUPPORT
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                {
                    if (app_cfg_const.rws_sniff_negotiation)
                    {
                        app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ANC_APT);
                    }
                }
                else
#endif
                {
                    if (app_cfg_const.rws_sniff_negotiation)
                    {
                        app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_ANC_APT);
                    }
                }

            }
#endif

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (app_db.ota_tooling_start)
            {
                app_ota_adv_stop();
                app_bt_policy_enter_ota_mode(false);
            }
#endif
            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_LAST_CONN_INDEX);
        }
        break;

    case BP_EVENT_SRC_AUTH_FAIL:
        {
            app_audio_tone_type_play(TONE_PAIRING_FAIL, false, false);
        }
        break;

    case BP_EVENT_SRC_DISCONN_LOST:
        {
#if F_APP_BUZZER_SUPPORT
            buzzer_set_mode(LINK_LOSS_BUZZER);
#endif
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                bool link_loss_play = true;

                link_loss_play = app_device_disconn_need_play(event_param->bd_addr, event_param->cause);

                if (event_param->is_first_src)
                {
                    if (link_loss_play)
                    {
#if F_APP_TEAMS_FEATURE_SUPPORT
                        app_teams_disconnected_state_vp_play(event_param->bd_addr);
#else
                        app_audio_tone_type_play(TONE_LINK_LOSS, false, true);
#endif
                    }
                }
                else
                {
#if F_APP_TEAMS_FEATURE_SUPPORT
                    app_teams_disconnected_state_vp_play(event_param->bd_addr);
#else
                    app_audio_tone_type_play(TONE_LINK_LOSS2, false, true);
#endif
                }

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
                T_APP_BR_LINK *p_teams_link = app_find_br_link(event_param->bd_addr);
                if (p_teams_link)
                {
                    app_teams_multilink_handle_link_disconnected(p_teams_link->id);
                }
#endif
            }

            if (0 == app_bt_policy_get_b2s_connected_num())
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK, app_cfg_const.timer_auto_power_off);
            }

#if (BROADCAST_BY_RTK_CHARGER_BOX && F_APP_AVP_INIT_SUPPORT)
            if (app_cfg_const.enable_rtk_charging_box)
            {
                if (app_charger_box_trig_hook && (app_cfg_const.enable_broadcasting_headset == 0))
                {
                    app_charger_box_trig_hook();
                }
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_src_disconnect_hook)
            {
                app_src_disconnect_hook();
            }
#endif

            memcpy(app_db.bt_addr_disconnect, event_param->bd_addr, 6);

            app_ble_handle_b2s_bt_disconnected(event_param->bd_addr);

#if F_APP_IAP_RTK_SUPPORT
            app_iap_rtk_delete(event_param->bd_addr);
#endif
        }
        break;

    case BP_EVENT_SRC_DISCONN_NORMAL:
        {
#if F_APP_ANC_SUPPORT
            //For ANC secondary measurement
            app_anc_execute_wdg_reset();
#endif

            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
#if (F_APP_AIRPLANE_SUPPORT == 1)
                if (!app_airplane_mode_get())
#endif
                {
                    bool link_disconn_play = true;
                    link_disconn_play = app_device_disconn_need_play(event_param->bd_addr, event_param->cause);
                    if (link_disconn_play)
                    {
                        if (event_param->is_first_src)
                        {
#if F_APP_TEAMS_FEATURE_SUPPORT
                            app_teams_disconnected_state_vp_play(event_param->bd_addr);
#else
                            app_audio_tone_type_play(TONE_LINK_DISCONNECT, false, true);
#endif
                        }
                        else
                        {
#if F_APP_TEAMS_FEATURE_SUPPORT
                            app_teams_disconnected_state_vp_play(event_param->bd_addr);
#else
                            app_audio_tone_type_play(TONE_LINK_DISCONNECT2, false, true);
#endif
                        }
                    }
                }
            }

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            T_APP_BR_LINK *p_teams_link = app_find_br_link(event_param->bd_addr);
            if (p_teams_link)
            {
                app_teams_multilink_handle_link_disconnected(p_teams_link->id);
            }
#endif
            if (0 == app_bt_policy_get_b2s_connected_num())
            {
#if GFPS_FEATURE_SUPPORT
                if (extend_app_cfg_const.gfps_support)
                {
                    app_gfps_handle_b2s_link_disconnected();
                }
#endif

                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK, app_cfg_const.timer_auto_power_off);
            }

#if (BROADCAST_BY_RTK_CHARGER_BOX && F_APP_AVP_INIT_SUPPORT)
            if (app_cfg_const.enable_rtk_charging_box)
            {
                if (app_charger_box_trig_hook && (app_cfg_const.enable_broadcasting_headset == 0))
                {
                    app_charger_box_trig_hook();
                }
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_src_disconnect_hook)
            {
                app_src_disconnect_hook();
            }
#endif

            memcpy(app_db.bt_addr_disconnect, event_param->bd_addr, 6);
            app_ble_handle_b2s_bt_disconnected(event_param->bd_addr);
        }
        break;

    case BP_EVENT_PAIRING_MODE_TIMEOUT:
        {
            app_audio_tone_type_play(TONE_PAIRING_TIMEOUT, false, false);
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_pairing_timeout_hook)
            {
                app_pairing_timeout_hook();
            }
#endif
        }
        break;

    case BP_EVENT_DEDICATED_PAIRING:
        {
            if (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
#if F_APP_TEAMS_BT_POLICY
#else
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
#endif
#endif
            }
#if F_APP_BUZZER_SUPPORT
            buzzer_set_mode(ENTER_PAIRING_BUZZER);
#endif
            app_ble_handle_enter_pair_mode();

        }
        break;

    case BP_EVENT_PROFILE_CONN_SUC:
        {
            if (event_param->is_first_prof)
            {
                if (enable_pairing_complete_led)
                {
                    enable_pairing_complete_led = false;
#if F_APP_BUZZER_SUPPORT
                    buzzer_set_mode(PAIRING_COMPLETE_BUZZER);
#endif
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                    led_change_mode(LED_MODE_PAIRING_COMPLETE, true, false);
#endif
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                led_change_mode(LED_MODE_PAIRING_COMPLETE, true, false);
#endif
                T_APP_BR_LINK *p_link = app_find_br_link(event_param->bd_addr);

                if (memcmp(app_db.resume_addr, event_param->bd_addr, 6) != 0 &&
                    app_db.disallow_connected_tone_after_inactive_connected == false)
                {
                    if (!p_link->b2s_connected_vp_is_played)
                    {
#if F_APP_TEAMS_FEATURE_SUPPORT
                        extern uint8_t conn_to_new_device;
                        app_teams_connected_state_vp_play(event_param->bd_addr, conn_to_new_device);
#else
#if (HARMAN_SUPPORT_CONNECT_VP_IN_HFP == 0)
                        if ((!app_hfp_sco_is_connected())
                            //&& (app_db.br_link[app_get_active_a2dp_idx()].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING)
                           )
#endif
                        {
                            if (app_cfg_const.only_primary_bud_play_connected_tone)
                            {
                                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                                {
#if F_APP_HARMAN_FEATURE_SUPPORT
                                    if (app_cfg_nv.language_status == 0)
                                    {
                                        app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_80, false, false);
                                    }
                                    else
#endif
                                    {
                                        app_audio_tone_type_play(TONE_LINK_CONNECTED, false, false);
                                    }
                                }
                            }
                            else
                            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                                if (app_cfg_nv.language_status == 0)
                                {
                                    app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_80, false, true);
                                }
                                else
#endif
                                {
                                    app_audio_tone_type_play(TONE_LINK_CONNECTED, false, true);
                                }
                            }
                        }
#endif

                        //Enable battery report when first phone connected
                        if (app_cfg_const.enable_bat_report_when_phone_conn && event_param->is_first_src)
                        {
//                            uint8_t bat_level = 0;
                            uint8_t state_of_charge = app_db.local_batt_level;

                            state_of_charge = ((state_of_charge + 9) / 10) * 10;
//                            bat_level = state_of_charge / 10;

#if F_APP_HARMAN_FEATURE_SUPPORT
                            app_charger_harman_low_bat_warning_vp(app_db.local_batt_level, __func__, __LINE__);
#else
                            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                     false);
#endif
                        }
                    }
                }
                else
                {
                    memset(app_db.resume_addr, 0, 6);
                }

                app_db.disallow_connected_tone_after_inactive_connected = false;
                p_link->b2s_connected_vp_is_played = true;

#if (F_APP_SENSOR_SUPPORT == 1)
                if (LIGHT_SENSOR_ENABLED && (app_cfg_const.enable_rtk_charging_box == 0))
                {
                    sensor_ld_start();
                    app_relay_async_single(APP_MODULE_TYPE_BUD_LOC, APP_BUD_LOC_EVENT_SENSOR_LD_CONFIG);
                }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_profile_connect_hook)
                {
                    app_profile_connect_hook();
                }
#endif
#if F_APP_HARMAN_FEATURE_SUPPORT
                ENGAGE_PRINT_TRACE2("app_device_link_policy_ind: conn %s, prof 0x%08x",
                                    TRACE_BDADDR(event_param->bd_addr),
                                    app_db.br_link[app_find_br_link(event_param->bd_addr)->id].connected_profile);
                au_dump_link_information();
#endif
            }
#if GFPS_SASS_SUPPORT
            if (app_sass_policy_support_easy_connection_switch())
            {
                app_sass_policy_profile_conn_handle(app_find_br_link(event_param->bd_addr)->id);
            }
#endif
        }
        break;

    case BP_EVENT_PROFILE_DISCONN:
        {
            ENGAGE_PRINT_TRACE3("app_device_link_policy_ind: disconn %s, prof 0x%08x, is_last %d",
                                TRACE_BDADDR(event_param->bd_addr), event_param->prof, event_param->is_last_prof);

            if (memcmp(app_db.br_link[app_get_active_a2dp_idx()].bd_addr, event_param->bd_addr, 6) == 0)
            {
                if (event_param->prof == A2DP_PROFILE_MASK)
                {
                    app_audio_set_avrcp_status(BT_AVRCP_PLAY_STATUS_STOPPED);
#if F_APP_LISTENING_MODE_SUPPORT
                    app_listening_judge_a2dp_event(APPLY_LISTENING_MODE_AVRCP_PLAY_STATUS_CHANGE);
#endif
                    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_PLAY_STATUS);

                    if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                    {
                        app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                    }
                }
            }

            if ((app_connected_profiles() & (HFP_PROFILE_MASK | A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)) == 0)
            {
                if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
                {
                    app_audio_set_bud_stream_state(BUD_STREAM_STATE_IDLE);
                }
            }

            if (event_param->is_last_prof)
            {
                if ((app_connected_profiles() & SPP_PROFILE_MASK) == 0)
                {
                    if (app_ota_link_loss_stop())
                    {
                        app_ota_error_clear_local(OTA_SPP_DISC);
                    }
                }
            }


            if ((app_connected_profiles() & SPP_PROFILE_MASK) == 0)
            {
#if F_APP_ANC_SUPPORT
                T_ANC_FEATURE_MAP feature_map;
                feature_map.d32 = anc_tool_get_feature_map();

                if (feature_map.user_mode == DISABLE &&
                    app_anc_get_measure_mode() != ANC_RESP_MEAS_MODE_EXIT)
                {
                    app_anc_exit_test_mode(feature_map.mic_path);
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_async_single(APP_MODULE_TYPE_ANC, APP_REMOTE_MSG_EXIT_TEST_MODE);
                    }
                }
#endif

#if F_APP_APT_SUPPORT
                if (app_apt_llapt_scenario_is_busy())
                {
                    app_apt_exit_llapt_scenario_choose_mode();

                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_async_single(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_EXIT_LLAPT_CHOOSE_MODE);
                    }
                }
#endif
#if F_APP_ANC_SUPPORT
                if (app_anc_scenario_select_is_busy())
                {
                    app_anc_exit_scenario_select_mode();

                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_async_single(APP_MODULE_TYPE_ANC, APP_REMOTE_MSG_EXIT_ANC_CHOOSE_MODE);
                    }
                }
#endif
            }

            if (event_param->prof == SPP_PROFILE_MASK)
            {
                app_cmd_update_eq_ctrl(app_cmd_get_tool_connect_status(), true);
            }

            T_APP_BR_LINK *p_link;
            p_link = app_find_br_link(event_param->bd_addr);

            if (p_link != NULL &&
                ((p_link->connected_profile & (HFP_PROFILE_MASK | A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)) == 0))
            {
                if (((p_link->connected_profile & SPP_PROFILE_MASK) != 0)
#if (F_APP_SPP_CAPTURE_DSP_DATA == 1)
                    && (app_cmd_dsp_capture_data_state() == 0)
#endif
                   )
                {
                    app_bt_policy_disconnect(app_db.br_link[p_link->id].bd_addr, SPP_PROFILE_MASK);
                }
            }
            if (p_link && event_param->prof == A2DP_PROFILE_MASK)
            {
                app_harman_total_playback_time_update();
                p_link->streaming_fg = false;
                if (app_find_a2dp_start_num() == 0)
                {
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_A2DP);

                    app_bt_policy_primary_engage_action_adjust();
                }
            }
#if F_APP_HARMAN_FEATURE_SUPPORT
            au_dump_link_information();
            if (event_param->prof == A2DP_PROFILE_MASK)
            {
                app_harman_recon_a2dp(p_link->id);
            }
#endif
        }
        break;

    default:
        break;
    }
}

void app_device_state_change(T_APP_DEVICE_STATE state)
{
    uint8_t report_data = 0;
    APP_PRINT_TRACE2("app_device_state_change: cur_state 0x%02x, next_state 0x%02x",
                     app_db.device_state, state);
    app_db.device_state = state;
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        report_data = APP_DEVICE_STATE_ON;
    }
    else
    {
        report_data = APP_DEVICE_STATE_OFF;
    }
    if (app_cfg_const.enable_data_uart)
    {
        app_report_event(CMD_PATH_UART, EVENT_DEVICE_STATE, 0, &report_data, sizeof(report_data));
    }
}

static void app_device_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    bool ota_tooling_not_force_engage = false;
    bool airplane_mode = false;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            app_db.power_on_start_time = sys_timestamp_get();
            app_cfg_nv.gpio_out_detected = 0;
            app_cfg_nv.app_is_power_on = 1;
            app_cfg_nv.rtc_wakeup_count = 0;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
#if F_APP_HARMAN_FEATURE_SUPPORT
            app_harman_ble_ota_power_on_handle();

            app_harman_eq_power_on_handle();

            app_harman_license_product_info_dump();

            au_set_power_on_link_back_fg(false);
#endif
            APP_PRINT_INFO5("app_device_dm_cback: bud_role %d, factory_reset_done %d, first_engaged %d, key enter pairing %d, %d",
                            app_cfg_nv.bud_role, app_cfg_nv.factory_reset_done, app_cfg_nv.first_engaged,
                            app_key_is_enter_pairing(), app_cfg_const.enable_multi_link);

            app_device_state_change(APP_DEVICE_STATE_ON);

            app_sniff_mode_startup();

            pairing_disable_power_off_fg = false;
            linkback_disable_power_off_fg = false;
            app_dlps_enable_auto_poweroff_stop_wdg_timer();

            app_db.mic_mp_verify_pri_sel_ori = APP_MIC_SEL_DISABLE;
            app_db.power_off_cause = POWER_OFF_CAUSE_UNKNOWN;
            app_db.play_min_max_vol_tone = true;

            app_db.remote_loc_received = false;
            app_db.disallow_play_gaming_mode_vp = false;
            app_db.hfp_report_batt = true;

            if (app_db.ignore_led_mode)
            {
                app_db.ignore_led_mode = false;
            }
            else
            {
                led_change_mode(LED_MODE_POWER_ON, true, false);
#if F_APP_TEAMS_FEATURE_SUPPORT
                teams_led_control_when_power_on();
#endif
            }

            app_key_check_vol_mmi();

            //Update battery volume after power on
            if (app_cfg_const.discharger_support)
            {
                app_charger_update();
            }

            if (app_cfg_const.charger_support)
            {
                app_db.local_in_case = app_device_is_in_the_box();
                app_db.local_loc = app_sensor_bud_loc_detect();
            }

            // clear this flag when power on to prevent flag not clear
            if (app_cfg_nv.report_box_bat_lv_again_after_sw_reset)
            {
                app_cfg_nv.report_box_bat_lv_again_after_sw_reset = false;
                app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
                app_cfg_store(&app_cfg_nv.case_battery, 4);
            }

            //reset to default eq after power on
            if (app_cfg_const.reset_eq_when_power_on)
            {
                app_cfg_nv.eq_idx_normal_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, NORMAL_MODE);
                app_cfg_nv.eq_idx_gaming_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, GAMING_MODE);
                app_cfg_nv.eq_idx_anc_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, ANC_MODE);
#if F_APP_LINEIN_SUPPORT
                app_cfg_nv.eq_idx_line_in_mode_record = eq_utils_get_default_idx(MIC_SW_EQ, APT_MODE);
#endif

                if (app_cfg_const.enable_enter_gaming_mode_after_power_on)
                {
                    app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_gaming_mode_record;
                }
                else
                {
                    app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;
                }
            }

#if F_APP_BUZZER_SUPPORT
            buzzer_set_mode(POWER_ON_BUZZER);
#endif

#if (BROADCAST_BY_RTK_CHARGER_BOX && F_APP_AVP_INIT_SUPPORT)
            if (app_cfg_const.enable_rtk_charging_box)
            {
                if (app_charger_box_trig_hook && (app_cfg_const.enable_broadcasting_headset == 0))
                {
                    app_charger_box_trig_hook();
                }
            }
#endif

#if F_APP_ANC_SUPPORT
            if (anc_tool_check_resp_meas_mode() == ANC_RESP_MEAS_MODE_NONE)
            {
                app_ble_handle_power_on();
            }
#else
            app_ble_handle_power_on();
#endif

#if (F_APP_VENDOR_SENSOR_INTERFACE_SUPPORT == 1)
            if (app_cfg_const.customized_driver_support)
            {
                app_vendor_driver_start_device();
            }
#endif

            if (app_cfg_const.enable_dlps)
            {
                power_mode_set(POWER_DLPS_MODE);
            }

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (app_db.ota_tooling_start && (app_db.jig_subcmd != APP_ADP_SPECIAL_CMD_RWS_FORCE_ENGAGE))
            {
                ota_tooling_not_force_engage = true;
                APP_PRINT_INFO2("app_device_dm_cback: ota_tooling_start %d, jig_subcmd %d",
                                app_db.ota_tooling_start, app_db.jig_subcmd);
            }
#endif

#if F_APP_LISTENING_MODE_SUPPORT
            //before airplane_power_on_handle
            app_listening_apply_when_power_on();
#endif

#if (F_APP_AIRPLANE_SUPPORT == 1)
            //before bt_policy_startup
            if (app_airplane_combine_key_power_on_get() || app_cfg_const.airplane_mode_when_power_on)
            {
                app_airplane_power_on_handle();
            }

            if (app_airplane_mode_get())
            {
                airplane_mode = true;
                APP_PRINT_INFO0("app_device_dm_cback: airplane mode");
            }
#endif

            if (!app_cfg_nv.factory_reset_done || app_key_is_enter_pairing()
                || ota_tooling_not_force_engage
                || airplane_mode
               )
            {
                app_bt_policy_startup(app_device_link_policy_ind, false);
            }
            else
            {
                app_bt_policy_startup(app_device_link_policy_ind, true);
            }

            if (!app_cfg_nv.factory_reset_done && !app_device_is_in_the_box()
                && app_cfg_const.auto_power_on_and_enter_pairing_mode_before_factory_reset)
            {
                app_bt_policy_enter_pairing_mode(false, true);
            }

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            if (app_db.ota_tooling_start)
            {
                app_adp_special_cmd_handle(app_db.jig_subcmd, app_db.jig_dongle_id);
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_power_on_hook)
            {
                app_power_on_hook();
            }

            const uint8_t zero_array[12] = {0};
            if (memcmp(app_avp_id_data.avp_id, zero_array, sizeof(zero_array)) == 0)
            {
                //There is no value in app_cfg_rw.dulian_id, copy id from app_cfg.
                APP_PRINT_INFO0("update durian id from default config");

                memcpy(app_avp_id_data.avp_id, app_cfg_const.avp_id, 12);
                app_avp_id_save_data(&app_avp_id_data);
            }

            if (memcmp(app_avp_id_data.avp_single_id, zero_array, sizeof(zero_array)) == 0)
            {
                APP_PRINT_INFO0("update durian single id from default config");

                memcpy(app_avp_id_data.avp_single_id, app_cfg_const.avp_single_id,
                       sizeof(app_avp_id_data.avp_single_id));
                app_avp_id_save_data(&app_avp_id_data);
            }
#endif

            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_POWER_ON, app_cfg_const.timer_auto_power_off);
            app_dlps_enable(APP_DLPS_ENTER_WAIT_RESET);
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_harman_total_power_on_time_update();
            mp_hci_test_set_mode(false);

#if F_APP_BUZZER_SUPPORT
            buzzer_set_mode(POWER_OFF_BUZZER);
#endif

            app_ble_handle_power_off();
            app_auto_power_off_enable(~AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF,
                                      app_cfg_const.timer_auto_power_off);

            if (app_cfg_const.discharger_support)
            {
                app_cfg_nv.local_level = app_db.local_batt_level;
                app_cfg_nv.remote_level = app_db.remote_batt_level;
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_power_off_hook)
            {
                app_power_off_hook();
            }
#endif

#if (F_APP_DUAL_AUDIO_EFFECT == 1)
            if (!app_cfg_const.dual_audio_default_save)
            {
                app_cfg_nv.audio_effect_type = app_cfg_const.dual_audio_default;
            }
#endif

            app_cfg_nv.app_is_power_on = 0;
            app_cfg_store_all();

            app_bt_stop_a2dp_and_sco();
            app_bt_policy_shutdown();

#if (F_APP_AIRPLANE_SUPPORT == 1)
            //after app_bt_policy_shutdown
            app_airplane_power_off_handle();
#endif
            app_bond_clear_sec_diff_link_record();

#if (F_APP_VENDOR_SENSOR_INTERFACE_SUPPORT == 1)
            if (app_cfg_const.customized_driver_support)
            {
                app_vendor_driver_handle_power_state(SENSOR_POWER_LOW_POWER_OFF_MODE);
            }
#endif
#if F_APP_LE_AUDIO_SM
            app_le_audio_device_sm(LE_AUDIO_POWER_OFF, NULL);
#endif

            app_dlps_power_off();
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_device_dm_cback: event_type 0x%04x", event_type);
    }
}

/* @brief  wheather the test equipment is required device
*
* @param  name and addr
* @return true/false
*/
static bool app_device_is_test_equipment(const char *name, uint8_t *addr)
{
    T_APP_TEST_EQUIPMENT_INFO equipment_info[] =
    {
        {{0x00, 0x30, 0xD3}, "AGILENT TECHNOLOGIES"},
        {{0x8C, 0x14, 0x7D}, "AGILENT TECHNOLOGIES"},
        {{0x00, 0x04, 0x43}, "AGILENT TECHNOLOGIES"},
        {{0xBD, 0xBD, 0xBD}, "AGILENT TECHNOLOGIES"},
        {{0x00, 0x02, 0xB1}, "ANRITSU MT8852"},
        {{0x00, 0x00, 0x91}, "ANRITSU MT8852"},
        {{0x00, 0x00, 0x91}, "ANRITSU MT8850A Bluetooth Test set"},
        {{0x70, 0xB3, 0xD5}, "R&S CMW"},
        {{0x00, 0x90, 0xB8}, "R&S CMW"},
        {{0xDC, 0x44, 0x27}, "R&S CMW"},
        {{0x70, 0xB3, 0xD5}, "R&S CMU"},
        {{0x00, 0x90, 0xB8}, "R&S CMU"},
        {{0xDC, 0x44, 0x27}, "R&S CMU"},
        {{0x70, 0xB3, 0xD5}, "R&S CBT"},
        {{0x00, 0x90, 0xB8}, "R&S CBT"},
        {{0xDC, 0x44, 0x27}, "R&S CBT"},
    };

    bool ret = false;
    uint8_t i = 0;
    uint8_t device_oui[3] = {0};

    device_oui[0] = addr[5];
    device_oui[1] = addr[4];
    device_oui[2] = addr[3];

    for (i = 0; i < sizeof(equipment_info) / sizeof(T_APP_TEST_EQUIPMENT_INFO); i++)
    {
        if (!strncmp(equipment_info[i].name, name, strlen(equipment_info[i].name)) &&
            !memcmp(device_oui, equipment_info[i].oui, sizeof(equipment_info[i].oui)))
        {
            ret = true;
            break;
        }
    }

    APP_PRINT_TRACE3("app_device_is_test_equipment: ret %d name %s oui %b", ret, TRACE_STRING(name),
                     TRACE_BINARY(3, device_oui));

    return ret;
}

static bool app_device_auto_power_on_before_factory_reset(void)
{
    bool ret = false;
    uint8_t state_of_charge;

    state_of_charge = app_charger_get_soc();

    if (state_of_charge > BAT_CAPACITY_0)
    {
        if (app_cfg_const.auto_power_on_and_enter_pairing_mode_before_factory_reset &&
            !app_cfg_nv.factory_reset_done &&
            !app_device_is_in_the_box())
        {
            ret = true;
        }
    }

    APP_PRINT_INFO1("app_device_auto_power_on_before_factory_reset: ret = %d", ret);
    return ret;
}

static bool app_device_boot_up_directly(void)
{
    bool ret = false;

    APP_PRINT_INFO4("app_device_boot_up_directly: %d, %d, %d, %d",
                    app_cfg_nv.adp_factory_reset_power_on,
                    app_cfg_nv.ota_tooling_start, app_cfg_nv.power_off_cause_cmd,
                    app_db.executing_charger_box_special_cmd);

    if (!app_cfg_const.mfb_replace_key0)
    {
        if (app_dlps_check_short_press_power_on())
        {
            ret = true;
            goto boot_up_directly;
        }
    }

    if (app_device_auto_power_on_before_factory_reset())
    {
        ret = true;
        goto boot_up_directly;
    }

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    if (app_cfg_nv.ota_tooling_start)
    {
        // add protect for timing issue
        if (app_db.executing_charger_box_special_cmd == 0)
        {
            app_db.local_loc = BUD_LOC_IN_CASE;

            // update data to ram
            app_db.ota_tooling_start = app_cfg_nv.ota_tooling_start;
            app_db.jig_subcmd = app_cfg_nv.jig_subcmd;
            app_db.jig_dongle_id = app_cfg_nv.jig_dongle_id;

            if (app_db.jig_subcmd == APP_ADP_SPECIAL_CMD_RWS_FORCE_ENGAGE)
            {
                app_cfg_const.rws_pairing_required_rssi = 0x7F;
            }

            // clear flash data
            app_cfg_nv.ota_tooling_start = 0;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

            app_cfg_nv.jig_subcmd = 0;
            app_cfg_nv.jig_dongle_id = 0;
            app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);

            ret = true;
            goto boot_up_directly;
        }
    }
#endif

    if (app_cfg_const.enable_rtk_charging_box)
    {
        if (app_cfg_nv.adp_factory_reset_power_on)
        {
            if (!app_cfg_nv.power_off_cause_cmd)
            {
                app_db.power_on_by_cmd = true;
                app_db.local_in_case = app_device_is_in_the_box();
                app_db.local_loc = app_sensor_bud_loc_detect();

                app_adp_case_battery_check(&app_cfg_nv.case_battery, &app_db.case_battery);
                app_cfg_nv.adp_factory_reset_power_on = 0;

                ret = true;
            }
            else
            {
                app_cfg_nv.power_off_cause_cmd = 0;
                app_cfg_store(&app_cfg_nv.remote_loc, 4);
                app_cfg_nv.adp_factory_reset_power_on = 0;
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_charger_box_reset_hook)
            {
                app_charger_box_reset_hook();
            }
#endif
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
            goto boot_up_directly;
        }
    }

    if (app_cfg_nv.auto_power_on_after_factory_reset)
    {
        app_cfg_nv.auto_power_on_after_factory_reset = 0;
        app_cfg_store(&app_cfg_nv.eq_idx_gaming_mode_record, 4);

        ret = true;
        goto boot_up_directly;
    }

    if ((app_cfg_const.box_detect_method == ADAPTOR_DETECT) &&
        app_cfg_const.adaptor_disallow_power_on && app_device_is_in_the_box()
#if F_APP_GPIO_ONOFF_SUPPORT
        ||
        (app_cfg_const.box_detect_method == GPIO_DETECT) && app_device_is_in_the_box()
#endif
       )
    {
#if F_APP_GPIO_ONOFF_SUPPORT
        if (app_cfg_const.box_detect_method == GPIO_DETECT)
        {
            app_cfg_nv.pre_3pin_status_unplug = 0;
        }
#endif
    }
    else
    {
#if F_APP_GPIO_ONOFF_SUPPORT
        if (app_cfg_const.box_detect_method == GPIO_DETECT)
        {
            app_cfg_nv.pre_3pin_status_unplug = 1;
        }
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
        APP_PRINT_INFO4("[SD_CHECK]power_on check %d,%d,%d,%d", app_cfg_nv.app_is_power_on,
                        app_cfg_const.box_detect_method,
                        app_cfg_const.adaptor_disallow_power_on,
                        app_device_is_in_the_box());
        if (app_adp_get_plug_state() == ADAPTOR_PLUG)
        {
            app_cfg_nv.app_is_power_on = 0;
        }
        else if (app_adp_get_plug_state() == ADAPTOR_UNPLUG)
#endif
        {
            ret = true;
        }
    }
    app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

boot_up_directly:
    APP_PRINT_INFO1("app_device_boot_up_directly: ret %d", ret);
    return ret;
}

static bool app_device_power_on_check(void)
{
    bool ret = false;

    APP_PRINT_INFO3("app_device_power_on_check: %d, %d, %d",
                    app_cfg_nv.gpio_out_detected,
                    app_cfg_nv.adaptor_changed,
                    app_cfg_nv.pre_3pin_status_unplug);

    if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
    {
        if (app_cfg_const.enable_outbox_power_on)
        {
            if (!app_device_is_in_the_box())
            {
                if (app_cfg_const.smart_charger_control && ((app_cfg_nv.case_battery & 0x7F) == 0))
                {
                    APP_PRINT_INFO0("app_device_power_on_check: ignore power on when box zero volume");
                }
                else
                {
                    if (!app_db.key0_wake_up && (!app_cfg_nv.is_app_reboot || app_cfg_nv.adaptor_changed))
                    {
                        ret = true;
                    }
                }
            }
        }
    }
#if F_APP_GPIO_ONOFF_SUPPORT
    else if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        if (!app_device_is_in_the_box())
        {
            if (app_cfg_nv.factory_reset_done)
            {
                if (app_cfg_nv.is_app_reboot)
                {
                    if (!app_cfg_nv.pre_3pin_status_unplug)
                    {
                        ret = true;
                    }
                }
                else if ((System_WakeUpInterruptValue(app_cfg_const.gpio_box_detect_pinmux) == SET) ||
                         // 3pin wake up
                         (app_cfg_nv.gpio_out_detected == 1) || // 3pin wake up
                         (app_cfg_nv.pre_3pin_status_unplug == 0) || // Abnormal reset
                         ((app_db.key0_wake_up == false) && (app_db.combine_poweron_key_wake_up == false))) // 5v wake up
                {
                    ret = true;
                }
            }

            app_cfg_nv.gpio_out_detected = 0;
            app_cfg_nv.pre_3pin_status_unplug = 1;
        }
        else
        {
            app_cfg_nv.pre_3pin_status_unplug = 0;
        }
        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
    }
#endif

    return ret;
}

static bool app_device_reboot_handle(void)
{
    bool ret = false;

    APP_PRINT_INFO3("app_device_reboot_handle: %d, %d, %d",
                    app_cfg_nv.power_on_cause_cmd,
                    app_cfg_nv.ota_tooling_start,
                    app_cfg_nv.auto_power_on_after_factory_reset);

    if (app_cfg_const.enable_rtk_charging_box && app_cfg_nv.power_on_cause_cmd)
    {
        app_cfg_nv.power_on_cause_cmd = false;
        app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);

        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT &&
            app_db.adp_high_wake_up_for_zero_box_bat_vol)
        {
            /* disallow adp out power direct power on when box is zero bat vol (5v will be dropped when we get zero box bat vol) */
            ret = false;
        }
        else
        {
            ret = true;
        }
    }
    else if ((!app_db.peri_wake_up)
             && (app_charger_get_soc() > BAT_CAPACITY_0)
             && (app_cfg_nv.adp_factory_reset_power_on
                 || app_cfg_nv.ota_tooling_start
                 || app_cfg_nv.auto_power_on_after_factory_reset))
    {
        if (app_device_boot_up_directly())
        {
            ret = true;
        }
    }
    else if (app_device_power_on_check())
    {
        ret = true;
    }

    return ret;
}

static void app_device_bt_and_ble_ready_check(void)
{
    bool power_on_flg = false;

    if (app_db.bt_is_ready && app_db.ble_is_ready)
    {
        APP_PRINT_INFO8("app_device_bt_and_ble_ready_check: %d, %d, %d, %d, %d, %d, %d, %d",
                        app_cfg_nv.is_dut_test_mode,
                        app_cfg_nv.app_is_power_on,
                        app_db.peri_wake_up,
                        app_cfg_nv.factory_reset_done,
                        app_cfg_nv.adaptor_changed,
                        app_db.key0_wake_up,
                        app_db.combine_poweron_key_wake_up,
                        System_WakeUpInterruptValue(app_cfg_const.gpio_box_detect_pinmux));
        if (app_cfg_nv.need_set_lps_mode)
        {
            APP_PRINT_INFO1("app_device_bt_and_ble_ready_check: enter shipping mode: %d",
                            app_cfg_nv.ota_parking_lps_mode);
            // In shipping mode, DUT should clean flag and ignore power on handle
            app_cfg_nv.need_set_lps_mode = 0;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

            app_cfg_nv.auto_power_on_after_factory_reset = 0;
            app_cfg_store(&app_cfg_nv.offset_is_dut_test_mode, 4);

            if (app_cfg_nv.ota_parking_lps_mode)
            {
                // set power off
                power_mode_set(POWER_POWEROFF_MODE);

                if (app_cfg_nv.ota_parking_lps_mode == OTA_TOOLING_SHIPPING_5V_WAKEUP_ONLY)
                {
                    System_SetMFBWakeUpFunction(DISABLE);
                }
            }
            else
            {
                power_mode_set(POWER_POWERDOWN_MODE);
            }
            return;
        }

        if (app_cfg_const.charger_control_by_mcu)
        {
#if F_APP_ADC_SUPPORT
            app_adc_start_monitor_adp_voltage();
#endif
        }

        if (app_cfg_const.rtk_app_adv_support || app_cfg_const.tts_support)
        {
            /* init here to avoid app_cfg_nv.bud_local_addr no mac info (due to factory reset) */
            le_common_adv_init();
#if F_APP_TTS_SUPPORT
            if (app_cfg_const.tts_support)
            {
                app_ble_tts_init();
            }
#endif
        }

#if F_APP_TUYA_SUPPORT
        tuya_ble_app_init();
#endif

        //update battery volume when boot up
        if (app_cfg_const.discharger_support)
        {
            app_db.local_batt_level = app_charger_get_soc();
            APP_PRINT_INFO1("app_device_bt_and_ble_ready_check: local_batt_level: %d", app_db.local_batt_level);
        }

#if F_APP_PERIODIC_WAKEUP_RECHARGE
        if (app_db.wake_up_reason & WAKE_UP_RTC)
        {
#if HARMAN_VBAT_ADC_DETECTION
            // read ADC and store to FTL
            app_dlps_disable(APP_DLPS_ENTER_VBAT_VALUE_UPDATE);
#endif

            app_cfg_nv.rtc_wakeup_count++;
            DBG_DIRECT("app_device_bt_and_ble_ready_check: Wakeup, rtc_counter %d",
                       app_cfg_nv.rtc_wakeup_count);
            app_cfg_store(&app_cfg_nv.rtc_wakeup_count, 4);
        }
#endif

#if GFPS_FEATURE_SUPPORT
        /*Resolvable private address can only be successfully generate after BLE stack ready,
        app_gfps_adv_init() and app_gfps_finder_init() need to generate RPA, so we call them here*/
        if (extend_app_cfg_const.gfps_support)
        {
            app_gfps_adv_init();
#if GFPS_FINDER_SUPPORT
            if (extend_app_cfg_const.gfps_finder_support)
            {
                app_gfps_finder_init();

#if GFPS_FINDER_DULT_SUPPORT
                app_dult_device_init();
                app_dult_handle_stack_ready();
#endif

#if HARMAN_VBAT_ONE_ADC_DETECTION
				if (app_db.wake_up_reason & WAKE_UP_RTC)
				{
					//APP_PRINT_INFO0("----> app_harman_adc_saved_nv_data111");
					app_harman_adc_saved_nv_data();
					//APP_PRINT_INFO0("----> app_harman_adc_saved_nv_data222");
				}
#endif


#if F_APP_PERIODIC_WAKEUP_RECHARGE
#if HARMAN_VBAT_ONE_ADC_DETECTION
                if ((!extend_app_cfg_const.disable_finder_adv_when_power_off && app_gfps_finder_provisioned()) 
					|| ((app_cfg_nv.nv_single_ntc_function_flag==1) && (app_cfg_nv.ntc_poweroff_wakeup_flag==0)))				
#else
                if (!extend_app_cfg_const.disable_finder_adv_when_power_off			
					&& app_gfps_finder_provisioned())
#endif                    
					

                {
                    uint32_t clock_value_delayed = 0;
                    uint32_t rtc_counter = 0;

                    rtc_counter = RTC_GetCounter();
                    clock_value_delayed = RTC_COUNTER_TO_SECOND(rtc_counter);

                    DBG_DIRECT("app_device_bt_and_ble_ready_check: Wakeup, rtc_counter %d, clock_value_delayed %d, rtc_wakeup_count %d",
                               rtc_counter, clock_value_delayed, app_cfg_nv.rtc_wakeup_count);

                    RTC_RunCmd(DISABLE);
                    power_mode_set(POWER_POWERDOWN_MODE);
                    app_gfps_finder_update_clock_value(clock_value_delayed);
                    if ((app_db.wake_up_reason & WAKE_UP_RTC)
#if (HARMAN_VBAT_ADC_DETECTION|HARMAN_VBAT_ONE_ADC_DETECTION)
					&& (app_cfg_nv.rtc_wakeup_count % (extend_app_cfg_const.gfps_finder_adv_skip_count_when_wakeup + 1)
						== 0)
					&& app_gfps_finder_provisioned()
#endif
                       )
                    {
                        power_mode_set(POWER_DLPS_MODE);
                        app_gfps_finder_handle_event_wakeup_by_rtc();
                    }
                }
				if(app_cfg_nv.ntc_poweroff_wakeup_flag != 0)
				{
					app_cfg_nv.ntc_poweroff_wakeup_flag = 0;
					app_cfg_store(&app_cfg_nv.ntc_poweroff_wakeup_flag, 4);
				}
#endif
            }
#endif
        }
#endif

        app_bt_update_pair_idx_mapping();

        if (app_cfg_const.smart_charger_control && app_cfg_const.enable_external_mcu_reset)
        {
            app_adp_load_adp_high_wake_up();
        }

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_const.one_wire_uart_support && !app_device_is_in_the_box())
        {
            app_one_wire_deinit();
        }
#endif

        if ((app_cfg_const.key_gpio_support) && (app_cfg_const.key_power_on_interval == 0) &&
            app_db.key0_wake_up)
        {
            app_device_unlock_vbat_disallow_power_on();
        }

        if (app_cfg_const.enable_rtk_charging_box)
        {
            app_adp_case_battery_check(&app_cfg_nv.case_battery, &app_db.case_battery);
        }

        if (app_cfg_nv.disallow_power_on_when_vbat_in)
        {
            // Power on is not allowed for the first time when vbat in after factory reset by mppgtool;
            // Unlock disallow power on mode by MFB power on or slide switch power on or bud starts chargring
#if F_APP_GPIO_ONOFF_SUPPORT
            if (app_cfg_const.box_detect_method == GPIO_DETECT)
            {
                // 3pin location get error in app_cfg_reset if sync_word is not initialized
                if (app_device_is_in_the_box())
                {
                    app_cfg_nv.pre_3pin_status_unplug = 0;
                }
                else
                {
                    app_cfg_nv.pre_3pin_status_unplug = 1;
                }
                app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
            }
#endif
        }
        else if (app_cfg_nv.is_dut_test_mode)
        {
            app_cfg_nv.is_dut_test_mode = 0;
            app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
        }
        else if (app_cfg_nv.is_app_reboot)
        {
            if (app_device_reboot_handle())
            {
                power_on_flg = true;
            }

            app_cfg_nv.is_app_reboot = 0;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        }
        else
        {
            if (app_device_auto_power_on_before_factory_reset()
                || (app_cfg_const.key_gpio_support && (app_cfg_const.key_power_on_interval == 0))
                || ((!app_db.peri_wake_up) &&
                    (app_charger_get_soc() > BAT_CAPACITY_0) &&
                    (app_cfg_nv.app_is_power_on
                     || app_cfg_nv.adp_factory_reset_power_on
                     || app_cfg_nv.ota_tooling_start
                     || app_cfg_nv.auto_power_on_after_factory_reset)))
            {
                if (app_device_boot_up_directly())
                {
                    power_on_flg = true;
                }
            }
            else if (app_device_power_on_check())
            {
                power_on_flg = true;
            }

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
            if (!app_slide_switch_power_on_valid_check())
            {
                power_on_flg = false;
            }
#endif

#if (F_APP_USB_AUDIO_SUPPORT == 1)
            if (app_usb_audio_plug_in_power_on_check())
            {
                power_on_flg = true;
                app_usb_audio_plug_in_power_on_clear();
            }
#endif
        }

        if (app_cfg_const.key_gpio_support && (app_cfg_const.key_power_on_interval == 0) &&
            key_get_press_state(0))
        {
            power_on_flg = false;
        }

        if (power_on_flg)
        {
#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
            if (!app_harman_discharger_ntc_check_valid())
            {
                app_db.need_delay_power_on = true;
                app_dlps_disable(APP_DLPS_ENTER_VBAT_VALUE_UPDATE);
            }
#endif
            app_mmi_handle_action(MMI_DEV_POWER_ON);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
    }
}

static void app_device_ble_cback(uint8_t subtype, T_LE_GAP_MSG *gap_msg)
{
    switch (subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            if (!app_db.ble_is_ready)
            {
                if (gap_msg->msg_data.gap_dev_state_change.new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
                {
                    app_db.ble_is_ready = true;
                    app_device_bt_and_ble_ready_check();
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_device_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

#if F_APP_LOCAL_PLAYBACK_SUPPORT
    bool usb_need_start = false;
#endif

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            uint8_t null_addr[6] = {0};

            memcpy(app_db.factory_addr, param->ready.bd_addr, 6);
            APP_PRINT_INFO1("app_gap_bt_cback: ready, bd_addr %b",
                            TRACE_BDADDR(param->ready.bd_addr));

            if (!memcmp(app_cfg_nv.bud_local_addr, null_addr, 6))
            {
                memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
                remote_local_addr_set(app_cfg_nv.bud_local_addr);
            }

            if (!memcmp(app_cfg_nv.bud_peer_addr, null_addr, 6))
            {
                memcpy(app_cfg_nv.bud_peer_addr, app_cfg_const.bud_peer_addr, 6);
                remote_peer_addr_set(app_cfg_nv.bud_peer_addr);
            }

            gap_set_bd_addr(app_cfg_nv.bud_local_addr);

#if C_APP_DEVICE_CMD_SUPPORT
            app_report_event(CMD_PATH_UART, EVENT_BT_READY, 0, NULL, 0);
#endif

            if (!app_db.bt_is_ready)
            {
                app_db.bt_is_ready = true;
                app_device_bt_and_ble_ready_check();
            }
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            //we need to get src name to judge whether enter dut mode or not.
            app_db.force_enter_dut_mode_when_acl_connected = false;
            legacy_get_remote_name(param->acl_conn_success.bd_addr);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            if (app_check_b2s_link(param->acl_conn_success.bd_addr))
            {
                if (app_cfg_const.enable_enter_gaming_mode_after_power_on)
                {
                    if (!app_db.gaming_mode)
                    {
                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                        {
                            app_mmi_handle_action(MMI_DEV_GAMING_MODE_SWITCH);
                        }
                        else
                        {
                            app_relay_sync_single(APP_MODULE_TYPE_MMI, MMI_DEV_GAMING_MODE_SWITCH, REMOTE_TIMER_HIGH_PRECISION,
                                                  0, false);
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (app_check_b2s_link(param->acl_conn_disconn.bd_addr))
            {
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                if (param->acl_conn_disconn.cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    if (app_db.remote_is_8753bau)
                    {
                        app_db.remote_is_8753bau = false;
                        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                        {
                            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_REMOTE_IS_8753BAU);
                        }

                        if (app_db.gaming_mode)
                        {
                            app_db.restore_gaming_mode = true;

                            app_mmi_switch_gaming_mode();
#if F_APP_ERWS_SUPPORT
                            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                            {
                                uint8_t cmd = true;
                                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                                                    APP_REMOTE_MSG_ASK_TO_EXIT_GAMING_MODE,
                                                                    (uint8_t *)&cmd, sizeof(cmd));
                            }
#endif
                        }

                        if (app_db.dongle_is_enable_mic)
                        {
                            app_dongle_stop_recording(param->acl_conn_disconn.bd_addr);
                            app_db.dongle_is_enable_mic = false;
#if F_APP_ERWS_SUPPORT
                            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                            {
                                app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_DONGLE_IS_ENABLE_MIC);
                            }
#endif
                        }
                    }
                }
#endif

                if (mp_hci_test_mode_is_running())
                {
                    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_DUT_MODE, app_cfg_const.timer_auto_power_off);
                }
            }
        }
        break;

    case BT_EVENT_REMOTE_NAME_RSP:
        {
            if (param->remote_name_rsp.name != NULL)
            {
                if (app_device_is_test_equipment(param->remote_name_rsp.name,
                                                 param->remote_name_rsp.bd_addr))
                {
                    app_db.force_enter_dut_mode_when_acl_connected = true;

                    if (!app_bt_policy_get_b2b_connected())
                    {
                        bt_sniff_mode_disable(param->remote_name_rsp.bd_addr);
                    }

                    app_mmi_handle_action(MMI_DUT_TEST_MODE);
                    app_db.force_enter_dut_mode_when_acl_connected = false;
                }
            }

#if HARMAN_REQ_REMOTE_DEVICE_NAME_TIME
            need_wait_remote_name_rsp = false;
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
            T_APP_BR_LINK *p_link = NULL;

            p_link = app_find_b2s_link((uint8_t *)&param->remote_name_rsp.bd_addr);
            if (p_link == NULL)
            {
                break;
            }

#if HARMAN_REQ_REMOTE_DEVICE_NAME_TIME
            T_APP_LE_LINK *p_le_link;

            p_le_link = app_find_le_link_by_conn_id(le_common_adv_get_conn_id());
            if ((app_find_b2s_link_num()) &&
                ((le_common_adv_get_conn_id() == 0xFF) ||
                 (p_le_link != NULL) && (p_le_link->state != LE_LINK_STATE_CONNECTED)))
            {
                remote_name_req_idx = p_link->id;
                app_harman_req_remote_device_name_timer_start();
            }
#endif
            if (param->remote_name_rsp.cause)
            {
                APP_PRINT_ERROR1("BT_EVENT_REMOTE_NAME_RSP: cause: 0x%x", param->remote_name_rsp.cause);
                break;
            }

            uint16_t new_device_name_crc = 0;
            uint32_t remote_name_len = strlen(param->remote_name_rsp.name);

            APP_PRINT_TRACE3("BT_EVENT_REMOTE_NAME_RSP: remote_device_name: %s, name_len: %d, active_bank: %d",
                             TRACE_STRING(param->remote_name_rsp.name), remote_name_len, app_ota_get_active_bank());

            if (remote_name_len >= 31)
            {
                remote_name_len = 31;
            }
            new_device_name_crc = harman_crc16_ibm((uint8_t *)param->remote_name_rsp.name, remote_name_len,
                                                   p_link->bd_addr);
            if (memcmp(&p_link->device_name_crc, (uint8_t *)&new_device_name_crc, 2))
            {
                APP_PRINT_TRACE2("BT_EVENT_REMOTE_NAME_RSP: new_device_name_crc: 0x%x, device_name_crc: %b",
                                 new_device_name_crc, TRACE_BINARY(2, p_link->device_name_crc));
                memcpy(&p_link->device_name_crc, (uint8_t *)&new_device_name_crc, 2);

                app_harman_remote_device_name_crc_set(p_link->bd_addr, true);

                if (app_cfg_const.rtk_app_adv_support)
                {
                    app_ble_rtk_adv_start();
                }

                app_harman_le_common_adv_update();
            }
#endif

#if F_APP_TEAMS_BT_POLICY
            temp_master_device_name_len = strlen(param->remote_name_rsp.name);
            memcpy(temp_master_device_name, param->remote_name_rsp.name, temp_master_device_name_len);
#endif
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
            switch (param->hfp_call_status.curr_status)
            {
            case BT_HFP_INCOMING_CALL_ONGOING:
#if F_APP_BUZZER_SUPPORT
                buzzer_set_mode(INCOMING_CALL_BUZZER);
#endif
                break;

            case BT_HFP_CALL_ACTIVE:
                {
                }
                break;

            default:
                break;
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            app_db.ignore_bau_first_gaming_cmd = true;
#endif
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            if (LIGHT_SENSOR_ENABLED &&
                (!app_cfg_const.in_ear_auto_playing))
            {
                if (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    app_db.detect_suspend_by_out_ear = false;
                    app_bud_loc_update_in_ear_recover_a2dp(false);
                }
            }

            if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off &&
                 (app_bt_policy_get_b2s_connected_num() != 0) &&
                 (REMOTE_SESSION_ROLE_SECONDARY != app_cfg_nv.bud_role)))
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_SOURCE_LINK);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {

        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                (app_teams_multilink_get_voice_status() == BT_HFP_CALL_IDLE))
#else
            if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
#endif
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_SOURCE_LINK,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }
        }
        break;

#if F_APP_ERWS_SUPPORT
    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            if (param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_SUCCESS)
            {
                new_pri_apply_app_db_info_when_roleswap_suc();
            }
            else if ((param->remote_roleswap_status.status == BT_ROLESWAP_STATUS_START_RSP) &&
                     (param->remote_roleswap_status.u.start_rsp.accept))
            {
                app_relay_async_single(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_SYNC_DB);
            }
        }
        break;
#endif

    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            if (app_db.remote_is_8753bau && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
            {
                app_db.remote_is_8753bau = false;

                if (app_db.gaming_mode)
                {
                    app_mmi_switch_gaming_mode();
                }
            }
#endif
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_device_bt_cback: event_type 0x%04x", event_type);
    }
}

bool app_device_is_in_the_box(void)
{
    bool ret = false;

    if (app_cfg_const.charger_support)
    {
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (app_adp_get_plug_state() == ADAPTOR_PLUG)
            {
                ret = true;
            }
        }
    }
#if HARMAN_EXTERNAL_CHARGER_SUPPORT
    else
    {
        if (app_adp_get_plug_state() == ADAPTOR_PLUG)
        {
            ret = true;
        }
    }
#endif

#if F_APP_GPIO_ONOFF_SUPPORT
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        if ((app_cfg_const.enable_rtk_charging_box && (app_adp_get_plug_state() == ADAPTOR_PLUG)) ||
            (!app_cfg_const.enable_rtk_charging_box && (gpio_detect_onoff_get_location() == IN_CASE)))
        {
            ret = true;
        }
    }
#endif
    APP_PRINT_INFO1("app_device_is_in_the_box: ret = %d", ret);

    return ret;
}

bool app_device_is_in_ear(uint8_t loc)
{
    bool in_ear = false;
    if (LIGHT_SENSOR_ENABLED)
    {
        in_ear = (loc == BUD_LOC_IN_EAR) ? true : false;
    }
    else
    {
        in_ear = (loc != BUD_LOC_IN_CASE) ? true : false;
    }
    return in_ear;
}

uint8_t app_device_get_bud_channel(void)
{
    uint8_t bud_channel;

    // the single bud role key map data was stored at channel left
    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        bud_channel = CHANNEL_L;
    }
    else
    {
        if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
        {
            bud_channel = CHANNEL_L;
        }
        else
        {
            bud_channel = CHANNEL_R;
        }
    }

    return bud_channel;
}

void app_device_bt_policy_startup(bool at_once_trigger)
{
    app_bt_policy_startup(app_device_link_policy_ind, at_once_trigger);
}

void app_device_enter_dut_mode(void)
{
    app_bt_policy_disconnect_all_link();
    gap_start_timer(&timer_handle_dut_mode_wait_link_disc, "dut_mode_wait_link_disc",
                    device_timer_queue_id,
                    APP_DEVICE_TIMER_DUT_MODE_WAIT_LINK_DISC, 0, 2000);
}

void app_device_unlock_vbat_disallow_power_on(void)
{
    if (app_cfg_nv.disallow_power_on_when_vbat_in)
    {
        app_cfg_nv.disallow_power_on_when_vbat_in = 0;
        app_cfg_store(&app_cfg_nv.offset_listening_mode_cycle, 1);
    }
}

#if F_APP_ERWS_SUPPORT

static void pri_collect_app_db_info_for_roleswap(T_ROLESWAP_APP_DB *roleswap_app_db)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        roleswap_app_db->remote_is_8753bau = app_db.remote_is_8753bau;
    }
}

static void sec_recv_app_db_info_for_roleswap(void *buf, uint16_t len)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        if (sizeof(T_ROLESWAP_APP_DB) == len)
        {
            memcpy((void *)&roleswap_app_db_temp, buf, len);
        }
    }
}

static void new_pri_apply_app_db_info_when_roleswap_suc(void)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_db.remote_is_8753bau = roleswap_app_db_temp.remote_is_8753bau;
    }
}

uint16_t app_device_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    T_ROLESWAP_APP_DB roleswap_app_db;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_SYNC_DB:
        {
            pri_collect_app_db_info_for_roleswap(&roleswap_app_db);

            payload_len = sizeof(roleswap_app_db);
            msg_ptr = (uint8_t *)&roleswap_app_db;
        }
        break;

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_DEVICE, payload_len, msg_ptr, skip, total);
}

static void app_device_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                   T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_SYNC_DB:
        {
            sec_recv_app_db_info_for_roleswap(buf, len);
        }
        break;

    case APP_REMOTE_MSG_LINK_RECORD_ADD:
    case APP_REMOTE_MSG_LINK_RECORD_DEL:
    case APP_REMOTE_MSG_LINK_RECORD_XMIT:
    case APP_REMOTE_MSG_LINK_RECORD_PRIORITY_SET:
        if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
        {
            app_handle_remote_link_record_msg(msg_type, buf);
        }
        break;

    case APP_REMOTE_MSG_DEVICE_NAME_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                memset(&app_cfg_nv.device_name_legacy[0], 0, sizeof(app_cfg_nv.device_name_legacy));
                memcpy(&app_cfg_nv.device_name_legacy[0], (uint8_t *)buf, len);
                bt_local_name_set((uint8_t *)buf, len);
                app_cfg_store(app_cfg_nv.device_name_legacy, 40);
            }
        }
        break;

    case APP_REMOTE_MSG_LE_NAME_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                memset(&app_cfg_nv.device_name_le[0], 0, sizeof(app_cfg_nv.device_name_le));
                memcpy(&app_cfg_nv.device_name_le[0], (uint8_t *)buf, len);
                le_set_gap_param(GAP_PARAM_DEVICE_NAME, len, (uint8_t *)buf);
                app_cfg_store(app_cfg_nv.device_name_le, 40);
            }
        }
        break;

    case APP_REMOTE_MSG_POWERING_OFF:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_db.disallow_sniff = true;
            }
        }
        break;

    case APP_REMOTE_MSG_REMOTE_SPK2_PLAY_SYNC:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT) || (status == REMOTE_RELAY_STATUS_ASYNC_RCVD))
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (app_cfg_const.rws_connected_show_channel_vp &&
                        (app_cfg_const.couple_speaker_channel == RWS_SPK_CHANNEL_RIGHT))
                    {
                        app_audio_tone_type_play(TONE_REMOTE_ROLE_SECONDARY, false, false);
                    }
                    else
                    {
                        app_audio_tone_type_play(TONE_REMOTE_ROLE_PRIMARY, false, false);
                    }
                }
                else
                {
                    if (app_cfg_const.rws_connected_show_channel_vp &&
                        (app_cfg_const.couple_speaker_channel == RWS_SPK_CHANNEL_LEFT))
                    {
                        app_audio_tone_type_play(TONE_REMOTE_ROLE_PRIMARY, false, false);
                    }
                    else
                    {
                        app_audio_tone_type_play(TONE_REMOTE_ROLE_SECONDARY, false, false);
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_SPK1_REPLY_SRC_IS_IOS:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t *p_info = (uint8_t *)buf;
                    T_APP_BR_LINK *p_link;
                    uint8_t bd_addr[6];

                    memcpy(bd_addr, &p_info[1], 6);

                    p_link = app_find_br_link(bd_addr);

                    if (p_link != NULL)
                    {
                        p_link->remote_device_vendor_id = (T_APP_REMOTE_DEVICE_VEDDOR_ID)p_info[0];
                        APP_PRINT_TRACE2("SPK2 update ios flag: %s (%d)", TRACE_BDADDR(bd_addr),
                                         p_link->remote_device_vendor_id);
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_SET_LPS_SYNC:
        {
            uint8_t dlps_stay_mode = *(uint8_t *)buf;

            APP_PRINT_INFO1("CMD_OTA_TOOLING_PARKING %d", dlps_stay_mode);

            app_cfg_nv.need_set_lps_mode = 1;

            if (dlps_stay_mode)
            {
                // shipping mode
                power_mode_set(POWER_POWEROFF_MODE);
                app_cfg_nv.ota_parking_lps_mode = dlps_stay_mode;
            }
            else
            {
                // power down mode
                power_mode_set(POWER_POWERDOWN_MODE);
                app_cfg_nv.ota_parking_lps_mode = dlps_stay_mode;
            }

            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        }
        break;

    case APP_REMOTE_MSG_SYNC_IO_PIN_PULL_HIGH:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t *data = (uint8_t *)buf;
                uint8_t pin_num = data[0];

                Pad_Config(pin_num, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            }
        }
        break;

    default:
        break;
    }
}
#endif

#if F_APP_LOSS_BATTERY_PROTECT
void app_device_loss_battery_gpio_detect_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    fmc_flash_nor_power_loss_protection(PSRAM_IDX_SPIC1);
}

//GPIO interrupt handler function
ISR_TEXT_SECTION
void app_device_loss_battery_gpio_detect_intr_handler(uint32_t param)
{
    T_IO_MSG gpio_msg;
    uint8_t pinmux = LOSS_BATTERY_IO_DETECT_PIN;

    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    /* Disable GPIO interrupt */
    gpio_irq_disable(pinmux);

    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_LOSS_BATTERY_IO_DETECT;

    app_io_send_msg(&gpio_msg);
}

void app_device_loss_battery_gpio_driver_init(void)
{
    gpio_init_pin(LOSS_BATTERY_IO_DETECT_PIN, GPIO_TYPE_AUTO, GPIO_DIR_INPUT);
    gpio_set_up_irq(LOSS_BATTERY_IO_DETECT_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW,
                    true, app_device_loss_battery_gpio_detect_intr_handler, 0); //Polarity Low
    //enable int
    gpio_irq_enable(LOSS_BATTERY_IO_DETECT_PIN);
}
#endif

void app_device_init(void)
{
	//app_cfg_nv.ntc_poweroff_wakeup_flag = false;
    ble_mgr_msg_cback_register(app_device_ble_cback);
    sys_mgr_cback_register(app_device_dm_cback);
    bt_mgr_cback_register(app_device_bt_cback);
    gap_reg_timer_cb(app_device_timer_callback, &device_timer_queue_id);
#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_device_relay_cback, app_device_parse_cback,
                             APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_DEVICE_TOTAL);
#endif
}
