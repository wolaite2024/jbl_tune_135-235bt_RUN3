/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include "trace.h"
#include "os_sched.h"
#include "test_mode.h"
#include "gap_timer.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hid.h"
#include "bt_bond.h"
#include "system_status_api.h"
#include "sysm.h"
#include "eq.h"
#if F_APP_APT_SUPPORT
#include "audio_passthrough.h"
#endif
#include "audio.h"
#include "audio_volume.h"
#include "audio_track.h"
#if F_APP_VAD_SUPPORT
#include "vad.h"
#endif
#include "remote.h"
#include "platform_utils.h"
#include "eq_utils.h"
#include "app_mmi.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cmd.h"
#include "app_multilink.h"
#include "app_hfp.h"
#include "app_key_process.h"
#include "app_led.h"
#include "app_audio_policy.h"
#include "app_link_util.h"
#include "app_ble_common_adv.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_dlps.h"
#include "app_wdg.h"
#include "pm.h"
#include "ringtone.h"
#include "app_wdg.h"

#if F_APP_NFC_SUPPORT
#include "app_nfc.h"
#endif

#if F_APP_LOCAL_PLAYBACK_SUPPORT
#include "app_playback.h"
#include "app_playback_update_file.h"
#include "audio_fs.h"
#if TARGET_RTL8773CO
#include "app_listen_save.h"
#endif
#endif

#include "app_adp.h"
#include "app_ble_gap.h"

#if BISTO_FEATURE_SUPPORT
#include "app_bisto_key.h"
#endif

#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#include "app_eq.h"
#include "app_bt_sniffing.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "app_relay.h"
#include "app_bond.h"
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif

#if AMA_FEATURE_SUPPORT
#include "ama_api.h"
#include "app_ama_accessory.h"
#include "app_ama_device.h"
#endif
#if F_APP_NFC_SUPPORT
#include "app_nfc.h"
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xiaoai_device.h"
#endif

#include "app_roleswap.h"
#include "app_roleswap_control.h"
#if F_APP_LINEIN_SUPPORT
#include "line_in.h"
#endif
#if F_APP_IO_OUTPUT_SUPPORT
#include "app_io_output.h"
#endif
#include "app_line_in.h"

#include "app_device.h"
#include "app_sniff_mode.h"
#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif
#include "eq.h"
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#if (F_APP_DUAL_AUDIO_EFFECT == 1)
#include "app_dual_audio_effect.h"
#endif
#include "app_sensor.h"
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_teams_cmd.h"
#include "app_asp_device.h"
#include "asp_device_api.h"
#include "asp_device_link.h"
#include "app_teams_bt_policy.h"
#include "app_teams_extend_led.h"
#include "app_teams_audio_policy.h"
#endif
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#endif
#include "single_tone.h"
#include "gap_lib_common.h"
#include "app_auto_power_off.h"
#include "gap_ext_scan.h"

#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "app_xiaowei_device.h"
#include "app_xiaowei.h"
#endif

#if F_APP_IPHONE_ABS_VOL_HANDLE
#include "app_iphone_abs_vol_handle.h"
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_dongle_record.h"
#endif
#if F_APP_IPHONE_ABS_VOL_HANDLE
#include "app_iphone_abs_vol_handle.h"
#endif

#include "hal_debug.h"

#if F_APP_BRIGHTNESS_SUPPORT
#include "app_audio_passthrough_brightness.h"
#endif

#if F_APP_MCP_SUPPORT
#include "app_le_audio_mgr.h"
#endif

#if F_APP_LE_AUDIO_SM
#include "app_le_audio_mgr.h"
#endif
#if F_APP_TMAP_BMR_SUPPORT
#include "app_broadcast_sync.h"
#include "app_le_audio_scan.h"
#endif

#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif

#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#include "app_usb_mmi.h"
#endif

#if (F_APP_HEARABLE_SUPPORT == 1)
#include "app_hearable.h"
#endif

#if F_APP_AMP_SUPPORT
#include "app_amp.h"
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
#include "app_harman_adc.h"
#endif

#if HARMAN_NTC_DETECT_PROTECT
#include "app_harman_adc.h"
#endif

#if GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#include "app_gfps_finder_adv.h"
#if GFPS_FINDER_DULT_SUPPORT
#include "app_dult_device.h"
#endif
#endif

#define A2DP_MUTE_WHEN_PAUSE_TIMER_MS  2000

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_mmi_voice_recognition_enable_hook)(void) = NULL;
void (*app_mmi_voice_recognition_disable_hook)(void) = NULL;
void (*app_mmi_av_play_pause_hook)(void) = NULL;
void (*app_mmi_av_fwd_hook)(void) = NULL;
void (*app_mmi_av_bwd_hook)(void) = NULL;
void (*app_mmi_listening_cycle_hook)(void) = NULL;
void (*app_mmi_enter_pairing_hook)(void) = NULL;
void (*app_anc_apt_tone_check_hook)(bool *) = NULL;
#endif

extern void *timer_handle_hfp_ring;
static uint8_t app_mmi_timer_queue_id = 0;
static void *timer_handle_app_mmi_reboot_check = NULL;
static uint8_t reboot_check_times = REBOOT_CHECK_MAX_TIMES;

typedef enum
{
    APP_TIMER_REBOOT_CHECK,
} T_APP_MMI_TIMER;

bool app_mmi_reboot_check_timer_started(void)
{
    return ((timer_handle_app_mmi_reboot_check != NULL) ? true : false);
}

static void app_mmi_power_off(void)
{
    if (app_db.waiting_factory_reset ||
        /* nonsmart box in case to power off */
        (app_cfg_const.enable_rtk_charging_box == false && app_db.local_loc == BUD_LOC_IN_CASE))
    {
        sys_mgr_power_off();
    }
    else
    {
        app_audio_tone_flush(false);
        app_audio_tone_type_stop();
#if F_APP_HARMAN_FEATURE_SUPPORT
        if (app_cfg_nv.language_status == 0)
        {
            app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_40, false, false);
        }
        else
#endif
        {
            app_audio_tone_type_play(TONE_POWER_OFF, false, false);
        }
        led_timer_stop();
        led_change_mode(LED_MODE_POWER_OFF, true, false);
    }
}

static void app_mmi_spk_mute_set(uint8_t action)
{
    uint8_t active_a2dp_idx = app_hfp_get_active_hf_index();
    uint8_t active_hf_idx = app_hfp_get_active_hf_index();
    uint8_t pair_idx_mapping;
    uint8_t cur_volume = 0;
    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_INFO5("app_mmi_spk_mute_set: bud_role %d, call_status %d, cur_volume %d, idx %d, %d",
                    app_cfg_nv.bud_role, app_hfp_get_call_status(), cur_volume, active_a2dp_idx, active_hf_idx);

    if (action == MMI_DEV_SPK_MUTE)
    {
        if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
        {
            app_bond_get_pair_idx_mapping(app_db.br_link[active_hf_idx].bd_addr, &pair_idx_mapping);

            p_link = app_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
            if (p_link != NULL)
            {
                if (!p_link->voice_muted)
                {
                    p_link->voice_muted = true;
                    audio_track_volume_out_mute(p_link->sco_track_handle);
                }
            }
        }
        else
        {
            if (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                p_link = &app_db.br_link[active_a2dp_idx];
                if (p_link != NULL)
                {
                    if (!p_link->playback_muted)
                    {
#if F_APP_IPHONE_ABS_VOL_HANDLE
                        if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                            (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true))
                        {
                            app_bond_get_pair_idx_mapping(p_link->bd_addr, &pair_idx_mapping);
                            cur_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];

                            if ((cur_volume == 0) && (app_cfg_nv.abs_vol[pair_idx_mapping] != 0))
                            {
                                // Current a2dp level is fake 0. It's need to set gain value of fake lv 0 when unmute.
                                p_link->set_a2dp_fake_lv0_gain = true;
                            }
                        }
#endif
                        p_link->playback_muted = true;
                        audio_track_volume_out_mute(p_link->a2dp_track_handle);
                    }
                }
            }
        }
    }
    else
    {
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
        {
            if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) &&
                (app_hfp_get_call_status() == BT_HFP_INCOMING_CALL_ONGOING))
            {
                //do nothing
            }
            else
            {
                if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
                {
                    p_link = app_find_br_link(app_db.br_link[active_hf_idx].bd_addr);
                    if (p_link != NULL)
                    {
                        app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
                    }
                }
                else
                {
                    p_link = app_find_br_link(app_db.br_link[active_a2dp_idx].bd_addr);
                    if (p_link != NULL)
                    {
                        if (p_link->playback_muted)
                        {
#if F_APP_IPHONE_ABS_VOL_HANDLE
                            if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                                (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true))
                            {
                                if (p_link->set_a2dp_fake_lv0_gain)
                                {
                                    //It's need to setup gain value of fake level 0.
                                    app_audio_route_dac_gain_set(AUDIO_CATEGORY_AUDIO, 0, DAC_GAIN_DB_NEGATIVE_65_DB);
                                    p_link->set_a2dp_fake_lv0_gain = false;
                                }
                            }
#endif
                            app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                        }
                    }
                }
            }
        }
        else
        {
#if F_APP_ERWS_SUPPORT
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                uint8_t cmd_ptr[9] = {0};
                uint8_t msg_type;
                if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
                {
                    app_bond_get_pair_idx_mapping(app_db.br_link[active_hf_idx].bd_addr, &pair_idx_mapping);
                    cur_volume = app_cfg_nv.voice_gain_level[pair_idx_mapping];

                    memcpy(&cmd_ptr[0], app_db.br_link[active_hf_idx].bd_addr, 6);
                    cmd_ptr[6] = AUDIO_STREAM_TYPE_VOICE;
                    msg_type = APP_REMOTE_MSG_HFP_VOLUME_SYNC;
                }
                else
                {
                    app_bond_get_pair_idx_mapping(app_db.br_link[active_a2dp_idx].bd_addr, &pair_idx_mapping);
                    cur_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];

                    memcpy(&cmd_ptr[0], app_db.br_link[active_a2dp_idx].bd_addr, 6);
                    cmd_ptr[6] = AUDIO_STREAM_TYPE_PLAYBACK;
                    msg_type = APP_REMOTE_MSG_A2DP_VOLUME_SYNC;
                }
                cmd_ptr[7] = cur_volume;
                cmd_ptr[8] = 0;
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, msg_type,
                                                   cmd_ptr, 9, REMOTE_TIMER_HIGH_PRECISION, 0, false);
            }
#endif
        }
    }

}

void app_mmi_reboot_check_timer_start(uint32_t period_timer)
{
    gap_start_timer(&timer_handle_app_mmi_reboot_check, "reboot_check", app_mmi_timer_queue_id,
                    APP_TIMER_REBOOT_CHECK, 0, period_timer);
}

static void app_mmi_reboot(void)
{
    if (app_db.waiting_factory_reset)
    {
        app_device_factory_reset();
        app_db.waiting_factory_reset = false;
    }

    if (app_db.ota_tooling_start)
    {
        app_adp_cmd_ota_tooling_nv_store();

        /* ensure reboot after factory reset or power off */
        app_cfg_const.disable_power_off_wdt_reset = 0;
    }

    if (app_cfg_const.disable_power_off_wdt_reset)
    {
        // power down mode
        app_device_state_change(APP_DEVICE_STATE_OFF);
        app_dlps_enable(APP_DLPS_ENTER_WAIT_RESET);
        app_dlps_power_off();

    }
    else
    {
        //The power off reboot triggered during the process of put bud out box should be able to power on
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (app_adp_unplug_debounce_timer_started())
            {
                app_cfg_nv.adaptor_is_plugged = 1;
            }
        }
#if F_APP_GPIO_ONOFF_SUPPORT
        else if ((app_cfg_const.box_detect_method == GPIO_DETECT) && !app_device_is_in_the_box() &&
                 app_gpio_detect_onoff_debounce_timer_started())
        {
            app_cfg_nv.pre_3pin_status_unplug = 0;
        }
#endif

        app_cfg_nv.is_app_reboot = 1;
        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

        os_delay(20);
        app_wdg_reset(RESET_ALL_EXCEPT_AON);
    }
}

void app_mmi_modify_reboot_check_times(uint32_t times)
{
    if (reboot_check_times > times)
    {
        reboot_check_times = times;
    }
}

static void app_mmi_reboot_check(void)
{
    gap_stop_timer(&timer_handle_app_mmi_reboot_check);

    if (reboot_check_times)
    {
        reboot_check_times--;
    }

    if (app_key_long_key_power_off_pressed_get() &&
        (app_db.power_off_cause == POWER_OFF_CAUSE_LONG_KEY ||
         app_db.power_off_cause == POWER_OFF_CAUSE_SHORT_KEY))
    {
        app_mmi_reboot_check_timer_start(100);
        return;
    }

    if (!reboot_check_times)
    {
        app_mmi_reboot();
    }
    else
    {
        if (ringtone_remaining_count_get()
            || voice_prompt_remaining_count_get()
            || app_bt_policy_get_b2b_connected()
            || app_bt_policy_get_b2s_connected_num()
            || led_get_led_stop_check_state()
#if F_APP_AMP_SUPPORT
            || !app_amp_is_off_state()
#endif
           )
        {
            app_mmi_reboot_check_timer_start(100);
        }
        else
        {
            app_mmi_reboot();
        }
    }
}

static void app_mmi_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_mmi_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
    case APP_TIMER_REBOOT_CHECK:
        {
            if (timer_handle_app_mmi_reboot_check != NULL)
            {
                app_mmi_reboot_check();
            }
        }
        break;

    default:
        break;
    }
}

