/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <trace.h>
#include "ftl.h"
#include "fmc_api.h"
#include "rtl876x.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_wdg.h"
#include "app_mmi.h"
#include "bt_hfp.h"
#include "eq_utils.h"
#include "app_adp.h"
#include "app_dsp_cfg.h"
#include "gap_le.h"
#include "app_charger.h"
#include "app_iphone_abs_vol_handle.h"
#include "app_multilink.h"
#include "app_sass_policy.h"
#if F_APP_BRIGHTNESS_SUPPORT
#include "app_audio_passthrough_brightness.h"
#endif
#if F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
#include "app_anc.h"
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_bt_policy_api.h"
#endif
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif
#if (F_APP_DUAL_AUDIO_EFFECT == 1)
#include "app_dual_audio_effect.h"
#endif
#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
#endif

static const T_APP_CFG_NV app_cfg_rw_default =
{
    .hdr.sync_word = DATA_SYNC_WORD,
    .hdr.length = sizeof(T_APP_CFG_NV),
    .single_tone_timeout_val = 20 * 1000, //20s
    .single_tone_tx_gain = 0,
    .single_tone_channel_num = 20,
    .anc_apt_state = 0,
    .apt_volume_out_level = 6,
    .app_pair_idx_mapping = {0, 1, 2, 3, 4, 5, 6, 7},
#if F_APP_LE_AUDIO_SM
    .bis_audio_gain_level = 0,
#endif
	.spp_disable_tongle_flag = 0,
};

const uint8_t codec_low_latency_table[9][LOW_LATENCY_LEVEL_MAX] =
{
    /* LOW_LATENCY_LEVEL1: 0;   LOW_LATENCY_LEVEL2: 1;  LOW_LATENCY_LEVEL3: 2;  LOW_LATENCY_LEVEL4: 3;  LOW_LATENCY_LEVEL5: 4. */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_PCM: 0 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_CVSD: 1 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_MSBC: 2 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_SBC: 3 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_AAC: 4 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_OPUS: 5 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_FLAC: 6 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_MP3: 7 */
    {40,                        60,                     80,                     100,                    120},      /* AUDIO_FORMAT_TYPE_LC3: 8 */
};

#if F_APP_ERWS_SUPPORT
extern const uint8_t sniffing_params_low_latency_table[3][LOW_LATENCY_LEVEL_MAX];
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_cfg_reset_hook)(void) = NULL;
void (*app_cfg_reset_name_hook)(void) = NULL;
#endif

T_APP_CFG_CONST app_cfg_const;
T_EXTEND_APP_CFG_CONST extend_app_cfg_const;
T_APP_CFG_NV app_cfg_nv;

