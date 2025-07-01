/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     led.c
* @brief    This file provides functions to set some LED working mode.
* @details
* @author   brenda_li
* @date     2016-12-15
* @version  v0.1
*********************************************************************************************************
*/
#include "string.h"
#include "rtl876x_pinmux.h"
#include "led_module.h"
#include "gap_timer.h"
#include "app_cfg.h"
#include "rtl876x.h"
#include "rtl876x_sleep_led.h"
#include "os_sched.h"
#include "os_mem.h"
#include "os_sync.h"
#include "board.h"
#include "pwm.h"
#include "hw_tim.h"
#include "trace.h"

typedef struct
{
    uint8_t led_breath_timer_count;
    uint8_t led_breath_blink_count;
    uint8_t pwm_id;
    uint8_t led_channel;
} T_LED_SW_BREATH_MODE;

#define PWM_OUT_COUNT (10000) //time Unit: 10ms(10000/1MHz)

static T_LED_SW_BREATH_MODE sw_breathe_mode[LED_NUM];
static T_PWM_HANDLE pwm_handle[LED_NUM];
static uint32_t led_creat_timer_point[LED_NUM];
static uint32_t led_creat_timer_record[LED_NUM];
static uint8_t led_polarity[LED_NUM];
static bool led_driver_by_sw;
static uint8_t *led0_pinmux = &app_cfg_const.led_0_pinmux;
static const char *led_timer_name[LED_NUM] = {"led0_on_off", "led1_on_off", "led2_on_off"};
static void *timer_handle_led[LED_NUM];
static uint8_t led_module_timer_queue_id = 0;
static T_LED_TABLE led_setting_record[LED_NUM];
static T_LED_TABLE led_count_record[LED_NUM];
static void led_clear_para(T_LED_CHANNEL channel);
static void led_blink_handle(T_LED_CHANNEL led_ch);
bool led_is_sleep_pinmux(uint8_t Pin_Num);
static void led_sw_breath_timer_control(uint8_t i, FunctionalState new_state);
static void led_sw_breath_init(uint8_t i);

uint8_t  sleep_led_pinmux[SLEEP_LED_PINMUX_NUM] = {P2_1, P2_2, ADC_0, ADC_1, P2_3, P2_4, P3_0, P3_1, P1_0, P1_1};

void led_set_active_polarity(T_LED_CHANNEL channel, T_LED_ACTIVE_POLARITY polarity)
{
    //SleepLed_SetIdleMode((SLEEP_LED_CHANNEL)channel, polarity);

    //FW work around
    switch (channel)
    {
    case LED_CH_0:
        led_polarity[0] = polarity;
        break;
    case LED_CH_1:
        led_polarity[1] = polarity;
        break;
    case LED_CH_2:
        led_polarity[2] = polarity;
        break;
    }
}

static void led_cmd_handle(uint8_t i, uint8_t channel, bool state)
{
    uint32_t s;

    if ((led_driver_by_sw) && (!led_is_sleep_pinmux(*(led0_pinmux + i))))
    {
        //LED should be handled simultaneously
        //Avoid APP be preempted by higher priority task
        s = os_lock();

        if (state)
        {
            //set pad and start timer
            if (LED_CH_0 & channel)
            {
                led_blink_handle(LED_CH_0);
            }
            if (LED_CH_1 & channel)
            {
                led_blink_handle(LED_CH_1);
            }
            if (LED_CH_2 & channel)
            {
                led_blink_handle(LED_CH_2);
            }
        }
        else
        {
            //reset pad and stop timer
            if (LED_CH_0 & channel)
            {
                led_deinit(LED_CH_0);
                led_clear_para(LED_CH_0);
                gap_stop_timer(&timer_handle_led[0]);
            }
            if (LED_CH_1 & channel)
            {
                led_deinit(LED_CH_1);
                led_clear_para(LED_CH_1);
                gap_stop_timer(&timer_handle_led[1]);
            }
            if (LED_CH_2 & channel)
            {
                led_deinit(LED_CH_2);
                led_clear_para(LED_CH_2);
                gap_stop_timer(&timer_handle_led[2]);
            }
        }

        os_unlock(s);
    }
    else
    {
        SleepLed_Cmd(channel, (FunctionalState)state);
    }
}

