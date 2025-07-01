#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include<stddef.h>
#include "trace.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "app_io_msg.h"
#include "gap_timer.h"
#include "sysm.h"
#include "app_gpio.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "app_mmi.h"
#include "app_slide_switch.h"
#include "section.h"
#include "app_main.h"
#include "app_adp.h"
#include "single_tone.h"
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif
#include "app_roleswap.h"

typedef enum
{
    APP_SLIDE_SWITCH_0,
    APP_SLIDE_SWITCH_1,//0x01

    APP_SLIDE_SWITCH_TOTAL
} T_APP_SLIDE_SWITCH_ID;

#define APP_SLIDE_SWITCH_DEBOUCE_TIME     500

#define APP_SLIDE_SWITCH_HIGH_LEVEL 1
#define APP_SLIDE_SWITCH_LOW_LEVEL  0

static void *slide_switch_timer_handle[APP_SLIDE_SWITCH_TOTAL] = {NULL};

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
static void *timer_handle_power_on = NULL;
#endif

static uint8_t app_slide_switch_timer_queue_id = 0xFF;

#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
static T_APP_SLIDE_SWITCH_ANC_APT_STATE slide_switch_anc_apt_state  = APP_SLIDE_SWITCH_ALL_OFF;
#endif

typedef enum
{
    APP_IO_TIMER_SLIDE_SWITCH_0_LOW_DEBOUNCE  = 0,
    APP_IO_TIMER_SLIDE_SWITCH_0_HIGH_DEBOUNCE = 1,
    APP_IO_TIMER_SLIDE_SWITCH_1_LOW_DEBOUNCE  = 2,
    APP_IO_TIMER_SLIDE_SWITCH_1_HIGH_DEBOUNCE = 3,
    APP_IO_TIMER_SLIDE_SWITCH_POWER_ON_CHECK = 4,
} T_APP_SLIDE_SWITCH_TIMER;

typedef enum
{
    ACTION_NONE,
    ACTION_POWER_ON,//0x01
    ACTION_POWER_OFF,//0x02
    ACTION_ANC_ON,//0x03
    ACTION_ANC_OFF,//0x04
    ACTION_APT_ON,//0x05
    ACTION_APT_OFF,//0x06
} T_SWITCH_ACTION;

typedef struct
{
    uint8_t support;
    T_SWITCH_ACTION low_action;
    T_SWITCH_ACTION high_action;
    uint8_t pinmux;
} T_APP_SLIDE_SWITCH_CFG;

T_APP_SLIDE_SWITCH_CFG slide_switch[APP_SLIDE_SWITCH_TOTAL];

static void app_slide_switch_update_intpolarity(uint8_t pin_num, uint8_t gpio_status)
{
    GPIO_TypeDef *p_gpio = (GPIO_GetNum(pin_num) <= GPIO31) ? GPIOA : GPIOB;

    if (gpio_status)
    {
        p_gpio->INTPOLARITY &= ~(GPIO_GetPin(pin_num));
    }
    else
    {

        p_gpio->INTPOLARITY |= GPIO_GetPin(pin_num);

    }
}

ISR_TEXT_SECTION
static void app_slide_switch_irq_enable(uint8_t pin_num, bool enable)
{
    if (enable)
    {
        gpio_clear_int_pending(pin_num);
        gpio_mask_int_config(pin_num, DISABLE);
        gpio_init_irq(GPIO_GetNum(pin_num));
        gpio_int_config(pin_num, ENABLE);
    }
    else
    {
        gpio_int_config(pin_num, DISABLE);
        gpio_mask_int_config(pin_num, ENABLE);
        gpio_clear_int_pending(pin_num);
    }
}