uint32_t app_cfg_reset(void)
{
    uint8_t temp_bd_addr[6];
    uint8_t temp_device_name_legacy[40];
    uint8_t temp_device_name_le[40];
    uint8_t temp_reset_power_on;
    uint8_t temp_reset_done;
    uint32_t temp_sync_word;
    uint8_t temp_power_off_cause_cmd = 0;
    uint8_t temp_le_single_random_addr[6] = {0};
    uint8_t temp_le_rws_random_addr[6] = {0};
    uint8_t temp_one_wire_aging_test_done = 0;
    uint8_t temp_one_wire_start_force_engage = 0;
    uint32_t temp_total_playback_time = 0;
    uint32_t temp_total_power_on_time = 0;
    uint8_t temp_xtal_k_times = 0;
    uint8_t temp_vbat_detect_normal = 0;
    uint32_t temp_vbat_ntc_value = 0;
	uint8_t temp_spp_disable_tongle_flag = 0;
#if HARMAN_VBAT_ONE_ADC_DETECTION	
	uint32_t temp_nv_saved_vbat_value;
	uint32_t temp_nv_saved_vbat_ntc_value;
	uint32_t temp_nv_saved_battery_err;
	uint32_t temp_nv_ntc_resistance_type;
	int temp_nv_ntc_vbat_temperature;
#endif

    //Keep for restore
    temp_reset_power_on = app_cfg_nv.adp_factory_reset_power_on;
    if (app_cfg_nv.adp_factory_reset_power_on && app_cfg_nv.power_off_cause_cmd)
    {
        temp_power_off_cause_cmd = app_cfg_nv.power_off_cause_cmd;
    }

    temp_reset_done = app_cfg_nv.factory_reset_done;
    temp_sync_word = app_cfg_nv.hdr.sync_word;
    temp_one_wire_aging_test_done = app_cfg_nv.one_wire_aging_test_done;
    temp_one_wire_start_force_engage = app_cfg_nv.one_wire_start_force_engage;
    temp_xtal_k_times = app_cfg_nv.xtal_k_times;
    temp_vbat_detect_normal = app_cfg_nv.vbat_detect_normal;
    temp_vbat_ntc_value = app_cfg_nv.vbat_ntc_value;

	temp_spp_disable_tongle_flag = app_cfg_nv.spp_disable_tongle_flag;

    memcpy(temp_bd_addr, app_cfg_nv.bud_peer_factory_addr, 6);
    memcpy(temp_device_name_legacy, app_cfg_nv.device_name_legacy, 40);
    memcpy(temp_device_name_le, app_cfg_nv.device_name_le, 40);
    //memcpy(temp_le_single_random_addr, app_cfg_nv.le_single_random_addr, 6);
    //memcpy(temp_le_rws_random_addr, app_cfg_nv.le_rws_random_addr, 6);

    temp_total_playback_time = app_cfg_nv.total_playback_time;
    temp_total_power_on_time = app_cfg_nv.total_power_on_time;
#if HARMAN_VBAT_ONE_ADC_DETECTION	
	temp_nv_saved_vbat_value = app_cfg_nv.nv_saved_vbat_value;
	temp_nv_saved_vbat_ntc_value = app_cfg_nv.nv_saved_vbat_ntc_value;
	temp_nv_saved_battery_err = app_cfg_nv.nv_saved_battery_err;
	temp_nv_ntc_resistance_type = app_cfg_nv.nv_ntc_resistance_type;
	temp_nv_ntc_vbat_temperature = app_cfg_nv.nv_ntc_vbat_temperature;
#endif	

    {
        memcpy(&app_cfg_nv, &app_cfg_rw_default, sizeof(T_APP_CFG_NV));
    }

#if HARMAN_VBAT_ONE_ADC_DETECTION	
	app_cfg_nv.nv_saved_vbat_value = temp_nv_saved_vbat_value;
	app_cfg_nv.nv_saved_vbat_ntc_value = temp_nv_saved_vbat_ntc_value;
	app_cfg_nv.nv_saved_battery_err = temp_nv_saved_battery_err;
	app_cfg_nv.nv_ntc_resistance_type = temp_nv_ntc_resistance_type;
	app_cfg_nv.nv_ntc_vbat_temperature = temp_nv_ntc_vbat_temperature;
#endif

    app_cfg_nv.total_playback_time = temp_total_playback_time;
    app_cfg_nv.total_power_on_time = temp_total_power_on_time;

    //memcpy(app_cfg_nv.le_single_random_addr, temp_le_single_random_addr, 6);
    //memcpy(app_cfg_nv.le_rws_random_addr, temp_le_rws_random_addr, 6);

    app_cfg_nv.adp_factory_reset_power_on = temp_reset_power_on;
    if (app_cfg_nv.adp_factory_reset_power_on && temp_power_off_cause_cmd)
    {
        app_cfg_nv.power_off_cause_cmd = temp_power_off_cause_cmd;
    }

    app_cfg_nv.factory_reset_done = temp_reset_done;
    memcpy(app_cfg_nv.bud_peer_factory_addr, temp_bd_addr, 6);
    memcpy(app_cfg_nv.device_name_legacy, temp_device_name_legacy, 40);
    memcpy(app_cfg_nv.device_name_le, temp_device_name_le, 40);

    app_cfg_nv.one_wire_aging_test_done = temp_one_wire_aging_test_done;
    app_cfg_nv.one_wire_start_force_engage = temp_one_wire_start_force_engage;
    app_cfg_nv.xtal_k_times = temp_xtal_k_times;
    app_cfg_nv.vbat_detect_normal = temp_vbat_detect_normal;
    app_cfg_nv.vbat_ntc_value = temp_vbat_ntc_value;

	app_cfg_nv.spp_disable_tongle_flag = temp_spp_disable_tongle_flag;
    {
        uint8_t state_of_charge = app_charger_get_soc();
        //Charger module report 0xFF when ADC not ready
        if (state_of_charge <= 100)
        {
            app_cfg_nv.local_level = state_of_charge;
        }
    }

    if ((app_cfg_const.enable_not_reset_device_name == 0) ||
        (temp_sync_word != DATA_SYNC_WORD))  //First init
    {
#if (F_APP_AVP_INIT_SUPPORT == 1)
        if (app_cfg_reset_name_hook)
        {
            app_cfg_reset_name_hook();
        }
#else
        memcpy(&app_cfg_nv.device_name_legacy[0], &app_cfg_const.device_name_legacy_default[0], 40);
#endif

        memcpy(&app_cfg_nv.device_name_le[0], &app_cfg_const.device_name_le_default[0], 40);

        // update device name when factory reset
        bt_local_name_set(&app_cfg_nv.device_name_legacy[0], GAP_DEVICE_NAME_LEN);
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, &app_cfg_nv.device_name_le[0]);

        if (temp_sync_word != DATA_SYNC_WORD)
        {
            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_cfg_nv.case_battery = (app_cfg_nv.case_battery & 0x80) | 0x50;
            }

            // Power on is not allowed for the first time when vbat in after factory reset by mppgtool;
            // Unlock disallow power on mode by MFB power on or slide switch power on or bud starts chargring
            app_cfg_nv.disallow_power_on_when_vbat_in = 1;

            app_cfg_nv.store_adc_ntc_value_when_vbat_in = 1;
        }
    }

    app_cfg_nv.bud_role = app_cfg_const.bud_role;
    app_cfg_nv.first_engaged = false;
    memcpy(app_cfg_nv.bud_peer_addr, app_cfg_const.bud_peer_addr, 6);
    memcpy(app_cfg_nv.bud_local_addr, app_db.factory_addr, 6);
    app_cfg_nv.app_is_power_on = 0;