void led_cmd(uint8_t channel, bool state)
{
    uint8_t i  = 0;

    if (LED_CH_0 & channel)
    {
        i  = 0 ;
        led_cmd_handle(i, LED_CH_0, state);
    }
    if (LED_CH_1 & channel)
    {
        i  = 1 ;
        led_cmd_handle(i, LED_CH_1, state);
    }
    if (LED_CH_2 & channel)
    {
        i  = 2 ;
        led_cmd_handle(i, LED_CH_2, state);
    }
}

void led_reset_sleep_led(void)
{
    SleepLed_Reset();
}

void led_deinit(T_LED_CHANNEL channel)
{
    uint8_t i = channel >> 1;

    if ((led_driver_by_sw) && (!led_is_sleep_pinmux(*(led0_pinmux + i))))
    {
        if (led_setting_record[i].type == LED_TYPE_BREATH)
        {
            pwm_pin_config(pwm_handle[i], *(led0_pinmux + i), PWM_FUNC);
        }
        else
        {
            //reset pad
            Pad_Config(*(led0_pinmux + i),
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, (PAD_OUTPUT_VAL)led_polarity[i]);
        }
    }
    else
    {
        if (led_polarity[i] == LED_ACTIVE_POLARITY_LOW)
        {
            SLEEP_LED_InitTypeDef led_param;

            SleepLed_StructInit(&led_param);

            led_param.phase_phase_tick[0] = 1023;
            led_param.phase_initial_duty[0] = LED_TIME_TICK_SCALE;
            //Bypass other phases

            SleepLed_Init((SLEEP_LED_CHANNEL)channel, &led_param);
            SleepLed_Cmd((SLEEP_LED_CHANNEL)channel, ENABLE);
        }
        else
        {
            SleepLed_DeInit((SLEEP_LED_CHANNEL)channel);
        }
    }
}

