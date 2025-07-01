/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "trace.h"
#include "board.h"
#include "rtl876x.h"
#include "wdg.h"
#include "led_module.h"
#include "gap_timer.h"
#include "app_msg.h"
#include "app_cfg.h"
#include "app_gpio.h"
#include "app_io_msg.h"
#include "app_main.h"
#include "app_charger.h"
#include "app_led.h"
#include "app_dlps.h"
#include "app_key_process.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_mmi.h"
#include "app_relay.h"
#include "sysm.h"
#include "remote.h"
#include "app_in_out_box.h"
#include "bt_hfp.h"
#include "rtk_charger.h"

#if BISTO_FEATURE_SUPPORT
#include "app_bisto_battery.h"
#endif

#include "app_audio_policy.h"
#include "app_adp.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_hfp.h"
#include "app_auto_power_off.h"
#include "app_sensor.h"
#include "app_bt_policy_api.h"
#include "app_bud_loc.h"
#if F_APP_ADC_SUPPORT
#include "app_adc.h"
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_asp_device.h"
#include "app_teams_audio_policy.h"
#endif
#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
#include "app_rtk_fast_pair_adv.h"
#endif

#if GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#include "app_gfps_battery.h"
#endif

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
#include "app_one_wire_uart.h"
#endif
#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
#endif
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
#include "app_ext_charger.h"
#endif
#if HARMAN_NTC_DETECT_PROTECT
#include "app_harman_adc.h"
#endif

#define RTK_CHARGER_ENABLE      1

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_charger_box_trig_hook)(void) = NULL;
void (*app_charger_box_stop_hook)(void) = NULL;
void (*app_charger_box_reset_hook)(void) = NULL;
#endif


static uint8_t charger_timer_queue_id = 0;
static void *timer_handle_low_bat_warning = NULL;
static void *timer_handle_low_bat_led = NULL;
static uint8_t battery_status = BATTERY_STATUS_NORMAL;
static uint8_t low_batt_warning_count = 1;
static uint8_t low_batt_led_count = 1;
static bool invalid_batt_rec = false;

#if (F_APP_SMOOTH_BAT_REPORT == 1)
#define BAT_HISTORY_SIZE   10
#define BAT_SMOOTH_TIMEOUT 10

typedef enum
{
    APP_SMOOTH_BAT_VOL_CHANGE,
    APP_SMOOTH_BAT_VOL_NO_CHANGE,
    APP_SMOOTH_BAT_VOL_DISALLOW,
} T_APP_SMOOTH_BAT_VOL;

static void app_handle_state_of_charge(uint8_t state_of_charge, uint8_t first_power_on);
uint8_t bat_history[BAT_HISTORY_SIZE] = {0};
static uint8_t min_discharge_bat_vol = 0;
static uint8_t last_report_discharge_bat_vol = 0;
static void *timer_handle_smooth_bat_report = NULL;
static void *timer_handle_delay_low_batt_poweroff = NULL;

static void app_charger_bat_history_clear(void)
{
    uint8_t i = 0;

    for (i = 0; i < BAT_HISTORY_SIZE; i++)
    {
        bat_history[i] = 0;
    }
}

static void app_charger_clear_smooth_bat_handle(void)
{
    gap_stop_timer(&timer_handle_smooth_bat_report);
    min_discharge_bat_vol = 0;
    last_report_discharge_bat_vol = 0;
}

static void app_charger_bat_history_enqueue(uint8_t bat_vol)
{
    uint8_t i = 0;
    uint8_t j = 0;

    for (i = 0; i < BAT_HISTORY_SIZE; i++)
    {
        if (bat_history[i] == 0)
        {
            bat_history[i] = bat_vol;
            break;
        }
    }

    if (i == BAT_HISTORY_SIZE)
    {
        for (j = 1; j < BAT_HISTORY_SIZE; j++)
        {
            bat_history[j - 1] = bat_history[j];
        }

        bat_history[BAT_HISTORY_SIZE - 1] = bat_vol;
    }
}

static uint8_t app_charger_bat_discharge_average(void)
{
    uint8_t i = 0;
    uint8_t cnt = 0;
    uint32_t bat_total = 0;
    uint8_t bat_average = 0;

    for (i = 0; i < BAT_HISTORY_SIZE; i++)
    {
        if (bat_history[i])
        {
            bat_total += bat_history[i];
            cnt++;
        }
        else
        {
            break;
        }
    }

    if (cnt != 0)
    {
        bat_average = (bat_total / cnt);
    }

    APP_PRINT_TRACE1("app_charger_bat_discharge_average: %d", bat_average);

    return bat_average;
}

