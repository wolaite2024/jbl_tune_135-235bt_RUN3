/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "wdg.h"
#include "sysm.h"
#include "gap_timer.h"
#include "ringtone.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_mmi.h"
#include "audio_volume.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_bt_policy_api.h"
#include "app_in_out_box.h"
#include "app_audio_policy.h"
#include "app_dlps.h"
#include "app_adp.h"
#include "app_key_process.h"
#include "string.h"
#include "app_bud_loc.h"
#include "app_led.h"
#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif

/* BBPro2 specialized feature */
#if (F_APP_HEARABLE_SUPPORT == 1)
#include "app_listening_mode.h"
#endif
// end of BBPro2 specialized feature

#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#include "anc.h"
#include "anc_tuning.h"
#endif


#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
#include "app_one_wire_uart.h"
#endif


typedef enum
{
    APP_IN_OUT_BOX_IGNORE_NONE,
    APP_IN_OUT_BOX_IGNORE_BY_OTA_TOOLING,
    APP_IN_OUT_BOX_IGNORE_BY_ADP_FACTORY_RESET,
    APP_IN_OUT_BOX_IGNORE_BY_DISABLE_OUTBOX_POWER_ON,
} T_APP_IN_OUT_BOX_IGNORE_REASON;

void app_in_out_box_handle(T_CASE_LOCATION_STATUS local)
{
    T_APP_IN_OUT_BOX_IGNORE_REASON reason = APP_IN_OUT_BOX_IGNORE_NONE;
    app_key_stop_timer();

    if (local == OUT_CASE)
    {
        APP_PRINT_TRACE0("app_in_out_box_handle: OUT_CASE");

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_const.one_wire_uart_support)
        {
            app_one_wire_deinit();
        }
#endif

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
        if (app_cfg_nv.ota_tooling_start)
        {
            reason = APP_IN_OUT_BOX_IGNORE_BY_OTA_TOOLING;
            goto exit;
        }
#endif

        if (app_mmi_reboot_check_timer_started())
        {
            if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
            {
                //app_cfg_nv.adaptor_is_plugged is cleared in app_adp_out_send_msg
                //before open case watchdog reset.
                //Keep adaptor_is_plugged to correct condition
                app_cfg_nv.adaptor_is_plugged = 1;
            }
#if F_APP_GPIO_ONOFF_SUPPORT
            else if (app_cfg_const.box_detect_method == GPIO_DETECT)
            {
                //Should be able to power on during power off ing after debounce timeout
                //Keep pre_3pin_status_unplug to correct condition
                app_cfg_nv.pre_3pin_status_unplug = 0;
            }
#endif
        }

        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);

        if (app_cfg_nv.adp_factory_reset_power_on)
        {
            reason = APP_IN_OUT_BOX_IGNORE_BY_ADP_FACTORY_RESET;
            goto exit;
        }

        if (!app_cfg_const.enable_outbox_power_on)
        {
            reason = APP_IN_OUT_BOX_IGNORE_BY_DISABLE_OUTBOX_POWER_ON;
            app_dlps_start_power_down_wdg_timer();
            goto exit;
        }

        if (app_cfg_nv.factory_reset_done)
        {
            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_adp_power_on_when_out_case();
            }
            else
            {
                app_mmi_handle_action(MMI_DEV_POWER_ON);
            }
        }
        else
        {
            APP_PRINT_TRACE0("app_in_out_box_handle: do nothing");
        }
    }
    else
    {
        APP_PRINT_TRACE0("app_in_out_box_handle: IN_CASE");
        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_const.one_wire_uart_support)
        {
            app_one_wire_init();
        }
#endif

#if F_APP_ANC_SUPPORT
        if (anc_tool_check_resp_meas_mode() != ANC_RESP_MEAS_MODE_NONE)
        {
            anc_tool_trigger_wdg_reset(ANC_RESP_MEAS_MODE_NONE);
        }
#endif

#if (F_APP_AIRPLANE_SUPPORT == 1)
        if (app_cfg_const.enable_rtk_charging_box)
        {
            app_airplane_in_box_handle();
        }
#endif

        //Enable immediately blink charging led and stop vp when adp in during power off
        led_disable_non_repeat_mode();
        app_audio_tone_flush(false);
        app_audio_tone_type_stop();

        /* BBPro2 specialized feature */
#if (F_APP_HEARABLE_SUPPORT == 1)
        app_listening_state_machine(EVENT_APT_OFF, false, true);
#endif
        // end of BBPro2 specialized feature

        if (app_db.device_state == APP_DEVICE_STATE_ON)
        {
            audio_volume_out_mute(AUDIO_STREAM_TYPE_PLAYBACK);
            audio_volume_out_mute(AUDIO_STREAM_TYPE_VOICE);
            audio_volume_in_mute(AUDIO_STREAM_TYPE_RECORD);
        }
    }

    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);

exit:
    if (reason != 0)
    {
        APP_PRINT_TRACE1("app_in_out_box_handle: ignore by reason %d", reason);
    }

}

static void app_in_out_box_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{

    switch (event_type)
    {
    default:
        break;
    }
}

void app_in_out_box_init(void)
{
    bt_mgr_cback_register(app_in_out_box_cback);
}
