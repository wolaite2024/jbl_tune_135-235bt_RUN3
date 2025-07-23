/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "board.h"
#include "rtl876x_gdma.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "rtl876x_uart.h"
#include "rtl876x_nvic.h"
#include "io_dlps.h"
#include "gap_timer.h"
#include "app_dlps.h"
#include "app_led.h"
#include "app_main.h"
#include "section.h"
#include "app_gpio.h"
#include "app_key_process.h"
#include "app_key_gpio.h"
#include "mfb_api.h"
#include "app_charger.h"
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
#include "app_cfg.h"
#include "app_mmi.h"
#include "app_audio_policy.h"
#include "app_sensor.h"
#include "pm.h"
#include "vector_table.h"
#include "app_charger.h"
#include "app_auto_power_off.h"
#include "os_timer.h"
#include "system_status_api.h"
#include "console_uart.h"
#include "os_sync.h"
#include "io_dlps.h"
#include "app_adp.h"

#if GFPS_FEATURE_SUPPORT
#include "app_gfps_device.h"
#include "app_gfps.h"
#endif
#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
#include "rtl876x_rtc.h"
#include "adapter.h"
#endif
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
#include "app_ext_charger.h"
#endif
#if HARMAN_NTC_DETECT_PROTECT
#include "app_harman_adc.h"
#endif
#if GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#endif

#define APP_IO_TIMER_POWER_DOWN_WDG (0)
#define APP_IO_TIMER_ADAPTOR_STABLE (1)

#define POWER_DOWN_WDG_TIMER     (500)
#define POWER_DOWN_WDG_CHK_TIMES (40)

/*********************************************
BB2 must use io_dlps.h
*********************************************/
static uint32_t dlps_bitmap;                /**< dlps locking bitmap */
static uint8_t app_dlps_timer_queue_id = 0;
static void *timer_handle_power_down_wdg = NULL;
static uint32_t pd_wdg_chk_times = 0;

#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)

static bool app_dlps_need_to_wakeup_by_rtc(void)
{
    bool ret = false;

    /* for smart charger control, get 0% should not wakeup recharge ==> this condition is same to judge adp in when enter dlps */
    // if (ADP_DET_status == ADP_DET_IN)
    // {
    //     ret = true;
    // }
#if HARMAN_VBAT_ADC_DETECTION
    if (extend_app_cfg_const.power_off_rtc_wakeup_timeout != 0)
    {
        ret = true;
    }
#endif

    return ret;
}

static uint32_t app_dlps_get_system_wakeup_time(void)
{
    uint32_t wakeup_time;

    wakeup_time = extend_app_cfg_const.power_off_rtc_wakeup_timeout;
    // uint32_t wakeup_time = 2 * 24 * 60 * 60; /* 2days */
    // wakeup_time = 60; /* 60s */

    return wakeup_time;
}

static void app_dlps_system_wakeup_by_rtc(uint32_t wakeup_time)
{
    uint8_t comparator_index = COMP0GT_INDEX;
    uint32_t prescaler_value = RTC_PRESCALER_VALUE; /* 1 counter : (prescaler_value + 1)/32000  sec*/

    uint32_t comparator_value = (uint32_t)(((uint64_t)wakeup_time * 32000) / (prescaler_value + 1));

    RTC_DeInit();
    RTC_SetPrescaler(prescaler_value);

    RTC_CompINTConfig(RTC_CMP0GT_INT, ENABLE);

    RTC_SystemWakeupConfig(ENABLE);
    RTC_RunCmd(ENABLE);

    uint32_t current_value = 0;

    current_value = RTC_GetCounter();
    RTC_SetComp(comparator_index, current_value + comparator_value);
}

void app_dlps_system_wakeup_clear_rtc_int(void)
{
    if (RTC_GetINTStatus(RTC_CMP0GT_INT) == SET)
    {
        RTC_ClearINTStatus(RTC_CMP0GT_INT);
    }

    if (0)
    {
        // Debug
        uint32_t rtc_counter = 0;
        rtc_counter = RTC_GetCounter();
        APP_PRINT_INFO1("app_dlps_system_wakeup_clear_rtc_int: RTC wakeup, rtc_counter %d", rtc_counter);
    }

    // RTC_RunCmd(DISABLE);
}
#endif

