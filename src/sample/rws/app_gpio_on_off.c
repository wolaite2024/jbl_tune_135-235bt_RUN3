/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_GPIO_ONOFF_SUPPORT
#include "trace.h"
#include "app_gpio_on_off.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "wdg.h"
#include "ftl.h"
#include "pm.h"
#include "gap_timer.h"
#include "sysm.h"
#include "remote.h"
#include "ringtone.h"
#include "app_mmi.h"
#include "board.h"
#include "app_cfg.h"
#include "app_gpio.h"
#include "app_io_msg.h"
#include "app_main.h"
#include "app_dlps.h"
#include "section.h"
#include "app_in_out_box.h"
#include "app_sensor.h"
#include "app_io_output.h"
#include "app_auto_power_off.h"
#include "app_adp.h"

static void *timer_handle_gpio_detect_debounce = NULL;
static uint8_t app_gpio_detect_timer_queue_id = 0;
static uint8_t gpio_onoff_pin_num = 0xFF;

typedef enum
{
    APP_IO_TIMER_GPIO_DETECT_DEBOUNCE,
} T_APP_GPIO_DETECT_TIMER;

ISR_TEXT_SECTION static uint8_t gpio_detect_onoff_read_pin_status(void)
{
    uint8_t value = 0;

    if (gpio_onoff_pin_num != 0xFF)
    {
        value = gpio_read_input_level(gpio_onoff_pin_num);
    }
    return value;
}

static void app_3pin_in_out_box_handle(T_CASE_LOCATION_STATUS local)
{
    bool enable_handle = false;

    app_cfg_nv.gpio_out_detected = 0;

    if (local == OUT_CASE)
    {
        if (app_cfg_nv.pre_3pin_status_unplug)
        {
            //do nothing
        }
        else
        {
            app_cfg_nv.gpio_out_detected = 1;
            app_cfg_nv.pre_3pin_status_unplug = 1;
            enable_handle = true;
        }
    }
    else
    {
        if (!app_cfg_nv.pre_3pin_status_unplug)
        {
            //do nothing
        }
        else
        {
            app_cfg_nv.pre_3pin_status_unplug = 0;
            enable_handle = true;
        }
    }

    app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

    APP_PRINT_TRACE4("app_3pin_in_out_box_handle: enable_handle %d, local %d, pre_3pin_status_unplug %d, gpio_out_detected %d",
                     enable_handle, local, app_cfg_nv.pre_3pin_status_unplug, app_cfg_nv.gpio_out_detected);

    if (enable_handle)
    {
        app_in_out_box_handle(local);
    }
}

static void app_gpio_detect_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    uint8_t gpio_num = 0;/* specified GPIO pin number*/
    GPIO_TypeDef *GPIOx;
    gpio_num = GPIO_GetNum(app_cfg_const.key_pinmux[0]);
    GPIOx = gpio_num <= GPIO31 ? GPIOA : GPIOB;
    switch (timer_id)
    {
    case APP_IO_TIMER_GPIO_DETECT_DEBOUNCE:
        {
            T_CASE_LOCATION_STATUS local;

            gap_stop_timer(&timer_handle_gpio_detect_debounce);

            if (app_cfg_const.key_gpio_support)
            {
                if (app_cfg_const.key_enable_mask & KEY0_MASK)
                {
                    GPIOx_INTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]),
                                    ENABLE);
                    // GPIO_INTConfig(GPIO_GetPin(app_cfg_const.key_pinmux[0]), ENABLE);
                }
            }

            local = gpio_detect_onoff_get_location();

            app_db.local_in_case = (local == IN_CASE) ? true : false;
            app_db.local_loc = app_sensor_bud_loc_detect();

            if (app_cfg_const.enable_rtk_charging_box)
            {
                if (app_db.local_loc == BUD_LOC_IN_CASE)
                {
                    app_adp_pending_cmd_exec();

                    app_3pin_in_out_box_handle(IN_CASE);
                }
                else
                {
                    app_adp_clear_pending();

                    /* disable_charger_by_box_battery flag must be clear when 3pin out */
                    app_adp_set_disable_charger_by_box_battery(false);

                    if (app_cfg_const.smart_charger_control && app_db.adp_high_wake_up_for_zero_box_bat_vol)
                    {
                        //disallow adp out power on feature when receive box zero volume
                        APP_PRINT_TRACE0("ignore 3pin out when get box zero volume");
                        if (!app_cfg_nv.adp_factory_reset_power_on)
                        {
                            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                        }
                        return;
                    }
                    else
                    {
                        app_3pin_in_out_box_handle(OUT_CASE);
                    }
                }

                app_sensor_bud_loc_sync();

                if (!app_cfg_nv.adp_factory_reset_power_on)
                {
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                }
                break;
            }

            if (app_db.local_loc == BUD_LOC_IN_CASE)
            {
                app_3pin_in_out_box_handle(IN_CASE);
            }
            else
            {
                app_3pin_in_out_box_handle(OUT_CASE);
            }
            app_sensor_bud_loc_sync();
            // Allow DLPS after debouncing.
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
        break;

    default:
        break;
    }
}

