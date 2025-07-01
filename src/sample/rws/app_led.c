/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include <stdbool.h>
#include "app_cfg.h"
#include "rtl876x_pinmux.h"
#include "platform_utils.h"
#include "board.h"
#include "trace.h"
#include "gap_timer.h"
#include "bt_avrcp.h"
#include "remote.h"
#include "ringtone.h"
#include "led_module.h"
#include "app_io_msg.h"
#include "app_main.h"
#include "app_led.h"
#include "app_charger.h"
#include "app_audio_policy.h"
#include "app_hfp.h"
#include "app_dlps.h"
#include "app_key_process.h"
#include "app_bt_policy_api.h"
#include "app_device.h"
#include "app_relay.h"

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif
#include "app_bt_policy_api.h"
#if (F_APP_SENSOR_SUPPORT == 1)
#include "app_sensor.h"
#endif
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#include "app_adp.h"
#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#include "app_usb_audio_hid.h"
#endif
#if F_APP_TEAMS_FEATURE_SUPPORT
#include "app_teams_extend_led.h"
#include "app_single_multilink_customer.h"
#endif

static uint8_t led_timer_queue_id = 0xff;
static void *timer_handle_led = NULL;
static void *timer_handle_off_on_led = NULL;
static void *timer_handle_rws_couple_led = NULL;
static void *timer_handle_led_resync = NULL;


#define LED_NUM_0               1
#define LED_NUM_1               2
#define LED_NUM_2               4
#define OFF_ON_LED_DISABLE      0
#define OFF_ON_LED_RESTART      1

static T_LED_TABLE led_setting[LED_NUM];
static T_LED_TYPE charging_led_type[LED_NUM];
static T_LED_MODE active_led_mode;                /**<currently led mode */
static T_LED_MODE active_charging_led_mode;       /**<currently charging led mode */
/* record repeat LED mode of primary bud which needs to sync with secondary */
static T_LED_MODE primary_active_led_mode = LED_MODE_TOTAL;
static uint8_t led_stop_check;
static T_LED_MODE led_pending_mode;
static bool rws_couple_led_ext = 0;

#define LED_RAM_MODE_TOTAL    (LED_MODE_TOTAL - LED_MODE_RAM_TABLE_START - 1)

// constant LED modes, define in ram
static T_LED_TABLE LED_RAM_TABLE[LED_RAM_MODE_TOTAL][LED_NUM] =
{
    //LED_MODE_ALL_BLINKING
    {
        {LED_TYPE_ON_OFF, 5, 5, 4, 0},
        {LED_TYPE_ON_OFF, 5, 5, 4, 0},
        {LED_TYPE_ON_OFF, 5, 5, 4, 0}
    },

    //LED_MODE_ALL_OFF
    {
        {LED_TYPE_KEEP_OFF},
        {LED_TYPE_KEEP_OFF},
        {LED_TYPE_KEEP_OFF}
    },

    //LED_MODE_ALL_ON
    {
        {LED_TYPE_KEEP_ON},
        {LED_TYPE_KEEP_ON},
        {LED_TYPE_KEEP_ON}
    },
};

/* for specified LED */
#if F_APP_ANC_SUPPORT
static bool anc_on_off_state = 0;
#endif
#if F_APP_APT_SUPPORT
static bool apt_on_off_state = 0;
#endif

#define LED_INDEX_0             0
#define LED_INDEX_1             1
#define LED_INDEX_2             2
#define LED_INDEX_ALL           3

#define RESYNC_LED_TIME_MS      180000 // 3 mins

typedef enum
{
    LED_SPECIFY_FUNCTION_NONE,
    LED_SPECIFY_FUNCTION_ANC_ON,
    LED_SPECIFY_FUNCTION_APT_ON,
    LED_SPECIFY_FUNCTION_ANC_APT,
} T_APP_LED_SPECIFY_MODE;
/* for specified LED */

typedef enum
{
    APP_IO_TIMER_DISABLE_LED,
    APP_IO_TIMER_OFF_ON_LED,
    APP_IO_TIMER_RWS_COUPLE_LED,
    APP_IO_TIMER_RESYNC_LED,
} T_APP_LED_TIMER;

typedef enum
{
    LED_MSG_SYNC_FROM_PRI,
    LED_MSG_REQUEST_NEW_MODE,
    LED_MSG_TOTAL
} T_LED_REMOTE_MSG;

static void led_reset_active_led_mode(void)
{
    active_led_mode = LED_MODE_TOTAL;
}

static void led_reset_charging_led_type(void)
{
    for (int i = 0; i < LED_NUM; i++)
    {
        charging_led_type[i] = LED_TYPE_BYPASS;
    }
}

static void led_set_active_charging_led(T_LED_MODE mode)
{
    active_charging_led_mode = mode;
}

/**
    * @brief  Check if the specific led mode is a charging-related mode.
    * @param  mode led mode
    * @return check result
    */
static bool led_is_charging_mode(T_LED_MODE mode)
{
    if ((mode == LED_MODE_CHARGING) ||
        (mode == LED_MODE_CHARGING_COMPLETE) ||
        (mode == LED_MODE_CHARGING_ERROR))
    {
        return true;
    }

    return false;
}

static bool led_is_specify_led_function(uint8_t led_idx)
{
    bool ret = true;

    switch (led_idx)
    {
    case LED_INDEX_0:
        {
            if (app_cfg_const.led0_function == LED_SPECIFY_FUNCTION_NONE)
            {
                ret = false;
            }
        }
        break;

    case LED_INDEX_1:
        {
            if (app_cfg_const.led1_function == LED_SPECIFY_FUNCTION_NONE)
            {
                ret = false;
            }
        }
        break;

    case LED_INDEX_2:
        {
            if (app_cfg_const.led2_function == LED_SPECIFY_FUNCTION_NONE)
            {
                ret = false;
            }
        }
        break;

    case LED_INDEX_ALL:
        {
            if (app_cfg_const.led0_function == LED_SPECIFY_FUNCTION_NONE &&
                app_cfg_const.led1_function == LED_SPECIFY_FUNCTION_NONE &&
                app_cfg_const.led2_function == LED_SPECIFY_FUNCTION_NONE)
            {
                ret = false;
            }
        }
        break;

    default:
        break;
    }

    return ret;
}

static void led_turn_off_no_specify_mode_led(void)
{
    if (app_cfg_const.led0_function == LED_SPECIFY_FUNCTION_NONE)
    {
        led_set_designate_led_off(LED_INDEX_0);
    }
    if (app_cfg_const.led1_function == LED_SPECIFY_FUNCTION_NONE)
    {
        led_set_designate_led_off(LED_INDEX_1);
    }
    if (app_cfg_const.led2_function == LED_SPECIFY_FUNCTION_NONE)
    {
        led_set_designate_led_off(LED_INDEX_2);
    }
}