static T_APP_SMOOTH_BAT_VOL app_charger_smooth_bat_discharge(uint8_t origin_vol, uint8_t *new_vol)
{
    T_APP_SMOOTH_BAT_VOL report = APP_SMOOTH_BAT_VOL_NO_CHANGE;
    APP_CHARGER_STATE charger_state = app_charger_get_charge_state();
    static uint8_t last_valid_discharge_vol = 0;
    static uint8_t ignore_cnt = 0;

    /* clear bat history when non discharge */
    if (charger_state != APP_CHARGER_STATE_NO_CHARGE)
    {
        last_valid_discharge_vol = 0;
        ignore_cnt = 0;

        app_charger_bat_history_clear();
    }
    else
    {
        if (last_valid_discharge_vol != 0)
        {
            if (((origin_vol < last_valid_discharge_vol) && ((last_valid_discharge_vol - origin_vol) > 3)) &&
                (ignore_cnt <= 2))
            {
                /* ignore this battery change */
                ignore_cnt++;
                report = APP_SMOOTH_BAT_VOL_DISALLOW;
                goto exit;
            }
        }

        ignore_cnt = 0;

        app_charger_bat_history_enqueue(origin_vol);
    }


    if (charger_state == APP_CHARGER_STATE_NO_CHARGE) //discharge
    {
        last_valid_discharge_vol = origin_vol;

        /* get bat average */
        *new_vol = app_charger_bat_discharge_average();
        report = APP_SMOOTH_BAT_VOL_CHANGE;

        if (min_discharge_bat_vol == 0) // first time get discharge bat vol
        {
            min_discharge_bat_vol = *new_vol;
            last_report_discharge_bat_vol = *new_vol;
        }
        else
        {
            if (*new_vol <= min_discharge_bat_vol)
            {
                if (*new_vol < min_discharge_bat_vol)
                {
                    min_discharge_bat_vol = *new_vol;
                }

                if (timer_handle_smooth_bat_report == NULL)
                {
                    /* start a timer to smooth bat consumption report */
                    gap_start_timer(&timer_handle_smooth_bat_report,
                                    "smooth_bat_report", charger_timer_queue_id,
                                    APP_IO_TIMER_SMOOTH_BAT_REPORT, 0, BAT_SMOOTH_TIMEOUT * 1000);
                }
            }
            else
            {
                /* ignore ping-ping effect */
            }
            report = APP_SMOOTH_BAT_VOL_DISALLOW;
        }
    }
    else
    {
        app_charger_clear_smooth_bat_handle();
    }

exit:
    return report;
}
#endif

static APP_CHARGER_STATE rtk2app_charger_state_convert(T_CHARGER_STATE rtk_charger_state)
{
    APP_CHARGER_STATE app_charger_state = APP_CHARGER_STATE_ERROR;

    switch (rtk_charger_state)
    {
    case STATE_CHARGER_START:
    case STATE_CHARGER_PRE_CHARGE:
    case STATE_CHARGER_FAST_CHARGE:
        app_charger_state = APP_CHARGER_STATE_CHARGING;
        break;

    case STATE_CHARGER_FINISH:
        app_charger_state = APP_CHARGER_STATE_CHARGE_FINISH;
        break;

    case STATE_CHARGER_ERROR:
        app_charger_state = APP_CHARGER_STATE_ERROR;
        break;

    case STATE_CHARGER_END:
        app_charger_state = APP_CHARGER_STATE_NO_CHARGE;
        break;

    default:
        break;
    }

    return app_charger_state;
}


void common_charge_state_send_msg(APP_CHARGER_STATE app_charger_state)
{
    T_IO_MSG adp_msg;

    if (app_cfg_const.led_support && (app_db.disallow_charging_led == 0))
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_CHARGER);
    }

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_CHARGER;
    adp_msg.u.param = CHG_MSG_CHARGER_STATE_CHANGE + (app_charger_state << 8);


    if (!app_io_send_msg(&adp_msg))
    {
        APP_PRINT_ERROR0("app_charger charge_state_send_msg_for_rtk: Send charger state msg error");
    }
}


static void common_soc_send_msg(uint8_t soc)
{
    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_CHARGER;
    adp_msg.u.param = CHG_MSG_STATE_OF_CHARGE_CHANGE + (soc << 8);

    APP_PRINT_TRACE1("app_charger soc_send_msg_for_rtk: state_of_charge %d", soc);

    if (!app_io_send_msg(&adp_msg))
    {
        APP_PRINT_ERROR0("app_charger soc_send_msg_for_rtk: Send state of charge msg error");
    }
}

void app_charger_enable_low_bat_warning(void)
{
#if HARMAN_LOW_BAT_WARNING_TIME_SET_SUPPORT
    gap_start_timer(&timer_handle_low_bat_warning, "low_bat_warning",
                    charger_timer_queue_id, APP_IO_TIMER_LOW_BAT_WARNING, 0,
                    (HARMAN_LOW_BAT_WARNING_TIME * 1000));
#else
    uint8_t multiplier = LOW_BAT_WARNING_TO_MULTIPLIER_SECOND;

    if (app_cfg_const.enable_new_low_bat_time_unit)
    {
        multiplier = LOW_BAT_WARNING_TO_MULTIPLIER_MINUTE;
    }

    gap_start_timer(&timer_handle_low_bat_warning, "low_bat_warning",
                    charger_timer_queue_id, APP_IO_TIMER_LOW_BAT_WARNING, 0,
                    (app_cfg_const.timer_low_bat_warning * multiplier * 1000));
#endif
}

void app_charger_disable_low_bat_warning(void)
{
    gap_stop_timer(&timer_handle_low_bat_warning);
}


/**
    * @brief  charger related timer timeout process
    * @note
    * @param  timer_id distinguish io timer type
    * @param  timer_chann indicate which timer channel
    * @return void
    */