static void app_slide_switch_action_handle(T_SWITCH_ACTION action)
{
    APP_PRINT_INFO1("app_slide_switch_action_handle: action %d", action);
    switch (action)
    {
#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
    case ACTION_ANC_ON:
        {
            slide_switch_anc_apt_state = APP_SLIDE_SWITCH_ANC_ON;
            if (app_db.device_state != APP_DEVICE_STATE_ON)
            {
                break;
            }
#if F_APP_ANC_SUPPORT
            if (!app_anc_is_anc_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                app_mmi_handle_action(MMI_ANC_ON_OFF);
            }
#endif
        }
        break;

    case ACTION_ANC_OFF:
        {
            slide_switch_anc_apt_state = APP_SLIDE_SWITCH_ALL_OFF;
            if (app_db.device_state != APP_DEVICE_STATE_ON)
            {
                break;
            }
#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                app_mmi_handle_action(MMI_ANC_ON_OFF);
            }
#endif
        }
        break;

    case ACTION_APT_ON:
        {
            slide_switch_anc_apt_state = APP_SLIDE_SWITCH_APT_ON;
            if (app_db.device_state != APP_DEVICE_STATE_ON)
            {
                break;
            }
#if F_APP_APT_SUPPORT
            if (!app_apt_is_apt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                app_mmi_handle_action(MMI_AUDIO_APT);
            }
#endif
        }
        break;

    case ACTION_APT_OFF:
        {
            slide_switch_anc_apt_state = APP_SLIDE_SWITCH_ALL_OFF;
            if (app_db.device_state != APP_DEVICE_STATE_ON)
            {
                break;
            }
#if F_APP_APT_SUPPORT
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
            {
                app_mmi_handle_action(MMI_AUDIO_APT);
            }
#endif
        }
        break;
#endif

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
    case ACTION_POWER_ON:
        {
            if (app_cfg_const.discharger_support &&
                (app_charger_get_charge_state() == APP_CHARGER_STATE_NO_CHARGE) &&
                (app_charger_get_soc() == BAT_CAPACITY_0))
            {
                //disallow power on
            }
            else if ((app_cfg_const.adaptor_disallow_power_on && (app_adp_get_plug_state() == ADAPTOR_PLUG)) ||
                     (app_cfg_const.enable_inbox_power_off && app_device_is_in_the_box())
                     || !app_slide_switch_power_on_valid_check())
            {
                //disallow power on
            }
            else
            {
                gap_start_timer(&timer_handle_power_on, "slide_switch_power_on check",
                                app_slide_switch_timer_queue_id,
                                APP_IO_TIMER_SLIDE_SWITCH_POWER_ON_CHECK, 0,
                                100);
            }

        }
        break;

    case ACTION_POWER_OFF:
        {
            app_db.power_off_cause = POWER_OFF_CAUSE_SLIDE_SWITCH;
            app_cfg_store(&app_cfg_nv.remote_loc, 4);

#if F_APP_ERWS_SUPPORT
            app_roleswap_poweroff_handle(false);
#else
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
#endif
        }
        break;
#endif

    default:
        break;
    }
}