uint16_t app_mmi_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    // uint16_t payload_len = 0;
    // uint8_t *msg_ptr = NULL;
    // bool skip = true;

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_MMI, 0, NULL, true, total);
}

static void app_mmi_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                T_REMOTE_RELAY_STATUS status)
{
    if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
        status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
        status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
        status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
    {
        uint8_t action = msg_type;

        if (action == MMI_DEV_POWER_OFF)
        {
            if (app_db.power_off_cause == POWER_OFF_CAUSE_UNKNOWN)
            {
                app_db.power_off_cause = POWER_OFF_CAUSE_RELAY;
            }
        }

        app_mmi_handle_action(action);
    }
}

void app_mmi_init(void)
{
    gap_reg_timer_cb(app_mmi_timeout_cb, &app_mmi_timer_queue_id);
    app_relay_cback_register(app_mmi_relay_cback, app_mmi_parse_cback,
                             APP_MODULE_TYPE_MMI, MMI_TOTAL);
}

#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsf for avp
#else
static void mmi_voice_recognition(void)
{
    uint8_t app_idx;
#if F_APP_MUTILINK_TRIGGER_HIGH_PRIORITY
    if (app_cfg_const.enable_multi_link)
    {
        app_idx = app_get_highest_priority_index(HFP_PROFILE_MASK);
    }
    else
#endif
    {
#if F_APP_HARMAN_FEATURE_SUPPORT
        uint8_t record_active_index = 0xff;
        uint8_t i, hfhs_profile_link_number = 0;

        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            APP_PRINT_INFO3("AU_MMI_INITIATE_VOICE_DIAL1 DUMP_INDEX: %d, addr: %s, play_state: %d", i,
                            TRACE_BDADDR(app_db.br_link[i].bd_addr),
                            app_db.br_link[i].avrcp_play_status);

            if ((app_check_b2s_link_by_id(i)) &&
                (app_db.br_link[i].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK)))
            {
                hfhs_profile_link_number++;
                //if (i == app_get_active_a2dp_idx())
                if ((i == au_get_harman_aling_active_idx()) &&
                    (au_get_harman_aling_active_idx() != 0xff))
                {
                    record_active_index = i;
                }
            }
        }
        APP_PRINT_TRACE7("AU_MMI_INITIATE_VOICE_DIAL:%d,%d,%d,%d,%d,%d,%d",
                         app_db.last_hf_index,
                         app_db.br_link[app_get_active_a2dp_idx()].avrcp_play_status,
                         app_get_active_a2dp_idx(),
                         app_hfp_sco_is_connected(),
                         app_hfp_get_active_hf_index(),
                         hfhs_profile_link_number,
                         record_active_index);

        if (hfhs_profile_link_number == 0)
        {
            if (record_active_index == 0xff)
            {
                APP_PRINT_TRACE1("VOICE_DIAL INDEX NO SUPPORT HF1:%d", record_active_index);
                return;
            }
        }
        else if (hfhs_profile_link_number == 1)
        {
            if (record_active_index == 0xff)
            {
                APP_PRINT_TRACE1("VOICE_DIAL INDEX NO SUPPORT HF2:%d", record_active_index);
                return;
            }
            else
            {
                app_idx = record_active_index;
            }
        }
        else if (hfhs_profile_link_number == 2)
        {
            app_idx = record_active_index;
        }

        APP_PRINT_TRACE1("AU_MMI_INITIATE_VOICE_DIAL FINAL_INDEX:%d", app_idx);
#else
        app_idx = app_hfp_get_active_hf_index();
#endif
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    if (app_db.br_link[app_idx].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_hfp_voice_recognition_enable_req(app_db.br_link[app_idx].bd_addr);

        app_audio_tone_type_play(TONE_HF_CALL_VOICE_DIAL, false, true);
    }
    else
    {
        uint8_t bd_addr[6];

        if (app_b2s_bond_addr_get(1, bd_addr) == true)
        {
            app_bt_policy_default_connect(bd_addr,
                                          HSP_PROFILE_MASK | HFP_PROFILE_MASK, true);
        }
    }
}
#endif

static void app_volume_max_min_tone_play_status_sync(void)
{
    app_key_set_volume_status(false);
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
#if F_APP_ERWS_SUPPORT
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_VOL_IS_CHANGED_BY_KEY);
#endif
    }
}

static void app_volume_up()
{
    uint8_t max_volume = 0;
    uint8_t curr_volume = 0;
    uint8_t active_idx;
    uint8_t pair_idx_mapping;
    T_AUDIO_STREAM_TYPE volume_type;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_key_set_volume_status(false);
        return;
    }

    APP_PRINT_INFO1("app_volume_up: avrcp_play_status: %d",
                    app_db.br_link[app_get_active_a2dp_idx()].avrcp_play_status);

    if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
        (app_db.br_link[app_get_active_a2dp_idx()].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING) &&
        (app_audio_get_bud_stream_state() != BUD_STREAM_STATE_AUDIO))
    {
        app_key_set_volume_status(false);

        if (app_cfg_const.disallow_adjust_volume_when_idle)
        {
            return;
        }
    }

#if F_APP_LOCAL_PLAYBACK_SUPPORT
    if (app_db.sd_playback_switch)
    {
        app_playback_volume_up();
        return ;
    }
#endif

#if F_APP_LINEIN_SUPPORT
    if (app_line_in_playing_state_get())
    {
        app_line_in_volume_up_handle();
        return;
    }
#endif

#if F_APP_TEAMS_BT_POLICY
    if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE) ||
        (app_find_sco_conn_num() != 0))
#else
    if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE))
#endif
    {
        volume_type = AUDIO_STREAM_TYPE_VOICE;
        active_idx = app_hfp_get_active_hf_index();

        if (app_db.br_link[active_idx].call_status == BT_HFP_INCOMING_CALL_ONGOING &&
            app_db.br_link[active_idx].is_inband_ring == false)
        {
            APP_PRINT_INFO0("app_volume_up: do not change vol when outband ringtone");

            app_volume_max_min_tone_play_status_sync();
            return;
        }

        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_up: find active hfp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.voice_gain_level[pair_idx_mapping];
        max_volume = app_cfg_const.voice_out_volume_max;
    }
    else
    {
        volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
        active_idx = app_get_active_a2dp_idx();
        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_up: find active a2dp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];
        max_volume = app_cfg_const.playback_volume_max;
    }

    if (curr_volume < max_volume)
    {
        curr_volume++;
    }
    else
    {
        if (app_is_need_to_enable_circular_volume_up())
        {
            curr_volume = 0;
            app_db.is_circular_vol_up = true;
        }
        else
        {
            curr_volume = max_volume;
        }
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        uint8_t cmd_ptr[9];
        memcpy(&cmd_ptr[0], app_db.br_link[active_idx].bd_addr, 6);
        cmd_ptr[6] = volume_type;
        cmd_ptr[7] = curr_volume;
        cmd_ptr[8] = 0;

        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

#if F_APP_IPHONE_ABS_VOL_HANDLE
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);

            if (p_link != NULL)
            {
                if (((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                     (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true)))
                {
                    if (curr_volume == 1 && app_cfg_nv.abs_vol[pair_idx_mapping] == 0)
                    {
                        //up to fake level 0 ( 0 -> 1).
                        curr_volume = 0;

                        //need to update sync information.
                        cmd_ptr[7] = curr_volume;
                    }

                    level = app_cfg_nv.abs_vol[pair_idx_mapping] = iphone_abs_vol[curr_volume];

                    //if enable circular vol up, set all value to 0.
                    if (app_db.is_circular_vol_up && curr_volume == 0)
                    {
                        level = app_cfg_nv.abs_vol[pair_idx_mapping] = 0;
                    }

                    app_iphone_abs_vol_sync_abs_vol(app_db.br_link[active_idx].bd_addr,
                                                    app_cfg_nv.abs_vol[pair_idx_mapping]);
                }
                else
#endif
                {
                    level = (curr_volume * 0x7F + app_cfg_const.playback_volume_max / 2) /
                            app_cfg_const.playback_volume_max;
                }

                app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_VOLUME_SYNC,
                                                   cmd_ptr, 9, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);
#if F_APP_IPHONE_ABS_VOL_HANDLE
            }
#endif
        }

        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_cfg_const.voice_out_volume_max / 2) /
                            app_cfg_const.voice_out_volume_max;

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;

            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_HFP_VOLUME_SYNC,
                                               cmd_ptr, 9, REMOTE_TIMER_HIGH_PRECISION, 0, false);

            bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
        }
    }
    else
    {
        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

#if F_APP_IPHONE_ABS_VOL_HANDLE
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);

            if (p_link != NULL)
            {
                if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                    (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true))
                {
                    if (curr_volume == 1 && app_cfg_nv.abs_vol[pair_idx_mapping] == 0)
                    {
                        //up to fake level 0 ( 0 -> 1).
                        curr_volume = 0;
                    }

                    level = app_cfg_nv.abs_vol[pair_idx_mapping] = iphone_abs_vol[curr_volume];

                    //if enable circular vol up, set all value to 0.
                    if (app_db.is_circular_vol_up && curr_volume == 0)
                    {
                        level = app_cfg_nv.abs_vol[pair_idx_mapping] = 0;
                    }
                }
                else
#endif
                {
                    level = (curr_volume * 0x7F + app_cfg_const.playback_volume_max / 2) /
                            app_cfg_const.playback_volume_max;
                }

                if ((!app_cfg_const.enable_rtk_charging_box) || (app_db.local_in_case == false))
                {
#if F_APP_IPHONE_ABS_VOL_HANDLE
                    app_iphone_abs_vol_wrap_audio_track_volume_out_set(app_db.br_link[active_idx].a2dp_track_handle,
                                                                       curr_volume,
                                                                       app_cfg_nv.abs_vol[pair_idx_mapping]);
#else
                    audio_track_volume_out_set(app_db.br_link[active_idx].a2dp_track_handle, curr_volume);
#endif
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                APP_PRINT_TRACE4("bt_avrcp_volume_change_req curr_vol=%d,idx=%d,stream=%d,avrcp=%d",
                                 curr_volume,
                                 active_idx,
                                 app_db.br_link[active_idx].streaming_fg,
                                 app_db.br_link[active_idx].avrcp_play_status);

                if (app_db.br_link[active_idx].streaming_fg == false)
                {
                    harman_push_max_tone_when_not_playing(AUDIO_STREAM_TYPE_PLAYBACK,
                                                          app_cfg_nv.audio_gain_level[pair_idx_mapping],
                                                          curr_volume);
                }
#endif
#if HARMAN_OPEN_LR_FEATURE
                app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, curr_volume, __func__, __LINE__);
#endif
                app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;
                bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);

#if F_APP_IPHONE_ABS_VOL_HANDLE
            }
#endif

        }
        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_cfg_const.voice_out_volume_max /
                             2) / app_cfg_const.voice_out_volume_max;
#if HARMAN_OPEN_LR_FEATURE
            app_harman_lr_balance_set(AUDIO_STREAM_TYPE_VOICE, curr_volume, __func__, __LINE__);
#endif
            if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) &&
                (app_hfp_get_call_status() == BT_HFP_INCOMING_CALL_ONGOING))
            {
                app_key_set_volume_status(false);
                app_hfp_inband_tone_mute_ctrl();
            }
            else if ((!app_cfg_const.enable_rtk_charging_box) || (app_db.local_in_case == false))
            {
                audio_track_volume_out_set(app_db.br_link[active_idx].sco_track_handle, curr_volume);

                app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
            }

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;
            bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
        }
    }
}

static void app_volume_down()
{
    bool already_vgs0 = false;
    uint8_t min_volume = 0;
    uint8_t curr_volume = 0;
    uint8_t active_idx;
    uint8_t pair_idx_mapping;
    T_AUDIO_STREAM_TYPE volume_type;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_key_set_volume_status(false);
        return;
    }

    if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
        (app_db.br_link[app_get_active_a2dp_idx()].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING) &&
        (app_audio_get_bud_stream_state() != BUD_STREAM_STATE_AUDIO))
    {
        app_key_set_volume_status(false);

        if (app_cfg_const.disallow_adjust_volume_when_idle)
        {
            return;
        }
    }

#if F_APP_LOCAL_PLAYBACK_SUPPORT
    if (app_db.sd_playback_switch)
    {
        app_playback_volume_down();
        return ;
    }
#endif

#if F_APP_LINEIN_SUPPORT
    if (app_line_in_playing_state_get())
    {
        app_line_in_volume_down_handle();
        return;
    }
#endif

#if F_APP_TEAMS_BT_POLICY
    if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE) ||
        (app_find_sco_conn_num() != 0))
#else
    if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE))