ISR_TEXT_SECTION T_CASE_LOCATION_STATUS gpio_detect_onoff_get_location(void)
{
    T_CASE_LOCATION_STATUS location = IN_CASE;

    if (gpio_detect_onoff_read_pin_status() == 1)
    {
        location = OUT_CASE;
    }
    return location;
}

ISR_TEXT_SECTION void gpio_detect_intr_handler(void)
{
    T_IO_MSG gpio_msg;

    uint8_t gpio_num = 0;/* specified GPIO pin number*/
    GPIO_TypeDef *GPIOx, *GPIOx_key0;
    gpio_num = GPIO_GetNum(gpio_onoff_pin_num);
    GPIOx = gpio_num <= GPIO31 ? GPIOA : GPIOB;
    gpio_num = GPIO_GetNum(app_cfg_const.key_pinmux[0]);
    GPIOx_key0 = gpio_num <= GPIO31 ? GPIOA : GPIOB;

    APP_PRINT_TRACE1("gpio_detect_intr_handler pin_level=%d", gpio_detect_onoff_read_pin_status());

    /* Disable GPIO interrupt */
    GPIOx_INTConfig(GPIOx, GPIO_GetPin(gpio_onoff_pin_num),
                    DISABLE);
    GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(gpio_onoff_pin_num),
                        ENABLE);
    GPIOx_ClearINTPendingBit(GPIOx, GPIO_GetPin(gpio_onoff_pin_num));

    //  GPIO_INTConfig(GPIO_GetPin(gpio_onoff_pin_num), DISABLE);
//   GPIO_MaskINTConfig(GPIO_GetPin(gpio_onoff_pin_num), ENABLE);
    //  GPIO_ClearINTPendingBit(GPIO_GetPin(gpio_onoff_pin_num));

    /* Change GPIO Interrupt Polarity */
    if (gpio_detect_onoff_read_pin_status())
    {
        if (app_cfg_const.key_gpio_support)
        {
            if (app_cfg_const.key_enable_mask & KEY0_MASK)
            {
                GPIOx_INTConfig(GPIOx_key0, GPIO_GetPin(app_cfg_const.key_pinmux[0]), DISABLE);
            }
        }

        GPIOx->INTPOLARITY &= ~(GPIO_GetPin(gpio_onoff_pin_num)); //Polarity Low
        gpio_msg.u.param = 1;
    }
    else
    {
        GPIOx->INTPOLARITY |= GPIO_GetPin(gpio_onoff_pin_num); //Polarity High
        gpio_msg.u.param = 0;
    }
    /*add  pad status check update for miss gpio interrupt when enter dlps*/
    if (app_cfg_const.box_detect_method == GPIO_DETECT)
    {
        app_gpio_on_off_set_pad_wkup_polarity();
    }
    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_CHARGERBOX_DETECT;

    if (app_io_send_msg(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("gpio_detect_intr_handler: Send gpio detect msg error");
    }

    /* Enable GPIO interrupt */
    GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(gpio_onoff_pin_num), DISABLE);
    GPIOx_INTConfig(GPIOx, GPIO_GetPin(gpio_onoff_pin_num), ENABLE);
}