#if F_APP_HARMAN_FEATURE_SUPPORT
    app_harman_cfg_reset();
#endif

    app_cfg_nv.apt_volume_out_level = app_cfg_const.apt_volume_out_default;
#if APT_SUB_VOLUME_LEVEL_SUPPORT
    app_apt_volume_update_sub_level();
#endif

    app_cfg_nv.voice_prompt_volume_out_level = app_cfg_const.voice_prompt_volume_default;

#if F_APP_ADJUST_TONE_VOLUME_SUPPORT
    app_cfg_nv.is_tone_volume_set_from_phone = 0;

    // set vp & tone volume to the same level
    app_cfg_nv.ringtone_volume_out_level = app_cfg_nv.voice_prompt_volume_out_level;
#else
    app_cfg_nv.ringtone_volume_out_level = app_cfg_const.ringtone_volume_default;
#endif

    app_cfg_nv.audio_effect_type = app_cfg_const.dual_audio_default;
    app_cfg_nv.dual_audio_effect_gaming_type = app_cfg_const.dual_audio_gaming;

    if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
    {
        if (app_adp_get_plug_state() == ADAPTOR_PLUG)
        {
            app_cfg_nv.adaptor_is_plugged = 1;
        }
        else
        {
            app_cfg_nv.adaptor_is_plugged = 0;
        }
    }
#if F_APP_GPIO_ONOFF_SUPPORT
    else if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        // 3pin location will get error if sync_word is not initialized
        if (app_device_is_in_the_box())
        {
            app_cfg_nv.pre_3pin_status_unplug = 0;
        }
        else
        {
            app_cfg_nv.pre_3pin_status_unplug = 1;
        }
    }
#endif

#if F_APP_IPHONE_ABS_VOL_HANDLE
    memset(&app_cfg_nv.abs_vol[0], iphone_abs_vol[app_cfg_const.playback_volume_default], 8);
#endif
    memset(&app_cfg_nv.audio_gain_level[0], app_cfg_const.playback_volume_default, 8);
    memset(&app_cfg_nv.voice_gain_level[0], app_cfg_const.voice_out_volume_default, 8);
    app_cfg_nv.line_in_gain_level = app_cfg_const.line_in_volume_out_default;

#if HARMAN_VP_DATA_HEADER_GET_SUPPORT
    app_harman_vp_data_header_get();
#else
    app_cfg_nv.voice_prompt_language = app_cfg_const.voice_prompt_language;