#endif
    {
        volume_type = AUDIO_STREAM_TYPE_VOICE;
        active_idx = app_hfp_get_active_hf_index();

        if (app_db.br_link[active_idx].call_status == BT_HFP_INCOMING_CALL_ONGOING &&
            app_db.br_link[active_idx].is_inband_ring == false)
        {
            APP_PRINT_INFO0("app_volume_down: do not change vol when outband ringtone");

            app_volume_max_min_tone_play_status_sync();
            return;
        }

        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_down: find active hfp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.voice_gain_level[pair_idx_mapping];
        min_volume = app_cfg_const.voice_out_volume_min;
    }
    else
    {
        volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
        active_idx = app_get_active_a2dp_idx();
        if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
        {
            APP_PRINT_ERROR0("app_volume_down: find active a2dp pair_index fail");

            app_volume_max_min_tone_play_status_sync();
            return;
        }
        curr_volume = app_cfg_nv.audio_gain_level[pair_idx_mapping];
        min_volume = app_cfg_const.playback_volume_min;
    }

    if (curr_volume > min_volume)
    {
        curr_volume--;
    }
    else
    {
        curr_volume = min_volume;

        /*if ios version is 13, send AT+VGS=0 repeatedly will make voice mute*/
        if (curr_volume == 0)
        {
            already_vgs0 = true;
        }
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        uint8_t cmd_ptr[9];
        memcpy(&cmd_ptr[0], app_db.br_link[active_idx].bd_addr, 6);
        cmd_ptr[6] = volume_type;
        cmd_ptr[7] = curr_volume;
        cmd_ptr[8] = 0;

        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

#if F_APP_IPHONE_ABS_VOL_HANDLE
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);

            if (p_link != NULL)
            {
                if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                    (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true))
                {
                    level = app_cfg_nv.abs_vol[pair_idx_mapping] = iphone_abs_vol[curr_volume];

                    if ((app_cfg_nv.audio_gain_level[pair_idx_mapping] == 0) && (curr_volume == 0))
                    {
                        //down to real level 0 (0 -> 0).
                        level = app_cfg_nv.abs_vol[pair_idx_mapping] = 0;
                    }

                    app_iphone_abs_vol_sync_abs_vol(app_db.br_link[active_idx].bd_addr,
                                                    app_cfg_nv.abs_vol[pair_idx_mapping]);
                }
                else
#endif
                {
                    level = (curr_volume * 0x7F + app_cfg_const.playback_volume_max / 2) /
                            app_cfg_const.playback_volume_max;
                }

                app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_A2DP_VOLUME_SYNC,
                                                   cmd_ptr, 9, REMOTE_TIMER_HIGH_PRECISION, 0, false);

                bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);

#if F_APP_IPHONE_ABS_VOL_HANDLE
            }
#endif
        }
        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_cfg_const.voice_out_volume_max / 2) /
                            app_cfg_const.voice_out_volume_max;

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;

            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_HFP_VOLUME_SYNC,
                                               cmd_ptr, 9, REMOTE_TIMER_HIGH_PRECISION, 0, false);

            if (already_vgs0 == false)
            {
                bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
            }
        }
    }
    else
    {
        if (volume_type == AUDIO_STREAM_TYPE_PLAYBACK)
        {
            uint8_t level;

#if F_APP_IPHONE_ABS_VOL_HANDLE
            T_APP_BR_LINK *p_link = NULL;
            p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);

            if (p_link != NULL)
            {
                if ((p_link->remote_device_vendor_id == APP_REMOTE_DEVICE_IOS) &&
                    (app_iphone_abs_vol_check_a2dp_total_gain_num_16() == true))
                {
                    level = app_cfg_nv.abs_vol[pair_idx_mapping] = iphone_abs_vol[curr_volume];

                    if ((app_cfg_nv.audio_gain_level[pair_idx_mapping] == 0) && (curr_volume == 0))
                    {
                        //down to real level 0 (0 -> 0).
                        level = app_cfg_nv.abs_vol[pair_idx_mapping] = 0;
                    }
                }
                else
#endif
                {
                    level = (curr_volume * 0x7F + app_cfg_const.playback_volume_max / 2) /
                            app_cfg_const.playback_volume_max;
                }

                if ((!app_cfg_const.enable_rtk_charging_box) || (app_db.local_in_case == false))
                {
#if F_APP_IPHONE_ABS_VOL_HANDLE
                    app_iphone_abs_vol_wrap_audio_track_volume_out_set(app_db.br_link[active_idx].a2dp_track_handle,
                                                                       curr_volume,
                                                                       app_cfg_nv.abs_vol[pair_idx_mapping]);
#else
                    audio_track_volume_out_set(app_db.br_link[active_idx].a2dp_track_handle, curr_volume);
#endif
                    app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
                }
#if HARMAN_OPEN_LR_FEATURE
                app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK, curr_volume, __func__, __LINE__);
#endif
                bt_avrcp_volume_change_req(app_db.br_link[active_idx].bd_addr, level);
                app_cfg_nv.audio_gain_level[pair_idx_mapping] = curr_volume;

#if F_APP_IPHONE_ABS_VOL_HANDLE
            }
#endif
        }
        else if (volume_type == AUDIO_STREAM_TYPE_VOICE)
        {
            uint8_t level = (curr_volume * 0x0F + app_cfg_const.voice_out_volume_max /
                             2) / app_cfg_const.voice_out_volume_max;
#if HARMAN_OPEN_LR_FEATURE
            app_harman_lr_balance_set(AUDIO_STREAM_TYPE_VOICE, curr_volume, __func__, __LINE__);
#endif
            if ((app_cfg_const.always_play_hf_incoming_tone_when_incoming_call) &&
                (app_hfp_get_call_status() == BT_HFP_INCOMING_CALL_ONGOING))
            {
                app_key_set_volume_status(false);
                app_hfp_inband_tone_mute_ctrl();
            }
            else if ((!app_cfg_const.enable_rtk_charging_box) || (app_db.local_in_case == false))
            {
                audio_track_volume_out_set(app_db.br_link[active_idx].sco_track_handle, curr_volume);

                app_audio_track_spk_unmute(AUDIO_STREAM_TYPE_VOICE);
            }

            if (already_vgs0 == false)
            {
                bt_hfp_speaker_gain_level_report(app_db.br_link[active_idx].bd_addr, level);
            }

            app_cfg_nv.voice_gain_level[pair_idx_mapping] = curr_volume;
        }
    }
}

void app_mmi_switch_gaming_mode()
{
    APP_PRINT_TRACE3("app_mmi_switch_gaming_mode: old status %d, b2b state %d, disallow tone %d",
                     app_db.gaming_mode,
                     app_db.remote_session_state, app_db.disallow_play_gaming_mode_vp);
    T_AUDIO_STREAM_USAGE usage;
    T_AUDIO_STREAM_MODE mode;
    uint8_t vol;
    bool vol_muted;
    T_AUDIO_FORMAT_INFO format;
    T_APP_BR_LINK *p_link = NULL;
#if F_APP_HARMAN_FEATURE_SUPPORT
    uint16_t latency_value = app_cfg_nv.cmd_latency_in_ms;
#else
    uint16_t latency_value = A2DP_LATENCY_MS;
#endif

    if (app_db.gaming_mode)
    {
        app_db.gaming_mode = false;

        if (app_db.spk_eq_mode == GAMING_MODE)
        {
            app_cfg_nv.eq_idx_gaming_mode_record = app_cfg_nv.eq_idx;
        }

#if F_APP_LISTENING_MODE_SUPPORT
        if (app_cfg_const.disallow_anc_apt_when_gaming_mode)
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_GAMING_MODE_END);
        }
#endif


        if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
             app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            && (app_db.disallow_play_gaming_mode_vp == false)
#endif
           )
        {
            app_audio_tone_type_play(TONE_EXIT_GAMING_MODE, false, true);
        }
    }
    else
    {
        app_db.gaming_mode = true;

#if F_APP_LISTENING_MODE_SUPPORT
        if (app_cfg_const.disallow_anc_apt_when_gaming_mode)
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_GAMING_MODE);
        }
#endif

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
        if (app_db.disallow_play_gaming_mode_vp == false)
#endif
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_audio_tone_type_play(TONE_ENTER_GAMING_MODE, false, true);
            }
            else if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_ENTER_GAMING_MODE, false, false);
            }
        }

        app_audio_get_last_used_latency(&latency_value);
    }


#if (F_APP_DUAL_AUDIO_EFFECT == 1)
    if (remote_session_state_get() == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        dual_audio_single_effect();
    }
    else
    {
        if (!app_cfg_const.dual_audio_gaming_align_default)
        {

            if (app_db.gaming_mode)
            {
                dual_audio_set_effect((DUAL_EFFECT_TYPE)app_cfg_nv.dual_audio_effect_gaming_type, false);
            }
            else
            {
                dual_audio_set_effect((DUAL_EFFECT_TYPE)app_cfg_nv.audio_effect_type, false);
            }

        }
    }
#endif

#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
    led_change_mode(LED_MODE_GAMING_MODE, true, false);
#endif

    app_bt_sniffing_set_low_latency();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_check_b2s_link_by_id(i))
            {
                p_link = &app_db.br_link[i];
                break;
            }
        }
    }
    else
    {
        if (app_check_b2s_link_by_id(app_get_active_a2dp_idx()))
        {
            p_link = &app_db.br_link[app_get_active_a2dp_idx()];
        }
    }

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
    if (p_link != NULL && app_db.remote_is_8753bau)
    {
        if (app_db.gaming_mode)
        {
            app_sniff_mode_b2s_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
        }
        else
        {
            app_sniff_mode_b2s_enable(p_link->bd_addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
        }
    }
#endif

    if (p_link == NULL || p_link->a2dp_track_handle == NULL)
    {
        app_eq_change_audio_eq_mode(false);
    }

    if (p_link != NULL)
    {
        if (p_link->a2dp_track_handle != NULL)
        {
            APP_PRINT_TRACE2("mmi set gaming mode: active link %s, mode %u", TRACE_BDADDR(p_link->bd_addr),
                             app_db.gaming_mode);
            audio_track_format_info_get(p_link->a2dp_track_handle, &format);
            audio_track_usage_get(p_link->a2dp_track_handle, &usage);
            audio_track_volume_out_is_muted(p_link->a2dp_track_handle, &vol_muted);
            audio_track_volume_out_get(p_link->a2dp_track_handle, &vol);

            if (p_link->eq_instance != NULL)
            {
                audio_track_effect_detach(p_link->a2dp_track_handle, p_link->eq_instance);
            }

            audio_track_release(p_link->a2dp_track_handle);

            if (app_db.gaming_mode)
            {
                if (app_db.remote_is_8753bau)
                {
                    mode = AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY;
                }
                else
                {
                    mode = AUDIO_STREAM_MODE_LOW_LATENCY;
                }
            }
            else
            {
                mode = AUDIO_STREAM_MODE_NORMAL;
            }

            p_link->a2dp_track_handle = audio_track_create(AUDIO_STREAM_TYPE_PLAYBACK,
                                                           mode,
                                                           AUDIO_STREAM_USAGE_SNOOP,
                                                           format,
                                                           vol,
                                                           0,
                                                           AUDIO_DEVICE_OUT_DEFAULT,
                                                           NULL,
                                                           NULL);

            if (vol_muted)
            {
                audio_track_volume_out_mute(p_link->a2dp_track_handle);
                p_link->playback_muted = true;
            }

            if (app_db.gaming_mode)
            {
                if (app_db.remote_is_8753bau)
                {
                    latency_value = (remote_session_role_get() == REMOTE_SESSION_ROLE_SINGLE ? ULTRA_NONE_LATENCY_MS :
                                     ULTRA_LOW_LATENCY_MS);
                    audio_track_latency_set(p_link->a2dp_track_handle,
                                            latency_value,
                                            RTK_GAMING_MODE_DYNAMIC_LATENCY_FIX);
                    bt_a2dp_stream_delay_report_request(p_link->bd_addr, latency_value);
                    app_sniff_mode_b2s_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE);
                }
                else
                {
                    latency_value = app_audio_set_latency(p_link->a2dp_track_handle,
                                                          app_cfg_nv.rws_low_latency_level_record,
                                                          GAMING_MODE_DYNAMIC_LATENCY_FIX);
                    bt_a2dp_stream_delay_report_request(p_link->bd_addr, latency_value);
                }

                app_db.last_gaming_mode_audio_track_latency = latency_value;
            }
            else
            {
                audio_track_latency_set(p_link->a2dp_track_handle, latency_value,
                                        NORMAL_MODE_DYNAMIC_LATENCY_FIX);
                bt_a2dp_stream_delay_report_request(p_link->bd_addr, latency_value);
            }

            if (p_link->eq_instance != NULL)
            {
                /* when streaming, changing eq after audio track recreate */
                app_eq_change_audio_eq_mode(false);
                audio_track_effect_attach(p_link->a2dp_track_handle, p_link->eq_instance);
            }

#if F_APP_LINEIN_SUPPORT
            if (app_cfg_const.line_in_support)
            {
                if (app_line_in_start_a2dp_check())
                {
                    audio_track_start(p_link->a2dp_track_handle);
                }
            }
            else
#endif
            {
                audio_track_start(p_link->a2dp_track_handle);
            }
        }
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_audio_report_low_latency_status(latency_value);

        if (app_db.spk_eq_mode != LINE_IN_MODE)
        {
            app_report_eq_index(EQ_INDEX_REPORT_BY_CHANGE_MODE);
        }
    }

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
    if (app_db.disallow_play_gaming_mode_vp && app_db.gaming_mode_request_is_received)
    {
        app_db.disallow_play_gaming_mode_vp = false;
    }

    app_db.gaming_mode_request_is_received = false;
#endif
}