static void app_charger_timer_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("charger_timer_callback: timer_id 0x%02x, timer_chann %d ",
                     timer_id, timer_chann);
    switch (timer_id)
    {
    case APP_IO_TIMER_LOW_BAT_WARNING:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                /* BBPRO2BUG-2774, timer is running on upperstack task and send interrupt msg to app task,
                so if timer timeout is little, when power off action running, timeout msg maybe has send to app task,
                there is happen power off action disable timer, but app task would run timer callback, */
                APP_PRINT_WARN0("discharger_timer_callback: device state is off, callback not run");
                return;
            }

            APP_CHARGER_STATE app_charger_state;
//            uint8_t bat_level;

            app_charger_disable_low_bat_warning();
            app_charger_state = app_charger_get_charge_state();

            if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE)
            {
                if (battery_status == BATTERY_STATUS_LOW)
                {
//                    bat_level = (app_db.local_batt_level + 9) / 10;

                    if (app_cfg_const.low_bat_warning_count == 0)
                    {
#if F_APP_HARMAN_FEATURE_SUPPORT
                        app_charger_harman_low_bat_warning_vp(app_db.local_batt_level, __func__, __LINE__);
#else
                        app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                 false);
#endif
                        app_charger_enable_low_bat_warning();
                    }
                    else
                    {
                        if (low_batt_warning_count < app_cfg_const.low_bat_warning_count)
                        {
#if F_APP_HARMAN_FEATURE_SUPPORT
                            app_charger_harman_low_bat_warning_vp(app_db.local_batt_level, __func__, __LINE__);
#else
                            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                     false);
#endif
                            app_charger_enable_low_bat_warning();

                            low_batt_warning_count += 1;
                        }
                    }
                }
            }
        }
        break;

    case APP_IO_TIMER_LOW_BAT_LED:
        {
            if (app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                /* BBPRO2BUG-2774, timer is running on upperstack task and send interrupt msg to app task,
                so if timer timeout is little, when power off action running, timeout msg maybe has send to app task,
                there is happen power off action disable timer, but app task would run timer callback, */
                APP_PRINT_WARN0("discharger_timer_callback: device state is off, callback not run");
                return;
            }

            APP_CHARGER_STATE app_charger_state;

            gap_stop_timer(&timer_handle_low_bat_led);
            app_charger_state = app_charger_get_charge_state();
            if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE)
            {
                if (battery_status == BATTERY_STATUS_LOW)
                {
                    if (app_cfg_const.low_bat_warning_count == 0)
                    {
                        led_change_mode(LED_MODE_LOW_BATTERY, true, false);
                        gap_start_timer(&timer_handle_low_bat_led, "low_bat_led",
                                        charger_timer_queue_id, APP_IO_TIMER_LOW_BAT_LED, 0,
                                        (app_cfg_const.timer_low_bat_led * 1000));
                    }
                    else
                    {
                        if (low_batt_led_count < app_cfg_const.low_bat_warning_count)
                        {
                            led_change_mode(LED_MODE_LOW_BATTERY, true, false);
                            gap_start_timer(&timer_handle_low_bat_led, "low_bat_led",
                                            charger_timer_queue_id, APP_IO_TIMER_LOW_BAT_LED, 0,
                                            (app_cfg_const.timer_low_bat_led * 1000));

                            low_batt_led_count += 1;
                        }
                    }
                }
            }
        }
        break;

#if (F_APP_SMOOTH_BAT_REPORT == 1)
    case APP_IO_TIMER_SMOOTH_BAT_REPORT:
        {
            app_handle_state_of_charge(last_report_discharge_bat_vol, 0);

            if (last_report_discharge_bat_vol > min_discharge_bat_vol)
            {
                gap_start_timer(&timer_handle_smooth_bat_report,
                                "smooth_bat_report", charger_timer_queue_id,
                                APP_IO_TIMER_SMOOTH_BAT_REPORT, 0, BAT_SMOOTH_TIMEOUT * 1000);

                if (last_report_discharge_bat_vol > 0)
                {
                    last_report_discharge_bat_vol--;
                }
            }
            else
            {
                gap_stop_timer(&timer_handle_smooth_bat_report);
            }
        }
        break;
#endif

    case APP_IO_TIMER_DELAY_LOW_BATT_POWEROFF:
        {
            gap_stop_timer(&timer_handle_delay_low_batt_poweroff);
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
        break;

    default:
        break;
    }
}

static void charge_state_send_msg_for_rtk(T_CHARGER_STATE rtk_charger_state)
{
    APP_CHARGER_STATE app_charger_state = APP_CHARGER_STATE_ERROR;

    app_charger_state = rtk2app_charger_state_convert(rtk_charger_state);

    APP_PRINT_TRACE2("app_charger charge_state_send_msg_for_rtk: rtk_charger_state %d, app_charger_state %d",
                     rtk_charger_state, app_charger_state);

    common_charge_state_send_msg(app_charger_state);
}

static void soc_send_msg_for_rtk(uint8_t state_of_charge)
{
    common_soc_send_msg(state_of_charge);
}