/**
    * @brief  Disables all LED feature.
    * @param  void
    * @return void
    */
static void led_disable_all(void)
{
    led_cmd(LED_CH_0 | LED_CH_1 | LED_CH_2, DISABLE);
}

/**
    * @brief  Reset led mode.
    *         When app task needs to reset led mode, app needs to call this function.
    *         If led reset mode is not equal to pending mode, app will set led mode to pending mode.
    * @param  from_non_repeat_mode now led mode is repeat or not
    * @return void
    */
static void led_reset_mode(bool from_non_repeat_mode)
{
    APP_PRINT_INFO0("led_reset_mode");
    if (led_is_specify_led_function(LED_INDEX_ALL))
    {
        led_turn_off_no_specify_mode_led();
    }
    else
    {
        led_disable_all();
    }

    led_stop_check = 0;
    led_reset_active_led_mode();
    led_check_charging_mode(from_non_repeat_mode);

    if (led_pending_mode != LED_MODE_TOTAL)
    {
        // if led pending mode is not empty, apply it
        led_change_mode(led_pending_mode, true, false);
    }
    else
    {
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            led_check_mode();
        }
    }
}

void led_disable_non_repeat_mode(void)
{
    gap_stop_timer(&timer_handle_led);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_LED);

    if (active_led_mode == LED_MODE_ENTER_PCBA_SHIPPING_MODE)
    {
        app_adp_cmd_enter_pcba_shipping_mode();
    }

    led_reset_mode(1);

}

#if (F_APP_ANC_SUPPORT ==1 || F_APP_APT_SUPPORT == 1)
//Turn off the specify led according to input specify mode
static void led_turn_off_specify_mode_led(T_APP_LED_SPECIFY_MODE led_specify_mode)
{
    if (app_cfg_const.led0_function == led_specify_mode)
    {
        led_set_designate_led_off(LED_INDEX_0);
    }

    if (app_cfg_const.led1_function == led_specify_mode)
    {
        led_set_designate_led_off(LED_INDEX_1);
    }

    if (app_cfg_const.led2_function == led_specify_mode)
    {
        led_set_designate_led_off(LED_INDEX_2);
    }
}