void app_mmi_execute_output_indication_action(uint8_t action)
{
    uint8_t *pinmux = &app_cfg_const.output_indication1_pinmux;
    uint8_t i = action - MMI_OUTPUT_INDICATION1_TOGGLE;

    if (app_cfg_const.output_indication_support)
    {
        if (*(pinmux + i) != 0xFF)
        {
            PAD_OUTPUT_VAL tmp_output_indication = PAD_OUT_HIGH;

            if (Pad_GetOutputCtrl(*(pinmux + i)) == PAD_AON_OUTPUT_HIGH)
            {
                tmp_output_indication = PAD_OUT_LOW;
            }
            else if (Pad_GetOutputCtrl(*(pinmux + i)) == PAD_AON_OUTPUT_LOW)
            {
                tmp_output_indication = PAD_OUT_HIGH;
            }

            if (action == MMI_OUTPUT_INDICATION3_TOGGLE)
            {
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                if (app_cfg_const.output_ind3_link_mic_toggle)
                {
                    tmp_output_indication = PAD_OUT_HIGH;
                    if (app_cfg_const.led_on_when_mic_mute)
                    {
                        if ((app_cfg_const.enable_output_ind3_high_active == 1) && !app_audio_is_mic_mute()
                            || (app_cfg_const.enable_output_ind3_high_active == 0) &&  app_audio_is_mic_mute())
                        {
                            tmp_output_indication = PAD_OUT_LOW;
                        }
                    }
                    else //led on when mic unmute
                    {
                        if ((app_cfg_const.enable_output_ind3_high_active == 1) && app_audio_is_mic_mute()
                            || (app_cfg_const.enable_output_ind3_high_active == 0) && (!app_audio_is_mic_mute()))
                        {
                            tmp_output_indication = PAD_OUT_LOW;
                        }
                    }
                }
#endif
            }

            Pad_OutputControlValue(*(pinmux + i), tmp_output_indication);
        }
    }
}


static void app_mmi_check_factory_reset_behavior(uint8_t action)
{
    app_db.factory_reset_ignore_led_and_vp = false;

    switch (action)
    {
    case MMI_DEV_TOOLING_FACTORY_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_NORMAL;
        app_db.factory_reset_ignore_led_and_vp = true;
        break;

    case MMI_DEV_CFG_FACTORY_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_CFG;
        break;

    case MMI_DEV_FACTORY_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_NORMAL;
        if (app_db.ignore_led_mode)
        {
            app_db.factory_reset_ignore_led_and_vp = true;
        }
        break;

    case MMI_DEV_PHONE_RECORD_RESET:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_PHONE;
        if (app_db.ignore_led_mode)
        {
            app_db.factory_reset_ignore_led_and_vp = true;
        }
        break;

    case MMI_DEV_FACTORY_RESET_BY_SPP:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_NORMAL;
        break;

    case MMI_DEV_PHONE_RECORD_RESET_BY_SPP:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_PHONE;
        break;

    case MMI_DEV_RESET_TO_UNINITIAL_STATE:
        app_db.factory_reset_clear_mode = FACTORY_RESET_CLEAR_ALL;
        break;
    }
}

bool app_mmi_is_allow_factory_reset(void)
{
    bool allow_factory_reset = false;

    if (app_cfg_const.enable_factory_reset_whitout_limit == 1)
    {
        allow_factory_reset = true;
    }
    else if ((app_cfg_const.enable_factory_reset_when_in_the_box == 1) &&
             app_device_is_in_the_box())
    {
        /* power off, only allow factory reset MMI */
        allow_factory_reset = true;
    }
    else if ((app_cfg_const.enable_factory_reset_when_in_the_box == 0)
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
             && (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
#endif
             && (!app_bt_policy_get_b2b_connected()))
    {
        allow_factory_reset = true;
    }

    return allow_factory_reset;
}

void app_mmi_handle_action(uint8_t action)
{
    APP_PRINT_TRACE1("app_mmi_handle_action: action 0x%02x", action);

#if F_APP_HARMAN_FEATURE_SUPPORT && GFPS_FINDER_SUPPORT
    if (extend_app_cfg_const.gfps_finder_support)
    {
        //finder stop ring
        T_GFPS_RFC_RING_STATE ring_type = app_gfps_rfc_get_ring_state();
        //dult stop ring
        T_DULT_RING_STATE dult_ring_state = app_dult_device_get_ring_state();

        if ((ring_type != GFPS_RFC_RING_STOP) || (dult_ring_state != DULT_RING_STATE_STOPPED))
        {
            APP_PRINT_INFO0("app_mmi_handle_action: keypress to stop gfps finder or dult ringtone");

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (ring_type != GFPS_RFC_RING_STOP)
                {
                    app_gfps_rfc_handle_ring_event(GFPS_ALL_STOP);

                    uint8_t ring_state = GFPS_FINDER_RING_BUTTON_STOP;
                    app_gfps_finder_send_ring_rsp(ring_state);
                }

                if (dult_ring_state != DULT_RING_STATE_STOPPED)
                {
                    app_dult_device_sound_stop(0xFF, NULL);
                }
            }
            else
            {
                //maybe relay to primary bud and stop ring
            }

            return;
        }
    }

    if (action == MMI_DEV_FACTORY_RESET)
    {
        if ((app_hfp_sco_is_connected()) || (timer_handle_hfp_ring != NULL))
        {
            return;
        }
        else if (app_find_b2s_link_num())
        {
            action = MMI_DEV_FACTORY_RESET_BY_SPP;
        }

        APP_PRINT_TRACE1("app_mmi_handle_action: refine action 0x%02x", action);
    }
#endif

    switch (action)
    {
    case MMI_HF_END_OUTGOING_CALL:
    case MMI_HF_ANSWER_CALL:
    case MMI_HF_REJECT_CALL:
    case MMI_HF_END_ACTIVE_CALL:
    case MMI_HF_INITIATE_VOICE_DIAL:
    case MMI_HF_CANCEL_VOICE_DIAL:
    case MMI_HF_LAST_NUMBER_REDIAL:
    case MMI_HF_SWITCH_TO_SECOND_CALL:
    case MMI_HF_TRANSFER_CALL:
    case MMI_HF_QUERY_CURRENT_CALL_LIST:
    case MMI_HF_JOIN_TWO_CALLS:
    case MMI_HF_RELEASE_HELD_OR_WAITING_CALL:
    case MMI_HF_RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL:
    case MMI_AV_PLAY_PAUSE:
    case MMI_AV_STOP:
    case MMI_AV_FWD:
    case MMI_AV_BWD:
    case MMI_AV_FASTFORWARD:
    case MMI_AV_REWIND:
    case MMI_AV_FASTFORWARD_STOP:
    case MMI_AV_REWIND_STOP:
    case MMI_DEV_SPK_VOL_UP:
    case MMI_DEV_SPK_VOL_DOWN:
    case MMI_DEV_GAMING_MODE_SWITCH:
        {
            if (app_roleswap_ctrl_get_status() != APP_ROLESWAP_STATUS_IDLE)
            {
                APP_PRINT_TRACE0("Roleswap is going on, please wait for a minute.");
                app_db.pending_mmi_action_in_roleswap = action;
                return;
            }
        }
        break;

    default:
        break;
    }


    uint8_t active_a2dp_idx = app_get_active_a2dp_idx();
    uint8_t active_hf_idx = app_hfp_get_active_hf_index();

    switch (action)
    {
    case MMI_DEV_GAMING_MODE_SWITCH:
        {
            if (app_db.b2s_connected_num == 0)
            {
                return;
            }

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            if ((app_db.remote_is_8753bau) && (app_db.gaming_mode_request_is_received == false))
            {
                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    app_dongle_gaming_mode_request(!app_db.gaming_mode);
                }

                return;
            }
#endif
            app_mmi_switch_gaming_mode();
        }
        break;

#if F_APP_TMAP_BMR_SUPPORT
    case MMI_BIG_START:
    case MMI_BIG_STOP:
        {
            app_le_audio_mmi_handle_action(action);
        }
        break;
#endif

#if (F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT)
    case MMI_CIG_START:
    case MMI_CIG_STOP:
        {
            app_le_audio_mmi_handle_action(action);
        }
        break;
#endif

    case MMI_AV_PLAY_PAUSE:
        {
#if F_APP_MCP_SUPPORT
            app_le_audio_mmi_handle_action(MMI_AV_PLAY_PAUSE);
            if (mtc_get_btmode())
            {
                return;
            }
#endif
#if F_APP_LOCAL_PLAYBACK_SUPPORT
            if (app_db.sd_playback_switch == 1)
            {
                app_playback_mmi_handle_action(MMI_SD_PLAYPACK_PAUSE);
                return;
            }
#endif

#if F_APP_MUTILINK_TRIGGER_HIGH_PRIORITY
            if (app_cfg_const.enable_multi_link)
            {
                if (app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PAUSED ||
                    app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_STOPPED)
                {
                    app_idx = app_get_highest_priority_index(A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK);
                    //When active a2dp index is idle, DUT will trigger highest priority SRC to play music
                }
            }
#endif
#if F_APP_HARMAN_FEATURE_SUPPORT
            if ((app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PAUSED ||
                 app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_STOPPED) &&
                (au_get_harman_aling_active_idx() != 0xff))
            {
                active_a2dp_idx = au_get_harman_aling_active_idx();
            }
#endif
            //Spk2 do not handle a2dp/avrcp
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if ((app_db.br_link[active_a2dp_idx].connected_profile & A2DP_PROFILE_MASK) &&
                (app_db.br_link[active_a2dp_idx].connected_profile & AVRCP_PROFILE_MASK))
            {
                APP_PRINT_TRACE3("MMI_AV_PLAY_PAUSE: active_a2dp_idx %d, avrcp_play_status %d, bud_stream_state %d",
                                 active_a2dp_idx, app_db.br_link[active_a2dp_idx].avrcp_play_status,
                                 app_audio_get_bud_stream_state());

                if (app_db.br_link[active_a2dp_idx].avrcp_play_status != BT_AVRCP_PLAY_STATUS_PLAYING)
                {
                    // Update play status after AVRCP play status event received
                    bt_avrcp_play(app_db.br_link[active_a2dp_idx].bd_addr);
#if F_APP_HARMAN_FEATURE_SUPPORT
                    au_set_record_a2dp_active_ever(true);
#endif
                    app_audio_tone_type_cancel(TONE_AUDIO_PAUSED, true);
                    app_audio_tone_type_play(TONE_AUDIO_PLAYING, true, true);
                }
                else
                {
                    bt_avrcp_pause(app_db.br_link[active_a2dp_idx].bd_addr);

                    app_audio_tone_type_cancel(TONE_AUDIO_PLAYING, true);
                    app_audio_tone_type_play(TONE_AUDIO_PAUSED, true, true);

                }
            }
            else
            {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_mmi_av_play_pause_hook)
                {
                    app_mmi_av_play_pause_hook();
                }
#else
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                uint8_t bd_addr[6];

                if (app_b2s_bond_addr_get(1, bd_addr) == true)
                {
                    app_bt_policy_default_connect(bd_addr,
                                                  A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK, true);
                }
#endif
#endif
            }
        }
        break;

    case MMI_AV_STOP:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_STOP);
                return;
            }
#endif
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_avrcp_stop(app_db.br_link[active_a2dp_idx].bd_addr);
        }
        break;

    case MMI_AV_FWD:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_FWD);
                return;
            }
#endif
#if F_APP_LOCAL_PLAYBACK_SUPPORT
            if (app_db.sd_playback_switch == 1)
            {
                app_playback_mmi_handle_action(MMI_SD_PLAYBACK_FWD);
                return;
            }
#endif
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_mmi_av_fwd_hook)
            {
                app_mmi_av_fwd_hook();
            }
#else
            if ((app_cfg_const.enable_av_fwd_bwd_only_when_playing == 0) ||
                (app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
            {
                bt_avrcp_forward(app_db.br_link[active_a2dp_idx].bd_addr);
                app_audio_tone_type_play(TONE_AUDIO_FORWARD, false, true);
            }
#endif
        }
        break;

    case MMI_AV_BWD:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_BWD);
                return;
            }
#endif
#if F_APP_LOCAL_PLAYBACK_SUPPORT
            if (app_db.sd_playback_switch == 1)
            {
                app_playback_mmi_handle_action(MMI_SD_PLAYBACK_BWD);
                return;
            }
#endif
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_mmi_av_bwd_hook)
            {
                app_mmi_av_bwd_hook();
            }
#else
            if ((app_cfg_const.enable_av_fwd_bwd_only_when_playing == 0) ||
                (app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
            {

                bt_avrcp_backward(app_db.br_link[active_a2dp_idx].bd_addr);
                app_audio_tone_type_play(TONE_AUDIO_BACKWARD, false, true);
            }
#endif
        }
        break;

    case MMI_AV_FASTFORWARD:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_FASTFORWARD);
                return;
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_avrcp_fast_forward_start(app_db.br_link[active_a2dp_idx].bd_addr);
        }
        break;

    case MMI_AV_REWIND:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_REWIND);
                return;
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_avrcp_rewind_start(app_db.br_link[active_a2dp_idx].bd_addr);
        }
        break;

    case MMI_AV_FASTFORWARD_STOP:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_FASTFORWARD_STOP);
                return;
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_avrcp_fast_forward_stop(app_db.br_link[active_a2dp_idx].bd_addr);
        }
        break;

    case MMI_AV_REWIND_STOP:
        {
#if F_APP_MCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_AV_REWIND_STOP);
                return;
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_avrcp_rewind_stop(app_db.br_link[active_a2dp_idx].bd_addr);
        }
        break;

    case MMI_HF_ANSWER_CALL:
        {
#if F_APP_CCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_HF_ANSWER_CALL);
                return;
            }
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
            app_asp_device_send_hook_button_status(0);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            if (app_teams_multilink_get_voice_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                if (app_teams_multilink_get_second_hfp_priority_index() == 0xff)
                {
#if F_APP_USB_AUDIO_SUPPORT
                    app_usb_mmi_handle_action(action);
#endif
                }
                else
                {
                    bt_hfp_call_answer_req(app_db.br_link[app_teams_multilink_get_second_hfp_priority_index()].bd_addr);
                }
                return;
            }
#endif
#endif
            T_APP_BR_LINK *p_link;
            T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_INVALID;
            p_link = &(app_db.br_link[active_hf_idx]);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            app_audio_tone_flush(true);

            if (p_link != NULL)
            {
                app_hfp_stop_ring_alert_timer();

                tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
                app_audio_tone_type_cancel(tone_type, true);
                bt_hfp_call_answer_req(p_link->bd_addr);
            }
        }
        break;

    case MMI_HF_REJECT_CALL:
#if F_APP_CCP_SUPPORT
    case MMI_CIG_HF_REJECT_CALL:
#endif
        {
#if F_APP_CCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_HF_REJECT_CALL);
                if (action == MMI_HF_REJECT_CALL)
                {
                    return;
                }
            }
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
            app_asp_device_send_hook_button_status(1);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            if (app_teams_multilink_get_voice_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                if (app_teams_multilink_get_second_hfp_priority_index() == 0xff)
                {
#if F_APP_USB_AUDIO_SUPPORT
                    app_usb_mmi_handle_action(action);
#endif
                }
                else
                {
                    bt_hfp_call_terminate_req(
                        app_db.br_link[app_teams_multilink_get_second_hfp_priority_index()].bd_addr);
                }
                return;
            }
#endif
#endif
            T_APP_BR_LINK *p_link;
            T_APP_AUDIO_TONE_TYPE tone_type = TONE_TYPE_INVALID;

            p_link = &(app_db.br_link[active_hf_idx]);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (p_link != NULL)
            {
                app_hfp_stop_ring_alert_timer();

                tone_type = (T_APP_AUDIO_TONE_TYPE)app_hfp_get_call_in_tone_type(p_link);
                app_audio_tone_type_cancel(tone_type, true);
                if (bt_hfp_call_terminate_req(p_link->bd_addr))
                {
                    app_db.reject_call_by_key = true;
                }
            }
        }
        break;

    case MMI_HF_END_ACTIVE_CALL:
        {
#if F_APP_CCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_HF_END_ACTIVE_CALL);
                if (action == MMI_HF_END_ACTIVE_CALL)
                {
                    return;
                }
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

#if F_APP_TEAMS_FEATURE_SUPPORT
            app_asp_device_send_hook_button_status(1);
#endif
#if HARMAN_AUTO_ACCEPT_SECOND_CALL_SUPPORT
            if (app_db.br_link[app_hfp_get_active_hf_index()].call_status > BT_HFP_CALL_ACTIVE)
            {
                bt_hfp_release_active_call_accept_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
            }
            else
#endif
            {
                bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
            }
        }
        break;

    case MMI_HF_END_OUTGOING_CALL:
#if (ISOC_AUDIO_SUPPORT == 1)
    case MMI_CIG_HF_END_OUTGOING_CALL:
#endif
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
        }
        break;

    case MMI_HF_TRANSFER_CALL:
        {
#if F_APP_CCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_HF_TRANSFER_CALL);
                return;
            }
#endif

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if ((app_hfp_get_call_status() == BT_HFP_INCOMING_CALL_ONGOING) &&
                !app_db.br_link[active_hf_idx].is_inband_ring)
            {
                //nothing
            }
            else
            {
                if (app_db.br_link[active_hf_idx].sco_handle)// to phone
                {
                    app_audio_tone_type_play(TONE_HF_TRANSFER_TO_PHONE, false, true);
                    bt_hfp_audio_disconnect_req(app_db.br_link[active_hf_idx].bd_addr);
                }
                else //to bud
                {
                    app_audio_tone_type_play(TONE_HF_TRANSFER_TO_PHONE, false, true);
                    app_bt_sniffing_hfp_connect(app_db.br_link[active_hf_idx].bd_addr);
                }
            }
        }
        break;

    case MMI_HF_CANCEL_VOICE_DIAL:
#if (ISOC_AUDIO_SUPPORT == 1)
    case MMI_CIG_HF_CANCEL_VOICE_DIAL:
#endif
        {
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_mmi_voice_recognition_disable_hook)
            {
                app_mmi_voice_recognition_disable_hook();
            }
#else

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_hfp_voice_recognition_disable_req(app_db.br_link[active_hf_idx].bd_addr);
#endif
        }
        break;

    case MMI_HF_LAST_NUMBER_REDIAL:
        {
            uint8_t app_idx = MAX_BR_LINK_NUM - 1;
            uint8_t link_number;
            link_number = app_connected_profile_link_num(HFP_PROFILE_MASK | HSP_PROFILE_MASK);
            if (link_number > 1)
            {
                if (app_cfg_const.enable_last_num_redial_always_by_first_phone)
                {
                    app_idx = app_db.first_hf_index;
                }
                else if (app_cfg_const.enable_last_num_redial_always_by_last_phone)
                {
                    app_idx = app_db.last_hf_index;
                }
            }
            else if (link_number == 1)
            {
                app_idx = app_hfp_get_active_hf_index();
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            if (app_db.br_link[app_idx].app_hf_state == APP_HF_STATE_CONNECTED)
            {
                if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
                {
                    bt_hfp_dial_last_number_req(app_db.br_link[app_idx].bd_addr);
                    app_audio_tone_type_play(TONE_HF_CALL_REDIAL, false, true);
                }
            }
            else
            {
                uint8_t bd_addr[6];

                if (app_b2s_bond_addr_get(1, bd_addr) == true)
                {
                    app_bt_policy_default_connect(bd_addr,
                                                  HSP_PROFILE_MASK | HFP_PROFILE_MASK, true);
                }
            }
        }
        break;

    case MMI_HF_JOIN_TWO_CALLS:
        {
#if F_APP_CCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_HF_JOIN_TWO_CALLS);
                return;
            }
#endif
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY ||
                (app_db.br_link[app_hfp_get_active_hf_index()].call_status <= BT_HFP_CALL_ACTIVE))
            {
                return;
            }

            bt_hfp_join_two_calls_req(app_db.br_link[active_hf_idx].bd_addr);
        }
        break;

    case MMI_HF_RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }
#if SWITCH_BETWEEN_SRC
            uint8_t inactive_hf_index;

            for (inactive_hf_index = 0; inactive_hf_index < MAX_BR_LINK_NUM; inactive_hf_index++)
            {
                if (inactive_hf_index != active_hf_idx)
                {
                    if (app_db.br_link[inactive_hf_index].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                    {
                        break;
                    }
                }
            }

            if (app_hfp_get_call_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                if (inactive_hf_index < MAX_BR_LINK_NUM)
                {
                    app_multi_active_hfp_transfer(inactive_hf_index, false, false);
                    bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
                    bt_hfp_call_answer_req(app_db.br_link[inactive_hf_index].bd_addr);
                }

            }
            else if (app_hfp_get_call_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                if (inactive_hf_index < MAX_BR_LINK_NUM)
                {
                    app_multi_active_hfp_transfer(inactive_hf_index, false, false);
                    bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
                }
            }
            else
#endif
            {
#if SWITCH_BETWEEN_SRC
                if (inactive_hf_index < MAX_BR_LINK_NUM &&
                    app_db.br_link[inactive_hf_index].call_status == BT_HFP_INCOMING_CALL_ONGOING)
                {
                    app_multi_active_hfp_transfer(inactive_hf_index, false, false);
                    bt_hfp_call_terminate_req(app_db.br_link[active_hf_idx].bd_addr);
                    bt_hfp_call_answer_req(app_db.br_link[inactive_hf_index].bd_addr);
                }
                else
#endif
                {
                    bt_hfp_release_active_call_accept_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
                }
            }
        }
        break;

    case MMI_HF_RELEASE_HELD_OR_WAITING_CALL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }
#if SWITCH_BETWEEN_SRC
            uint8_t inactive_hf_index;

            for (inactive_hf_index = 0; inactive_hf_index < MAX_BR_LINK_NUM; inactive_hf_index++)
            {
                if (inactive_hf_index != active_hf_idx)
                {
                    if (app_db.br_link[inactive_hf_index].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                    {
                        break;
                    }
                }
            }

            if (app_hfp_get_call_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                if (inactive_hf_index < MAX_BR_LINK_NUM)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[inactive_hf_index].bd_addr);
                }
            }
            else if (app_hfp_get_call_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                if (inactive_hf_index < MAX_BR_LINK_NUM)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[inactive_hf_index].bd_addr);
                }
            }
            else
#endif
            {
#if SWITCH_BETWEEN_SRC
                if (inactive_hf_index < MAX_BR_LINK_NUM &&
                    app_db.br_link[inactive_hf_index].call_status == BT_HFP_INCOMING_CALL_ONGOING)
                {
                    bt_hfp_call_terminate_req(app_db.br_link[inactive_hf_index].bd_addr);
                }
                else
#endif
                {
                    bt_hfp_release_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
                }
            }
        }
        break;

    case MMI_HF_SWITCH_TO_SECOND_CALL:
        {
#if F_APP_CCP_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_HF_SWITCH_TO_SECOND_CALL);
                return;
            }
#endif
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }
#if SWITCH_BETWEEN_SRC
            uint8_t inactive_hf_index;

            for (inactive_hf_index = 0; inactive_hf_index < MAX_BR_LINK_NUM; inactive_hf_index++)
            {
                if (inactive_hf_index != active_hf_idx)
                {
                    if (app_db.br_link[inactive_hf_index].connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
                    {
                        break;
                    }
                }
            }

            if (app_hfp_get_call_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT)
            {
                if (inactive_hf_index < MAX_BR_LINK_NUM)
                {
                    app_multi_active_hfp_transfer(inactive_hf_index, true, false);
                    bt_hfp_call_answer_req(app_db.br_link[inactive_hf_index].bd_addr);
                }
            }
            else if (app_hfp_get_call_status() == BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD)
            {
                if (inactive_hf_index < MAX_BR_LINK_NUM)
                {
                    app_multi_active_hfp_transfer(inactive_hf_index, true, false);
                }
            }
            else
#endif
            {
#if SWITCH_BETWEEN_SRC
                if (inactive_hf_index < MAX_BR_LINK_NUM &&
                    app_db.br_link[inactive_hf_index].call_status == BT_HFP_INCOMING_CALL_ONGOING)
                {
                    app_multi_active_hfp_transfer(inactive_hf_index, true, false);
                    bt_hfp_call_answer_req(app_db.br_link[inactive_hf_index].bd_addr);
                }
                else
#endif
                {
                    bt_hfp_hold_active_call_accept_held_or_waiting_call_req(app_db.br_link[active_hf_idx].bd_addr);
                }
            }
        }
        break;

    case MMI_HF_QUERY_CURRENT_CALL_LIST:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                return;
            }

            bt_hfp_current_call_list_req(app_db.br_link[active_hf_idx].bd_addr);
        }
        break;

    case MMI_HF_INITIATE_VOICE_DIAL:
        {
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_mmi_voice_recognition_enable_hook)
            {
                app_mmi_voice_recognition_enable_hook();
            }
#else
            mmi_voice_recognition();
#endif
        }
        break;

    case MMI_DEV_POWER_ON:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
            {
                app_mmi_modify_reboot_check_times(REBOOT_CHECK_POWER_ON_MAX_TIMES);
            }
            else if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
                if (!app_harman_discharger_ntc_check_valid())
                {
                    app_db.need_delay_power_on = true;
                    return;
                }
#endif

#if HARMAN_OTA_COMPLETE_NONEED_POWERON
                APP_PRINT_TRACE1("MMI_DEV_POWER_ON: harman_ota_complete: %d", app_cfg_nv.harman_ota_complete);
                if (app_cfg_nv.harman_ota_complete)
                {
                    app_cfg_nv.harman_ota_complete = 0;
                    app_cfg_store(&app_cfg_nv.ota_continuous_packets_max_count, 4);

                    app_db.is_long_press_power_on_play_vp = true;
                }
#endif
                if (!app_db.is_long_press_power_on_play_vp && !mp_hci_test_mode_is_running())
                {
#if F_APP_HARMAN_FEATURE_SUPPORT
                    if (app_cfg_nv.language_status == 0)
                    {
                        app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_30, false, false);
                    }
                    else
#endif
                    {
                        app_audio_tone_type_play(TONE_POWER_ON, false, false);
                    }
                }
                app_db.is_long_press_power_on_play_vp = false;

                sys_mgr_power_on();
            }
        }
        break;

    case MMI_DEV_POWER_OFF:
        {
            APP_PRINT_TRACE3("app_mmi_handle_action: %d, 0x%x, %d", app_db.device_state,
                             app_db.power_off_cause, app_key_is_enter_pairing());
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
#if HARMAN_FIND_MY_BUDS_TONE_SUPPORT
                app_harman_find_my_buds_ring_stop();
#endif
#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
                if (app_cfg_const.discharge_mode_protection_thermistor_option)
                {
                    app_harman_adc_ntc_check_timer_stop();
                }
#endif
                app_device_state_change(APP_DEVICE_STATE_OFF_ING);

                app_dlps_disable(APP_DLPS_ENTER_WAIT_RESET);

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    app_relay_async_single(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_POWERING_OFF);
                }

                gap_stop_timer(&timer_handle_app_mmi_reboot_check);
                app_mmi_reboot_check_timer_start(500);
                power_register_excluded_handle(&timer_handle_app_mmi_reboot_check, PM_EXCLUDED_TIMER);

                if (mp_hci_test_mode_is_running())
                {
                    mp_hci_test_mmi_handle_action(action);
                }
                else
                {
                    app_sniff_mode_disable_all();
                    app_mmi_power_off();
                }
#if F_APP_HARMAN_FEATURE_SUPPORT
                au_set_record_a2dp_active_ever(false);
                au_set_harman_already_connect_one(false);