static void app_handle_charger_state(APP_CHARGER_STATE app_charger_state)
{
#if HARMAN_EXTERNAL_CHARGER_SUPPORT
    app_db.local_charger_state = app_charger_state;
#endif
    if (app_charger_state == APP_CHARGER_STATE_CHARGING)
    {
        app_device_unlock_vbat_disallow_power_on();
    }

    if (app_db.device_state != APP_DEVICE_STATE_OFF)
    {
        app_db.local_charger_state = app_charger_state;
        app_battery_evt_handle(APP_CHARGER_EVENT_BATT_STATE_CHANGE, 0, app_db.local_charger_state);

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

    APP_PRINT_TRACE1("app_handle_charger_state = %d", app_db.local_charger_state);
#if HARMAN_NTC_DETECT_PROTECT && F_APP_EXT_CHARGER_FEATURE_SUPPORT
    if (app_db.local_charger_state != APP_CHARGER_STATE_NO_CHARGE)
    {
        app_harman_adc_io_read();
        app_harman_adc_ntc_check_timer_start(CHARGING_NTC_CHECK_PERIOD);
    }
#endif

#if HARMAN_DISCHARGER_NTC_DETECT_PROTECT
    if (app_db.local_charger_state == APP_CHARGER_STATE_NO_CHARGE)
    {
        app_harman_discharger_ntc_timer_start();
    }
#endif

    if (app_cfg_const.charger_control_by_mcu)
    {
        if (app_charger_state == APP_CHARGER_STATE_CHARGE_FINISH)
        {
#if F_APP_ADC_SUPPORT
            app_adc_stop_monitor_adp_voltage();
#endif
            {
                app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_ADP_VOLTAGE);
            }
        }
    }
}

/**
    * @brief  All the state of charge change message will be handled in this function.
    *         App will do some action according to charger battery capacity.
    * @param  state_of_charge Charger battery state, such as %10, 50% etc
    * @param  first_power_on The first time power-on
    * @return void
    */
static void app_handle_state_of_charge(uint8_t state_of_charge, uint8_t first_power_on)
{
    APP_CHARGER_STATE app_charger_state;

    app_charger_state = app_charger_get_charge_state();

    if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE && app_cfg_nv.is_dut_test_mode &&
        state_of_charge == 0)
    {
        /* dut mode direct power off */
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
        return;
    }

    if ((app_db.device_state != APP_DEVICE_STATE_OFF) || (first_power_on == 1))
    {
//        uint8_t bat_level;
        uint8_t ori_state_of_charge = 0;

        APP_PRINT_TRACE5("app_handle_state_of_charge: charger_state %d, battery_status %d, charger level %d, local_batt_level %d, first_power_on %d",
                         app_charger_state, battery_status,
                         state_of_charge, app_db.local_batt_level, first_power_on);

        if (first_power_on == 0)
        {
            // adc ready
            if (invalid_batt_rec)
            {
                // Charger module report normal batt level for the first time,need to update on time
                app_db.local_batt_level = state_of_charge;
#if F_APP_HARMAN_FEATURE_SUPPORT
                app_harman_battery_status_notify();
#endif
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (state_of_charge <= 10)
                {
                    APP_PRINT_TRACE0("charge refine 1");
                }
                else
#endif
                {
                    state_of_charge = MULTIPLE_OF_TEN(state_of_charge);
                }
                invalid_batt_rec = false;
            }
            else if (((app_charger_state == APP_CHARGER_STATE_NO_CHARGE) && //Unplug
                      (state_of_charge >= app_db.local_batt_level)) ||
                     ((app_charger_state == APP_CHARGER_STATE_CHARGING) && //Plug
                      (state_of_charge <= app_db.local_batt_level)))
            {
                //Avoid battry ping-pong detection
                // 1. Adaptor unplug: Detected battery level > current battery level
                // 2. Adaptor plug: Detected battery level < current battery level

                return;
            }
            else
            {
                if (app_charger_state != APP_CHARGER_STATE_ERROR)
                {
                    ori_state_of_charge = app_db.local_batt_level;
                    app_db.local_batt_level = state_of_charge;
#if F_APP_HARMAN_FEATURE_SUPPORT
                    app_harman_battery_status_notify();
#endif
                    app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_BATTERY);

                    bool need_to_sync_batt = true;
                    static uint8_t last_sent_local_batt = 0xFF;
                    T_APP_BR_LINK *p_link = app_find_br_link(app_cfg_nv.bud_peer_addr);

                    if (p_link && p_link->acl_link_in_sniffmode_flg &&
                        (last_sent_local_batt - state_of_charge) < 10)
                    {
                        /* sync batt every 10% when b2b in sniff mode */
                        need_to_sync_batt = false;
                    }

                    if (need_to_sync_batt)
                    {
                        last_sent_local_batt = state_of_charge;
                        app_battery_evt_handle(APP_CHARGER_EVENT_BATT_CHANGE, 0, app_db.local_batt_level);
                    }

#if F_APP_TEAMS_FEATURE_SUPPORT
                    app_asp_device_handle_battery(app_db.local_batt_level, false);
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

                    //Only single digit change - not report
                    if (ori_state_of_charge == state_of_charge)
                    {
                        return;
                    }
                }
            }
        }
        else
        {
            /*first get local batt,maybe oxFF*/
            if (state_of_charge > 100)
            {
                // Charger module report 0xFF when ADC not ready, assume nv is reliable
                app_db.local_batt_level = app_cfg_nv.local_level;
#if F_APP_HARMAN_FEATURE_SUPPORT
                app_harman_battery_status_notify();
#endif
                invalid_batt_rec = true;
                return;
            }
            else
            {
                // adc ready
                app_db.local_batt_level = state_of_charge;
                app_cfg_nv.local_level  = state_of_charge;
#if F_APP_HARMAN_FEATURE_SUPPORT
                app_harman_battery_status_notify();
#endif
#if F_APP_HARMAN_FEATURE_SUPPORT
                if (state_of_charge <= 10)
                {
                    APP_PRINT_TRACE0("charge refine 2");
                }
                else
#endif
                {
                    state_of_charge = MULTIPLE_OF_TEN(state_of_charge);
                }
#if F_APP_TEAMS_FEATURE_SUPPORT
                app_asp_device_handle_battery(app_db.local_batt_level, true);
#endif
            }
#if F_APP_TEAMS_FEATURE_SUPPORT
            app_teams_battery_vp_play(app_db.local_batt_level);
#endif
        }

        if (app_charger_state != APP_CHARGER_STATE_ERROR)
        {
//            bat_level = state_of_charge / 10;

            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_db.br_link[i].connected_profile & HFP_PROFILE_MASK)
                {
                    {
#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
                        app_hfp_batt_level_report(app_db.br_link[i].bd_addr);
#endif
                    }
                }
            }

            app_report_rws_bud_info();
        }

        APP_PRINT_TRACE2("app_handle_state_of_charge p2: charger level %d, local_batt_level %d",
                         state_of_charge,
                         app_db.local_batt_level);

        if (app_charger_state == APP_CHARGER_STATE_NO_CHARGE) //Adaptor unplug
        {
            if (state_of_charge == BAT_CAPACITY_0) //Shut down
            {
                battery_status = BATTERY_STATUS_EMPTY;
#if (F_APP_AVP_INIT_SUPPORT == 1)
                app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_10, false, true);
#endif

#if F_APP_TEAMS_FEATURE_SUPPORT
                if (first_power_on == 1)
                {
                    app_audio_tone_type_play(TONE_CHARGE_NOW, false, false);//Charge now
                }
#endif

                app_db.power_off_cause = POWER_OFF_CAUSE_LOW_VOLTAGE;

#if F_APP_ERWS_SUPPORT
                app_roleswap_poweroff_handle(true);
#else
                //delay 5s to power off
                gap_start_timer(&timer_handle_delay_low_batt_poweroff,
                                "delay_low_batt_power_off", charger_timer_queue_id,
                                APP_IO_TIMER_DELAY_LOW_BATT_POWEROFF, 0, 5 * 1000);
#endif
            }