void led_config(T_LED_CHANNEL channel, T_LED_TABLE *table)
{
    uint8_t ch_idx = 0;

    uint16_t interval = table->blink_interval * 10;// Unit: 100ms

    //FW work around low active LED
    switch (channel)
    {
    case LED_CH_0:
        ch_idx = 0;
        break;
    case LED_CH_1:
        ch_idx = 1;
        break;
    case LED_CH_2:
        ch_idx = 2;
        break;
    }

    if ((led_driver_by_sw)  && (!led_is_sleep_pinmux(*(led0_pinmux + ch_idx))))
    {
        led_setting_record[ch_idx].type = table->type;// new set type
        led_setting_record[ch_idx].on_time = table->on_time;
        led_setting_record[ch_idx].off_time = table->off_time;
        led_setting_record[ch_idx].blink_count = 2 * table->blink_count;
        led_setting_record[ch_idx].blink_interval = table->blink_interval;

        led_count_record[ch_idx].blink_count = 2 * table->blink_count;
        led_deinit(channel);
    }
    else
    {
        if (table->type == LED_TYPE_KEEP_OFF)
        {
            if (led_polarity[ch_idx] == LED_ACTIVE_POLARITY_LOW)
            {
                SLEEP_LED_InitTypeDef led_param;

                SleepLed_StructInit(&led_param);

                led_param.phase_phase_tick[0] = 1023;
                led_param.phase_initial_duty[0] = LED_TIME_TICK_SCALE;
                //Bypass other phases

                SleepLed_Init((SLEEP_LED_CHANNEL)channel, &led_param);
                SleepLed_Cmd((SLEEP_LED_CHANNEL)channel, ENABLE);
            }
            else
            {
                SleepLed_DeInit((SLEEP_LED_CHANNEL)channel);
            }
        }
        else if (table->type == LED_TYPE_KEEP_ON)
        {
            SLEEP_LED_InitTypeDef led_param;

            SleepLed_StructInit(&led_param);

            if (led_polarity[ch_idx] == LED_ACTIVE_POLARITY_LOW)
            {
                led_param.polarity = LED_OUTPUT_INVERT;
            }

            led_param.phase_phase_tick[0] = 1023;
            led_param.phase_initial_duty[0] = LED_TIME_TICK_SCALE;
            //Bypass other phases

            SleepLed_Init((SLEEP_LED_CHANNEL)channel, &led_param);
        }
        else
        {
            if ((table->type == LED_TYPE_BREATH) ||
                (table->blink_count == 4)) //Use 8 phases to achive 4 blinks
            {
                SLEEP_LED_InitTypeDef led_param;

                SleepLed_StructInit(&led_param);

                if (led_polarity[ch_idx] == LED_ACTIVE_POLARITY_LOW)
                {
                    led_param.polarity = LED_OUTPUT_INVERT;
                }

                if (table->blink_count == 1)
                {
                    led_param.phase_phase_tick[0] = table->on_time;
                    led_param.phase_increase_duty[0] = 1;
                    led_param.phase_duty_step[0] = (LED_TIME_TICK_SCALE / table->on_time);

                    led_param.phase_phase_tick[1] = table->off_time + interval;
                    led_param.phase_initial_duty[1] = LED_TIME_TICK_SCALE;
                    led_param.phase_duty_step[1] = (LED_TIME_TICK_SCALE / table->off_time);
                }
                else if (table->blink_count == 2)
                {
#if HARMAN_LED_BREATH_SUPPORT
#if 1
                    led_param.prescale                    = 320;// 100Hz
                    led_param.mode                        = LED_BREATHE_MODE;
                    led_param.polarity                    = LED_OUTPUT_NORMAL;
                    led_param.phase_uptate_rate[0]         = 5;// update every 5 clk
                    led_param.phase_phase_tick[0]          = 100;
                    led_param.phase_initial_duty[0]        = 0;
                    led_param.phase_increase_duty[0]       = 1;
                    led_param.phase_duty_step[0]           = 16;
                    led_param.phase_uptate_rate[1]         = 0;
                    led_param.phase_phase_tick[1]          = 50;
                    led_param.phase_initial_duty[1]        = 320;
                    led_param.phase_increase_duty[1]       = 1;
                    led_param.phase_duty_step[1]           = 0;
                    led_param.phase_uptate_rate[2]         = 5;
                    led_param.phase_phase_tick[2]          = 100;
                    led_param.phase_initial_duty[2]        = 320;
                    led_param.phase_increase_duty[2]       = 0;
                    led_param.phase_duty_step[2]           = 16;
                    led_param.phase_uptate_rate[3]         = 0;
                    led_param.phase_phase_tick[3]          = 50;
                    led_param.phase_initial_duty[3]        = 0;
                    led_param.phase_increase_duty[3]       = 0;
                    led_param.phase_duty_step[3]           = 0;
#else
                    led_param.prescale                    = 256;//125Hz
                    led_param.mode                        = LED_BREATHE_MODE;
                    led_param.polarity                    = LED_OUTPUT_NORMAL;

                    led_param.phase_uptate_rate[0]              = 0;
                    led_param.phase_phase_tick[0]               = 125;
                    led_param.phase_initial_duty[0]             = 0;
                    led_param.phase_increase_duty[0]            = 1;
                    led_param.phase_duty_step[0]                = 2;

                    led_param.phase_uptate_rate[1]              = 0;
                    led_param.phase_phase_tick[1]               = 67;
                    led_param.phase_initial_duty[1]             = 256;
                    led_param.phase_increase_duty[1]            = 1;
                    led_param.phase_duty_step[1]                = 0;

                    led_param.phase_uptate_rate[2]              = 0;
                    led_param.phase_phase_tick[2]               = 125;
                    led_param.phase_initial_duty[2]             = 256;
                    led_param.phase_increase_duty[2]            = 0;
                    led_param.phase_duty_step[2]                = 2;

                    led_param.phase_uptate_rate[3]              = 0;
                    led_param.phase_phase_tick[3]               = 67;
                    led_param.phase_initial_duty[3]             = 0;
                    led_param.phase_increase_duty[3]            = 0;
                    led_param.phase_duty_step[3]                = 0;
#endif
#else
                    led_param.phase_phase_tick[0] = table->on_time;
                    led_param.phase_increase_duty[0] = 1;
                    led_param.phase_duty_step[0] = (LED_TIME_TICK_SCALE / table->on_time);

                    led_param.phase_phase_tick[1] = table->off_time;
                    led_param.phase_initial_duty[1] = LED_TIME_TICK_SCALE;
                    led_param.phase_duty_step[1] = (LED_TIME_TICK_SCALE / table->off_time);

                    led_param.phase_phase_tick[2] = table->on_time;
                    led_param.phase_increase_duty[2] = 1;
                    led_param.phase_duty_step[2] = (LED_TIME_TICK_SCALE / table->on_time);

                    led_param.phase_phase_tick[3] = table->off_time + interval;
                    led_param.phase_initial_duty[3] = LED_TIME_TICK_SCALE;
                    led_param.phase_duty_step[3] = (LED_TIME_TICK_SCALE / table->off_time);
#endif
                }
                else if (table->blink_count == 3)
                {
                    led_param.phase_phase_tick[0] = table->on_time;
                    led_param.phase_increase_duty[0] = 1;
                    led_param.phase_duty_step[0] = (LED_TIME_TICK_SCALE / table->on_time);

                    led_param.phase_phase_tick[1] = table->off_time;
                    led_param.phase_initial_duty[1] = LED_TIME_TICK_SCALE;
                    led_param.phase_duty_step[1] = (LED_TIME_TICK_SCALE / table->off_time);

                    led_param.phase_phase_tick[2] = table->on_time;
                    led_param.phase_increase_duty[2] = 1;
                    led_param.phase_duty_step[2] = (LED_TIME_TICK_SCALE / table->on_time);

                    led_param.phase_phase_tick[3] = table->off_time;
                    led_param.phase_initial_duty[3] = LED_TIME_TICK_SCALE;
                    led_param.phase_duty_step[3] = (LED_TIME_TICK_SCALE / table->off_time);

                    led_param.phase_phase_tick[4] = table->on_time;
                    led_param.phase_increase_duty[4]  = 1;
                    led_param.phase_duty_step[4] = (LED_TIME_TICK_SCALE / table->on_time);

                    led_param.phase_phase_tick[5] = table->off_time + interval;
                    led_param.phase_initial_duty[5] = LED_TIME_TICK_SCALE;
                    led_param.phase_duty_step[5] = (LED_TIME_TICK_SCALE / table->off_time);
                }
                else //Blink 4 times
                {
                    led_param.phase_phase_tick[0] = table->on_time;
                    led_param.phase_initial_duty[0] = LED_TIME_TICK_SCALE;

                    led_param.phase_phase_tick[1] = table->off_time;

                    led_param.phase_phase_tick[2] = table->on_time;
                    led_param.phase_initial_duty[2] = LED_TIME_TICK_SCALE;

                    led_param.phase_phase_tick[3] = table->off_time;

                    led_param.phase_phase_tick[4] = table->on_time;
                    led_param.phase_initial_duty[4] = LED_TIME_TICK_SCALE;

                    led_param.phase_phase_tick[5] = table->off_time;

                    led_param.phase_phase_tick[6] = table->on_time;
                    led_param.phase_initial_duty[6] = LED_TIME_TICK_SCALE;

                    led_param.phase_phase_tick[7] = table->off_time + interval;
                }

                SleepLed_Init((SLEEP_LED_CHANNEL)channel, &led_param);
            }
            else
            {
                SLEEP_LED_InitTypeDef led_param;

                SleepLed_StructInit(&led_param);

                if (led_polarity[ch_idx] == LED_ACTIVE_POLARITY_LOW)
                {
                    led_param.polarity = LED_OUTPUT_INVERT;
                }

                if (table->type == LED_TYPE_ON_OFF)
                {
                    led_param.polarity ^= 1;
                }

                uint8_t i = 0;
                for (i = 0; i < table->blink_count; i++)
                {
                    led_param.phase_phase_tick[(2 * i)] = table->on_time;

                    led_param.phase_initial_duty[(2 * i) + 1] = LED_TIME_TICK_SCALE;
                    led_param.phase_phase_tick[(2 * i) + 1] = table->off_time;
                    if ((i + 1) == table->blink_count)
                    {
                        led_param.phase_phase_tick[(2 * i) + 1] += interval;
                        if (led_param.phase_phase_tick[(2 * i) + 1] > 1023)
                        {
                            // Only 10 bits in HW register
                            led_param.phase_phase_tick[(2 * i) + 1] = 1023;
                        }
                    }

                    if (table->type == LED_TYPE_BREATH)
                    {
                        led_param.phase_duty_step[(2 * i)] = (LED_TIME_TICK_SCALE / table->on_time);
                        led_param.phase_increase_duty[(2 * i)] = 1;
                        led_param.phase_duty_step[(2 * i) + 1] = (LED_TIME_TICK_SCALE / table->off_time);
                    }
                }

                SleepLed_Init((SLEEP_LED_CHANNEL)channel, &led_param);
            }
        }
    }
}