#endif
            }
        }
        break;

    case MMI_DEV_FORCE_ENTER_PAIRING_MODE:
    case MMI_DEV_ENTER_PAIRING_MODE:
        {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
            if (app_teams_multilink_get_voice_status() != BT_HFP_CALL_IDLE)
#else
#if F_APP_HARMAN_FEATURE_SUPPORT
            if ((app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
#if HARMAN_NOT_ALLOW_ENTER_PAIR_MODE_ON_CALLING_SUPPORT
                || (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
#endif
               )
#else
            if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
#endif
#endif
            {
                //Not allow to enter pairing mode when calling
                break;
            }
            else
            {
                if (app_db.device_state == APP_DEVICE_STATE_OFF)
                {
                    sys_mgr_power_on();
                }

#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_mmi_enter_pairing_hook)
                {
                    app_mmi_enter_pairing_hook();
                }
#endif

                if (action == MMI_DEV_FORCE_ENTER_PAIRING_MODE)
                {
                    app_bt_policy_enter_pairing_mode(true, true);
                }
                else
                {
                    app_bt_policy_enter_pairing_mode(false, true);
                }
            }
#if F_APP_LE_AUDIO_SM
            app_le_audio_device_sm(LE_AUDIO_ENTER_PAIRING, NULL);
#endif
        }
        break;

    case MMI_DEV_EXIT_PAIRING_MODE:
        {
            app_bt_policy_exit_pairing_mode();
#if F_APP_LE_AUDIO_SM
            app_le_audio_device_sm(LE_AUDIO_EXIT_PAIRING, NULL);
#endif
        }
        break;

    case MMI_DEV_MULTILINK_ON:
        {
            if (app_cfg_const.enable_multi_link == 0)
            {
                app_multilink_switch_by_mmi(true);
                app_audio_tone_type_play(TONE_DEV_MULTILINK_ON, false, false);
                app_adp_factory_reset_link_dis(1000);
            }
        }
        break;

    case MMI_DEV_LINK_LAST_DEVICE:
        {
            uint32_t bond_flag;
            uint32_t plan_profs;

            uint8_t bond_num = app_b2s_bond_num_get();
            if (app_bt_policy_get_state() == BP_STATE_STANDBY)
            {
                for (uint8_t i = 1; i <= bond_num; i++)
                {
                    if (app_b2s_bond_addr_get(i, app_db.bt_addr_disconnect))
                    {
                        bt_bond_flag_get(app_db.bt_addr_disconnect, &bond_flag);
                        if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
                        {
                            plan_profs = get_profs_by_bond_flag(bond_flag);

                            app_bt_policy_default_connect(app_db.bt_addr_disconnect,  plan_profs, false);
                            break;
                        }
                    }
                }
            }
        }
        break;

    case MMI_DEV_TOOLING_FACTORY_RESET:
    case MMI_DEV_CFG_FACTORY_RESET:
    case MMI_DEV_FACTORY_RESET:
    case MMI_DEV_FACTORY_RESET_BY_SPP:
    case MMI_DEV_PHONE_RECORD_RESET:
    case MMI_DEV_PHONE_RECORD_RESET_BY_SPP:
    case MMI_DEV_RESET_TO_UNINITIAL_STATE:
        {
#if HARMAN_FIND_MY_BUDS_TONE_SUPPORT
            app_harman_find_my_buds_ring_stop();
#endif
            app_dlps_disable(APP_DLPS_ENTER_WAIT_RESET);
            app_mmi_check_factory_reset_behavior(action);

#if F_APP_IO_OUTPUT_SUPPORT
            if (app_cfg_const.enable_external_mcu_reset)
            {
                app_io_output_ctrl_ext_mcu_pin(DISABLE);
            }
#endif

            if (!app_db.factory_reset_ignore_led_and_vp)
            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (app_cfg_nv.language_status == 0)
                {
                    app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_50, false, false);
                }
                else
#endif
                {
                    app_audio_tone_type_play(TONE_FACTORY_RESET, false, false);
                }
                /*
                    Stop led timer, and start factory reset LED immediately,
                    then enter power down mode as soon as possible.
                */
                led_timer_stop();
                if (led_check_set_mode_available(LED_MODE_FACTORY_RESET) == true)
                {
                    led_change_mode(LED_MODE_FACTORY_RESET, true, false);
                }
            }

            gap_stop_timer(&timer_handle_app_mmi_reboot_check);
            app_mmi_reboot_check_timer_start(500);

            app_db.waiting_factory_reset = true;
            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                app_device_state_change(APP_DEVICE_STATE_OFF_ING);
                app_sniff_mode_disable_all();
                sys_mgr_power_off();
            }
        }
        break;

    case MMI_MEMORY_DUMP:
        {
            hal_debug_memory_dump();
        }
        break;

    case MMI_DEV_SPK_VOL_UP:
        {
#if F_APP_VCS_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_DEV_SPK_VOL_UP);
                return;
            }
#endif
            app_key_set_volume_status(true);
            app_volume_up();
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
#if F_APP_VCS_SUPPORT
            if (mtc_get_btmode())
            {
                app_le_audio_mmi_handle_action(MMI_DEV_SPK_VOL_DOWN);
                return;
            }
#endif
            if ((app_cfg_const.rws_circular_vol_up_when_solo_mode) &&
                (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED))
            {
                APP_PRINT_TRACE0("rws_circular_vol_up_when_solo_mode");
                app_key_set_volume_status(true);
                app_volume_up();
                break;
            }
            app_key_set_volume_status(true);
            app_volume_down();
        }
        break;

    case MMI_DEV_SPK_MUTE:
    case MMI_DEV_SPK_UNMUTE:
        {
            app_mmi_spk_mute_set(action);
        }
        break;

    case MMI_DEV_MIC_MUTE_UNMUTE:
        {
            if (app_audio_check_mic_mute_enable() == true)
            {
                if (app_audio_is_mic_mute())
                {
                    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
                    {
                        app_key_enable_play_mic_unmute_tone(true);
                    }
                    app_audio_set_mic_mute_status(0);
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                    app_teams_audio_set_bt_mute_status(false);
#endif
                }
                else
                {
                    app_audio_set_mic_mute_status(1);
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                    app_teams_audio_set_bt_mute_status(true);
#endif
                }
                app_hfp_mute_ctrl();

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                if (app_cfg_const.output_ind3_link_mic_toggle)
                {
                    app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION3_TOGGLE);
                }
#endif
            }
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
            else
            {
                uint8_t teams_active_device_idx = app_hfp_get_active_hf_index();
                if (app_audio_is_mic_mute())
                {
                    app_audio_set_mic_mute_status(false);
                    app_teams_audio_set_bt_mute_status(false);
                    app_audio_tone_type_play(TONE_MIC_MUTE_OFF, true, true);
                }
                else
                {
                    app_audio_set_mic_mute_status(true);
                    app_teams_audio_set_bt_mute_status(true);
                    app_audio_tone_type_play(TONE_MIC_MUTE_ON, true, true);
                }
            }
#endif
        }
        break;

    case MMI_DEV_MIC_MUTE:
        {
            if (app_audio_check_mic_mute_enable() == true)
            {
                app_audio_set_mic_mute_status(1);
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                app_teams_audio_set_bt_mute_status(true);
#endif
                app_hfp_mute_ctrl();

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                if (app_cfg_const.output_ind3_link_mic_toggle)
                {
                    app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION3_TOGGLE);
                }
#endif
            }
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
            else
            {
                app_audio_set_mic_mute_status(1);
                app_teams_audio_set_bt_mute_status(true);
                app_audio_tone_type_play(TONE_MIC_MUTE_ON, true, true);
            }
#endif
        }
        break;

    case MMI_DEV_MIC_UNMUTE:
        {
            if (app_audio_check_mic_mute_enable() == true)
            {
                if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                    (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
                {
                    app_key_enable_play_mic_unmute_tone(true);
                }
                app_audio_set_mic_mute_status(0);
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
                app_teams_audio_set_bt_mute_status(false);
#endif
                app_hfp_mute_ctrl();

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                if (app_cfg_const.output_ind3_link_mic_toggle)
                {
                    app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION3_TOGGLE);
                }
#endif
            }
#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
            else
            {
                app_audio_set_mic_mute_status(0);
                app_teams_audio_set_bt_mute_status(false);
                app_audio_tone_type_play(TONE_MIC_MUTE_OFF, true, true);
            }
#endif
        }
        break;

    case MMI_DEV_MIC_VOL_UP:
        {
            app_key_set_volume_status(true);
            //microphone_vol_change(1);
        }
        break;

    case MMI_DEV_MIC_VOL_DOWN:
        {
            app_key_set_volume_status(true);
            //microphone_vol_change(0);
        }
        break;

    case MMI_DEV_MIC_SWITCH:
        {
            app_audio_mic_switch(0);
        }
        break;

    case MMI_DEV_DISCONNECT_ALL_LINK:
        {
            mmi_disconnect_all_link_event_handle();
        }
        break;

    case MMI_AUDIO_EQ_SWITCH:
        {
            uint8_t eq_num = eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode);
            T_EQ_TYPE eq_type = SPK_SW_EQ;

            if (eq_num != 0)
            {
                app_cfg_nv.eq_idx++;

                if (app_cfg_nv.eq_idx >= eq_num)
                {
                    app_cfg_nv.eq_idx = 0;
                }

                app_eq_sync_idx_accord_eq_mode(app_db.spk_eq_mode, app_cfg_nv.eq_idx);


                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_eq_play_audio_eq_tone();


                    if (app_db.eq_ctrl_by_src
#if F_APP_LINEIN_SUPPORT
                        && (app_db.spk_eq_mode != LINE_IN_MODE)    /* line in eq not support adjust from phone */
#endif
                       )
                    {
                        app_report_eq_index(EQ_INDEX_REPORT_BY_SWITCH_EQ_INDEX);
                    }
                }

                //not set default eq when audio connect,enable when set eq para from phone
                if (!app_db.eq_ctrl_by_src
#if F_APP_LINEIN_SUPPORT
                    || (app_db.spk_eq_mode == LINE_IN_MODE)    /* line in eq not support adjust from phone */
#endif
                   )
                {
                    app_eq_index_set(eq_type, app_db.spk_eq_mode, app_cfg_nv.eq_idx);
                }
            }
            else
            {
                APP_PRINT_ERROR0("app_mmi_handle_action: eq need to enable");
            }
        }
        break;

#if F_APP_APT_SUPPORT
    case MMI_AUDIO_APT:
        {
            if (!app_apt_open_condition_check())
            {
                //If APT is not allowed to be triggered, disable MMI
                break;
            }

            if (app_cfg_const.enable_direct_apt_on_off)
            {
                app_listening_direct_apt_on_off_handling();
                break;
            }

            if (app_listening_get_is_special_event_ongoing())
            {
                //MMI triggered, change final to current
                *app_db.final_listening_state = app_db.current_listening_state;
            }

            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                if (app_listening_is_allow_all_off_condition_check())
                {
                    app_listening_state_machine(EVENT_APT_OFF, true, true);
                }
            }
            else
            {
                if (app_cfg_const.normal_apt_support)
                {
                    app_listening_state_machine(EVENT_NORMAL_APT_ON, true, true);
                }
                else if (app_cfg_const.llapt_support)
                {
                    if (app_apt_is_llapt_on_state((T_ANC_APT_STATE)app_db.last_llapt_on_state))
                    {
                        app_listening_state_machine(LLAPT_STATE_TO_EVENT(app_db.last_llapt_on_state), true, true);
                    }
                }
            }
        }
        break;

    case MMI_AUDIO_APT_VOL_UP:
        {
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                bool is_need_tone_relay = true;

#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
                if (app_cfg_nv.rws_disallow_sync_apt_volume)
                {
                    is_need_tone_relay = false;
                }
#endif

                if (app_cfg_nv.apt_volume_out_level == app_cfg_const.apt_volume_out_max)
                {
                    led_change_mode(LED_MODE_VOL_MAX_MIN, true, false);
                }
                else
                {
                    led_change_mode(LED_MODE_VOL_ADJUST, true, false);
                }

                if (app_cfg_const.enable_apt_circular_vol_up)
                {
                    app_cfg_nv.apt_volume_out_level = (app_cfg_nv.apt_volume_out_level <
                                                       app_cfg_const.apt_volume_out_max) ? (app_cfg_nv.apt_volume_out_level + 1) : 0;
                }
                else
                {
                    if (app_cfg_nv.apt_volume_out_level < app_cfg_const.apt_volume_out_max)
                    {
                        app_cfg_nv.apt_volume_out_level++;
                    }
                    else
                    {
                        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                            app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED ||
                            app_cfg_nv.rws_disallow_sync_apt_volume)
                        {
                            app_audio_tone_type_play(TONE_APT_VOL_MAX, false, is_need_tone_relay);
                        }
                    }
                }

#if APT_SUB_VOLUME_LEVEL_SUPPORT
                app_apt_main_volume_set(app_cfg_nv.apt_volume_out_level);
#else
                audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
#endif

                if (app_apt_volume_relay(false, true) == false)
                {
                    app_apt_report(EVENT_APT_VOLUME_STATUS, NULL, 0);
                }

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED ||
                    app_cfg_nv.rws_disallow_sync_apt_volume)
                {
                    if ((app_cfg_const.enable_apt_circular_vol_up) &&
                        (app_cfg_nv.apt_volume_out_level == app_cfg_const.apt_volume_out_max))
                    {
                        app_audio_tone_type_play(TONE_APT_VOL_MAX, false, is_need_tone_relay);
                    }
                    else
                    {
                        app_audio_tone_type_play(TONE_APT_VOLUME_UP, false, is_need_tone_relay);
                    }
                    app_apt_play_apt_volume_tone();
                }
            }
        }
        break;

    case MMI_AUDIO_APT_VOL_DOWN:
        {

            if (app_cfg_const.enable_apt_circular_vol_up)
            {
                break;
            }

            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                bool is_need_tone_relay = true;

#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
                if (app_cfg_nv.rws_disallow_sync_apt_volume)
                {
                    is_need_tone_relay = false;
                }
#endif

                if (app_cfg_nv.apt_volume_out_level == app_cfg_const.apt_volume_out_min)
                {
                    led_change_mode(LED_MODE_VOL_MAX_MIN, true, false);
                }
                else
                {
                    led_change_mode(LED_MODE_VOL_ADJUST, true, false);
                }

                if (app_cfg_nv.apt_volume_out_level > app_cfg_const.apt_volume_out_min)
                {
                    app_cfg_nv.apt_volume_out_level--;
                }
                else
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED ||
                        app_cfg_nv.rws_disallow_sync_apt_volume)
                    {
                        app_audio_tone_type_play(TONE_APT_VOL_MIN, false, is_need_tone_relay);
                    }
                }