static bool led_specify_led_is_config(T_APP_LED_SPECIFY_MODE led_specify_mode)
{
    if (app_cfg_const.led0_function == led_specify_mode ||
        app_cfg_const.led1_function == led_specify_mode ||
        app_cfg_const.led2_function == led_specify_mode)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif

void led_set_designate_led_on(uint8_t led_number)
{
    T_LED_TABLE led_conf_table = {LED_TYPE_BYPASS};
    T_LED_CHANNEL led_channel = (T_LED_CHANNEL) BIT(led_number);
    led_conf_table.type = LED_TYPE_KEEP_ON;
    led_cmd(led_channel, DISABLE);
    led_config(led_channel, &led_conf_table);
    led_cmd(led_channel, ENABLE);
}

void led_set_designate_led_off(uint8_t led_number)
{
    T_LED_CHANNEL led_channel = (T_LED_CHANNEL) BIT(led_number);
    led_cmd(led_channel, DISABLE);
}

static bool led_load_mode(T_LED_MODE mode, T_LED_TABLE *new_led_setting, uint8_t led_num)
{
    // UI tool only support 32 types of LED, these modes are defined in ram
    if (mode > LED_MODE_RAM_TABLE_START)
    {
        uint8_t ram_mode_idx = (uint8_t)(mode - LED_MODE_RAM_TABLE_START) - 1;

        memcpy((void *)new_led_setting,
               (const void *)&LED_RAM_TABLE[ram_mode_idx], led_num * sizeof(T_LED_TABLE));
    }
    else
    {
        /* Load led mode configuration from flash */
        if (app_cfg_load_led_table(new_led_setting, mode, led_num * sizeof(T_LED_TABLE)) == false)
        {
            APP_PRINT_INFO0("led_load_mode: load cfg table failed");
            return false;
        }
    }

    return true;
}

/*
 * Load led table configuration from flash or local ram.
 * When app set check led data flag, load led table configuration from flash.
 * mode led mode, such as LED_MODE_ALL_OFF, LED_MODE_ALL_ON, LED_MODE_PAIRING etc.
 */
static void led_load_table(T_LED_MODE mode)
{
    if (led_load_mode(mode, led_setting, LED_NUM) == false)
    {
        return;
    }

    //Keep current charging type
    if (led_is_charging_mode(mode))
    {
        for (int i = 0; i < LED_NUM; i++)
        {
            charging_led_type[i] = led_setting[i].type;
        }
    }
}

static void led_set_pad_config(uint8_t led_x_pinmux, uint8_t enable_low_active)
{
    if (enable_low_active)
    {
        Pad_Config(led_x_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    else
    {
        Pad_Config(led_x_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
}

static bool led_is_all_specific_type(T_LED_MODE mode, T_LED_TYPE type)
{
    bool is_all_specific_type = true;
    T_LED_TABLE led_peek_setting[LED_NUM];

    led_load_mode(mode, led_peek_setting, LED_NUM);

    for (int i = 0; i < LED_NUM; i++)
    {
        if (led_peek_setting[i].type != type)
        {
            is_all_specific_type = false;
        }
    }

    return is_all_specific_type;
}

static T_LED_MODE led_set_secondary_bud_led_mode(T_LED_MODE mode)
{
    if (mode == LED_MODE_BUD_ROLE_PRIMARY)
    {
        mode = LED_MODE_BUD_ROLE_SECONDARY;
    }

    return mode;
}

static void led_restart_resync_timer(bool restart)
{
    gap_stop_timer(&timer_handle_led_resync);

    if (restart)
    {
        gap_start_timer(&timer_handle_led_resync, "led_resync", led_timer_queue_id,
                        APP_IO_TIMER_RESYNC_LED, 0, RESYNC_LED_TIME_MS);
    }
}

static bool led_is_independent_led_mode(T_LED_MODE mode)
{
    bool ret = false;

    // these LED modes may be different from another bud when b2b connected
    if ((mode == LED_MODE_POWER_ON) ||
        (mode == LED_MODE_POWER_OFF) ||
        (mode == LED_MODE_PAIRING_COMPLETE) ||
        (mode == LED_MODE_FACTORY_RESET) ||
        (mode == LED_MODE_LOW_BATTERY) ||
        (!app_db.disallow_charging_led && led_is_charging_mode(mode)))
    {
        ret = true;
    }

    return ret;
}

static bool led_is_blink_alone(T_LED_MODE mode)
{
    bool ret = false;

    // no need to relay if led is in all-off/bypass state or an independent mode
    if (led_is_all_specific_type(mode, LED_TYPE_BYPASS) || led_is_independent_led_mode(mode))
    {
        ret = true;
    }

    return ret;
}

static void led_sync_mode(T_LED_MODE mode, bool forced_sec_change)
{
    uint8_t mode_info[2];

    mode_info[0] = mode;
    mode_info[1] = forced_sec_change;
    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_LED, LED_MSG_SYNC_FROM_PRI,
                                       (uint8_t *)&mode_info, sizeof(mode_info), REMOTE_TIMER_HIGH_PRECISION, 0, false);
}

static void led_primary_bud_sync_mode(void)
{
    if (!led_is_blink_alone(active_led_mode) && (active_led_mode != LED_MODE_TOTAL) &&
        (app_db.b2b_role_switch_led_pending == 0))
    {
        led_sync_mode(active_led_mode, false);
    }
}

static void led_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("led_timer_callback: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_IO_TIMER_DISABLE_LED:
        {
            if (timer_handle_led != NULL)
            {
                led_disable_non_repeat_mode();
            }
        }
        break;

    case APP_IO_TIMER_OFF_ON_LED:
        {
            if (timer_handle_off_on_led != NULL)
            {
                gap_stop_timer(&timer_handle_off_on_led);

                uint8_t led_trans = timer_chann;
                uint8_t led_event = timer_chann >> 8;
                uint8_t led_num = 0;

                if (led_event == OFF_ON_LED_DISABLE)
                {
                    if (led_trans & LED_NUM_0)
                    {
                        led_num = 0;
                        Pad_FunctionConfig(app_cfg_const.led_0_pinmux, AON_GPIO);
                        led_set_pad_config(app_cfg_const.led_0_pinmux, app_cfg_const.enable_led0_low_active);
                    }
                    if (led_trans & LED_NUM_1)
                    {
                        led_num = 1;
                        Pad_FunctionConfig(app_cfg_const.led_1_pinmux, AON_GPIO);
                        led_set_pad_config(app_cfg_const.led_1_pinmux, app_cfg_const.enable_led1_low_active);
                    }
                    if (led_trans & LED_NUM_2)
                    {
                        led_num = 2;
                        Pad_FunctionConfig(app_cfg_const.led_2_pinmux, AON_GPIO);
                        led_set_pad_config(app_cfg_const.led_2_pinmux, app_cfg_const.enable_led2_low_active);
                    }

                    uint8_t interval_time = led_setting[led_num].blink_interval;
                    if (interval_time != 0)
                    {
                        gap_start_timer(&timer_handle_off_on_led, "restart_off_on_led",
                                        led_timer_queue_id, APP_IO_TIMER_OFF_ON_LED,
                                        ((1 << 8) | led_trans), interval_time * 100);
                    }

                }
                else if (led_event == OFF_ON_LED_RESTART)
                {
                    uint8_t index;
                    for (index = LED_NUM_0; index <= LED_NUM_2; index <<= 1)
                    {
                        if (led_trans & index)
                        {
                            led_cmd(BIT(index >> 1), DISABLE);
                            led_config((T_LED_CHANNEL)BIT(index >> 1), &led_setting[index >> 1]);
                            led_cmd(BIT(index >> 1), ENABLE);
                            led_num = index >> 1;
                        }
                    }

                    Pad_FunctionConfig(app_cfg_const.led_0_pinmux, LED0);
                    Pad_FunctionConfig(app_cfg_const.led_1_pinmux, LED1);

                    uint16_t disable_time = led_setting[led_num].blink_count *
                                            (led_setting[led_num].on_time + led_setting[led_num].off_time);
                    if (disable_time != 0)
                    {
                        gap_start_timer(&timer_handle_off_on_led, "disable_off_on_led",
                                        led_timer_queue_id, APP_IO_TIMER_OFF_ON_LED,
                                        led_trans, disable_time * 10);
                    }

                }
            }
        }
        break;

    case APP_IO_TIMER_RWS_COUPLE_LED:
        {
            if (timer_handle_rws_couple_led != NULL)
            {
                gap_stop_timer(&timer_handle_rws_couple_led);
                rws_couple_led_ext = 0;
            }
        }
        break;

    case APP_IO_TIMER_RESYNC_LED:
        {
            if (timer_handle_led_resync != NULL)
            {
                led_restart_resync_timer(false);

                if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                    (!led_is_all_specific_type(active_led_mode, LED_TYPE_KEEP_OFF))) // no need to relay keep off type
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        led_primary_bud_sync_mode();
                    }
                    else // changed to secondary due to roleswap process
                    {
                        if (!led_is_blink_alone(active_led_mode) && (app_db.b2b_role_switch_led_pending == 0))
                        {
                            // indicate primary bud to sync led mode
                            app_relay_async_single(APP_MODULE_TYPE_LED, LED_MSG_REQUEST_NEW_MODE);
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

static void led_pad_config(void)
{
    if (app_cfg_const.led_support)
    {
        // HW LED pinmux for rtl87x3c/rtl87x3d: P0_0, P0_1, P0_6, P0_7, P1_0, P1_1, P1_4, P2_0, P2_1, P2_2
        // HW LED pinmux for rtl87x3e/rtl87x3g: P0_0, P0_1, P1_0, P1_1, P2_1, P2_2, P2_3, P2_4, P3_0, P3_1
        if (app_cfg_const.led_0_pinmux != 0xFF)
        {
            if (led_is_sleep_pinmux(app_cfg_const.led_0_pinmux))
            {
                Pad_FunctionConfig(app_cfg_const.led_0_pinmux, LED0);
            }

            if (app_cfg_const.enable_led0_low_active)
            {
                led_set_active_polarity(LED_CH_0, LED_ACTIVE_POLARITY_LOW);
            }

            led_set_pad_config(app_cfg_const.led_0_pinmux, app_cfg_const.enable_led0_low_active);
        }

        if (app_cfg_const.led_1_pinmux != 0xFF)
        {
            if (led_is_sleep_pinmux(app_cfg_const.led_1_pinmux))
            {
                Pad_FunctionConfig(app_cfg_const.led_1_pinmux, LED1);
            }

            if (app_cfg_const.enable_led1_low_active)
            {
                led_set_active_polarity(LED_CH_1, LED_ACTIVE_POLARITY_LOW);
            }

            led_set_pad_config(app_cfg_const.led_1_pinmux, app_cfg_const.enable_led1_low_active);
        }

        if (app_cfg_const.led_2_pinmux != 0xFF)
        {
            if (led_is_sleep_pinmux(app_cfg_const.led_2_pinmux))
            {
                Pad_FunctionConfig(app_cfg_const.led_2_pinmux, LED2);
            }

            if (app_cfg_const.enable_led2_low_active)
            {
                led_set_active_polarity(LED_CH_2, LED_ACTIVE_POLARITY_LOW);
            }

            led_set_pad_config(app_cfg_const.led_2_pinmux, app_cfg_const.enable_led2_low_active);
        }

        led_reset_sleep_led();
        led_reset_charging_led_type();
        led_set_active_charging_led(LED_MODE_ALL_OFF);
    }
}

/**
    * @brief  Check if the specific led mode is a repeat mode or not.
    * @param  mode led mode, such as LED_MODE_ALL_OFF, LED_MODE_ALL_ON, LED_MODE_PAIRING, etc.
    * @return check result
    */
static bool led_is_repeat_mode(T_LED_MODE mode)
{
    uint8_t non_repeat_mode_start_point;
    uint8_t non_repeat_mode_end_point = LED_MODE_RAM_TABLE_START;

    if (app_cfg_const.enable_repeat_gaming_mode_led)
    {
        non_repeat_mode_start_point = LED_MODE_POWER_ON;
    }
    else
    {
        non_repeat_mode_start_point = LED_MODE_GAMING_MODE;
    }

    // Non-repeat LED mode
    if ((mode >= non_repeat_mode_start_point) && (mode < non_repeat_mode_end_point))
    {
        return false;
    }

    return true;
}

static void led_change_specify_function_mode(void)
{
    /* Check ANC/APT on state */
#if F_APP_ANC_SUPPORT
    if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state) && !anc_on_off_state)
    {
        if (led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_ON) ||
            led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
        {
            anc_on_off_state = 1;
            led_change_mode(LED_MODE_ANC_ON, true, false);
        }
    }
#endif
#if F_APP_APT_SUPPORT
    if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state) && !apt_on_off_state)
    {
        if (led_specify_led_is_config(LED_SPECIFY_FUNCTION_APT_ON) ||
            led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
        {
            apt_on_off_state = 1;
            led_change_mode(LED_MODE_APT_ON, true, false);
        }
    }
#endif

    /* Check ANC/APT off state */
#if F_APP_ANC_SUPPORT
    if (!app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state) && anc_on_off_state)
    {
        anc_on_off_state = 0;

        if (led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_ON))
        {
            led_turn_off_specify_mode_led(LED_SPECIFY_FUNCTION_ANC_ON);
        }

        if (led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
        {
#if F_APP_APT_SUPPORT
            if (!app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
#endif
            {
                led_turn_off_specify_mode_led(LED_SPECIFY_FUNCTION_ANC_APT);
            }
        }
    }
#endif
#if F_APP_APT_SUPPORT
    if (!app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state) && apt_on_off_state)
    {
        apt_on_off_state = 0;

        if (led_specify_led_is_config(LED_SPECIFY_FUNCTION_APT_ON))
        {
            led_turn_off_specify_mode_led(LED_SPECIFY_FUNCTION_APT_ON);
        }

        if (led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
        {
#if F_APP_ANC_SUPPORT
            if (!app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
#endif
            {
                led_turn_off_specify_mode_led(LED_SPECIFY_FUNCTION_ANC_APT);
            }
        }
    }
#endif
}

/**
    * @brief  App set led mode.
    *         First app need to load led table, disable led before config led register.
    *         Then config led register according to led mode, and enable led last.
    * @param  LED mode, such as LED_MODE_ALL_OFF, LED_MODE_ALL_ON, LED_MODE_PAIRING etc.
    * @return void
    */
static void led_set_mode(T_LED_MODE mode)
{
    uint8_t i;
    uint16_t time = 0;
    uint8_t count = 0;
    uint8_t channel = 0;
    uint8_t *led0_pinmux = &app_cfg_const.led_0_pinmux;
    static T_LED_MODE pre_mode = LED_MODE_ALL_OFF;

    APP_PRINT_INFO4("led_set_mode: pre_mode 0x%02x, mode 0x%02x, led_stop_check %d, a2dp_play_status = %d",
                    pre_mode, mode, led_stop_check, app_db.a2dp_play_status);

    if (app_db.device_state == APP_DEVICE_STATE_OFF_ING)
    {
        // if currently led mode isn't LED_MODE_POWER_OFF, stop it.
        if ((mode != LED_MODE_POWER_OFF)
            && (mode != LED_MODE_FACTORY_RESET)
            && (mode != LED_MODE_ALL_OFF))
        {
            return;
        }
    }
#if F_APP_HARMAN_FEATURE_SUPPORT
    if ((active_led_mode == LED_MODE_GAMING_MODE) &&
        (mode != LED_MODE_GAMING_MODE) &&
        (mode != LED_MODE_VOL_ADJUST) &&
        (mode != LED_MODE_VOL_MAX_MIN))
    {
        if (timer_handle_led != NULL)
        {
            gap_stop_timer(&timer_handle_led);
            led_disable_non_repeat_mode();
        }
    }
#endif

#if F_APP_HARMAN_FEATURE_SUPPORT
    APP_PRINT_INFO6("led_set_mode: active_led_mode: 0x%02x, mode 0x%02x, device_state: %d, "
                    "app_bt_policy_get_state: %d, timer_handle_led: %d, low_bat_state: %d",
                    active_led_mode, mode, app_db.device_state,
                    app_bt_policy_get_state(), timer_handle_led, app_charger_get_low_bat_state());

    if ((mode == LED_MODE_LOW_BATTERY) &&
        ((app_bt_policy_get_state() == BP_STATE_IDLE) ||
         (app_bt_policy_get_state() == BP_STATE_LINKBACK) ||
         (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE) ||
         (app_db.device_state == APP_DEVICE_STATE_OFF) ||
         (app_db.device_state == APP_DEVICE_STATE_OFF_ING) ||
         ((active_led_mode == LED_MODE_GAMING_MODE) && (timer_handle_led != NULL)) ||
         ((active_led_mode == LED_MODE_PAIRING_COMPLETE) && (timer_handle_led != NULL)) ||
         (active_led_mode == LED_MODE_FACTORY_RESET) ||
         (active_led_mode == LED_MODE_DUT_TEST_MODE)))
    {
        return;
    }

#if HARMAN_DISABLE_STANDBY_LED_WHEN_LOW_BATTERY
    if (mode == LED_MODE_STANDBY)
    {
        if ((app_charger_get_low_bat_state() == BATTERY_STATUS_LOW) ||
            (active_led_mode == LED_MODE_LOW_BATTERY))
        {
            return;
        }
    }
#endif

    if ((mode == LED_MODE_GAMING_MODE) &&
        (active_led_mode != LED_MODE_CONNECTED_SINGLE_LINK) &&
        (active_led_mode != LED_MODE_CONNECTED_MULTI_LINK))
    {
        return;
    }

    if (mode == LED_MODE_PAIRING_COMPLETE)
    {
        led_check_set_mode_available(mode);
    }

    if (active_led_mode == LED_MODE_LOW_BATTERY)
    {
        if ((mode == LED_MODE_LINK_BACK) ||
            (mode == LED_MODE_PAIRING_COMPLETE) ||
            (mode == LED_MODE_PAIRING))
        {
            led_timer_stop();
        }
    }
#endif

    if ((app_cfg_const.led_support) && (led_stop_check == 0))
    {
        led_pending_mode = LED_MODE_TOTAL; //reset it
        led_load_table(mode);

        if (((app_cfg_const.led_0_pinmux == 0xFF) || (led_setting[0].type == LED_TYPE_BYPASS)) &&
            ((app_cfg_const.led_1_pinmux == 0xFF) || (led_setting[1].type == LED_TYPE_BYPASS)) &&
            ((app_cfg_const.led_2_pinmux == 0xFF) || (led_setting[2].type == LED_TYPE_BYPASS)))
        {
            //Power off and factory reset will be entered dlps
            if ((mode == LED_MODE_POWER_OFF) || (mode == LED_MODE_FACTORY_RESET))
            {
                app_dlps_enable(APP_DLPS_ENTER_CHECK_LED);
            }

            if (mode == LED_MODE_ENTER_PCBA_SHIPPING_MODE)
            {
                app_adp_cmd_enter_pcba_shipping_mode();
            }

            //ALL led types are bypass, NO need to set led
            return;
        }

        active_led_mode = mode;

        //LED counts must be 2
        //The settings of LED1 must be the same as that of LED0
        //LED behavior is only applied LED0 in normal mode, only LED1 is applied in low battery mode and charging mode
        if ((app_cfg_const.normal_bat_led0_low_bat_led1 == 1))
        {
            if (app_charger_local_battery_status_is_low() ||
                (app_charger_get_soc() <= app_cfg_const.battery_warning_percent) ||
                (app_charger_get_charge_state() != APP_CHARGER_STATE_NO_CHARGE))
            {
                led_setting[0].type = LED_TYPE_KEEP_OFF;
            }
            else
            {
                led_setting[1].type = LED_TYPE_KEEP_OFF;
            }
        }

        bool is_repeat_led_mode = led_is_repeat_mode(mode);

        //Switch to GPIO first to avoid low-active led abnormal flash
        if (led_is_sleep_pinmux(app_cfg_const.led_0_pinmux))
        {
            Pad_FunctionConfig(app_cfg_const.led_0_pinmux, AON_GPIO);
        }

        if (led_is_sleep_pinmux(app_cfg_const.led_1_pinmux))
        {
            Pad_FunctionConfig(app_cfg_const.led_1_pinmux, AON_GPIO);
        }

        if (led_is_sleep_pinmux(app_cfg_const.led_2_pinmux))
        {
            Pad_FunctionConfig(app_cfg_const.led_2_pinmux, AON_GPIO);
        }

        gap_stop_timer(&timer_handle_off_on_led);

        for (i = 0; i < LED_NUM; i++)
        {
            if (led_is_charging_mode(mode) || //Charging mode
                (!is_repeat_led_mode) || //non-repeat mode
                (charging_led_type[i] == LED_TYPE_BYPASS) ||  //Charging LED has higher priority
                (app_cfg_const.charger_support == 0))
            {
                if ((led_setting[i].type != LED_TYPE_KEEP_OFF) && (*(led0_pinmux + i) != 0xFF))
                {
                    if (led_setting[i].type != LED_TYPE_BYPASS)
                    {
                        //Disable LED first before config register
                        led_set_designate_led_off(i);
                        channel |= BIT(i);
                        led_config((T_LED_CHANNEL)BIT(i), &led_setting[i]);

                        if (!is_repeat_led_mode) //Non-Repeat LED Mode
                        {
                            count++;
                            time += led_setting[i].blink_count * (led_setting[i].on_time + led_setting[i].off_time);
                        }
                    }
                }
                else
                {
                    if (!led_is_specify_led_function(i)) // should not turn off specify func LED
                    {
                        led_set_designate_led_off(i); //Avoid sleed led abnormal blink in "LOW active" case
                        platform_delay_us(100);
                        //Clear LED register
                        led_deinit((T_LED_CHANNEL)BIT(i));
                    }
                }
            }
            else if (charging_led_type[i] != LED_TYPE_BYPASS) //Charging LED is working
            {
                channel |= BIT(i);
            }
        }

        led_cmd(channel, ENABLE);

        if (led_is_sleep_pinmux(app_cfg_const.led_0_pinmux))
        {
            Pad_FunctionConfig(app_cfg_const.led_0_pinmux, LED0);
        }

        if (led_is_sleep_pinmux(app_cfg_const.led_1_pinmux))
        {
            Pad_FunctionConfig(app_cfg_const.led_1_pinmux, LED1);
        }

        if (led_is_sleep_pinmux(app_cfg_const.led_2_pinmux))
        {
            Pad_FunctionConfig(app_cfg_const.led_2_pinmux, LED2);
        }

        if (!is_repeat_led_mode) //Non-Repeat LED Mode
        {
            if (time != 0)
            {
                time /= count; //For non-repeat mode, ON/OFF time should set the same for each channel
                time -= 1; //Avoid mis-blink of next LED period, disable LED 10ms earier
            }
            else
            {
                time = 1; //Avoid non-repeat LED not return to orignal mode
            }
#if F_APP_HARMAN_FEATURE_SUPPORT
            if ((mode == LED_MODE_PAIRING_COMPLETE) ||
                (mode == LED_MODE_GAMING_MODE))
            {
                time = 500;
            }
#endif

            app_dlps_disable(APP_DLPS_ENTER_CHECK_LED);
            led_stop_check = 1;
            gap_start_timer(&timer_handle_led, "led_disable", led_timer_queue_id,
                            APP_IO_TIMER_DISABLE_LED, mode, time * 10);
        }

        // For Repeat + off_on LED, BBPro2 specified
        if ((!led_is_charging_mode(mode)) && is_repeat_led_mode)
        {
            uint8_t sync_led_record = 0;
            uint8_t search_1;
            uint8_t search_2;

            for (search_1 = 0; search_1 < LED_NUM; search_1++)
            {
                if (led_setting[search_1].type == LED_TYPE_OFF_ON && led_setting[search_1].blink_interval != 0)
                {
                    for (search_2 = 0; search_2 < LED_NUM; search_2++)
                    {
                        if (led_setting[search_2].type == LED_TYPE_ON_OFF || led_setting[search_2].type == LED_TYPE_OFF_ON)
                        {
                            //force ON-OFF, OFF-ON LED settings are same
                            if ((memcmp(&(led_setting[search_1].on_time), &(led_setting[search_2].on_time), 4) == 0))
                            {
                                sync_led_record |= 1 << search_2;
                            }
                        }
                    }
                    if (sync_led_record)
                    {
                        uint16_t disable_time = led_setting[search_1].blink_count *
                                                (led_setting[search_1].on_time + led_setting[search_1].off_time);

                        gap_start_timer(&timer_handle_off_on_led, "disable_off_on_led",
                                        led_timer_queue_id, APP_IO_TIMER_OFF_ON_LED,
                                        sync_led_record, disable_time * 10);
                        break;
                    }
                }
            }
        }

        pre_mode = mode;
    }
}

bool led_change_mode(T_LED_MODE change_mode, bool forced_sec_change, bool relay_to_sec)
{
    if (change_mode == LED_MODE_TOTAL) // invalid LED mode
    {
        return false;
    }

    APP_PRINT_TRACE3("led_change_mode 0x%02x forced_sec_change %d relay_to_sec %d",
                     change_mode, forced_sec_change, relay_to_sec);

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (relay_to_sec)
            {
                led_sync_mode(change_mode, true);
            }
            else
            {
                led_set_mode(change_mode);
            }
        }
        else // secondary
        {
            if (forced_sec_change || (!led_is_independent_led_mode(active_led_mode)))
            {
                change_mode = led_set_secondary_bud_led_mode(change_mode);
                led_set_mode(change_mode);
            }
            else // no need to change
            {
                return false;
            }
        }
    }
    else
    {
        led_set_mode(change_mode);
    }

    return true;
}

/**
    * @brief  A decision-making function set between led_check_mode and led_change_mode.
    * @param  LED mode
    * @return void
    */
static void led_check_change_mode(T_LED_MODE change_mode)
{
    T_LED_MODE final_mode = LED_MODE_TOTAL;

    if (led_stop_check == 0)
    {
        if (led_is_specify_led_function(LED_INDEX_ALL))
        {
            led_change_specify_function_mode();
        }

        if ((final_mode == LED_MODE_TOTAL) && (change_mode != active_led_mode)) // need to change LED mode
        {
            {
#if F_APP_TEAMS_FEATURE_SUPPORT
#else
                if (!app_db.disallow_charging_led &&
                    led_is_charging_mode(active_charging_led_mode))
                {
                    //Charging LED has higher priority
                    return;
                }
#endif
            }

            if (app_cfg_nv.trigger_dut_mode_from_power_off)
            {
                if ((change_mode != LED_MODE_ALL_ON) && (change_mode != LED_MODE_POWER_OFF))
                {
                    /* disallow config led when trigger dut mode from power off */
                    return;
                }
            }

            if (app_cfg_nv.is_dut_test_mode == 1)
            {
                //Disallow other led when dut test mode
                return;
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                bool print_log_fg = false;
                T_LED_MODE pre_active_mode = active_led_mode;
                /* for LED modes of secondary bud that need to sync */
                T_LED_MODE secondary_expected_mode = led_set_secondary_bud_led_mode(primary_active_led_mode);

                if (led_is_blink_alone(change_mode))
                {
                    led_restart_resync_timer(false); // stop LED mode re-sync timer
                    final_mode = change_mode;
                    print_log_fg = true;
                }
                else // need to sync
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        if ((active_led_mode == LED_MODE_TOTAL) && // from non-repeat mode/led reset
                            (change_mode == primary_active_led_mode) && // restore last repeat mode
                            led_is_all_specific_type(change_mode, LED_TYPE_KEEP_OFF))
                        {
                            led_restart_resync_timer(false); // stop LED mode re-sync timer
                            final_mode = change_mode;
                            print_log_fg = true;
                        }
                        else if (app_db.b2b_role_switch_led_pending == 0)
                        {
                            active_led_mode = change_mode; // to avoid duplicate relay msg
                            led_sync_mode(change_mode, false);
                            print_log_fg = true;
                        }
                    }
                    else // secondary
                    {
                        if (active_led_mode != secondary_expected_mode)
                        {
                            if ((active_led_mode == LED_MODE_TOTAL) && // from non-repeat mode/led reset
                                (change_mode == secondary_expected_mode) && // restore last repeat mode
                                led_is_all_specific_type(change_mode, LED_TYPE_KEEP_OFF))
                            {
                                final_mode = secondary_expected_mode;
                            }
                            else
                            {
                                // indicate primary bud to sync led mode
                                app_relay_async_single(APP_MODULE_TYPE_LED, LED_MSG_REQUEST_NEW_MODE);

                                if (led_is_independent_led_mode(active_led_mode))
                                {
                                    // to avoid stucking in an independent mode
                                    led_change_mode(LED_MODE_ALL_OFF, true, false);
                                    final_mode = LED_MODE_TOTAL;
                                }

                                active_led_mode = secondary_expected_mode; // to avoid duplicate request
                            }

                            print_log_fg = true;
                        }
                    }
                }

                if (print_log_fg)
                {
                    APP_PRINT_TRACE8("led_check_change_mode 0x%02x (ind %d) -> 0x%02x (ind %d, off %d) / 0x%02x (ind %d, off %d)",
                                     pre_active_mode, led_is_blink_alone(pre_active_mode),
                                     change_mode, led_is_blink_alone(change_mode),
                                     led_is_all_specific_type(change_mode, LED_TYPE_KEEP_OFF),
                                     secondary_expected_mode, led_is_blink_alone(secondary_expected_mode),
                                     led_is_all_specific_type(secondary_expected_mode, LED_TYPE_KEEP_OFF));
                }
            }
            else // single mode
            {
                final_mode = change_mode;
            }
        }

        if (final_mode != LED_MODE_TOTAL) // need to update LED mode
        {
            led_change_mode(final_mode, false, false);
        }
    }
}

uint8_t led_get_led_stop_check_state(void)
{
    return led_stop_check;
}

void led_timer_stop(void)
{
    led_stop_check = 0;
    gap_stop_timer(&timer_handle_led);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_LED);
}