RAM_TEXT_SECTION void app_dlps_enable(uint32_t bit)
{
    if (dlps_bitmap & bit)
    {
        APP_PRINT_TRACE3("app_dlps_enable: %08x %08x -> %08x", bit, dlps_bitmap,
                         (dlps_bitmap & ~bit));
    }

    uint32_t s = os_lock();
    dlps_bitmap &= ~bit;
    os_unlock(s);
}

RAM_TEXT_SECTION void app_dlps_disable(uint32_t bit)
{
    if (app_cfg_const.charger_control_by_mcu && (bit == APP_DLPS_ENTER_CHECK_CHARGER))
    {
        /* charger control by mcu */
        return;
    }
    if ((dlps_bitmap & bit) == 0)
    {
        APP_PRINT_TRACE3("app_dlps_disable: %08x %08x -> %08x", bit, dlps_bitmap,
                         (dlps_bitmap | bit));
    }

    uint32_t s = os_lock();
    dlps_bitmap |= bit;
    os_unlock(s);
}

RAM_TEXT_SECTION bool app_dlps_check_callback(void)
{
    static uint32_t dlps_bitmap_pre;
    bool dlps_enter_en = false;
    POWERMode lps_mode = power_mode_get();

    if ((app_cfg_const.enable_dlps) && (dlps_bitmap == 0))
    {
        dlps_enter_en = true;
        if ((POWER_DLPS_MODE == lps_mode) && (app_db.device_state == APP_DEVICE_STATE_ON))
        {
            set_io_power_in_lps_mode(true);
        }
    }

    if ((dlps_bitmap_pre != dlps_bitmap) && !dlps_enter_en && app_cfg_const.enable_dlps)
    {
        APP_PRINT_WARN2("app_dlps_check_callback: dlps_bitmap_pre 0x%x dlps_bitmap 0x%x",
                        dlps_bitmap_pre,
                        dlps_bitmap);
    }
    dlps_bitmap_pre = dlps_bitmap;


#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
    if (lps_mode == POWER_POWERDOWN_MODE && dlps_enter_en)
    {
        extern void (*set_clk_32k_power_in_powerdown)(bool);
        set_clk_32k_power_in_powerdown(true);

#if GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support &&
            !extend_app_cfg_const.disable_finder_adv_when_power_off)
        {
            //save some information into flash
            app_gfps_finder_rtc_wakeup_save_info();
        }
#endif
    }
#endif

    return dlps_enter_en;
}

/**
    * @brief   Need to handle message in this callback function,when App enter dlps mode
    * @param  void
    * @return void
    */
void app_dlps_enter_callback(void)
{
    POWERMode lps_mode = power_mode_get();
    uint32_t i;

    if ((lps_mode == POWER_POWERDOWN_MODE) || (lps_mode == POWER_POWEROFF_MODE))
    {
#if F_APP_HARMAN_FEATURE_SUPPORT
        app_cfg_nv.app_is_power_on = 0;
#endif
        DBG_DIRECT("app_dlps_enter_callback: lps_mode %d", lps_mode);
    }

#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
    if (lps_mode == POWER_POWERDOWN_MODE)
    {
        uint32_t wakeup_time = app_dlps_get_system_wakeup_time();
        uint32_t total_wakeup_time = wakeup_time * app_cfg_nv.rtc_wakeup_count;

        if ((wakeup_time > 0)
#if HARMAN_PACE_SUPPORT || HARMAN_RUN3_SUPPORT
            && (total_wakeup_time <= 7 * 24 * 60 * 60)
#endif
           )
        {
            app_dlps_system_wakeup_by_rtc(wakeup_time);
            DBG_DIRECT("app_dlps_system_wakeup_by_rtc: %d sec, total: %d", wakeup_time, total_wakeup_time);
			app_ext_charger_current_ctrl_pin_pull_down();
        }
        else
        {
            power_mode_set(POWER_POWEROFF_MODE);
        }
    }
#endif

    if (app_cfg_const.key_gpio_support)
    {
        if (((app_adp_get_plug_state() == ADAPTOR_UNPLUG) &&
             (app_cfg_const.discharger_support &&
              (app_charger_get_soc() == BAT_CAPACITY_0))) ||
            (app_cfg_const.key_disable_power_on_off && lps_mode == POWER_POWERDOWN_MODE))
        {
            if (app_cfg_const.key_enable_mask & KEY0_MASK)
            {
                System_WakeUpPinDisable(app_cfg_const.key_pinmux[0]);
            }
            else
            {
                /* disable MFB wakeup */
                Pad_WakeUpCmd(MFB_MODE, POL_LOW, DISABLE);
            }
        }
        else
        {
            if (app_cfg_const.key_enable_mask & KEY0_MASK)
            {
                app_dlps_pad_wake_up_enable(app_cfg_const.key_pinmux[0]);
            }
        }

        if (app_cfg_const.enable_combinekey_power_onoff)
        {
            app_power_onoff_combinekey_dlps_process();
        }
    }

    if (app_cfg_const.enable_data_uart || app_cfg_const.one_wire_uart_support)
    {
        console_uart_enter_low_power(lps_mode);
    }

#if F_APP_GPIO_ONOFF_SUPPORT
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        app_gpio_on_off_enter_dlps_pad_set();
    }
