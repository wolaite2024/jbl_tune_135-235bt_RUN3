
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "trace.h"
#include "gap_timer.h"
#include "rtl876x_gpio.h"
#include "app_cfg.h"
#include "app_gpio.h"
#include "app_amp.h"
#include "audio_plugin.h"
#include "app_flags.h"

#if F_APP_AMP_SUPPORT
typedef enum t_app_amp_state
{
    APP_AMP_STATE_OFF,
    APP_AMP_STATE_WAIT_OFF,
    APP_AMP_STATE_WAIT_ON,
    APP_AMP_STATE_ON,
} T_APP_AMP_STATE;

typedef enum t_app_amp_event
{
    APP_AMP_EVENT_SET_ON,
    APP_AMP_EVENT_ON_FINISH,
    APP_AMP_EVENT_READY_FINISH,
    APP_AMP_EVENT_SET_OFF,
    APP_AMP_EVENT_OFF_FINISH,
} T_APP_AMP_EVENT;

typedef enum t_app_amp_timer
{
    APP_AMP_TIMER_OFF = 0,
    APP_AMP_TIMER_ON,
    APP_AMP_TIMER_READY,
    APP_AMP_TIMER_NUM,
} T_APP_AMP_TIMER;

typedef struct t_app_amp_db
{
    T_APP_AMP_STATE         state;
    T_AUDIO_PLUGIN_HANDLE       plugin_handle;
    void                       *plugin_context;
    void                       *amp_timer;
    uint8_t                     amp_queue_id;
} T_APP_AMP_DB;

static T_APP_AMP_DB app_amp_db;

static void app_amp_send_to_plugin(void *context)
{
    T_AUDIO_PLUGIN_MSG amp_msg;

    APP_PRINT_TRACE1("app_amp_send_to_plugin: context %p", context);
    amp_msg.device = app_amp_db.plugin_handle;
    amp_msg.context = context;
    audio_plugin_msg_send(&amp_msg);
}

static void app_amp_control(uint8_t activate_fg)
{
    uint8_t level = 0;
    uint8_t pin_num = GPIO_GetNum(app_cfg_const.ext_amp_pinmux);

    if (activate_fg)
    {
        if (app_cfg_const.enable_ext_amp_high_active)
        {
            level = 1;
        }
    }
    else //Turn off
    {
        if (app_cfg_const.enable_ext_amp_high_active == 0)
        {
            level = 1;
        }
    }
    GPIOx_WriteBit(pin_num <= GPIO31 ? GPIOA : GPIOB, GPIO_GetPin(app_cfg_const.ext_amp_pinmux),
                   (BitAction)level);
}

static bool app_amp_state_machine(T_APP_AMP_EVENT event, uint32_t param)
{
    bool ret;
    T_APP_AMP_STATE old_state;

    ret = false;
    old_state = app_amp_db.state;

    switch (event)
    {
    case APP_AMP_EVENT_SET_ON:
        {
            gap_stop_timer(&app_amp_db.amp_timer);
            app_amp_db.state = APP_AMP_STATE_WAIT_ON;
            gap_start_timer(&app_amp_db.amp_timer, "amp_timer_on", app_amp_db.amp_queue_id,
                            APP_AMP_TIMER_ON, 0, app_cfg_const.timer_ctrl_ext_amp_on * 100);
        }
        break;

    case APP_AMP_EVENT_ON_FINISH:
        {
            gap_stop_timer(&app_amp_db.amp_timer);
            if (app_amp_db.state == APP_AMP_STATE_WAIT_ON)
            {
                app_amp_control(1);
                app_amp_db.state = APP_AMP_STATE_ON;
                if (app_cfg_const.timer_ctrl_ext_amp_ready)
                {
                    gap_start_timer(&app_amp_db.amp_timer, "amp_timer_ready",
                                    app_amp_db.amp_queue_id,
                                    APP_AMP_TIMER_READY, 0, app_cfg_const.timer_ctrl_ext_amp_ready * 100);
                }
                else
                {
                    app_amp_send_to_plugin(app_amp_db.plugin_context);
                }
            }
        }
        break;

    case APP_AMP_EVENT_READY_FINISH:
        {
            gap_stop_timer(&app_amp_db.amp_timer);
            app_amp_send_to_plugin(app_amp_db.plugin_context);
        }
        break;

    case APP_AMP_EVENT_SET_OFF:
        {
            if (app_amp_db.state == APP_AMP_STATE_ON)
            {
                gap_start_timer(&app_amp_db.amp_timer, "amp_timer_off", app_amp_db.amp_queue_id,
                                APP_AMP_TIMER_OFF, 0, app_cfg_const.timer_ctrl_ext_amp_off * 100);
                app_amp_db.state = APP_AMP_STATE_WAIT_OFF;
            }
            else if (app_amp_db.state == APP_AMP_STATE_WAIT_OFF)
            {
                app_amp_send_to_plugin(app_amp_db.plugin_context);
            }
        }
        break;

    case APP_AMP_EVENT_OFF_FINISH:
        {
            gap_stop_timer(&app_amp_db.amp_timer);
            if (app_amp_db.state == APP_AMP_STATE_WAIT_OFF)
            {
                app_amp_control(0);
                app_amp_db.state = APP_AMP_STATE_OFF;
                app_amp_send_to_plugin(app_amp_db.plugin_context);
            }
        }
        break;
    }

    if (old_state != app_amp_db.state)
    {
        ret = true;
    }

    APP_PRINT_TRACE5("app_amp_state_machine: event 0x%02x, param 0x%02x, "
                     "old_state %d, new_state %d, ret %d", event, param,
                     old_state, app_amp_db.state, ret);

    return ret;
}