static T_LED_MODE led_check_connected_mode(T_LED_MODE change_mode)
{
    if (app_bt_policy_get_b2b_connected() &&
        (app_cfg_const.enable_connected_led_over_role_led == 0))
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            change_mode = LED_MODE_BUD_ROLE_PRIMARY;
        }
        else
        {
            change_mode = LED_MODE_BUD_ROLE_SECONDARY;
        }
    }
    else
    {
        if (app_cfg_const.enable_multi_link &&
            (((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY) &&
              (app_bt_policy_get_b2s_connected_num_with_profile() > 1)) || // for primary/single bud
             ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && (app_db.b2s_connected_num > 1))))
        {
            change_mode = LED_MODE_CONNECTED_MULTI_LINK;
        }
        else
        {
            change_mode = LED_MODE_CONNECTED_SINGLE_LINK;
        }
    }

    return change_mode;
}

static T_LED_MODE led_check_b2s_connected_mode(T_LED_MODE change_mode)
{
    if (app_db.a2dp_play_status)
    {
        change_mode = LED_MODE_AUDIO_PLAYING;
    }
    else if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY) &&
             (!app_cfg_const.disallow_linkback_led_when_b2s_connected) &&
             (app_bt_policy_get_state() == BP_STATE_LINKBACK))
    {
        change_mode = LED_MODE_LINK_BACK;
    }