static void app_slide_switch_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    uint8_t gpio_status = APP_SLIDE_SWITCH_LOW_LEVEL;
    T_APP_SLIDE_SWITCH_ID channel = APP_SLIDE_SWITCH_0;

    APP_PRINT_INFO1("app_slide_switch_timeout_cb timer_id %d ", timer_id);
    switch (timer_id)
    {
    case APP_IO_TIMER_SLIDE_SWITCH_0_LOW_DEBOUNCE:
    case APP_IO_TIMER_SLIDE_SWITCH_0_HIGH_DEBOUNCE:
    case APP_IO_TIMER_SLIDE_SWITCH_1_LOW_DEBOUNCE:
    case APP_IO_TIMER_SLIDE_SWITCH_1_HIGH_DEBOUNCE:
        {
            if (timer_id == APP_IO_TIMER_SLIDE_SWITCH_0_HIGH_DEBOUNCE ||
                timer_id == APP_IO_TIMER_SLIDE_SWITCH_1_HIGH_DEBOUNCE)
            {
                gpio_status = APP_SLIDE_SWITCH_HIGH_LEVEL;
            }

            if (timer_id == APP_IO_TIMER_SLIDE_SWITCH_1_LOW_DEBOUNCE ||
                timer_id == APP_IO_TIMER_SLIDE_SWITCH_1_HIGH_DEBOUNCE)
            {
                channel = APP_SLIDE_SWITCH_1;
            }

            gap_stop_timer(&slide_switch_timer_handle[channel]);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            if (gpio_status == gpio_read_input_level(slide_switch[channel].pinmux))
            {
                if ((slide_switch[channel].low_action == ACTION_POWER_ON ||
                     slide_switch[channel].high_action == ACTION_POWER_ON) &&
                    (app_db.device_state != APP_DEVICE_STATE_OFF_ING))
                {
                    app_cfg_nv.slide_switch_power_on_off_gpio_status = gpio_status;
                    app_cfg_store(&app_cfg_nv.remote_loc, 4);
                }

                if (gpio_status)
                {
                    app_slide_switch_action_handle(slide_switch[channel].high_action);
                }
                else
                {
                    app_slide_switch_action_handle(slide_switch[channel].low_action);
                }
            }
            app_slide_switch_irq_enable(slide_switch[channel].pinmux, true);
        }
        break;

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
    case APP_IO_TIMER_SLIDE_SWITCH_POWER_ON_CHECK:
        {
            gap_stop_timer(&timer_handle_power_on);
            if (app_db.device_state != APP_DEVICE_STATE_ON)
            {
                /* dut mode, no bt_ready msg, do power on mmi action immediately*/
                if ((app_db.bt_is_ready || mp_hci_test_mode_is_running()) &&
                    app_db.device_state == APP_DEVICE_STATE_OFF)
                {
                    app_mmi_handle_action(MMI_DEV_POWER_ON);
                    app_device_unlock_vbat_disallow_power_on();
                }
                else
                {
                    gap_start_timer(&timer_handle_power_on, "slide_switch_power_on check",
                                    app_slide_switch_timer_queue_id,
                                    APP_IO_TIMER_SLIDE_SWITCH_POWER_ON_CHECK, 0,
                                    100);
                }
            }
        }
        break;
#endif

    default:
        break;
    }
}

static uint8_t app_slide_switch_status_read(uint8_t pin_num)
{
    gpio_periphclk_config(pin_num, ENABLE);

    GPIO_InitTypeDef gpio_param;
    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit  = GPIO_GetPin(pin_num);
    gpio_param.GPIO_Mode = GPIO_Mode_IN;
    gpio_param.GPIO_ITCmd = DISABLE;
    gpio_param_config(pin_num, &gpio_param);

    return gpio_read_input_level(pin_num);
}

static void app_slide_switch_x_driver_init(T_APP_SLIDE_SWITCH_ID id)
{
    uint8_t gpio_status = 0;
    uint8_t pinmux = slide_switch[id].pinmux;

    T_SWITCH_ACTION high_action = slide_switch[id].high_action;
    T_SWITCH_ACTION low_action = slide_switch[id].low_action;
    bool irq_enable_now = false;

    gpio_periphclk_config(pinmux, ENABLE);

    GPIO_InitTypeDef gpio_param;
    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(pinmux);
    gpio_param.GPIO_ITCmd = ENABLE;
    gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
    gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;

    gpio_status = app_slide_switch_status_read(pinmux);

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
    if (((high_action == ACTION_POWER_ON) && (low_action == ACTION_POWER_OFF)) ||
        ((high_action == ACTION_POWER_OFF) && (low_action == ACTION_POWER_ON)))
    {
        if (app_cfg_nv.is_app_reboot && (app_cfg_nv.slide_switch_power_on_off_gpio_status == gpio_status))
        {
            //APP_PRINT_INFO0("app_slide_switch_x_driver_init: avoid power on interrupt");
            //slide_switch_state not changed, avoid power on interrupt
            gpio_param.GPIO_ITPolarity = (gpio_status) ? GPIO_INT_POLARITY_ACTIVE_LOW :
                                         GPIO_INT_POLARITY_ACTIVE_HIGH;
            irq_enable_now = true;
        }
        else
        {
            if (low_action == ACTION_POWER_ON)
            {
                gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
                irq_enable_now = true;
            }
            else if (high_action == ACTION_POWER_ON)
            {
                gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
                irq_enable_now = true;
            }
        }
    }
#endif

    gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    gpio_param.GPIO_DebounceTime = GPIO_DETECT_DEBOUNCE_TIME;
    gpio_param_config(pinmux, &gpio_param);

    if (irq_enable_now)
    {
        app_slide_switch_irq_enable(pinmux, true);
    }

}