#if F_APP_TEAMS_FEATURE_SUPPORT
            else if (state_of_charge <= 2)
            {
                app_audio_tone_type_play(TONE_CHARGE_NOW, false, false);
                battery_status = BATTERY_STATUS_LOW;
            }
#endif
            else if (state_of_charge <= app_cfg_const.battery_warning_percent) //Battery low
            {
                if (!timer_handle_low_bat_warning)
                {
                    if (app_cfg_const.timer_low_bat_warning)
                    {
                        if (battery_status != BATTERY_STATUS_LOW)
                        {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                     true);
#else
#if F_APP_HARMAN_FEATURE_SUPPORT
                            app_charger_harman_low_bat_warning_vp(state_of_charge, __func__, __LINE__);
#else
                            app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                     false);
#endif
                            app_charger_enable_low_bat_warning();
#endif
                        }
                    }
                    else
                    {
#if F_APP_HARMAN_FEATURE_SUPPORT
                        app_charger_harman_low_bat_warning_vp(state_of_charge, __func__, __LINE__);
#else
                        app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                                 false);
#endif
                    }
                }

                if (!timer_handle_low_bat_led)
                {
                    if (app_cfg_const.timer_low_bat_led)
                    {
                        led_change_mode(LED_MODE_LOW_BATTERY, true, false);
                        gap_start_timer(&timer_handle_low_bat_led, "low_bat_led",
                                        charger_timer_queue_id, APP_IO_TIMER_LOW_BAT_LED, 0,
                                        (app_cfg_const.timer_low_bat_led * 1000));
                    }
                    else
                    {
                        led_change_mode(LED_MODE_LOW_BATTERY, true, false);
                    }
                }

                battery_status = BATTERY_STATUS_LOW;
            }
            else
            {
                //Battry normal
                battery_status = BATTERY_STATUS_NORMAL;

                //Reset to 10% base
                state_of_charge     = MULTIPLE_OF_TEN(state_of_charge);
                ori_state_of_charge = MULTIPLE_OF_TEN(ori_state_of_charge);

                //Enable battery report when level drop or power on
                if ((app_cfg_const.enable_bat_report_when_level_drop && !first_power_on &&
                     (ori_state_of_charge != state_of_charge)) ||
                    (!app_cfg_const.disable_bat_report_when_power_on && first_power_on))
                {
#if F_APP_HARMAN_FEATURE_SUPPORT
                    app_charger_harman_low_bat_warning_vp(state_of_charge, __func__, __LINE__);
#else
                    app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_BATTERY_PERCENTAGE_10 + bat_level - 1), false,
                                             false);
#endif
                }
            }
        }
    }

}

void app_charger_update(void)
{
    uint8_t state_of_charge;

    state_of_charge = app_charger_get_soc();
    app_handle_state_of_charge(state_of_charge, 1);
}

bool app_charger_remote_battery_status_is_low(void)
{
    return (MULTIPLE_OF_TEN(app_db.remote_batt_level) <= app_cfg_const.battery_warning_percent);
}