#if F_APP_ANC_SUPPORT
    else if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_ON) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
    {
        change_mode = LED_MODE_ANC_ON;
    }
#endif
#if F_APP_APT_SUPPORT
    else if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_APT_ON) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
    {
        change_mode = LED_MODE_APT_ON;
    }
#endif
    else
    {
        change_mode = led_check_connected_mode(change_mode);
    }

    return change_mode;
}

static T_LED_MODE led_check_b2b_connected_mode(T_LED_MODE change_mode)
{
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_bt_policy_get_state() == BP_STATE_LINKBACK))
    {
        change_mode = LED_MODE_LINK_BACK;
    }
#if F_APP_ANC_SUPPORT
    else if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_ON) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
    {
        change_mode = LED_MODE_ANC_ON;
    }
#endif
#if F_APP_APT_SUPPORT
    else if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_APT_ON) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
    {
        change_mode = LED_MODE_APT_ON;
    }
#endif
    else if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
             (app_bt_policy_get_state() == BP_STATE_STANDBY))
    {
        change_mode = LED_MODE_STANDBY;
    }
    else
    {
        change_mode = led_check_connected_mode(change_mode);
    }

    return change_mode;
}

static T_LED_MODE led_check_single_bud_mode(T_LED_MODE change_mode)
{
    if (app_bt_policy_get_state() == BP_STATE_FIRST_ENGAGE)
    {
        change_mode = LED_MODE_BUD_FIRST_ENGAGE_CREATION;
    }
    else if (app_bt_policy_get_state() == BP_STATE_ENGAGE)
    {
        change_mode = LED_MODE_BUD_COUPLING;

        if (app_cfg_const.timer_rws_couple_led != 0)
        {
            if (led_stop_check == 0)
            {
                if (timer_handle_rws_couple_led == NULL)
                {
                    gap_start_timer(&timer_handle_rws_couple_led, "rws_couple_led", led_timer_queue_id,
                                    APP_IO_TIMER_RWS_COUPLE_LED, change_mode, app_cfg_const.timer_rws_couple_led * 1000);
                }
            }
            else
            {
                rws_couple_led_ext = 1;
            }
        }
    }
    else if (app_bt_policy_get_state() == BP_STATE_LINKBACK)
    {
        change_mode = LED_MODE_LINK_BACK;
    }
#if F_APP_ANC_SUPPORT
    else if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_ON) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
    {
        change_mode = LED_MODE_ANC_ON;
    }