static void led_create_timer(uint8_t i, uint32_t time, uint16_t led_ch)
{
    led_creat_timer_point[i] = os_sys_time_get();
    led_creat_timer_record[i] = time;

    if (time != 0)
    {
        gap_start_timer(&timer_handle_led[i], led_timer_name[i],
                        led_module_timer_queue_id, 0, led_ch, time);
    }
}

static void led_blink_handle(T_LED_CHANNEL led_ch)
{
    uint8_t i = led_ch >> 1;

    if (*(led0_pinmux + i) == 0xFF)
    {
        return;
    }

    if ((led_setting_record[i].type == LED_TYPE_KEEP_ON) ||
        (led_setting_record[i].type == LED_TYPE_ON_OFF))
    {
        if (led_polarity[i])
        {
            Pad_OutputControlValue(*(led0_pinmux + i), PAD_OUT_LOW);
        }
        else
        {
            Pad_OutputControlValue(*(led0_pinmux + i), PAD_OUT_HIGH);
        }
    }
    else
    {
        led_deinit(led_ch);
    }

    if ((led_setting_record[i].type == LED_TYPE_ON_OFF) ||
        (led_setting_record[i].type == LED_TYPE_OFF_ON))
    {
        led_create_timer(i, led_setting_record[i].on_time * 10, led_ch);
    }
    else if (led_setting_record[i].type == LED_TYPE_BREATH)
    {
        led_sw_breath_init(i);
        led_sw_breath_timer_control(i, ENABLE);
    }
}