#endif
    app_cfg_nv.light_sensor_enable = !(app_cfg_const.light_sensor_default_disabled);
    app_cfg_nv.eq_idx_normal_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, NORMAL_MODE);
    app_cfg_nv.eq_idx_gaming_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, GAMING_MODE);
    app_cfg_nv.eq_idx_anc_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, ANC_MODE);
    app_cfg_nv.apt_eq_idx = eq_utils_get_default_idx(MIC_SW_EQ, APT_MODE);
#if F_APP_LINEIN_SUPPORT
    app_cfg_nv.eq_idx_line_in_mode_record = eq_utils_get_default_idx(SPK_SW_EQ, LINE_IN_MODE);
#endif
    app_cfg_nv.listening_mode_cycle = app_cfg_const.listening_mode_cycle;
    app_cfg_nv.rws_low_latency_level_record = app_cfg_const.rws_low_latency_level;
#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
    app_cfg_nv.rws_disallow_sync_apt_volume = app_cfg_const.rws_disallow_sync_apt_volume;
#endif
#if F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
    app_cfg_nv.time_delay_to_open_apt_when_power_on =
        app_cfg_const.time_delay_to_open_apt_when_power_on;
#endif

#if F_APP_BRIGHTNESS_SUPPORT
    app_cfg_nv.main_brightness_level = brightness_level_default;
    app_apt_brightness_update_sub_level();
    app_cfg_nv.rws_disallow_sync_llapt_brightness = app_cfg_const.rws_disallow_sync_llapt_brightness;
#endif
    app_sass_policy_reset();

    app_cfg_nv.enable_multi_link = (app_multilink_is_on_by_mmi() == true) ? 1 : 0;

    app_cfg_nv.either_bud_has_vol_ctrl_key = false;

    if (app_cfg_const.enable_enter_gaming_mode_after_power_on)
    {
        app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_gaming_mode_record;
    }
    else
    {
        app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;
    }
#if (F_APP_DUAL_AUDIO_EFFECT == 1)
    if (app_cfg_const.dual_audio_switch_mode && app_cfg_const.dual_audio_default == VENDOR_NONE_EFFECT)
    {
        app_cfg_nv.audio_effect_type = VENDOR_MUSIC_MODE;
    }
    APP_PRINT_ERROR3("app_cfg_reset: audio_effect_type =%d,gaming effect =%d,%d",
                     app_cfg_nv.audio_effect_type, app_cfg_nv.dual_audio_effect_gaming_type,
                     app_cfg_const.dual_audio_default);
    dual_audio_effect_reset();
    dual_audio_set_effect((DUAL_EFFECT_TYPE)app_cfg_nv.audio_effect_type, true);
    for (uint8_t i = 0; i < 16; i++)
    {
        dual_audio_set_app_key_val(i, 0x7fff);
    }
    dual_audio_save_app_key_val();
#endif

#if (ISOC_AUDIO_SUPPORT == 1)
    app_cfg_nv.bis_audio_gain_level  = app_cfg_const.playback_volume_default;
#endif

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
    app_slide_switch_power_on_off_gpio_status_reset();
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_cfg_reset_hook)
    {
        app_cfg_reset_hook();
    }
#endif

    app_cfg_nv.pin_code_size = app_cfg_const.pin_code_size;
    memcpy(&app_cfg_nv.pin_code[0], &app_cfg_const.pin_code[0], 8);

    app_cfg_nv.anc_selected_list = 0xFFFF;
    app_cfg_nv.llapt_selected_list = 0xFFFF;

    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