#if APT_SUB_VOLUME_LEVEL_SUPPORT
                app_apt_main_volume_set(app_cfg_nv.apt_volume_out_level);
#else
                audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
#endif

                if (app_apt_volume_relay(false, true) == false)
                {
                    app_apt_report(EVENT_APT_VOLUME_STATUS, NULL, 0);
                }

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED ||
                    app_cfg_nv.rws_disallow_sync_apt_volume)
                {
                    app_audio_tone_type_play(TONE_APT_VOLUME_DOWN, false, is_need_tone_relay);
                    app_apt_play_apt_volume_tone();
                }
            }
        }
        break;

#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
    case MMI_AUDIO_APT_DISALLOW_SYNC_VOLUME_TOGGLE:
        {
            app_cfg_nv.rws_disallow_sync_apt_volume ^= 1;
            app_cfg_store(&app_cfg_nv.offset_is_dut_test_mode, 4);

            // UI config apt volume sync
            if (!app_cfg_nv.rws_disallow_sync_apt_volume)
            {
                // set apt volume as medium value
#if APT_SUB_VOLUME_LEVEL_SUPPORT
                app_apt_sub_volume_set(APT_SUB_VOLUME_LEVEL_MAX / 2);
#else
                app_cfg_nv.apt_volume_out_level = APT_MAIN_VOLUME_LEVEL_MAX / 2;
                audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
#endif

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    //informate primary volume
                    app_apt_volume_relay(false, true);
                }
                else  // single or rws primary
                {
                    //don't report immediately, waiting for report of secondary
                    if (app_apt_volume_relay(false, false) == false)
                    {
                        app_apt_report(EVENT_APT_VOLUME_STATUS, NULL, 0);
                    }
                }
            }

            uint8_t buf[2];

            buf[0] = GET_STATUS_RWS_SYNC_APT_VOL;
            buf[1] = RWS_SYNC_APT_VOLUME;
            app_report_event_broadcast(EVENT_REPORT_STATUS, buf, 2);
        }
        break;
#endif
#endif

    /*
    #if BISTO_FEATURE_SUPPORT == 1
        case MMI_BISTO_FETCH_DOWN:
        case MMI_BISTO_FETCH_UP:
        case MMI_BISTO_FETCH_CLICK:
        case MMI_BISTO_PTT_DOWN:
        case MMI_BISTO_PTT_LONG_PRESS:
        case MMI_BISTO_PTT_UP:
        case MMI_BISTO_FETCH:
        case MMI_BISTO_PTT:
            app_bisto_btns_action((T_MMI_ACTION)action);
            break;
    #endif
    */

    case MMI_DEV_NEXT_VOICE_PROMPT_LANGUAGE:
        {
            uint8_t buf[2];

            if (voice_prompt_language_roll() == true)
            {
                app_cfg_nv.voice_prompt_language = voice_prompt_language_get();

                app_cfg_store(&app_cfg_nv.voice_prompt_language, 1);

                buf[0] = (uint8_t)app_cfg_nv.voice_prompt_language;
                buf[1] = voice_prompt_supported_languages_get();

                app_report_event_broadcast(EVENT_LANGUAGE_REPORT, buf, 2);

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_tone_type_play(TONE_SWITCH_VP_LANGUAGE, false, true);
                }
            }
        }
        break;

    case MMI_OTA_OVER_BLE_START:
        {
#if HARMAN_DISABLE_SPP_CMD_AFTER_PRODTCT_TEST_FINISH
            // need to restart after set this flag
            APP_PRINT_TRACE1("MMI_OTA_OVER_BLE_START: set SPP tongle flag %d",
                             app_cfg_nv.spp_disable_tongle_flag);
            app_cfg_nv.spp_disable_tongle_flag = !app_cfg_nv.spp_disable_tongle_flag;
            app_cfg_store(&app_cfg_nv.vp_ota_status, 4);
#else        
            if (app_db.device_state != APP_DEVICE_STATE_OFF)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    rws_le_common_adv_start(app_cfg_const.timer_ota_adv_timeout * 100);
                    app_audio_tone_type_play(TONE_OTA_OVER_BLE_START, false, true);
                }
            }
#endif			
        }
        break;

#if F_APP_ANC_SUPPORT
    case MMI_ANC_ON_OFF:
        {
            if (!app_anc_open_condition_check())
            {
                //If ANC is not allowed to be triggered, disable MMI
                break;
            }

            if (app_listening_get_is_special_event_ongoing())
            {
                //MMI triggered, change final to current
                *app_db.final_listening_state = app_db.current_listening_state;
            }

            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                if (app_listening_is_allow_all_off_condition_check())
                {
                    app_listening_state_machine(EVENT_ANC_OFF, true, true);
                }
            }
            else
            {
                if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.last_anc_on_state))
                {
                    app_listening_state_machine(ANC_STATE_TO_EVENT(app_db.last_anc_on_state), true, true);
                }
            }
        }
        break;
#endif

#if (F_APP_AIRPLANE_SUPPORT == 1)
    case MMI_AIRPLANE_MODE:
        {
            if (!app_airplane_mode_get())
            {
                // enter airplane mode
                app_airplane_mmi_handle();
            }
            else
            {
                // exit airplane mode need all link disconnected
                if (app_airplane_all_link_idle())
                {
                    app_airplane_mmi_handle();
                }
            }
        }
        break;
#endif

#if (F_APP_HEARABLE_SUPPORT == 1)
    case MMI_HA_PROG_CYCLE:
        {
            app_ha_switch_hearable_prog();
        }
        break;
#endif

#if F_APP_LISTENING_MODE_SUPPORT
    case MMI_LISTENING_MODE_CYCLE:
        {
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_mmi_listening_cycle_hook)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    return;
                }
                app_mmi_listening_cycle_hook();
            }
#else
            if (app_cfg_const.disallow_trigger_listening_mode_again_time)
            {
                if (app_db.key_action_disallow_too_close == MMI_LISTENING_MODE_CYCLE)
                {
                    break;
                }
                else
                {
                    app_listening_set_disallow_trigger_again();
                    app_db.key_action_disallow_too_close = MMI_LISTENING_MODE_CYCLE;
                }
            }

            if (app_listening_get_is_special_event_ongoing())
            {
                //MMI triggered, change final to current
                *app_db.final_listening_state = app_db.current_listening_state;
            }

            app_listening_state_machine(EVENT_LISTENING_MODE_CYCLE, true, true);
#endif
        }
        break;

    case MMI_ANC_CYCLE:
        {
            if (app_listening_get_is_special_event_ongoing())
            {
                //MMI triggered, change final to current
                *app_db.final_listening_state = app_db.current_listening_state;
            }

            app_listening_state_machine(EVENT_ANC_CYCLE, true, true);
        }
        break;

    case MMI_LLAPT_CYCLE:
        {
            if (app_listening_get_is_special_event_ongoing())
            {
                //MMI triggered, change final to current
                *app_db.final_listening_state = app_db.current_listening_state;
            }

            app_listening_state_machine(EVENT_LLAPT_CYCLE, true, true);
        }
        break;
#endif

#if (F_APP_SENSOR_SUPPORT == 1)
    case MMI_LIGHT_SENSOR_ON_OFF:
        {
            uint8_t new_state = app_cfg_nv.light_sensor_enable ? 0 : 1;
            app_sensor_control(new_state, true, false);

            app_report_event_broadcast(EVENT_EAR_DETECTION_STATUS, &new_state, 1);
        }
        break;
#endif

    case MMI_START_ROLESWAP:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_MMI_TRIGGER_ROLESWAP);
            }
        }
        break;

    case MMI_ENTER_SINGLE_MODE:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            {
                break;
            }
            //le_scan_stop();
            //le_adv_stop();//fixme later

            for (uint8_t app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
            {
                if (app_check_b2b_link_by_id(app_idx))
                {
                    app_bt_policy_disconnect(app_db.br_link[app_idx].bd_addr, RDTP_PROFILE_MASK);
                    break;
                }
            }
            app_cfg_nv.bud_role = REMOTE_SESSION_ROLE_SINGLE;

            remote_session_role_set(REMOTE_SESSION_ROLE_SINGLE);
        }
        break;

#if F_APP_NFC_SUPPORT
    case MMI_DEV_NFC_DETECT:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                app_dlps_enable_auto_poweroff_stop_wdg_timer();
                app_mmi_handle_action(MMI_DEV_POWER_ON);
            }
            else
            {
                if (app_cfg_const.enable_nfc_multi_link_switch)
                {
                    app_nfc_multi_link_switch_trigger();
                    app_bt_policy_enter_pairing_mode(false, false);
                }
            }
        }
        break;
#endif

    case MMI_ENTER_DUT_FROM_SPP:
        {
            app_device_enter_dut_mode();
        }
        break;

    case MMI_DUT_TEST_MODE:
        {
            if ((app_db.force_enter_dut_mode_when_acl_connected) && (!app_bt_policy_get_b2b_connected()))
            {
                app_bt_policy_enter_dut_test_mode();

                /* WROKAROUND: need to stop le ext scan before entering dut test mode otherwise it would test fail. */
                le_ext_scan_stop();
                ble_ext_adv_mgr_disable_all(APP_STOP_ADV_CAUSE_DUT_TEST);

                app_cfg_nv.is_dut_test_mode = 1;
                app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);

                led_change_mode(LED_MODE_DUT_TEST_MODE, true, false);

                mp_hci_test_auto_enter_dut_init();
                mp_hci_test_set_mode(true);

                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_DUT_TEST_MODE);
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_DUT_MODE);

#if F_APP_DUT_MODE_AUTO_POWER_OFF
                if (app_cfg_const.enable_5_mins_auto_power_off_under_dut_test_mode)
                {
                    dut_test_start_auto_power_off_timer();
                }
#endif

                //Enable Device Under Test mode
                gap_vendor_cmd_req(0x1803, 0, NULL);
            }
            else if ((app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE) &&
                     (app_bt_policy_get_state() != BP_STATE_LINKBACK) &&
                     (!app_bt_policy_get_b2b_connected()) && (app_bt_policy_get_b2s_connected_num() == 0)
                     && (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) ||
                     (app_cfg_nv.trigger_dut_mode_from_power_off))
            {
                switch_into_single_tone_test_mode();
            }
        }
        break;

#if F_APP_APT_SUPPORT
    case MMI_AUDIO_APT_EQ_SWITCH:
        {
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                uint8_t eq_num;

                eq_num = eq_utils_num_get(MIC_SW_EQ, APT_MODE);
                if (eq_num != 0)
                {
                    app_cfg_nv.apt_eq_idx++;

                    if (app_cfg_nv.apt_eq_idx >= eq_num)
                    {
                        app_cfg_nv.apt_eq_idx = 0;
                    }

                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_eq_play_apt_eq_tone();

                        if (app_db.eq_ctrl_by_src)
                        {
                            app_report_apt_index_info(EQ_INDEX_REPORT_BY_SWITCH_APT_EQ_INDEX);
                        }
                    }

                    //not set default eq when audio connect,enable when set eq para from phone
                    if (!app_db.eq_ctrl_by_src)
                    {
                        app_eq_index_set(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("app_mmi_handle_action: apt eq need to enable");
                }
            }
        }
        break;
#endif

#if F_APP_LISTENING_MODE_SUPPORT
    case MMI_AUDIO_ANC_APT_ALL_OFF:
        {
#if (F_APP_ANC_SUPPORT | F_APP_APT_SUPPORT | F_APP_AVP_INIT_SUPPORT)
            bool need_tone = true;
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_anc_apt_tone_check_hook)
            {
                app_anc_apt_tone_check_hook(&need_tone);
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            app_listening_state_machine(EVENT_ALL_OFF, need_tone, true);
#else

#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                app_listening_state_machine(EVENT_ALL_OFF, need_tone, true);
            }
            else
#endif
            {
#if F_APP_APT_SUPPORT
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
                {
                    app_listening_state_machine(EVENT_ALL_OFF, need_tone, true);
                }
#endif
            }
#endif
        }
        break;
#endif

    case MMI_TAKE_PICTURE:
        {
            uint8_t keyboard_vol_up[5] = {0x02, 0, 0, 0x80, 0};
            uint8_t keyboard_release[5] = {0x02, 0, 0, 0, 0};

            bt_hid_interrupt_data_send(app_db.br_link[active_a2dp_idx].bd_addr, BT_HID_REPORT_TYPE_INPUT,
                                       keyboard_vol_up, sizeof(keyboard_vol_up));
            bt_hid_interrupt_data_send(app_db.br_link[active_a2dp_idx].bd_addr, BT_HID_REPORT_TYPE_INPUT,
                                       keyboard_release, sizeof(keyboard_release));
        }
        break;

    case MMI_AUDIO_SWITCH_CHANNEL:
        {
            uint8_t channel = 0;

            /*L<->R; R/L->(L+R)/2; (L+R)/2 -> L/R*/
            if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
            {
                if (app_cfg_nv.spk_channel == CHANNEL_L)
                {
                    channel = CHANNEL_R;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_R)
                {
                    channel = CHANNEL_LR_HELF;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_LR_HELF)
                {
                    channel = CHANNEL_L;
                }
            }
            else if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
            {
                if (app_cfg_nv.spk_channel == CHANNEL_R)
                {
                    channel = CHANNEL_L;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_L)
                {
                    channel = CHANNEL_LR_HELF;
                }
                else if (app_cfg_nv.spk_channel == CHANNEL_LR_HELF)
                {
                    channel = CHANNEL_R;
                }
            }

            app_audio_speaker_channel_set(AUDIO_SPECIFIC_SET, channel);
        }
        break;