#endif
#if F_APP_APT_SUPPORT
    else if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_APT_ON) &&
             !led_specify_led_is_config(LED_SPECIFY_FUNCTION_ANC_APT))
    {
        change_mode = LED_MODE_APT_ON;
    }
#endif
    else
    {
        change_mode = LED_MODE_STANDBY;
    }

    return change_mode;
}

void led_check_mode(void)
{
    T_LED_MODE change_mode;

    if (app_cfg_const.led_support)
    {
        change_mode = LED_MODE_STANDBY;

        if (app_db.device_state == APP_DEVICE_STATE_ON)
        {
            if (app_hfp_get_call_status())
            {
                switch (app_hfp_get_call_status())
                {
                case BT_HFP_CALL_ACTIVE:
                case BT_HFP_CALL_ACTIVE_WITH_CALL_HOLD:
                case BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_HOLD:
                case BT_HFP_VOICE_ACTIVATION_ONGOING:
                    if (app_audio_is_mic_mute())
                    {
                        change_mode = LED_MODE_MIC_MUTE;
                    }
                    else
                    {
                        change_mode = LED_MODE_TALKING;
                    }
                    break;

                case BT_HFP_INCOMING_CALL_ONGOING:
                case BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING:
                case BT_HFP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT:
                    change_mode = LED_MODE_INCOMING_CALL;
                    break;

                case BT_HFP_OUTGOING_CALL_ONGOING:
                    change_mode = LED_MODE_OUTCOMING_CALL;
                    break;

                default:
                    change_mode = active_led_mode;
                    break;
                }
            }
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            else if ((app_db.dongle_is_enable_mic) && (app_audio_is_mic_mute()))
            {
                change_mode = LED_MODE_MIC_MUTE;
            }
#endif
            else
            {
                if ((timer_handle_rws_couple_led != NULL)
#if F_APP_LINEIN_SUPPORT
                    || app_line_in_playing_state_get()
#endif
                   )
                {
                    /*
                        line in usually used in single mode production, use LED_MODE_BUD_COUPLING
                        as line in mode LED.
                    */
                    change_mode = LED_MODE_BUD_COUPLING;
                }
                else
                {
                    /**
                     * rws_couple_led_ext = 1 means that the headset has passed the rws creation mode when the led pre
                     * mode has not stopped, but the rws creation led cannot be flashed at that time, so after the pre
                     * mode ends (led_stop_check = 0), it starts to flash rws_creation_led;
                     */
                    if ((rws_couple_led_ext == 1) && (led_stop_check == 0))
                    {
                        if (timer_handle_rws_couple_led == NULL)
                        {
                            gap_start_timer(&timer_handle_rws_couple_led, "rws_couple_led", led_timer_queue_id,
                                            APP_IO_TIMER_RWS_COUPLE_LED, change_mode, app_cfg_const.timer_rws_couple_led * 1000);
                        }
                    }

                    if (app_bt_policy_get_state() == BP_STATE_PAIRING_MODE)
                    {
                        change_mode = LED_MODE_PAIRING;
                    }
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
                    else if (app_cfg_const.enable_repeat_gaming_mode_led && app_db.gaming_mode)
                    {
                        //LED_MODE_GAMING_MODE is repeat
                        change_mode = LED_MODE_GAMING_MODE;
                    }
#endif
                    else
                    {
                        if ((app_bt_policy_get_b2s_connected_num_with_profile() != 0) || // for primary/single bud
                            (app_db.b2s_connected_num != 0)) // for secondary bud
                        {
                            change_mode = led_check_b2s_connected_mode(change_mode);
                        }
                        else if (app_bt_policy_get_b2b_connected() && (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE))
                        {
                            change_mode = led_check_b2b_connected_mode(change_mode);
                        }
                        else
                        {
                            change_mode = led_check_single_bud_mode(change_mode);
                        }
                    }
                }
            }

#if (F_APP_SENSOR_SUPPORT == 1)
            if (app_sensor_is_i2c_device_fail())
            {
                change_mode = LED_MODE_ALL_BLINKING;
            }
#endif
        }
        else // device power off
        {
            if (app_key_is_enter_pairing())
            {
                change_mode = LED_MODE_PAIRING;
            }
            else
            {
                change_mode = LED_MODE_ALL_OFF;
            }
        }

        led_check_change_mode(change_mode);
    }
}