void app_cfg_load(void)
{
    uint32_t sync_word = 0;

    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUDATA) +
                        APP_CONFIG_OFFSET),
                       (uint8_t *)&sync_word, DATA_SYNC_WORD_LEN);

    if (sync_word == DATA_SYNC_WORD)
    {
        fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUDATA) +
                            APP_CONFIG_OFFSET),
                           (uint8_t *)&app_cfg_const, sizeof(T_APP_CFG_CONST));
    }
    else
    {
        app_wdg_reset(RESET_ALL);
    }

    app_dsp_cfg_init();
    app_dsp_cfg_init_gain_level();

    //read-write data
    ftl_load_from_storage(&app_cfg_nv.hdr, APP_RW_DATA_ADDR, sizeof(app_cfg_nv.hdr));

    if (app_cfg_nv.hdr.sync_word != DATA_SYNC_WORD)
    {
        //Load factory reset bit first when mppgtool factory reset
        if (app_cfg_nv.hdr.length == 0)
        {
            ftl_load_from_storage(&app_cfg_nv.eq_idx_anc_mode_record, APP_RW_DATA_ADDR + FACTORY_RESET_OFFSET,
                                  4);
        }

        app_cfg_reset();
    }
    else
    {
        uint32_t load_len = app_cfg_nv.hdr.length;

        if (load_len > sizeof(T_APP_CFG_NV))
        {
            APP_PRINT_ERROR0("app_cfg_load, error");
        }
        else
        {
            uint32_t res = ftl_load_from_storage(&app_cfg_nv, APP_RW_DATA_ADDR, load_len);

            if (res == 0)
            {
                if (load_len < sizeof(T_APP_CFG_NV))
                {
                    uint8_t *p_dst = ((uint8_t *)&app_cfg_nv) + load_len;
                    uint8_t *p_src = ((uint8_t *)&app_cfg_rw_default) + load_len;
                    memcpy(p_dst, p_src, sizeof(T_APP_CFG_NV) - load_len);
                }
                app_cfg_nv.hdr.length = sizeof(T_APP_CFG_NV);
            }
            else
            {
                app_cfg_reset();
            }
        }
    }

#if F_APP_ERWS_SUPPORT
    app_cfg_const.recovery_link_a2dp_interval = A2DP_INTERVAL;
    app_cfg_const.recovery_link_a2dp_rsvd_slot = A2DP_CTRL_RSVD_SLOT;

    app_cfg_const.recovery_link_a2dp_interval_gaming_mode =
        sniffing_params_low_latency_table[0][app_cfg_nv.rws_low_latency_level_record];
    app_cfg_const.recovery_link_a2dp_flush_timeout_gaming_mode =
        sniffing_params_low_latency_table[1][app_cfg_nv.rws_low_latency_level_record];
    app_cfg_const.recovery_link_a2dp_rsvd_slot_gaming_mode =
        sniffing_params_low_latency_table[2][app_cfg_nv.rws_low_latency_level_record];
#endif

    app_cfg_const.timer_link_avrcp = 1500;

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    if (app_cfg_nv.ota_tooling_start)
    {
        app_cfg_const.box_detect_method = 0;
        // disable sensor
        app_cfg_const.sensor_support = 0;
        // disalbe auto power off
        app_cfg_const.timer_auto_power_off = 0xFFFF;

        app_cfg_const.enable_power_on_linkback = 0;
        app_cfg_const.enable_disconneted_enter_pairing = 0;

        app_cfg_const.normal_apt_support = 0;
    }
#endif

#if (F_APP_MULTI_LINK_ENABLE)
    if ((app_cfg_const.enable_multi_link == 0) && (app_cfg_nv.enable_multi_link != 0))
    {
        app_cfg_const.enable_multi_link = 1;
#if (F_APP_AVP_INIT_SUPPORT == 1)
        app_cfg_const.enter_pairing_while_only_one_device_connected = 1;
        app_cfg_const.timer_pairing_while_one_conn_timeout = 30;
#endif
    }

    if (app_cfg_const.enable_multi_link)
    {
        app_cfg_const.max_legacy_multilink_devices = 2;
        app_cfg_const.enable_always_discoverable = 0;
    }
#endif

#if F_APP_QOL_MONITOR_SUPPORT
    app_cfg_const.enable_link_monitor_roleswap = 1;
    app_cfg_const.roleswap_rssi_threshold = 8;
    app_cfg_const.rssi_roleswap_judge_timeout = 1;
    app_cfg_const.enable_low_bat_role_swap = 0;
    app_cfg_const.enable_high_batt_primary = 0;
#endif
}

void extend_app_cfg_load(void)
{
    uint32_t sync_word = 0;

    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUDATA) +
                        EXTEND_APP_CONFIG_OFFSET),
                       (uint8_t *)&sync_word, DATA_SYNC_WORD_LEN);
    if (sync_word == DATA_SYNC_WORD)
    {
        fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUDATA) +
                            EXTEND_APP_CONFIG_OFFSET),
                           (uint8_t *)&extend_app_cfg_const, sizeof(T_EXTEND_APP_CFG_CONST));
    }
    else
    {
        app_wdg_reset(RESET_ALL);
    }

#if BISTO_FEATURE_SUPPORT
    extend_app_cfg_const.bisto_support = 1;