void app_gpio_detect_onoff_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint32_t in_out_box_detect_time = 0;
    gap_stop_timer(&timer_handle_gpio_detect_debounce);

    // Disallow DLPS when debouncing.
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    if (io_driver_msg_recv->u.param == 0)
    {
        if (app_cfg_const.enable_inbox_power_off)
        {
            if (app_cfg_const.enable_rtk_charging_box == 0)//for smart box, plug not to power off
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_IN_BOX);
            }
        }
    }
    else
    {
        if (app_cfg_const.enable_outbox_power_on)
        {
            if (app_cfg_const.enable_rtk_charging_box == 0)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_IN_BOX, app_cfg_const.timer_auto_power_off);
            }
        }
    }

    if (app_cfg_const.enable_rtk_charging_box)
    {
        if (io_driver_msg_recv->u.param && app_db.local_in_case) //detect out
        {
            in_out_box_detect_time = SMART_CHARGERBOX_DETECT_OUT_TIMER;
        }
        else if (!io_driver_msg_recv->u.param && (!app_db.local_in_case)) //detect in
        {
            if (app_cfg_const.smart_charger_box_bit_length == 0) // 1: 20ms, 0 40ms
            {
                in_out_box_detect_time = SMART_CHARGERBOX_DETECT_IN_TIMER_40MS;
            }
            else
            {
                in_out_box_detect_time = SMART_CHARGERBOX_DETECT_IN_TIMER_20MS;
            }
        }
        else
        {
            //do nothing
        }
    }
    else
    {
        in_out_box_detect_time = CHARGERBOX_DETECT_TIMER;
    }

    if (in_out_box_detect_time != 0)
    {
        gap_start_timer(&timer_handle_gpio_detect_debounce, "detect_debounce",
                        app_gpio_detect_timer_queue_id, APP_IO_TIMER_GPIO_DETECT_DEBOUNCE,
                        0, in_out_box_detect_time);
    }
}

void app_gpio_detect_onoff_board_init(uint8_t pin_num)
{
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    gpio_onoff_pin_num = pin_num;
    Pinmux_Config(gpio_onoff_pin_num, DWGPIO);
    Pad_Config(gpio_onoff_pin_num,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

bool app_gpio_detect_onoff_debounce_timer_started(void)
{
    return ((timer_handle_gpio_detect_debounce != NULL) ? true : false);
}

bool gpio_on_off_dlps_check(void)
{
    if (timer_handle_gpio_detect_debounce != NULL)
    {
        return false;
    }

    return true;
}

void app_gpio_detect_onoff_driver_init(uint8_t pin_num)
{
    /*gpio init*/
    GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */
    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(pin_num);
    gpio_param.GPIO_ITCmd = ENABLE;
    gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    gpio_param.GPIO_DebounceTime = GPIO_DETECT_DEBOUNCE_TIME;

    if (GPIO_GetNum(pin_num) <= GPIO31)
    {
        /* Enable  Peripheral clodk,Interrupt*/
        RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
        GPIOx_Init(GPIOA, &gpio_param);
        GPIOx_MaskINTConfig(GPIOA, GPIO_GetPin(pin_num), DISABLE);
        GPIOx_INTConfig(GPIOA, GPIO_GetPin(pin_num), ENABLE);
    }
    else
    {
        RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
        GPIOx_Init(GPIOB, &gpio_param);
        GPIOx_MaskINTConfig(GPIOB, GPIO_GetPin(pin_num), DISABLE);
        GPIOx_INTConfig(GPIOB, GPIO_GetPin(pin_num), ENABLE);
    }

    /* Enable  CPU NVIC) */
    gpio_init_irq(GPIO_GetNum(pin_num));
    /*register gpio detect timer callback queue,device manager callback*/
    gap_reg_timer_cb(app_gpio_detect_timeout_cb, &app_gpio_detect_timer_queue_id);

    power_check_cb_register(gpio_on_off_dlps_check);
}

ISR_TEXT_SECTION void app_gpio_on_off_set_pad_wkup_polarity(void)
{
    /*GPIO detect pin - High: power on, Low: power off*/
    PAD_WAKEUP_POL_VAL wake_up_val;

    wake_up_val = (gpio_detect_onoff_read_pin_status()) ? PAD_WAKEUP_POL_LOW : PAD_WAKEUP_POL_HIGH;
    Pad_WakeupPolarityValue(gpio_onoff_pin_num, wake_up_val);

    APP_PRINT_INFO2("app_gpio_on_off_set_pad_wkup_polarity SET detct_pin = %d wake_up_val =%d",
                    gpio_onoff_pin_num, wake_up_val);
}

void  app_gpio_on_off_enter_dlps_pad_set(void)
{
    app_dlps_pad_wake_up_enable(gpio_onoff_pin_num);
}

void app_gpio_detect_onoff_exit_dlps_process(void)
{
    app_dlps_restore_pad(gpio_onoff_pin_num);
}
#endif