static void app_slide_switch_power_on_check(void)
{
    uint8_t gpio_status = 0;
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            gpio_status = gpio_read_input_level(slide_switch[i].pinmux);

            if (((slide_switch[i].low_action == ACTION_POWER_ON) &&
                 (slide_switch[i].high_action == ACTION_POWER_OFF))  ||
                ((slide_switch[i].low_action == ACTION_POWER_OFF) &&
                 (slide_switch[i].high_action == ACTION_POWER_ON)))
            {
                //do nothing
            }
            else
            {
                if (gpio_status)
                {
                    app_slide_switch_action_handle(slide_switch[i].high_action);
                }
                else
                {
                    app_slide_switch_action_handle(slide_switch[i].low_action);
                }

                app_slide_switch_update_intpolarity(slide_switch[i].pinmux, gpio_status);
                app_slide_switch_irq_enable(slide_switch[i].pinmux, true);
            }
        }
    }
}

static void app_slide_switch_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            app_slide_switch_power_on_check();
        }
        break;

    default:
        break;
    }
}

void app_slide_switch_cfg_init(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (i == 0)
        {
            slide_switch[i].support = app_cfg_const.slide_switch_0_support;
            slide_switch[i].low_action = (T_SWITCH_ACTION)app_cfg_const.slide_switch_0_low_action;
            slide_switch[i].high_action = (T_SWITCH_ACTION)app_cfg_const.slide_switch_0_high_action;
            slide_switch[i].pinmux = app_cfg_const.slide_switch_0_pinmux;
        }
        else if (i == 1)
        {
            slide_switch[i].support = app_cfg_const.slide_switch_1_support;
            slide_switch[i].low_action = (T_SWITCH_ACTION)app_cfg_const.slide_switch_1_low_action;
            slide_switch[i].high_action = (T_SWITCH_ACTION)app_cfg_const.slide_switch_1_high_action;
            slide_switch[i].pinmux = app_cfg_const.slide_switch_1_pinmux;
        }
    }
}

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
void app_slide_switch_power_on_off_gpio_status_reset(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            if (((slide_switch[i].low_action == ACTION_POWER_ON) &&
                 (slide_switch[i].high_action == ACTION_POWER_OFF)))
            {
                app_cfg_nv.slide_switch_power_on_off_gpio_status = 1;
            }
            else if (((slide_switch[i].low_action == ACTION_POWER_OFF) &&
                      (slide_switch[i].high_action == ACTION_POWER_ON)))
            {
                app_cfg_nv.slide_switch_power_on_off_gpio_status = 0;
            }
        }
    }
}
#endif

void app_slide_switch_board_init(void)
{
    PAD_Pull_Mode pull_mode = PAD_PULL_UP;
    uint8_t i = 0;

    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (!slide_switch[i].support)
        {
            continue;
        }

        Pinmux_Config(slide_switch[i].pinmux, DWGPIO);

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
        if (slide_switch[i].low_action == ACTION_POWER_ON)
        {
            //reduce power consumption
            pull_mode = PAD_PULL_DOWN;
        }
        else if (slide_switch[i].high_action == ACTION_POWER_ON)
        {
            //reduce power consumption
            pull_mode = PAD_PULL_UP;
        }
        else
#endif
        {
            pull_mode = PAD_PULL_UP;
        }

        Pad_Config(slide_switch[i].pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, pull_mode, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    }
}