bool app_charger_local_battery_status_is_low(void)
{
    return (battery_status < BATTERY_STATUS_NORMAL);
}


static void app_charger_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    switch (event_type)
    {
    case SYS_EVENT_POWER_OFF:
        {
            battery_status = BATTERY_STATUS_NORMAL;

            app_charger_disable_low_bat_warning();
            gap_stop_timer(&timer_handle_low_bat_led);
#if (F_APP_SMOOTH_BAT_REPORT == 1)
            app_charger_clear_smooth_bat_handle();
#endif
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_charger_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_charger_plug_status_cb(APP_ADP_STATUS status)
{
    APP_PRINT_TRACE1("app_charger_plug_status_cb: status %d", status);

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
    if ((app_cfg_const.one_wire_uart_support &&
         app_one_wire_get_aging_test_state() == ONE_WIRE_AGING_TEST_STATE_TESTING) &&
        (status == APP_ADP_STATUS_PLUG_STABLE || status == APP_ADP_STATUS_UNPLUG_STABLE))
    {
        /* don't handle in/out box when aging testing */
        return;
    }
#endif

    switch (status)
    {
    case APP_ADP_STATUS_PLUGING:
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (app_cfg_const.enable_inbox_power_off)
            {
                if (app_cfg_const.enable_rtk_charging_box == 0)//for smart box, plug not to power off
                {
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_IN_BOX);
                }
            }

            app_dlps_disable(APP_DLPS_ENTER_CHECK_ADAPTOR);
        }

        break;

    case APP_ADP_STATUS_UNPLUGING:
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (app_cfg_const.enable_outbox_power_on)
            {
                if (app_cfg_const.enable_rtk_charging_box == 0)
                {
                    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_IN_BOX, app_cfg_const.timer_auto_power_off);
                }
            }

            app_dlps_stop_power_down_wdg_timer();
            app_dlps_disable(APP_DLPS_ENTER_CHECK_ADAPTOR);
        }

        break;

    case APP_ADP_STATUS_PLUG_STABLE:
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_db.local_in_case = true;
                app_db.local_loc = app_sensor_bud_loc_detect();

                app_adp_pending_cmd_exec();

                if (app_db.local_loc == BUD_LOC_IN_CASE)
                {
                    app_in_out_box_handle(IN_CASE);
                }

                app_sensor_bud_loc_sync();

                app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
                break;
            }

            app_db.local_in_case = true;
            app_db.local_loc = app_sensor_bud_loc_detect();

            if (app_db.local_loc == BUD_LOC_IN_CASE)
            {
                app_in_out_box_handle(IN_CASE);
            }

            app_sensor_bud_loc_sync();
            app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
        }
        break;

    case APP_ADP_STATUS_UNPLUG_STABLE:
        if (app_cfg_const.box_detect_method == ADAPTOR_DETECT)
        {
            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_db.local_in_case = false;
                app_db.local_loc = app_sensor_bud_loc_detect();

                /*not rec open case adp cmd, but unplug from inbox,need update remote_case_closed flag*/
                app_bud_loc_evt_handle(APP_BUD_LOC_EVENT_CASE_OPEN_CASE, 0, 0);

                app_adp_clear_pending();

                if (app_db.local_loc != BUD_LOC_IN_CASE)
                {
                    /* disable_charger_by_box_battery flag must be clear when adp out */
                    app_adp_set_disable_charger_by_box_battery(false);

                    if (app_cfg_const.smart_charger_control && app_db.adp_high_wake_up_for_zero_box_bat_vol)
                    {
                        APP_PRINT_TRACE0("ignore adp out when get box zero volume");
                        if (!app_cfg_nv.adp_factory_reset_power_on)
                        {
                            app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
                        }
                        return;
                    }
                    else
                    {
                        app_in_out_box_handle(OUT_CASE);
                    }
                }

                app_sensor_bud_loc_sync();

                if (!app_cfg_nv.adp_factory_reset_power_on)
                {
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
                }
                break;
            }

            app_db.local_in_case = false;
            app_db.local_loc = app_sensor_bud_loc_detect();

            if (app_db.local_loc != BUD_LOC_IN_CASE)
            {
                app_in_out_box_handle(OUT_CASE);
            }

            app_sensor_bud_loc_sync();
            app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
        }
        break;

    default:
        break;
    }

}



APP_CHARGER_STATE app_charger_get_charge_state(void)
{
    APP_CHARGER_STATE app_charger_state = APP_CHARGER_STATE_ERROR;

#if RTK_CHARGER_ENABLE
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
    if (app_ext_charger_check_support())
    {
        app_charger_state = app_db.local_charger_state;
    }
    else
#endif
    {
        T_CHARGER_STATE rtk_charger_state = rtk_charger_get_charge_state();
        app_charger_state = rtk2app_charger_state_convert(rtk_charger_state);
    }
#else

#endif

    return app_charger_state;
}


uint8_t app_charger_get_soc(void)
{
    uint8_t soc = 0;

#if RTK_CHARGER_ENABLE
    soc = rtk_charger_get_soc();
#else

#endif
    return soc;
}

#if F_APP_TEAMS_CUSTOMIZED_CMD_SUPPORT
int32_t app_charger_get_bat_curr(void)
{
    int32_t bat_curr = 0;

#if RTK_CHARGER_ENABLE
    bat_curr = rtk_charger_get_bat_curr();
#else

#endif

    return bat_curr;
}

