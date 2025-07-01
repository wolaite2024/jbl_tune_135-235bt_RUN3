/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     led_demo.c
* @brief    This file provides functions to set some LED working mode.
* @details
* @author   brenda_li
* @date     2016-12-15
* @version  v0.1
*********************************************************************************************************
*/
#include "string.h"
#include "rtl876x_pinmux.h"
#include "led_demo.h"
#include "gap_timer.h"
#include "app_cfg.h"
#include "rtl876x.h"
#include "rtl876x_sleep_led.h"
#include "os_sched.h"
#include "os_mem.h"
#include "os_sync.h"
#include "board.h"
#include "trace.h"
#include "rtl876x_tim.h"
#include "app_hw_timer.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"

#define LED_OUT_0       P2_1
#define LED_OUT_1       P2_2
#define LED_OUT_2       P2_7
#define LED_OUT_3       P1_6
#define LED_OUT_4       P1_7
#define LED_OUT_5       P0_1

typedef enum
{
    LED_ACTIVE_POLARITY_HIGH    = 0x00, /**< Setting led to high active */
    LED_ACTIVE_POLARITY_LOW     = 0x01, /**< Setting led to low active  */
} T_LED_ACTIVE_POLARITY;

typedef enum
{
    /* _____________________ Always OFF */
    LED_TYPE_KEEP_OFF,

    /*   _________________              */
    /* _|                 |_ Always ON  */
    LED_TYPE_KEEP_ON,

    /*   __    __    __    _            */
    /* _|  |__|  |__|  |__|  ON/OFF repeat */
    LED_TYPE_ON_OFF,

    /*      __    __    __              */
    /* ____|  |__|  |__|  |_ OFF/ON repeat */
    LED_TYPE_OFF_ON,

    LED_TYPE_BREATH,

    /* Bypass LED config */
    LED_TYPE_BYPASS = 0xFF
} T_LED_TYPE;

/**
 * @brief LED table structure.
 *
 * Here defines LED display parameters.
*/
typedef struct
{
    T_LED_TYPE type;        //!< led type @ref T_LED_TYPE
    uint8_t on_time;        //!< on time Unit: 10ms
    uint8_t off_time;       //!< off time Unit: 10ms
    uint8_t blink_count;    //!< Range: 0~4
    uint8_t blink_interval; //!< Unit: 100ms
} T_LED_TABLE;


static T_LED_TABLE LED_TABLE[2][LED_DEMO_NUM] =
{
    //normal led
    {
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
        {LED_TYPE_ON_OFF, 10, 10, 2, 20},
    },

    //breath led
    {
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
        {LED_TYPE_BREATH, 200, 200, 2, 20},
    },
};

/**
 * @brief LED channels definition.
*/
typedef enum
{
    LED_CH_0 = 0x01,
    LED_CH_1 = 0x02,
    LED_CH_2 = 0x04,
    LED_CH_3 = 0x08,
    LED_CH_4 = 0x10,
    LED_CH_5 = 0x20
} T_LED_CHANNEL;

#define PWM_OUT_COUNT (400000) //time Unit: 10ms(400000/40MHz)
static uint32_t led_breath_timer_count;
static uint8_t  led_breath_blink_count;

static T_LED_TABLE led_setting[LED_DEMO_NUM];
static uint32_t led_creat_timer_point[LED_DEMO_NUM];
static uint32_t led_creat_timer_record[LED_DEMO_NUM];
static uint8_t led_polarity[LED_DEMO_NUM];
static bool led_driver_by_sw;
static const char *led_timer_name[LED_DEMO_NUM] = {"led0_on_off", "led1_on_off", "led2_on_off", "led3_on_off", "led4_on_off", "led5_on_off"};
static void *timer_handle_led[LED_DEMO_NUM];
static uint8_t led_module_timer_queue_id = 0;
static T_LED_TABLE led_setting_record[LED_DEMO_NUM];
static T_LED_TABLE led_count_record[LED_DEMO_NUM];
static void led_demo_clear_para(T_LED_CHANNEL channel);
static void led_demo_blink_handle(T_LED_CHANNEL led_ch);
static void led_demo_deinit(T_LED_CHANNEL channel);
static void led_demo_breath_timer_control(TIM_TypeDef *index, FunctionalState new_state);