#if F_APP_LISTENING_MODE_SUPPORT
#if F_APP_ANC_SUPPORT
    case MMI_AUDIO_ANC_ON:
        {
            bool need_tone = true;
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_anc_apt_tone_check_hook)
            {
                app_anc_apt_tone_check_hook(&need_tone);
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (!app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
            {
                app_cfg_const.listening_mode_does_not_depend_on_in_ear_status = true;
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            app_listening_state_machine(EVENT_ANC_ON_SCENARIO_1, need_tone, true);
#else
            if (!app_anc_is_anc_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                app_listening_state_machine(EVENT_ANC_ON_SCENARIO_1, need_tone, true);
            }
#endif
        }
        break;
#endif

    case MMI_AUDIO_APT_ON:
        {
            bool need_tone = true;
#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_anc_apt_tone_check_hook)
            {
                app_anc_apt_tone_check_hook(&need_tone);
            }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (!app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
            {
                app_cfg_const.listening_mode_does_not_depend_on_in_ear_status = true;
            }
#endif

            if (app_cfg_const.normal_apt_support)
            {
                app_listening_state_machine(EVENT_NORMAL_APT_ON, need_tone, true);
            }
            else if (app_cfg_const.llapt_support)
            {
                app_listening_state_machine(EVENT_LLAPT_ON_SCENARIO_1, need_tone, true);
            }
        }
        break;
#endif

    case MMI_OUTPUT_INDICATION1_TOGGLE:
        {
            app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION1_TOGGLE);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_audio_tone_type_play(TONE_OUTPUT_INDICATION_1, false, true);
            }
        }
        break;

    case MMI_OUTPUT_INDICATION2_TOGGLE:
        {
            app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION2_TOGGLE);
        }
        break;

    case MMI_OUTPUT_INDICATION3_TOGGLE:
        {
            app_mmi_execute_output_indication_action(MMI_OUTPUT_INDICATION3_TOGGLE);
        }
        break;

#if F_APP_VAD_SUPPORT
    case MMI_AUDIO_VAD_OPEN:
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                vad_enable();
                app_cfg_const.enable_vad = 1;
            }
        }
        break;

    case MMI_AUDIO_VAD_CLOSE:
        {
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                vad_disable();
                app_cfg_const.enable_vad = 0;
            }
        }
        break;
#endif
#if (F_APP_DUAL_AUDIO_EFFECT == 1)
    case MMI_AUDIO_DUAL_EFFECT_SWITCH:
        {
            /*JUST SUPPORT default effect changing.
            *If need dynamic gaming mode effect, must add flow here.
            */
            if (app_db.gaming_mode ||
                (remote_session_state_get() == REMOTE_SESSION_STATE_DISCONNECTED &&
                 (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY ||
                  remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)))
            {
                APP_PRINT_INFO0("app_handle_mmi_message: When headset comes to gaming mode, not support default effect change!");
                APP_PRINT_INFO0("app_handle_mmi_message: When RWS Single, not support default effect change!");
                break;
            }
            dual_audio_effect_switch();
        }
        break;
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
    case MMI_MS_TEAMS_BUTTON_SHORT_PRESS:
        {
            uint8_t *bd_addr;
            uint8_t ret = 0;

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                bd_addr = asp_device_api_get_active_link_addr();
                if (bd_addr)
                {
                    ret = app_asp_device_send_teams_button_invoked_msg(bd_addr, ASP_DEVICE_SOURCE_ID_SHORT_PRESS);
                    APP_PRINT_INFO2("app_mmi_send_teams_button: bd_addr %s, ret %d, short press", TRACE_BDADDR(bd_addr),
                                    ret);
                }
                if (!ret)
                {
                    app_audio_tone_type_play(TONE_ERROR, false, false);//Tone for error
                }
            }
            teams_led_control_when_press_teams_button();
        }
        break;
    case MMI_MS_TEAMS_BUTTON_LONG_PRESS:
        {
            uint8_t *bd_addr;
            uint8_t ret = 0;

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                bd_addr = asp_device_api_get_active_link_addr();
                if (bd_addr)
                {
                    ret = app_asp_device_send_teams_button_invoked_msg(bd_addr, ASP_DEVICE_SOURCE_ID_LONG_PRESS);
                    APP_PRINT_INFO2("app_mmi_send_teams_button: bd_addr %s, ret %d, long press", TRACE_BDADDR(bd_addr),
                                    ret);
                }
                if (!ret)
                {
                    app_audio_tone_type_play(TONE_ERROR, false, false);
                }
            }
        }
        break;
#endif

#if AMA_FEATURE_SUPPORT
    case MMI_ALEXA_WAKEWORD:
        {
            app_ama_va_start();
        }
        break;
#endif

#if BISTO_FEATURE_SUPPORT
    case MMI_BISTO_PTT:
        app_bisto_btns_action(MMI_BISTO_PTT);
        break;

    case MMI_BISTO_FETCH:
        app_bisto_btns_action(MMI_BISTO_FETCH);
        break;
#endif


#if XM_XIAOAI_FEATURE_SUPPORT
    case MMI_XIAOAI_WAKEWORD:
        {
            APP_PRINT_INFO1("app_handle_mmi_message: MMI_XIAOAI_WAKEWORD bud_role %d", app_cfg_nv.bud_role);

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && extend_app_cfg_const.xiaoai_support)
            {
                app_xiaoai_device_va_start(app_db.br_link[active_a2dp_idx].bd_addr, true, false);
            }
        }
        break;
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    case MMI_XIAOWEI_START_RECORDING:
        {
            APP_PRINT_INFO1("app_handle_mmi_message: MMI_XIAOWEI_START_RECORDING bud_role %d",
                            app_cfg_nv.bud_role);
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && extend_app_cfg_const.xiaowei_support)
            {
                app_xiaowei_device_va_start(app_db.br_link[active_a2dp_idx].bd_addr, true, false);
            }
        }
        break;

    case MMI_XIAOWEI_STOP_RECORDING:
        {
            APP_PRINT_INFO1("app_handle_mmi_message: MMI_XIAOWEI_STOP_RECORDING bud_role %d",
                            app_cfg_nv.bud_role);

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && extend_app_cfg_const.xiaowei_support)
            {
                app_xiaowei_device_va_stop(app_db.br_link[active_a2dp_idx].bd_addr);
            }
        }
        break;
#endif

#if GFPS_FINDER_SUPPORT
    case MMI_GFPS_FINDER_STOP_RING:
        {
            if (extend_app_cfg_const.gfps_finder_support)
            {
                APP_PRINT_INFO1("app_handle_mmi_message: MMI_GFPS_FINDER_STOP_RING bud_role %d",
                                app_cfg_nv.bud_role);

                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY && extend_app_cfg_const.gfps_support)
                {
                    app_gfps_rfc_handle_ring_event(GFPS_ALL_STOP);

                    uint8_t ring_state = GFPS_FINDER_RING_BUTTON_STOP;
                    app_gfps_finder_send_ring_rsp(ring_state);
                }
            }
        }
        break;

    case MMI_GFPS_FINDER_STOP_ADV:
        {
#if HARMAN_ADD_GFPS_FINDER_STOP_ADV_TONE
            app_audio_tone_type_play(TONE_ANC_SCENARIO_1, false, true);
#endif
            app_gfps_finder_adv_handle_mmi_action();
        }
        break;
#if GFPS_FINDER_DULT_SUPPORT
    case MMI_DULT_ENTER_ID_READ_STATE:
        {
            app_dult_id_read_state_enable();
        }
        break;
#endif
#endif

#if F_APP_BRIGHTNESS_SUPPORT
    case MMI_LLAPT_BRIGHTNESS_UP:
        {
            if (app_apt_brightness_llapt_scenario_support((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                if (app_cfg_nv.main_brightness_level < brightness_level_max)
                {
                    app_cfg_nv.main_brightness_level++;

                    app_apt_brightness_main_level_set(app_cfg_nv.main_brightness_level);

                    if (app_apt_brightness_relay(false, true) == false)
                    {
                        app_apt_brightness_report(EVENT_LLAPT_BRIGHTNESS_STATUS, NULL, 0);
                    }
                }
            }
        }
        break;

    case MMI_LLAPT_BRIGHTNESS_DOWN:
        {
            if (app_apt_brightness_llapt_scenario_support((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                if (app_cfg_nv.main_brightness_level > brightness_level_min)
                {
                    app_cfg_nv.main_brightness_level--;

                    app_apt_brightness_main_level_set(app_cfg_nv.main_brightness_level);

                    if (app_apt_brightness_relay(false, true) == false)
                    {
                        app_apt_brightness_report(EVENT_LLAPT_BRIGHTNESS_STATUS, NULL, 0);
                    }
                }
            }
        }
        break;
#endif

#if F_APP_LOCAL_PLAYBACK_SUPPORT
    case MMI_SD_PLAYBACK_SWITCH:
        {
            T_APP_BR_LINK *p_link = NULL;

            if (app_playback_trans_is_busy())
            {
                APP_PRINT_WARN0("app_playback_trans_is_busy switch playback mode fail !!!");
                return;
            }
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_check_b2s_link_by_id(i))
                    {
                        p_link = &app_db.br_link[i];
                        break;
                    }
                }
            }
            else
            {
                if (app_check_b2s_link_by_id(app_get_active_a2dp_idx()))
                {
                    p_link = &app_db.br_link[app_get_active_a2dp_idx()];
                }
            }

            if (0 == app_db.sd_playback_switch)
            {
                if (p_link != NULL && p_link->a2dp_track_handle != NULL)
                {
                    T_AUDIO_TRACK_STATE state;
                    audio_track_state_get(p_link->a2dp_track_handle, &state);
                    if ((state == AUDIO_TRACK_STATE_STARTED) || \
                        (state == AUDIO_TRACK_STATE_PAUSED))
                    {
                        audio_track_stop(p_link->a2dp_track_handle);
                        app_db.wait_resume_a2dp_flag = true;
                    }
                }
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if ((app_db.br_link[i].connected_profile & (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK)))
                    {
                        if ((app_db.br_link[i].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
                            && (app_db.br_link[i].streaming_fg == true))
                        {
                            bt_avrcp_pause(app_db.br_link[i].bd_addr);
                            app_db.br_link[i].avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                        }
                    }
                }

                if (app_playback_mode_init() == 0)
                {
                    app_db.sd_playback_switch = 1;
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_PLAYBACK_MODE);
                }
            }
            else
            {
                app_playback_mode_deinit();
                app_db.sd_playback_switch = 0;
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_PLAYBACK_MODE, app_cfg_const.timer_auto_power_off);
                if (app_db.wait_resume_a2dp_flag == true)
                {
                    app_db.wait_resume_a2dp_flag = false;
                    if (p_link != NULL && p_link->a2dp_track_handle != NULL)
                    {
                        audio_track_start(p_link->a2dp_track_handle);
                    }
                }
            }
            APP_PRINT_TRACE1("current sd_playback_switch:%d", app_db.sd_playback_switch);
        }
        break;

    case MMI_SD_PLAYPACK_PAUSE:
    case MMI_SD_PLAYBACK_FWD:
    case MMI_SD_PLAYBACK_BWD:
    case MMI_SD_PLAYBACK_FWD_PLAYLIST:
    case MMI_SD_PLAYBACK_BWD_PLAYLIST:
        if (app_db.sd_playback_switch == 1)
        {
            app_playback_mmi_handle_action(action);
        }
        else
        {
            APP_PRINT_ERROR1("sd_playback_switch %d is fail!", app_db.sd_playback_switch);
        }
        break;
#if F_APP_LOCAL_PLAYBACK_SUPPORT & TARGET_RTL8773CO
    case MMI_SAVE_WHILE_LISTENING_ON_OFF:
        {
            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) || \
                (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED))
            {
                if (app_db.listen_save_mode == false)
                {
                    app_db.listen_save_mode = true;
                    app_listen_save_mode_enter();
                }
                else if (app_db.listen_save_mode == true)
                {
                    app_db.listen_save_mode = false;
                    app_listen_save_mode_quit();
                }
            }
        }
        break;
#endif
#endif

    default:
        break;
    }
}

bool app_mmi_is_local_execute(uint8_t action)
{
    bool ret = false;

    if (action == MMI_DEV_FACTORY_RESET ||
        (app_cfg_nv.rws_disallow_sync_apt_volume && action == MMI_AUDIO_APT_VOL_UP) ||
        (app_cfg_nv.rws_disallow_sync_apt_volume && action == MMI_AUDIO_APT_VOL_DOWN) ||
        (app_cfg_nv.rws_disallow_sync_llapt_brightness && action == MMI_LLAPT_BRIGHTNESS_UP) ||
        (app_cfg_nv.rws_disallow_sync_llapt_brightness && action == MMI_LLAPT_BRIGHTNESS_DOWN))
    {
        ret = true;
    }

    return ret;
}