void app_slide_switch_driver_init(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            app_slide_switch_x_driver_init((T_APP_SLIDE_SWITCH_ID)i);
            app_dlps_pad_wake_up_polarity_invert(slide_switch[i].pinmux);
        }
    }
}

void app_slide_switch_init(void)
{
    sys_mgr_cback_register(app_slide_switch_dm_cback);
    gap_reg_timer_cb(app_slide_switch_timeout_cb, &app_slide_switch_timer_queue_id);
}

#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
#if F_APP_APT_SUPPORT
void app_slide_switch_resume_apt(void)
{
    bool gpio_status;
    uint8_t i = 0;

    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            gpio_status = gpio_read_input_level(slide_switch[i].pinmux);

            if (((slide_switch[i].high_action == ACTION_APT_ON) && (gpio_status == 1)) ||
                ((slide_switch[i].low_action == ACTION_APT_ON) && (gpio_status == 0)))
            {
                if (!app_apt_is_apt_on_state((T_ANC_APT_STATE)(app_db.current_listening_state)))
                {
                    if (app_cfg_const.normal_apt_support)
                    {
                        app_listening_state_machine(EVENT_NORMAL_APT_ON, false, true);
                    }
                    else if (app_cfg_const.llapt_support)
                    {
                        T_ANC_APT_STATE first_llapt_scenario;

                        if (app_apt_set_first_llapt_scenario(&first_llapt_scenario))
                        {
                            app_listening_state_machine(LLAPT_STATE_TO_EVENT(first_llapt_scenario), false, true);
                        }
                    }
                }
            }
        }
    }
}
#endif
#endif

#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
bool app_slide_switch_power_on_valid_check(void)
{
    uint8_t gpio_status = 0;
    bool ret = true;
    uint8_t i = 0;

    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            gpio_status = gpio_read_input_level(slide_switch[i].pinmux);
            if (slide_switch[i].high_action == ACTION_POWER_OFF &&
                slide_switch[i].low_action == ACTION_NONE)
            {
                if (gpio_status)
                {
                    ret = false;
                    break;
                }
            }
            else if (slide_switch[i].low_action == ACTION_POWER_OFF &&
                     slide_switch[i].high_action == ACTION_NONE)
            {
                if (!gpio_status)
                {
                    ret = false;
                    break;
                }
            }
        }
    }
    return ret;
}
#endif

ISR_TEXT_SECTION
void app_slide_switch_intr_handler(T_APP_SLIDE_SWITCH_ID id)
{
    T_IO_MSG gpio_msg;
    uint8_t pin_num = slide_switch[id].pinmux;
    uint8_t gpio_status = gpio_read_input_level(pin_num);

    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);
    app_slide_switch_irq_enable(pin_num, false);

    app_slide_switch_update_intpolarity(pin_num, gpio_status);

    app_dlps_pad_wake_up_polarity_invert(pin_num);

    gpio_msg.type = IO_MSG_TYPE_GPIO;
    if (id == APP_SLIDE_SWITCH_0)
    {
        gpio_msg.subtype = IO_MSG_GPIO_SLIDE_SWITCH_0;
    }
    else if (id == APP_SLIDE_SWITCH_1)
    {
        gpio_msg.subtype = IO_MSG_GPIO_SLIDE_SWITCH_1;
    }

    gpio_msg.u.param = gpio_status;

    if (!app_io_send_msg(&gpio_msg))
    {
        APP_PRINT_ERROR0("app_slide_switch_intr_handler: Send gpio_msg fail!");
    }
}

ISR_TEXT_SECTION
bool app_slide_switch_gpio_handler(uint8_t gpio_num)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            if (GPIO_GetNum(slide_switch[i].pinmux) == gpio_num)
            {
                app_slide_switch_intr_handler((T_APP_SLIDE_SWITCH_ID)i);
                return true;
            }
        }
    }

    return false;
}