static void led_demo_set_active_polarity(T_LED_CHANNEL channel, T_LED_ACTIVE_POLARITY polarity)
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
    case LED_CH_3:
        led_polarity[3] = polarity;
        break;
    case LED_CH_4:
        led_polarity[4] = polarity;
        break;
    case LED_CH_5:
        led_polarity[5] = polarity;
        break;
    }
}

static uint8_t led_demo_bit_to_idx(T_LED_CHANNEL channel)
{
    uint8_t num = 0;

    if (channel != 0x01)
    {
        for (;;)
        {
            channel >>= 1;
            num ++;

            if (channel == 1)
            {
                break;
            }
        }
    }

    return num;
}

static uint8_t led_demo_get_pin_num(T_LED_CHANNEL channel)
{
    uint8_t Pin_Num;

    switch (channel)
    {
    case LED_CH_0:
        Pin_Num = LED_OUT_0;
        break;

    case LED_CH_1:
        Pin_Num = LED_OUT_1;
        break;

    case LED_CH_2:
        Pin_Num = LED_OUT_2;
        break;

    case LED_CH_3:
        Pin_Num = LED_OUT_3;
        break;

    case LED_CH_4:
        Pin_Num = LED_OUT_4;
        break;

    case LED_CH_5:
        Pin_Num = LED_OUT_5;
        break;

    default:
        break;
    }

    return Pin_Num;
}

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void led_demo_board_led_init(void)
{
    led_demo_set_active_polarity(LED_CH_0, LED_ACTIVE_POLARITY_HIGH);
    Pad_Config(LED_OUT_0,
               PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    led_demo_set_active_polarity(LED_CH_1, LED_ACTIVE_POLARITY_HIGH);
    Pad_Config(LED_OUT_1,
               PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    led_demo_set_active_polarity(LED_CH_2, LED_ACTIVE_POLARITY_HIGH);
    Pad_Config(LED_OUT_2,
               PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    led_demo_set_active_polarity(LED_CH_3, LED_ACTIVE_POLARITY_HIGH);
    Pad_Config(LED_OUT_3,
               PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    led_demo_set_active_polarity(LED_CH_4, LED_ACTIVE_POLARITY_HIGH);
    Pad_Config(LED_OUT_4,
               PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    led_demo_set_active_polarity(LED_CH_5, LED_ACTIVE_POLARITY_HIGH);
    Pad_Config(LED_OUT_5,
               PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
}

static void led_demo_cmd(uint8_t channel, bool state)
{
    uint32_t s;

    if (led_driver_by_sw)
    {
        //LED should be handled simultaneously
        //Avoid APP be preempted by higher priority task
        s = os_lock();

        if (state)
        {
            //set pad and start timer
            if (LED_CH_0 & channel)
            {
                led_demo_blink_handle(LED_CH_0);
            }

            if (LED_CH_1 & channel)
            {
                led_demo_blink_handle(LED_CH_1);
            }

            if (LED_CH_2 & channel)
            {
                led_demo_blink_handle(LED_CH_2);
            }

            if (LED_CH_3 & channel)
            {
                led_demo_blink_handle(LED_CH_3);
            }

            if (LED_CH_4 & channel)
            {
                led_demo_blink_handle(LED_CH_4);
            }

            if (LED_CH_5 & channel)
            {
                led_demo_blink_handle(LED_CH_5);
            }
        }
        else
        {
            //reset pad and stop timer
            if (LED_CH_0 & channel)
            {
                led_demo_deinit(LED_CH_0);
                led_demo_clear_para(LED_CH_0);
                gap_stop_timer(&timer_handle_led[0]);
            }

            if (LED_CH_1 & channel)
            {
                led_demo_deinit(LED_CH_1);
                led_demo_clear_para(LED_CH_1);
                gap_stop_timer(&timer_handle_led[1]);
            }

            if (LED_CH_2 & channel)
            {
                led_demo_deinit(LED_CH_2);
                led_demo_clear_para(LED_CH_2);
                gap_stop_timer(&timer_handle_led[2]);
            }

            if (LED_CH_3 & channel)
            {
                led_demo_deinit(LED_CH_3);
                led_demo_clear_para(LED_CH_3);
                gap_stop_timer(&timer_handle_led[3]);
            }

            if (LED_CH_4 & channel)
            {
                led_demo_deinit(LED_CH_4);
                led_demo_clear_para(LED_CH_4);
                gap_stop_timer(&timer_handle_led[4]);
            }

            if (LED_CH_5 & channel)
            {
                led_demo_deinit(LED_CH_5);
                led_demo_clear_para(LED_CH_5);
                gap_stop_timer(&timer_handle_led[5]);
            }
        }

        os_unlock(s);
    }
}

static void led_demo_deinit(T_LED_CHANNEL channel)
{
    uint8_t i = led_demo_bit_to_idx(channel);
    uint8_t Pin_Num = led_demo_get_pin_num(channel);

    if (led_driver_by_sw)
    {
        if (led_setting_record[i].type == LED_TYPE_BREATH)
        {
            Pad_Config(Pin_Num,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
            Pinmux_Config(Pin_Num, LED_BREATH_TIMER_PINMUX);
        }
        else
        {
            //reset pad
            Pad_Config(Pin_Num,
                       PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, (PAD_OUTPUT_VAL)led_polarity[i]);
        }
    }
}

static void led_demo_config(T_LED_CHANNEL channel, T_LED_TABLE *table)
{
    uint8_t ch_idx = 0;

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

    case LED_CH_3:
        ch_idx = 3;
        break;

    case LED_CH_4:
        ch_idx = 4;
        break;

    case LED_CH_5:
        ch_idx = 5;
        break;
    }

    if (led_driver_by_sw)
    {
        led_setting_record[ch_idx].type = table->type;// new set type
        led_setting_record[ch_idx].on_time = table->on_time;
        led_setting_record[ch_idx].off_time = table->off_time;
        led_setting_record[ch_idx].blink_count = 2 * table->blink_count;
        led_setting_record[ch_idx].blink_interval = table->blink_interval;

        led_count_record[ch_idx].blink_count = 2 * table->blink_count;
        led_demo_deinit(channel);
    }
}

static void led_demo_create_timer(uint8_t i, uint32_t time, uint16_t led_ch)
{
    led_creat_timer_point[i] = os_sys_time_get();
    led_creat_timer_record[i] = time;

    if (time != 0)
    {
        gap_start_timer(&timer_handle_led[i], led_timer_name[i],
                        led_module_timer_queue_id, 0, led_ch, time);
    }
}

/*
**
  * @brief  enable or disable led breath imer.
    * @param  TIM_TypeDef *index TIM_Type.
    * @param  FunctionalState NewState.
  * @return  void
  */
void led_demo_breath_timer_control(TIM_TypeDef *index,
                                   FunctionalState new_state)
{
    if (new_state)
    {
        app_hw_timer_enable(index);
    }
    else
    {
        app_hw_timer_disable(index);
    }
}

void led_demo_breath_init(void)
{
    TIM_TimeBaseInitTypeDef time_param;
    NVIC_InitTypeDef nvic_param;

    RCC_PeriphClockCmd(APBPeriph_TIMER, APBPeriph_TIMER_CLOCK, ENABLE);

    TIM_StructInit(&time_param);
    time_param.TIM_PWM_En = PWM_ENABLE;
    time_param.TIM_PWM_High_Count = PWM_OUT_COUNT - 1;
    time_param.TIM_PWM_Low_Count = PWM_OUT_COUNT - 1;
    time_param.TIM_Mode = TIM_Mode_UserDefine;
    TIM_TimeBaseInit(LED_BREATH_TIMER, &time_param);

    /*  Enable TIMER3 IRQ  */
    nvic_param.NVIC_IRQChannel = LED_BREATH_TIMER_IRQ;
    nvic_param.NVIC_IRQChannelPriority = 3;
    nvic_param.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_param);

    TIM_ClearINT(LED_BREATH_TIMER);
    TIM_INTConfig(LED_BREATH_TIMER, ENABLE);
}

void led_demo_breath_timer_handler(void)
{
    uint8_t i = 0;
    uint8_t led_num = 0;
    uint32_t high_count = 0;
    uint32_t low_count = 0;

    for (i = 0; i < LED_DEMO_NUM; i++)
    {
        if ((led_demo_get_pin_num((T_LED_CHANNEL)BIT(i)) != 0xFF) &&
            (led_setting_record[i].type == LED_TYPE_BREATH) &&
            (led_setting_record[i].on_time != 0))
        {
            led_num = i + 1;
            break;
        }
    }

    if (led_num != 0)
    {
        led_breath_timer_count++;

        if (led_breath_timer_count <=
            2 * led_setting_record[led_num - 1].on_time) //set led breath duty during on time
        {
            high_count = PWM_OUT_COUNT * led_breath_timer_count /
                         led_setting_record[led_num - 1].on_time / 2;
            low_count = PWM_OUT_COUNT - PWM_OUT_COUNT * led_breath_timer_count /
                        led_setting_record[led_num - 1].on_time / 2;
        }
        else if ((led_breath_timer_count >
                  2 * led_setting_record[led_num - 1].on_time) && //set led breath duty during off time
                 (led_breath_timer_count <=
                  2 * (led_setting_record[led_num - 1].on_time + led_setting_record[led_num - 1].off_time)))
        {
            high_count = PWM_OUT_COUNT - PWM_OUT_COUNT * (led_breath_timer_count -
                                                          2 * led_setting_record[led_num - 1].on_time) / led_setting_record[led_num - 1].off_time / 2;
            low_count = PWM_OUT_COUNT * (led_breath_timer_count -
                                         2 * led_setting_record[led_num - 1].on_time) / led_setting_record[led_num - 1].off_time / 2;
        }
        else if (led_breath_timer_count >
                 2 * (led_setting_record[led_num - 1].on_time + led_setting_record[led_num - 1].off_time))
        {
            //restart blink when led_breath_blink_count is less than blink_count
            if (led_breath_blink_count < led_setting_record[led_num - 1].blink_count / 2)
            {
                led_breath_timer_count = 0;
            }
            else
            {
                if (led_breath_timer_count <= (2 * (led_setting_record[led_num - 1].on_time +
                                                    led_setting_record[led_num - 1].off_time) +
                                               led_setting_record[led_num - 1].blink_interval *
                                               10)) // disable led breath when blink_interval is not zero
                {
                    high_count = 0;
                    low_count = PWM_OUT_COUNT;
                }
                else
                {
                    led_breath_timer_count = 0;
                    led_breath_blink_count = 0;
                }
            }
        }

        if (led_breath_timer_count == 2 * (led_setting_record[led_num - 1].on_time +
                                           led_setting_record[led_num - 1].off_time))
        {
            led_breath_blink_count++;
        }

        /*up the change duty cnt */
        if (led_breath_timer_count != 0)
        {
            TIM_ClearINT(LED_BREATH_TIMER);
            /*change duty */
            TIM_PWMChangeFreqAndDuty(LED_BREATH_TIMER, high_count, low_count);
        }
    }
}

static void led_demo_blink_handle(T_LED_CHANNEL led_ch)
{
    uint8_t idx = led_demo_bit_to_idx(led_ch);
    uint8_t Pin_Num = led_demo_get_pin_num(led_ch);

    if (Pin_Num == 0xFF)
    {
        return;
    }

    if ((led_setting_record[idx].type == LED_TYPE_KEEP_ON) ||
        (led_setting_record[idx].type == LED_TYPE_ON_OFF))
    {
        if (led_polarity[idx])
        {
            Pad_OutputControlValue(Pin_Num, PAD_OUT_LOW);
        }
        else
        {
            Pad_OutputControlValue(Pin_Num, PAD_OUT_HIGH);
        }
    }
    else
    {
        led_demo_deinit(led_ch);
    }

    if ((led_setting_record[idx].type == LED_TYPE_ON_OFF) ||
        (led_setting_record[idx].type == LED_TYPE_OFF_ON))
    {
        led_demo_create_timer(idx, led_setting_record[idx].on_time * 10, led_ch);
    }
    else if (led_setting_record[idx].type == LED_TYPE_BREATH)
    {
        led_demo_breath_timer_control(LED_BREATH_TIMER, ENABLE);
    }
}

static void led_demo_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    uint8_t i = led_demo_bit_to_idx((T_LED_CHANNEL)timer_chann);
    uint8_t Pin_Num = led_demo_get_pin_num((T_LED_CHANNEL)timer_chann);

    if ((timer_handle_led[i] != NULL) &&
        ((os_sys_time_get() - led_creat_timer_point[i]) >= led_creat_timer_record[i]))
    {
        if (led_setting_record[i].blink_count == 0) //restart repeat blink when blink_interval is not zero
        {
            led_demo_deinit((T_LED_CHANNEL)timer_chann);
            led_setting_record[i].blink_count = led_count_record[i].blink_count;
            led_demo_cmd((T_LED_CHANNEL)timer_chann, true);
        }
        else if (led_setting_record[i].blink_count != 1)
        {
            led_setting_record[i].blink_count--;
            if (Pad_GetOutputCtrl(Pin_Num) == PAD_OUT_LOW)
            {
                Pad_OutputControlValue(Pin_Num, PAD_OUT_HIGH);
            }
            else
            {
                Pad_OutputControlValue(Pin_Num, PAD_OUT_LOW);
            }

            if (led_setting_record[i].blink_count % 2 == 1) //start off timer when blink_cnt is odd
            {
                led_demo_create_timer(i, led_setting_record[i].off_time * 10, timer_chann);
            }
            else
            {
                led_demo_create_timer(i, led_setting_record[i].on_time * 10, timer_chann);
            }
        }
        else
        {
            if (led_setting_record[i].blink_interval != 0) //blink_interval is not zero
            {
                led_demo_deinit((T_LED_CHANNEL)timer_chann);
                led_setting_record[i].blink_count = 0;
                led_demo_create_timer(i, led_setting_record[i].blink_interval * 100, timer_chann);
            }
            else
            {
                led_demo_deinit((T_LED_CHANNEL)timer_chann);
                led_setting_record[i].blink_count = led_count_record[i].blink_count;
                led_demo_cmd((T_LED_CHANNEL)timer_chann, true);
            }
        }
    }
}

static void led_demo_clear_para(T_LED_CHANNEL channel)
{
    uint8_t i = led_demo_bit_to_idx(channel);

    memset(&led_setting_record[i], 0, sizeof(T_LED_TABLE));
}

void led_demo_blink(void)
{
    uint8_t i;
    uint8_t channel = 0;

    for (i = 0; i < LED_DEMO_NUM; i++)
    {
        //Disable LED first before config register
        led_demo_cmd(BIT(i), DISABLE);
        channel |= BIT(i);

        led_demo_config((T_LED_CHANNEL)BIT(i), &led_setting[i]);
    }

    led_demo_cmd(channel, ENABLE);
}

static void led_demo_set_driver_mode(void)
{
    led_driver_by_sw = true;
    gap_reg_timer_cb(led_demo_timer_callback, &led_module_timer_queue_id);
}

static void led_demo_load_table(uint8_t i)
{
    memcpy((void *)&led_setting,
           (const void *)&LED_TABLE[i], LED_DEMO_NUM * sizeof(T_LED_TABLE));
}

/**
  * @brief  demo code of operation about LED breathe mode.
  * @param   No parameter.
  * @return  void
  */
void led_demo_init(void)
{
    led_demo_set_driver_mode();
    led_demo_board_led_init();

    //index 0-->normal
    //index 1-->breath
    led_demo_load_table(1);

}