void led_check_charging_mode(bool from_non_repeat_mode)
{
    T_LED_MODE charging_mode = LED_MODE_ALL_OFF;
    APP_CHARGER_STATE app_charger_state;

    app_charger_state = app_charger_get_charge_state();

    if ((app_db.disallow_charging_led == 1) || (app_cfg_nv.is_dut_test_mode == 1))
    {
        //Disallow charging led about 3s when mcu received valid chargerbox cmd
        //Disallow charging led when dut test mode

        led_reset_charging_led_type();
        led_set_active_charging_led(LED_MODE_ALL_OFF);
        return;
    }


    if (app_cfg_const.led_support)
    {
        if (app_charger_state != APP_CHARGER_STATE_NO_CHARGE)
        {
            switch (app_charger_state)
            {
            case APP_CHARGER_STATE_CHARGING:
                charging_mode = LED_MODE_CHARGING;
                break;

            case APP_CHARGER_STATE_CHARGE_FINISH:
                charging_mode = LED_MODE_CHARGING_COMPLETE;
                break;

            case APP_CHARGER_STATE_ERROR:
                charging_mode = LED_MODE_CHARGING_ERROR;
                break;

            default:
                break;
            }

            if (led_stop_check == 0)
            {
                if ((charging_mode != active_charging_led_mode) || (from_non_repeat_mode == 1))
                {
                    led_set_active_charging_led(charging_mode);
                    led_reset_active_led_mode();
                    led_change_mode(charging_mode, true, false);
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_CHARGER);
                }
            }
        }
        else
        {
            if (charging_mode != active_charging_led_mode)
            {
                led_reset_charging_led_type();
                led_set_active_charging_led(charging_mode);
                led_reset_active_led_mode();
                led_change_mode(charging_mode, true, false);
            }
            app_dlps_enable(APP_DLPS_ENTER_CHECK_CHARGER);
        }
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_CHARGER);
    }
}