uint32_t app_charger_get_bat_vol(void)
{
    uint32_t bat_vol = 0;

#if RTK_CHARGER_ENABLE
    bat_vol = rtk_charger_get_bat_vol();
#else

#endif

    return bat_vol;
}
#endif

void app_battery_evt_handle(uint8_t event, bool from_remote, uint8_t para)
{
#if F_APP_ERWS_SUPPORT
    if (event == APP_CHARGER_EVENT_BATT_CHANGE)
    {
        if (from_remote)
        {
            app_db.remote_batt_level = para;
            app_report_rws_bud_info();
            APP_PRINT_TRACE1("app_battery_evt_handle: remote_batt_level %d", app_db.remote_batt_level);

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if ((app_cfg_const.enable_report_lower_battery_volume) &&
                    (app_db.remote_batt_level < app_db.local_batt_level) && (app_db.remote_batt_level != 0) &&
                    (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
                {
                    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_db.br_link[i].connected_profile & HFP_PROFILE_MASK)
                        {
                            app_hfp_batt_level_report(app_db.br_link[i].bd_addr);
                        }
                    }
                }
            }
        }
        else
        {
            app_db.local_batt_level = para;
#if F_APP_HARMAN_FEATURE_SUPPORT
            app_harman_battery_status_notify();
#endif
            APP_PRINT_TRACE1("app_battery_evt_handle: local_batt_level %d", app_db.local_batt_level);

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_roleswap_req_battery_level();
                }
            }

        }

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
        if (app_cfg_const.enable_rtk_fast_pair_adv)
        {
            app_rtk_fast_pair_handle_batt_change();
        }
#endif

#if GFPS_FEATURE_SUPPORT
        if (extend_app_cfg_const.gfps_support || (app_cfg_const.supported_profile_mask & GFPS_PROFILE_MASK))
        {
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_STATE_OF_CHARGER);
        }
#endif
    }
    else if (event == APP_CHARGER_EVENT_BATT_STATE_CHANGE)
    {
        if (from_remote)
        {
            app_db.remote_charger_state = (APP_CHARGER_STATE)para;
            APP_PRINT_TRACE1("app_battery_evt_handle: remote_charger_state %d",
                             app_db.remote_charger_state);
        }
        else
        {
            app_db.local_charger_state = (APP_CHARGER_STATE)para;
            APP_PRINT_TRACE1("app_battery_evt_handle: local_charger_state %d", app_db.local_charger_state);
        }

#if GFPS_FEATURE_SUPPORT
        if (extend_app_cfg_const.gfps_support || (app_cfg_const.supported_profile_mask & GFPS_PROFILE_MASK))
        {
            app_gfps_battery_info_report(GFPS_BATTERY_REPORT_CHARGER_STATE);
        }
#endif
    }
    else if (event == APP_CHARGER_EVENT_BATT_ROLESWAP_FLAG)
    {
        app_db.batt_roleswap = para;
    }
    else if (event == APP_CHARGER_EVENT_BATT_CASE_LEVEL)
    {
        if (app_db.local_loc != BUD_LOC_IN_CASE)
        {
            app_adp_case_battery_check(&para, &app_db.case_battery);
            app_cfg_nv.case_battery = app_db.case_battery;

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_report_rws_bud_info();
            }
        }
    }
    else if (event == APP_CHARGER_EVENT_BATT_REQ_LEVEL)
    {
        if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && from_remote)
        {
            if (app_cfg_const.charger_support || app_cfg_const.discharger_support)
            {
                APP_PRINT_TRACE2("app_battery_evt_handle: local_batt_level %d local_charger_state %d",
                                 app_db.local_batt_level, app_db.local_charger_state);
                app_relay_async_single(APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_CHANGE);
            }
        }
    }
    else
    {
        APP_PRINT_WARN1("app_battery_evt_handle: Invalid event 0x%02x", event);
    }

#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_adp_battery_change_hook)
    {
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY &&
            (event == APP_CHARGER_EVENT_BATT_CHANGE ||
             event == APP_CHARGER_EVENT_BATT_STATE_CHANGE))
        {
            app_adp_battery_change_hook();
        }
    }
#endif

    if (!from_remote)
    {
        app_relay_async_single(APP_MODULE_TYPE_CHARGER, event);
    }
#else
#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_adp_battery_change_hook)
    {
        app_adp_battery_change_hook();
    }
#endif
#endif
}

uint16_t app_charger_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case APP_CHARGER_EVENT_BATT_ROLESWAP_FLAG:
        {
            payload_len = 1;
            msg_ptr = &app_db.batt_roleswap;
        }
        break;

    case APP_CHARGER_EVENT_BATT_CASE_LEVEL:
        {
            payload_len = 1;
            msg_ptr = &app_db.case_battery;
        }
        break;

    case APP_CHARGER_EVENT_BATT_CHANGE:
        {
            payload_len = 1;
            msg_ptr = &app_db.local_batt_level;
        }
        break;

    case APP_CHARGER_EVENT_BATT_STATE_CHANGE:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&app_db.local_charger_state;
        }
        break;

    default:
        break;

    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_CHARGER, payload_len, msg_ptr, skip,
                              total);
}