#endif

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    if (app_cfg_nv.ota_tooling_start)
    {
        extend_app_cfg_const.gfps_support = 0;
    }
#endif

#if 0
    extend_app_cfg_const.gfps_finder_support = 1;
    extend_app_cfg_const.disable_finder_adv_when_power_off = 0;
    extend_app_cfg_const.gfps_power_on_finder_adv_interval = 800;
    extend_app_cfg_const.gfps_power_off_finder_adv_interval = 1600;
    extend_app_cfg_const.gfps_power_off_start_finder_adv_timer_timeout_value = 120;
    extend_app_cfg_const.gfps_power_off_finder_adv_duration = 120;
    extend_app_cfg_const.power_off_rtc_wakeup_timeout = 60;
    extend_app_cfg_const.gfps_finder_adv_skip_count_when_wakeup = 0;
#endif
    DBG_DIRECT("extend_app_cfg_load: GFPS fineder Support %d, %d; param %d, %d, %d, %d, %d, %d",
               extend_app_cfg_const.gfps_finder_support,
               extend_app_cfg_const.disable_finder_adv_when_power_off,
               extend_app_cfg_const.gfps_power_on_finder_adv_interval,
               extend_app_cfg_const.gfps_power_off_finder_adv_interval,
               extend_app_cfg_const.gfps_power_off_start_finder_adv_timer_timeout_value,
               extend_app_cfg_const.gfps_power_off_finder_adv_duration,
               extend_app_cfg_const.power_off_rtc_wakeup_timeout,
               extend_app_cfg_const.gfps_finder_adv_skip_count_when_wakeup
              );

}

bool app_cfg_load_led_table(void *p_data, uint8_t mode, uint16_t led_table_size)
{
    uint16_t sync_word = 0;
    fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUDATA) +
                        APP_LED_OFFSET),
                       (uint8_t *)&sync_word, APP_DATA_SYNC_WORD_LEN);
    if (sync_word == APP_DATA_SYNC_WORD)
    {
        fmc_flash_nor_read((flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUDATA) +
                            APP_LED_OFFSET + APP_DATA_SYNC_WORD_LEN + (mode * led_table_size)),
                           (uint8_t *)p_data, led_table_size);
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t app_cfg_store_all(void)
{
    return ftl_save_to_storage(&app_cfg_nv, APP_RW_DATA_ADDR, sizeof(T_APP_CFG_NV));
}

uint32_t app_cfg_store(void *pdata, uint16_t size)
{
    uint16_t offset = (uint8_t *)pdata - (uint8_t *)&app_cfg_nv;

    if ((offset % 4) != 0)
    {
        uint16_t old_offset = offset;

        pdata = (uint8_t *)pdata - (offset % 4);
        offset -= (offset % 4);
        size += (old_offset % 4);
    }

    if ((size % 4) != 0)
    {
        size = (size / 4 + 1) * 4;
    }

    APP_PRINT_TRACE2("app_cfg_store: off 0x%x size %d", offset, size);

    return ftl_save_to_storage(pdata, offset + APP_RW_DATA_ADDR, size);
}

void app_cfg_init(void)
{
    app_cfg_load();
    extend_app_cfg_load();
#if F_APP_TUYA_SUPPORT
    extend_app_cfg_const.tuya_support = 1;//danni for test
    extend_app_cfg_const.tuya_adv_timeout = 0;
#endif
#if F_APP_POWER_TEST
    app_cfg_const.key_table[SHORT][BT_HFP_CALL_IDLE][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_VOICE_ACTIVATION_ONGOING][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_INCOMING_CALL_ONGOING][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_OUTGOING_CALL_ONGOING][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_CALL_ACTIVE][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD][KEY6]    = MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT][KEY6]    =
        MMI_START_ROLESWAP;
    app_cfg_const.key_table[SHORT][BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD][KEY6]    =
        MMI_START_ROLESWAP;

    APP_PRINT_INFO2("app_cfg_init: IDLE Key6 -> %d, Call Active Key6 -> %d",
                    app_cfg_const.key_table[SHORT][BT_HFP_CALL_IDLE][KEY6],
                    app_cfg_const.key_table[SHORT][BT_HFP_CALL_ACTIVE][KEY6]);
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
    app_slide_switch_cfg_init();
#endif

#if HARMAN_VP_DATA_HEADER_GET_SUPPORT
    app_harman_vp_data_header_get();
#endif
}