RAM_TEXT_SECTION
bool led_is_all_keep_off(void)
{
    if ((led_setting[0].type != LED_TYPE_KEEP_OFF) || (led_setting[1].type != LED_TYPE_KEEP_OFF) ||
        (led_setting[2].type != LED_TYPE_KEEP_OFF))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool led_check_set_mode_available(T_LED_MODE mode)
{
    if (led_stop_check == 1)
    {
        led_pending_mode = mode;
    }

    return !led_stop_check;
}

static uint16_t app_led_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    // uint16_t payload_len = 0;
    // uint8_t *msg_ptr = NULL;
    // bool skip = true;

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_LED, 0, NULL, true, total);
}

static void app_led_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_led_parse_cback: event 0x%04x, status %d", msg_type, status);

    switch (msg_type)
    {
    case LED_MSG_SYNC_FROM_PRI:
        {
            if ((status == REMOTE_RELAY_STATUS_SYNC_TOUT) ||
                (status == REMOTE_RELAY_STATUS_SYNC_EXPIRED) ||
                (status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED) ||
                (status == REMOTE_RELAY_STATUS_SEND_FAILED))
            {
                uint8_t *p_info        = (uint8_t *)buf;
                T_LED_MODE mode        = (T_LED_MODE)p_info[0];
                bool forced_sec_change = (bool)p_info[1];

                APP_PRINT_TRACE3("app_led_parse_cback LED_MSG_SYNC_FROM_PRI 0x%02x -> 0x%02x forced %d",
                                 primary_active_led_mode, mode, forced_sec_change);

                if (led_is_repeat_mode(mode))
                {
                    // update active led mode of primary bud
                    primary_active_led_mode = mode;
                }

                led_change_mode(mode, forced_sec_change, false);

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    led_restart_resync_timer(true);
                }
            }
        }
        break;

    case LED_MSG_REQUEST_NEW_MODE:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
            {
                led_primary_bud_sync_mode();
            }
        }
        break;

    default:
        break;
    }
}

static void app_led_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case BT_EVENT_ACL_ROLE_MASTER:
    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            if (app_db.b2b_role_switch_led_pending == 1)
            {
                app_db.b2b_role_switch_led_pending = 0;

                if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                    (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
                {
                    APP_PRINT_INFO0("led_roleswitch_end_check");
                    led_check_mode(); // check current LED state
                }
            }
        }
        break;

    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                led_primary_bud_sync_mode();
            }
        }
        break;

    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
            led_restart_resync_timer(false);
        }
        break;

    default:
        break;
    }
}

void led_init(void)
{
    bt_mgr_cback_register(app_led_bt_cback);
    led_pad_config();
    led_set_driver_mode();
    gap_reg_timer_cb(led_timer_callback, &led_timer_queue_id);
    app_relay_cback_register(app_led_relay_cback, app_led_parse_cback,
                             APP_MODULE_TYPE_LED, LED_MSG_TOTAL);
}