void app_slide_switch_handle_msg(T_IO_MSG *msg)
{
    uint16_t subtype = msg->subtype;
    uint8_t gpio_status = msg->u.param;
    uint16_t timer_chann = 0;
    T_APP_SLIDE_SWITCH_ID channel = APP_SLIDE_SWITCH_0;

    if (subtype == IO_MSG_GPIO_SLIDE_SWITCH_0)
    {
        channel = APP_SLIDE_SWITCH_0;
        timer_chann = (gpio_status) ? APP_IO_TIMER_SLIDE_SWITCH_0_HIGH_DEBOUNCE :
                      APP_IO_TIMER_SLIDE_SWITCH_0_LOW_DEBOUNCE;
    }
    else if (subtype == IO_MSG_GPIO_SLIDE_SWITCH_1)
    {
        channel = APP_SLIDE_SWITCH_1;
        timer_chann = (gpio_status) ? APP_IO_TIMER_SLIDE_SWITCH_1_HIGH_DEBOUNCE :
                      APP_IO_TIMER_SLIDE_SWITCH_1_LOW_DEBOUNCE;
    }

    gap_start_timer(&slide_switch_timer_handle[channel], "app_slide_switch_debouce_timer",
                    app_slide_switch_timer_queue_id,
                    timer_chann, 0,
                    APP_SLIDE_SWITCH_DEBOUCE_TIME);
}

#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
T_APP_SLIDE_SWITCH_ANC_APT_STATE app_slide_switch_anc_apt_state_get(void)
{
    return slide_switch_anc_apt_state;
}

bool app_slide_switch_between_anc_apt_support(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            if (((slide_switch[i].high_action == ACTION_ANC_ON) &&
                 (slide_switch[i].low_action == ACTION_APT_ON)) ||
                ((slide_switch[i].high_action == ACTION_APT_ON) && (slide_switch[i].low_action == ACTION_ANC_ON)))
            {
                return true;
            }
        }
    }

    return false;
}

bool app_slide_switch_anc_on_off_support(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            if (((slide_switch[i].high_action == ACTION_ANC_ON) &&
                 (slide_switch[i].low_action == ACTION_ANC_OFF)) ||
                ((slide_switch[i].high_action == ACTION_ANC_OFF) && (slide_switch[i].low_action == ACTION_ANC_ON)))
            {
                return true;
            }
        }
    }

    return false;
}

bool app_slide_switch_apt_on_off_support(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            if (((slide_switch[i].high_action == ACTION_APT_ON) &&
                 (slide_switch[i].low_action == ACTION_APT_OFF)) ||
                ((slide_switch[i].high_action == ACTION_APT_OFF) && (slide_switch[i].low_action == ACTION_APT_ON)))
            {
                return true;
            }
        }
    }

    return false;
}
#endif

void  app_slide_switch_enter_dlps_pad_set(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            app_dlps_pad_wake_up_enable(slide_switch[i].pinmux);
        }
    }
}

void  app_slide_switch_exit_dlps_pad_set(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
        if (slide_switch[i].support)
        {
            app_dlps_restore_pad(slide_switch[i].pinmux);
        }
    }
}

void  app_slide_switch_enter_power_down_pad_set(void)
{
    uint8_t i = 0;
    for (i = 0; i < APP_SLIDE_SWITCH_TOTAL; i++)
    {
#if (F_APP_SLIDE_SWITCH_POWER_ON_OFF == 1)
        if (slide_switch[i].support &&
            ((slide_switch[i].low_action == ACTION_POWER_ON) ||
             (slide_switch[i].high_action == ACTION_POWER_ON)))
        {
            app_dlps_pad_wake_up_enable(slide_switch[i].pinmux);
        }
#endif

        //to reduce power consumption
        if (slide_switch[i].support)
        {
            if (gpio_read_input_level(slide_switch[i].pinmux))
            {
                Pad_Config(slide_switch[i].pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
            }
            else
            {
                Pad_Config(slide_switch[i].pinmux,
                           PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);
            }
        }
    }
}

#endif