#endif
    if (lps_mode == POWER_DLPS_MODE)
    {
        if (app_db.device_state != APP_DEVICE_STATE_OFF)
        {
            if (app_cfg_const.key_gpio_support)
            {
                //Key1 ~ Key7 are allowed to wake up system in non-off state
                for (i = 1; i < MAX_KEY_NUM; i++)
                {
                    if (app_cfg_const.key_enable_mask & (1U << i))
                    {
                        app_dlps_pad_wake_up_enable(app_cfg_const.key_pinmux[i]);
                    }
                }
            }
        }
    }
    else if (power_mode_get() == POWER_POWERDOWN_MODE)
    {
        if (app_cfg_const.key_gpio_support)
        {
            for (i = 1; i < MAX_KEY_NUM; i++)
            {
                if ((app_cfg_const.key_enable_mask & BIT(i)) && !(app_key_is_combinekey_power_on_off(i)))
                {
                    System_WakeUpPinDisable(app_cfg_const.key_pinmux[i]);
                    System_WakeUpInterruptDisable(app_cfg_const.key_pinmux[i]);
                }
            }
        }
    }

    if (app_cfg_const.smart_charger_control)
    {
        /* after get zero box vol; the 5v will drop */
        if (app_db.adp_high_wake_up_for_zero_box_bat_vol)
        {
            Pad_WakeUpCmd(ADP_MODE, POL_HIGH, ENABLE);
        }
    }
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
    if (app_ext_charger_check_support())
    {
        app_ext_charger_handle_enter_dlps();
    }
#endif
}

extern void app_adp_det_handler(void);
void app_dlps_exit_callback(void)
{
    sys_hall_get_dlps_aon_info();

    //POWER_POWERDOWN_MODE and LPM_HIBERNATE_MODE will reboot directly and not execute exit callback
    // need to add power off mode
    if ((power_mode_get() == POWER_DLPS_MODE))
    {
        if (app_cfg_const.key_gpio_support)
        {
            uint32_t i;

            for (i = 0; i < MAX_KEY_NUM; i++)
            {
                if (app_cfg_const.key_enable_mask & (1U << i))
                {
                    Pad_ControlSelectValue(app_cfg_const.key_pinmux[i], PAD_PINMUX_MODE);

                    //Key1 ~ Key5 are edge trigger. Handle key press directly
                    if ((i >= 1) && (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[i]) == 1))
                    {
                        //Edge trigger will mis-detect when wake up
                        APP_PRINT_INFO0("app_dlps_exit_callback: key_gpio_support gpio_handler");
                        gpio_handler(GPIO_GetNum(app_cfg_const.key_pinmux[i]));
                    }
                }
            }
        }

        if (app_cfg_const.enable_data_uart || app_cfg_const.one_wire_uart_support)
        {
            console_uart_exit_low_power(POWER_DLPS_MODE);
        }
#if F_APP_GPIO_ONOFF_SUPPORT
        if (app_cfg_const.box_detect_method == GPIO_DETECT)
        {
            app_gpio_detect_onoff_exit_dlps_process();
        }
#endif

    }
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
    if (app_ext_charger_check_support())
    {
        app_ext_charger_handle_exit_dlps();
    }
#endif
}