static void led_module_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    uint8_t i = timer_chann >> 1;

    if ((timer_handle_led[i] != NULL) &&
        ((os_sys_time_get() - led_creat_timer_point[i]) >= led_creat_timer_record[i]))
    {
        if (led_setting_record[i].blink_count == 0) //restart repeat blink when blink_interval is not zero
        {
            led_deinit((T_LED_CHANNEL)timer_chann);
            led_setting_record[i].blink_count = led_count_record[i].blink_count;
            led_cmd((T_LED_CHANNEL)timer_chann, ENABLE);
        }
        else if (led_setting_record[i].blink_count != 1)
        {
            led_setting_record[i].blink_count--;
            if (Pad_GetOutputCtrl(*(led0_pinmux + i)) == PAD_OUT_LOW)
            {
                Pad_OutputControlValue(*(led0_pinmux + i), PAD_OUT_HIGH);
            }
            else
            {
                Pad_OutputControlValue(*(led0_pinmux + i), PAD_OUT_LOW);
            }

            if (led_setting_record[i].blink_count % 2 == 1) //start off timer when blink_cnt is odd
            {
                led_create_timer(i, led_setting_record[i].off_time * 10, timer_chann);
            }
            else
            {
                led_create_timer(i, led_setting_record[i].on_time * 10, timer_chann);
            }
        }
        else
        {
            if (led_setting_record[i].blink_interval != 0) //blink_interval is not zero
            {
                led_deinit((T_LED_CHANNEL)timer_chann);
                led_setting_record[i].blink_count = 0;
                led_create_timer(i, led_setting_record[i].blink_interval * 100, timer_chann);
            }
            else
            {
                led_deinit((T_LED_CHANNEL)timer_chann);
                led_setting_record[i].blink_count = led_count_record[i].blink_count;
                led_cmd((T_LED_CHANNEL)timer_chann, ENABLE);
            }
        }
    }
}

static void led_clear_para(T_LED_CHANNEL channel)
{
    uint8_t i = channel >> 1;

    memset(&led_setting_record[i], 0, sizeof(T_LED_TABLE));
}

void led_set_driver_mode(void)
{
    if (app_cfg_const.sw_led_support) //led setting using sw timer
    {
        led_driver_by_sw = true;
        gap_reg_timer_cb(led_module_timer_callback, &led_module_timer_queue_id);
    }
    else
    {
        led_driver_by_sw = false;
    }
}

bool led_is_sleep_pinmux(uint8_t pinmux)
{
    bool ret = false;
    int i = 0;

    for (; i <  SLEEP_LED_PINMUX_NUM ; i++)
    {
        if (pinmux == sleep_led_pinmux[i])
        {
            ret = true;
            break;
        }
    }

    return ret;
}