static void app_charger_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                    T_REMOTE_RELAY_STATUS status)
{
    if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
    {
        uint8_t *p_info = buf;
        uint8_t event;
        uint8_t para;

        event = msg_type;
        if (buf != NULL)
        {
            para = p_info[0];
        }
        else
        {
            para = 0;
        }

        app_battery_evt_handle(event, 1, para);
    }
}

#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
void app_discharger_init(void)
{
#if RTK_CHARGER_ENABLE
    rtk_charger_init(NULL, soc_send_msg_for_rtk);
#else
#endif

    gap_reg_timer_cb(app_charger_timer_cb, &charger_timer_queue_id);
    if (app_cfg_const.discharger_support)
    {
        sys_mgr_cback_register(app_charger_dm_cback);
    }
}
#endif

void app_charger_init(void)
{
#if RTK_CHARGER_ENABLE
    rtk_charger_init(charge_state_send_msg_for_rtk, soc_send_msg_for_rtk);
#else

#endif
    app_adp_cb_reg(app_charger_plug_status_cb);

    gap_reg_timer_cb(app_charger_timer_cb, &charger_timer_queue_id);

    if (app_cfg_const.discharger_support)
    {
        sys_mgr_cback_register(app_charger_dm_cback);
    }
    app_relay_cback_register(app_charger_relay_cback, app_charger_parse_cback,
                             APP_MODULE_TYPE_CHARGER, APP_CHARGER_MSG_TOTAL);
}

void app_charger_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint8_t chg_msg;
    uint8_t chg_param;

    chg_msg = io_driver_msg_recv->u.param & 0xFF;
    chg_param = (io_driver_msg_recv->u.param >> 8) & 0xFF;

    switch (chg_msg)
    {
    case CHG_MSG_CHARGER_STATE_CHANGE:
        app_handle_charger_state((APP_CHARGER_STATE)chg_param);
#if BISTO_FEATURE_SUPPORT
        if (extend_app_cfg_const.bisto_support)
        {
            app_bisto_bat_state_cb((APP_CHARGER_STATE)chg_param);
        }
#endif
        led_check_charging_mode(0); //LED mode has higher priority
        led_check_mode();
        break;

    case CHG_MSG_STATE_OF_CHARGE_CHANGE:
        if (!app_cfg_const.discharger_support)
        {
            break;
        }

#if (F_APP_SMOOTH_BAT_REPORT == 1)
        if ((app_db.device_state == APP_DEVICE_STATE_ON) && (chg_param != 0))
        {
            uint8_t new_vol;
            T_APP_SMOOTH_BAT_VOL result;

            result = app_charger_smooth_bat_discharge(chg_param, &new_vol);

            if (result == APP_SMOOTH_BAT_VOL_DISALLOW)
            {
                break;
            }
            else if (result == APP_SMOOTH_BAT_VOL_CHANGE)
            {
                chg_param = new_vol;
            }
        }
#endif

        app_handle_state_of_charge(chg_param, 0);
#if BISTO_FEATURE_SUPPORT
        if (extend_app_cfg_const.bisto_support)
        {
            app_bisto_bat_state_of_charge_cb(chg_param);
        }
#endif
        break;
    }
}

void app_charger_get_battery_info(bool *left_charging, uint8_t *left_battery,
                                  bool *right_charging, uint8_t *right_battery,
                                  bool *case_charging, uint8_t *case_battery)
{
    bool local_charger_state = (app_db.local_loc == BUD_LOC_IN_CASE) ? true : false;
    bool remote_charger_state = (app_db.remote_loc == BUD_LOC_IN_CASE) ? true : false;
    uint8_t local_batt_level = app_db.local_batt_level;
    uint8_t remote_batt_level = app_db.remote_batt_level;

    if (app_bt_policy_get_b2b_connected() == false)
    {
        remote_batt_level = 0;
        remote_charger_state = false;
    }

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        *left_charging = local_charger_state;
        *left_battery = local_batt_level;

        *right_charging = remote_charger_state;
        *right_battery = remote_batt_level;
    }
    else
    {
        *right_charging = local_charger_state;
        *right_battery = local_batt_level;

        *left_charging = remote_charger_state;
        *left_battery = remote_batt_level;
    }

    *case_charging = ((app_db.case_battery >> 7) == 0) ? 1 : 0;
    *case_battery = app_db.case_battery & 0x7F;

    APP_PRINT_TRACE6("app_charger_get_battery_info: left %d %d right %d %d case %d %d",
                     *left_charging, *left_battery,
                     *right_charging, *right_battery,
                     *case_charging, *case_battery);

}

uint8_t app_charger_get_low_bat_state(void)
{
    return battery_status;
}

#if F_APP_HARMAN_FEATURE_SUPPORT
void app_charger_harman_low_bat_warning_vp(uint8_t state_of_charge, const char *func_name,
                                           const uint32_t line_no)
{
    APP_PRINT_INFO3("app_charger_harman_low_bat_warning_vp: charger level %d, %s,%d",
                    state_of_charge,
                    TRACE_STRING(func_name),
                    line_no);
    if (state_of_charge <= app_cfg_const.battery_warning_percent)
    {
        if (app_cfg_nv.language_status == 0)
        {
            app_audio_tone_type_play(TONE_ANC_SCENARIO_2, false,
                                     false);
        }
        else
        {
            app_audio_tone_type_play(TONE_BATTERY_PERCENTAGE_10, false,
                                     false);
        }
    }
}
#endif