static bool app_dlps_platform_pm_check(void)
{
    uint8_t platform_pm_error_code = power_get_error_code();
    APP_PRINT_INFO1("app_dlps_platform_pm_check, ERR Code:%d", platform_pm_error_code);
    return (platform_pm_error_code == PM_ERROR_WAKEUP_TIME);
    //pmu ctrl, must no error
    //BB2 must flow pmu flow
}

static void app_dlps_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{

    APP_PRINT_TRACE3("app_dlps_timer_callback: timer_id 0x%02x, timer_chann %d, chk_times:%d", timer_id,
                     timer_chann, pd_wdg_chk_times);

    switch (timer_id)
    {
    case APP_IO_TIMER_POWER_DOWN_WDG:
        {
            //handle timing issue
            if (!timer_handle_power_down_wdg)
            {
                break;
            }
            gap_stop_timer(&timer_handle_power_down_wdg);

            pd_wdg_chk_times++;
            if (pd_wdg_chk_times == POWER_DOWN_WDG_CHK_TIMES)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF);
                app_dlps_enable(0xFFFF);
            }

            if (app_dlps_platform_pm_check() && app_db.device_state == APP_DEVICE_STATE_OFF)
            {
                pd_wdg_chk_times = 0;
                power_stop_all_non_excluded_timer();
                os_timer_dump();
            }
            else
            {
                if (app_db.device_state != APP_DEVICE_STATE_ON)
                {
                    gap_start_timer(&timer_handle_power_down_wdg, "power_down_wdg",
                                    app_dlps_timer_queue_id,
                                    APP_IO_TIMER_POWER_DOWN_WDG, 0, POWER_DOWN_WDG_TIMER);
                }
                else
                {
                    pd_wdg_chk_times = 0;
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_dlps_power_off(void)
{
    if (app_cfg_const.enable_power_off_to_dlps_mode)
    {
        power_mode_set(POWER_DLPS_MODE);
    }
    else
    {

        if (app_cfg_const.enter_shipping_mode_if_outcase_power_off
            && (app_device_is_in_the_box() == false)
#if (F_APP_PERIODIC_WAKEUP_RECHARGE == 1)
            && !app_dlps_need_to_wakeup_by_rtc()
#endif
           )
        {
            power_mode_set(POWER_POWEROFF_MODE);
        }
        else
        {
            power_mode_set(POWER_POWERDOWN_MODE); // POWER_DLPS_MODE
        }
        DBG_DIRECT("app_dlps_power_off: set power mode %d", power_mode_get());

        gap_start_timer(&timer_handle_power_down_wdg, "power_down_wdg",
                        app_dlps_timer_queue_id,
                        APP_IO_TIMER_POWER_DOWN_WDG, 0, POWER_DOWN_WDG_TIMER);

        app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF);
        power_register_excluded_handle(&timer_handle_power_down_wdg, PM_EXCLUDED_TIMER);
    }
}

void app_dlps_enable_auto_poweroff_stop_wdg_timer(void)
{
    pd_wdg_chk_times = 0;
    app_auto_power_off_enable(AUTO_POWER_OFF_MASK_ALREADY_POWER_OFF,
                              app_cfg_const.timer_auto_power_off);
    gap_stop_timer(&timer_handle_power_down_wdg);
}

void app_dlps_stop_power_down_wdg_timer(void)
{
    pd_wdg_chk_times = 0;
    gap_stop_timer(&timer_handle_power_down_wdg);
}

void app_dlps_start_power_down_wdg_timer(void)
{
    if (app_db.device_state != APP_DEVICE_STATE_ON)
    {
        gap_start_timer(&timer_handle_power_down_wdg, "power_down_wdg",
                        app_dlps_timer_queue_id,
                        APP_IO_TIMER_POWER_DOWN_WDG, 0, POWER_DOWN_WDG_TIMER);
    }
}

bool app_dlps_check_short_press_power_on(void)
{
    bool ret = false;

    //When use POWER_POWERDOWN_MODE,
    //system will re-boot after wake up and not execute DLPS exit callback
    if ((app_cfg_const.key_gpio_support) && (app_cfg_const.key_power_on_interval == 0))
    {
        if (System_WakeUpInterruptValue(app_cfg_const.key_pinmux[0]) == 1)
        {
            //GPIO INT not triggered before short click release MFB key
            //Use direct power on for short press power on case
            if (app_cfg_const.discharger_support)
            {
                APP_CHARGER_STATE app_charger_state;
                uint8_t state_of_charge; //MUST be detected after task init

                app_charger_state = app_charger_get_charge_state();
                state_of_charge = app_charger_get_soc();
                if ((app_charger_state == APP_CHARGER_STATE_NO_CHARGE) && (state_of_charge == BAT_CAPACITY_0))
                {
                }
                else
                {
                    ret = true;
                }
            }
            else
            {
                ret = true;
            }
        }
    }

    APP_PRINT_INFO1("app_dlps_check_short_press_power_on: ret %d", ret);
    return ret;
}

RAM_TEXT_SECTION uint32_t app_dlps_get_dlps_bitmap(void)
{
    return dlps_bitmap;
}

ISR_TEXT_SECTION void app_dlps_set_pad_wake_up(uint8_t pinmux,
                                               PAD_WAKEUP_POL_VAL wake_up_val)
{
    Pad_ControlSelectValue(pinmux, PAD_SW_MODE);
    System_WakeUpPinEnable(pinmux, wake_up_val);
    System_WakeUpInterruptEnable(pinmux);
}

ISR_TEXT_SECTION void app_dlps_pad_wake_up_enable(uint8_t pinmux)
{
    Pad_ControlSelectValue(pinmux, PAD_SW_MODE);
    Pad_WakeupEnableValue(pinmux, 1);
    System_WakeUpInterruptEnable(pinmux);
}

ISR_TEXT_SECTION void app_dlps_pad_wake_up_polarity_invert(uint8_t pinmux)
{
    uint8_t gpio_level = gpio_read_input_level(pinmux);
    Pad_WakeupPolarityValue(pinmux,
                            gpio_level ? PAD_WAKEUP_POL_LOW : PAD_WAKEUP_POL_HIGH);
}

void app_dlps_restore_pad(uint8_t pinmux)
{
    Pad_ControlSelectValue(pinmux, PAD_PINMUX_MODE);
    System_WakeUpPinDisable(pinmux);

    if (System_WakeUpInterruptValue(pinmux) == 1)
    {
        //Edge trigger will mis-detect when wake up
        APP_PRINT_INFO1("app_dlps_restore_pad gpio_handler pin= %s",
                        TRACE_STRING(Pad_GetPinName(pinmux)));
        gpio_handler(GPIO_GetNum(pinmux));
    }
}

void app_dlps_init(void)
{
    if (!app_cfg_const.enable_dlps)
    {
        return;
    }
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);

    DLPS_IORegister();

    /* register of call back function */
    if (power_check_cb_register(app_dlps_check_callback) == false)
    {
        APP_PRINT_ERROR0("app_dlps_init: dlps_check_cb_reg failed");
    }

    DLPS_IORegUserDlpsEnterCb(app_dlps_enter_callback);
    DLPS_IORegUserDlpsExitCb(app_dlps_exit_callback);

    gap_reg_timer_cb(app_dlps_timer_callback, &app_dlps_timer_queue_id);

    app_dlps_power_off();

    if (app_cfg_nv.factory_reset_done == 0)
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_MPTEST);
    }
}

/* When system is wakeup from dlps mode, peripheral will call this function.
 * App will disable correspondent wakeup pinmux.
 */
void System_Handler(void)
{
    uint32_t i;

    APP_PRINT_INFO0("System_Handler exit dlps cb enable System_IRQn if wakup pin set triggle this handler");

    NVIC_DisableIRQ(System_IRQn);  //disable System_Handler Interrupt

    for (i = 0; i < TOTAL_PIN_NUM; i++)
    {
        if (System_WakeUpInterruptValue(i) == SET)
        {
            if (i == app_cfg_const.key_pinmux[0])
            {
                app_db.key0_wake_up = true;
            }

            if (app_key_is_gpio_combinekey_power_on_off(i))
            {
                app_db.combine_poweron_key_wake_up = true;
            }

            app_db.peri_wake_up = true;
            //DBG_DIRECT("DBG_DIRECT System_Handler wakeup pin = %s ", Pad_GetPinName(i));
            APP_PRINT_INFO1("wakeup for power down pin= %d ", i);
        }
    }
}