static void app_amp_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_amp_timer_cback: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);
    switch (timer_id)
    {
    case APP_AMP_TIMER_ON:
        {
            app_amp_state_machine(APP_AMP_EVENT_ON_FINISH, 0);
        }
        break;

    case APP_AMP_TIMER_READY:
        {
            app_amp_state_machine(APP_AMP_EVENT_READY_FINISH, 0);
        }
        break;

    case APP_AMP_TIMER_OFF:
        {
            app_amp_state_machine(APP_AMP_EVENT_OFF_FINISH, 0);
        }
        break;

    default:
        break;
    }
}

static void app_amp_enable(T_AUDIO_PLUGIN_HANDLE handle, void *param)
{
    app_amp_db.plugin_context = param;
    if (app_amp_db.state != APP_AMP_STATE_ON)
    {
        app_amp_state_machine(APP_AMP_EVENT_SET_ON, 0);
    }
    else
    {
        app_amp_send_to_plugin(app_amp_db.plugin_context);
    }

    return;
}


static void app_amp_power_off(T_AUDIO_PLUGIN_HANDLE handle, void *param)
{
    app_amp_db.plugin_context = param;
    if (app_amp_db.state != APP_AMP_STATE_OFF)
    {
        app_amp_state_machine(APP_AMP_EVENT_SET_OFF, 0);
    }
    else
    {
        app_amp_send_to_plugin(app_amp_db.plugin_context);
    }
    return;
}

const T_AUDIO_PLUGIN_POLICY app_amp_policies[] =
{
    /* category */          /* occasion */                  /* action */                 /* action handler */
    /* 0 */ { AUDIO_CATEGORY_AUDIO, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },
    /* 1 */ { AUDIO_CATEGORY_VOICE, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },
    /* 2 */ { AUDIO_CATEGORY_ANALOG, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },
    /* 3 */ { AUDIO_CATEGORY_TONE, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },
    /* 4 */ { AUDIO_CATEGORY_VP, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },
    /* 5 */ { AUDIO_CATEGORY_LLAPT, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },
    /* 6 */ { AUDIO_CATEGORY_ANC, AUDIO_PLUGIN_OCCASION_CODEC_ON, AUDIO_PLUGIN_ACTION_ENABLE, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_enable },

    /* 7 */ { AUDIO_CATEGORY_AUDIO, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
    /* 8 */ { AUDIO_CATEGORY_VOICE, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
    /* 9 */ { AUDIO_CATEGORY_ANALOG, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
    /*10 */ { AUDIO_CATEGORY_TONE, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
    /*11 */ { AUDIO_CATEGORY_VP, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
    /*12 */ { AUDIO_CATEGORY_LLAPT, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
    /*13 */ { AUDIO_CATEGORY_ANC, AUDIO_PLUGIN_OCCASION_SYS_IDLE, AUDIO_PLUGIN_ACTION_POWER_OFF, (P_AUDIO_PLUGIN_ACTION_HANDLER)app_amp_power_off },
};

bool app_amp_is_off_state(void)
{
    if (app_amp_db.state == APP_AMP_STATE_OFF)
    {
        return true;
    }
    return false;
}

bool app_amp_init(void)
{
    GPIO_InitTypeDef gpio_param;
    int8_t ret = 0;

    gpio_periphclk_config(app_cfg_const.ext_amp_pinmux, (FunctionalState)ENABLE);
    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit  = GPIO_GetPin(app_cfg_const.ext_amp_pinmux);
    gpio_param.GPIO_Mode = GPIO_Mode_OUT;
    gpio_param_config(app_cfg_const.ext_amp_pinmux, &gpio_param);

    memset(&app_amp_db, 0, sizeof(T_APP_AMP_DB));
    app_amp_db.state = APP_AMP_STATE_OFF;
    if (gap_reg_timer_cb((P_TIMEOUT_CB)app_amp_timer_cback, &app_amp_db.amp_queue_id) == false)
    {
        ret = -1;
        goto fail_reg_amp_timer;
    }

    app_amp_db.plugin_handle = audio_plugin_create(app_amp_policies,
                                                   sizeof(app_amp_policies) / sizeof(app_amp_policies[0]));
    if (app_amp_db.plugin_handle == NULL)
    {
        ret = -2;
        goto fail_create_plugin;
    }

    return true;

fail_create_plugin:
fail_reg_amp_timer:
    APP_PRINT_TRACE1("app_amp_init: ret %d", ret);

    return false;
}
#endif