static void led_sw_breath_timer_control(uint8_t i, FunctionalState new_state)
{
    if (new_state)
    {
        pwm_start(pwm_handle[i]);
    }
    else
    {
        pwm_stop(pwm_handle[i]);
    }
}

void led_sw_breath_timer_handler(void *handle)
{
    uint8_t i = 0;
    uint8_t led_num = 0;
    uint32_t high_count = 0;
    uint32_t low_count = 0;

    for (i = 0; i < LED_NUM; i++)
    {
        if (sw_breathe_mode[i].pwm_id == hw_timer_get_id(handle) &&
            (led_setting_record[i].type == LED_TYPE_BREATH) &&
            (led_setting_record[i].on_time != 0))
        {
            led_num = i + 1;
            break;
        }
    }

    if (led_num != 0)
    {
        sw_breathe_mode[i].led_breath_timer_count++;

        if (sw_breathe_mode[i].led_breath_timer_count <=
            2 * led_setting_record[led_num - 1].on_time) //set led breath duty during on time
        {
            high_count = PWM_OUT_COUNT * sw_breathe_mode[i].led_breath_timer_count /
                         led_setting_record[led_num - 1].on_time / 2;
            low_count = PWM_OUT_COUNT - PWM_OUT_COUNT * sw_breathe_mode[i].led_breath_timer_count /
                        led_setting_record[led_num - 1].on_time / 2;
        }
        else if ((sw_breathe_mode[i].led_breath_timer_count >
                  2 * led_setting_record[led_num - 1].on_time) && //set led breath duty during off time
                 (sw_breathe_mode[i].led_breath_timer_count <=
                  2 * (led_setting_record[led_num - 1].on_time + led_setting_record[led_num - 1].off_time)))
        {
            high_count = PWM_OUT_COUNT - PWM_OUT_COUNT * (sw_breathe_mode[i].led_breath_timer_count -
                                                          2 * led_setting_record[led_num - 1].on_time) / led_setting_record[led_num - 1].off_time / 2;
            low_count = PWM_OUT_COUNT * (sw_breathe_mode[i].led_breath_timer_count -
                                         2 * led_setting_record[led_num - 1].on_time) / led_setting_record[led_num - 1].off_time / 2;
        }
        else if (sw_breathe_mode[i].led_breath_timer_count >
                 2 * (led_setting_record[led_num - 1].on_time + led_setting_record[led_num - 1].off_time))
        {
            //restart blink when led_breath_blink_count is less than blink_count
            if (sw_breathe_mode[i].led_breath_blink_count < led_setting_record[led_num - 1].blink_count / 2)
            {
                sw_breathe_mode[i].led_breath_timer_count = 0;
            }
            else
            {
                if (sw_breathe_mode[i].led_breath_timer_count <= (2 * (led_setting_record[led_num - 1].on_time +
                                                                       led_setting_record[led_num - 1].off_time) +
                                                                  led_setting_record[led_num - 1].blink_interval *
                                                                  10)) // disable led breath when blink_interval is not zero
                {
                    high_count = 0;
                    low_count = PWM_OUT_COUNT;
                }
                else
                {
                    sw_breathe_mode[i].led_breath_timer_count = 0;
                    sw_breathe_mode[i].led_breath_blink_count = 0;
                }
            }
        }

        if (sw_breathe_mode[i].led_breath_timer_count == 2 * (led_setting_record[led_num - 1].on_time +
                                                              led_setting_record[led_num - 1].off_time))
        {
            sw_breathe_mode[i].led_breath_blink_count++;
        }

        /*up the change duty cnt */
        if (sw_breathe_mode[i].led_breath_timer_count != 0)
        {
            /*change duty */
            pwm_change_duty_and_frequency(pwm_handle[i], high_count, low_count);
        }
    }
}

static void led_sw_breath_init(uint8_t i)
{
    if (pwm_handle[i] == NULL)
    {
        pwm_handle[i] = pwm_create("ext_led", PWM_OUT_COUNT, PWM_OUT_COUNT, false);
        if (pwm_handle[i] == NULL)
        {
            APP_PRINT_ERROR0("Could not create extend led pwm!!!");
            return;
        }
        sw_breathe_mode[i].pwm_id = hw_timer_get_id(pwm_handle[i]);
        pwm_register_timeout_callback(pwm_handle[i], led_sw_breath_timer_handler);
    }
}
