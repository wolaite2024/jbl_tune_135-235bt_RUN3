/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include "string.h"
#include "os_mem.h"
#include "trace.h"
#include "rtl876x_nvic.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "app_key_gpio.h"
#include "rtl876x_pinmux.h"
#include "vector_table.h"
#include "app_gpio.h"
#include "mfb_api.h"
#include "app_io_msg.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "app_main.h"
#include "section.h"
#include "vector_table.h"
#include "app_sensor.h"

#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
#include "hw_tim.h"
#else
#include "gap_timer.h"
#endif

static T_KEY_GPIO_DATA *p_gpio_data = NULL;
#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
#define PERIOD_KEY_GPIO_TIM   10
static T_HW_TIMER_HANDLE key_gpio_hw_timer_handle = NULL;
#else
uint8_t key_gpio_timer_queue_id = 0;
void *timer_handle_key0_debounce = NULL;
#endif
static uint8_t key0_debounce_initial_flag;
uint8_t mfb_state = 1;
void  key_mfb_level_change_cb(void);

bool key_get_press_state(uint8_t key_index)
{
    return (p_gpio_data->key_press[key_index] == KEY_PRESS);
}

void key_mfb_init(void)
{
    key0_debounce_initial_flag = 1;
    mfb_init((P_MFB_LEVEL_CHANGE_CBACK)key_mfb_level_change_cb);
    Pad_WakeUpCmd(MFB_MODE, POL_LOW, ENABLE);

#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
    hw_timer_start(key_gpio_hw_timer_handle);
#else
    gap_start_timer(&timer_handle_key0_debounce, "key0_debounce",
                    key_gpio_timer_queue_id,
                    APP_IO_TIMER_KEY0_DEBOUNCE, 0, GPIO_INIT_DEBOUNCE_TIME);
#endif
}

ISR_TEXT_SECTION
void  key_mfb_level_change_cb(void)
{
#if F_APP_SENSOR_SL7A20_SUPPORT
    if (app_cfg_const.gsensor_support && app_cfg_const.enable_mfb_pin_as_gsensor_interrupt_pin)
    {
        // do nothing
    }
    else
#endif
    {
        app_db.key0_wake_up = true;
    }

    key0_gpio_intr_handler();
}

static void key0_debounce_handler(void)
{
    T_IO_MSG button_msg;
    uint8_t key_status;
    uint8_t key_status_update_fg = 0;

    uint8_t gpio_num = 0;/* specified GPIO pin number*/
    GPIO_TypeDef *GPIOx;
    gpio_num = GPIO_GetNum(app_cfg_const.key_pinmux[0]);
    GPIOx = gpio_num <= GPIO31 ? GPIOA : GPIOB;


    key_status = p_gpio_data->key_status[0];

    if (app_cfg_const.key_enable_mask & KEY0_MASK)
    {
        if (!app_cfg_const.mfb_replace_key0)
        {
            if (key_status != gpio_read_input_level(app_cfg_const.key_pinmux[0]))
            {
                /* Enable GPIO interrupt */
                GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]), DISABLE);
                GPIOx_INTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]), ENABLE);
                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                return;
            }
        }
    }
    else
    {
        key_status = mfb_get_level();
        if (key_status == mfb_state)
        {
            APP_PRINT_ERROR1("mfb level same with previous state, ignore it %x", mfb_state);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
            return;
        }
        mfb_state = key_status;
    }
    button_msg.type = IO_MSG_TYPE_GPIO;
    button_msg.subtype = IO_MSG_GPIO_KEY;

    if (app_cfg_const.key_high_active_mask & BIT0)
    {
        if (key_status) //Button pressed
        {
            if (!app_cfg_const.mfb_replace_key0)
            {
                GPIOx->INTPOLARITY &= ~(GPIO_GetPin(app_cfg_const.key_pinmux[0])); //Polarity Low
            }
            if (p_gpio_data->key_press[0] == KEY_RELEASE)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_PRESS;
            }
        }
        else //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
            if (!app_cfg_const.mfb_replace_key0)
            {
                GPIOx->INTPOLARITY |= GPIO_GetPin(app_cfg_const.key_pinmux[0]); //Polarity High
            }

            if (p_gpio_data->key_press[0] == KEY_PRESS)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_RELEASE;
            }
        }
    }
    else
    {
        if (key_status) //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
            if (!app_cfg_const.mfb_replace_key0)
            {
                GPIOx->INTPOLARITY &= ~(GPIO_GetPin(app_cfg_const.key_pinmux[0])); //Polarity Low
            }
            if (p_gpio_data->key_press[0] == KEY_PRESS)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_RELEASE;
            }
        }
        else //Button pressed
        {
            if (!app_cfg_const.mfb_replace_key0)
            {
                GPIOx->INTPOLARITY |= GPIO_GetPin(app_cfg_const.key_pinmux[0]); //Polarity High
            }
            if (p_gpio_data->key_press[0] == KEY_RELEASE)
            {
                button_msg.u.param = (KEY0_MASK << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[0] = KEY_PRESS;
            }
        }
    }

    /* Send MSG to APP task */
    if (key_status_update_fg)
    {
        if (app_io_send_msg(&button_msg) == false)
        {
            APP_PRINT_ERROR0("key0_debounce_handler: Send key msg error");
        }
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    }
    if (!app_cfg_const.mfb_replace_key0)
    {
        if (app_cfg_const.key_enable_mask & KEY0_MASK)
        {
            /* Enable GPIO interrupt */
            GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]), DISABLE);
            GPIOx_INTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]), ENABLE);
        }
    }
}

#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
void key_gpio_hw_timer_isr_callback(T_HW_TIMER_HANDLE handle)
{
    hw_timer_stop(key_gpio_hw_timer_handle);
    if (key0_debounce_initial_flag == 0)
    {
        key0_debounce_handler();
    }
}
#else
static void key_gpio_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case APP_IO_TIMER_KEY0_DEBOUNCE:
        {
            gap_stop_timer(&timer_handle_key0_debounce);

            if (key0_debounce_initial_flag == 0)
            {
#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
                if (app_cfg_const.gsensor_support && app_cfg_const.enable_mfb_pin_as_gsensor_interrupt_pin)
                {
                    app_sensor_sl_mfb_intr_handler();
                }
                else
#endif
                {
                    key0_debounce_handler();
                }
            }
        }
        break;

    default:
        break;
    }
}
#endif

void key_gpio_initial(void)
{
    uint32_t i;
    if (MAX_KEY_NUM != 0)
    {
        uint8_t pin_num = 0;/* specified GPIO pin number*/
        uint32_t gpioa_pin, gpiob_pin;/* specified GPIOA and GPIOB pin temporary variable*/

        if (p_gpio_data == NULL)
        {
            p_gpio_data = (T_KEY_GPIO_DATA *)calloc(1, sizeof(T_KEY_GPIO_DATA));
        }
        else
        {
            memset(p_gpio_data, 0, sizeof(T_KEY_GPIO_DATA));// set p_gpio_data->key_press[i] = KEY_RELEASE;
        }

#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
        key_gpio_hw_timer_handle = hw_timer_create("app_key_gpio", PERIOD_KEY_GPIO_TIM, false,
                                                   key_gpio_hw_timer_isr_callback);
        if (key_gpio_hw_timer_handle == NULL)
        {
            APP_PRINT_ERROR0("key_gpio HW timer create failed !");
            return;
        }
#else
        gap_reg_timer_cb(key_gpio_timer_callback, &key_gpio_timer_queue_id);
#endif

        GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */
        //Key 0 is used at POWER_POWERDOWN_MODE wake up. Need to set level trigger
        if (app_cfg_const.key_enable_mask & KEY0_MASK)
        {
            GPIO_InitTypeDef key0_param; /* Define GPIO parameter structure */

            key0_debounce_initial_flag = 1;
#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
            hw_timer_start(key_gpio_hw_timer_handle);
#else
            gap_start_timer(&timer_handle_key0_debounce, "key0_debounce",
                            key_gpio_timer_queue_id,
                            APP_IO_TIMER_KEY0_DEBOUNCE, 0, GPIO_INIT_DEBOUNCE_TIME);
#endif

            GPIO_StructInit(&key0_param);
            key0_param.GPIO_PinBit = GPIO_GetPin(app_cfg_const.key_pinmux[0]);
            key0_param.GPIO_ITCmd = ENABLE;

            if (app_cfg_const.key_high_active_mask & BIT0)
            {
                key0_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
            }

            pin_num = GPIO_GetNum(app_cfg_const.key_pinmux[0]);
            if (pin_num <= GPIO31)
            {
                RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
                GPIOx_Init(GPIOA, &key0_param);
            }
            else
            {
                RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
                GPIOx_Init(GPIOB, &key0_param);
            }
        }

        GPIO_StructInit(&gpio_param);
        gpioa_pin = 0;
        gpiob_pin = 0;
        /*key0 key 2  key4  key6  make as high edge trigger*/
        for (i = 1; i < MAX_KEY_NUM; i++)
        {
            if ((app_cfg_const.key_enable_mask & BIT(i))
                && (app_cfg_const.key_high_active_mask & BIT(i)))
            {
                pin_num = GPIO_GetNum(app_cfg_const.key_pinmux[i]);
                if (pin_num <= GPIO31)
                {
                    gpioa_pin |= GPIO_GetPin(app_cfg_const.key_pinmux[i]);
                }
                else
                {
                    gpiob_pin |= GPIO_GetPin(app_cfg_const.key_pinmux[i]);
                }
            }
        }
        gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
        gpio_param.GPIO_ITCmd = ENABLE;
        gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
        gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
        gpio_param.GPIO_DebounceTime = GPIO_DETECT_DEBOUNCE_TIME;

        if (gpioa_pin != 0)
        {
            RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
            gpio_param.GPIO_PinBit = gpioa_pin;
            GPIOx_Init(GPIOA, &gpio_param);
        }
        if (gpiob_pin != 0)
        {
            RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
            gpio_param.GPIO_PinBit = gpiob_pin;
            GPIOx_Init(GPIOB, &gpio_param);
        }

        /*key1 key 3  key5  key7  make as low edge trigger*/
        GPIO_StructInit(&gpio_param);
        gpioa_pin = 0;
        gpiob_pin = 0;

        for (i = 1; i < MAX_KEY_NUM; i++)
        {
            if ((app_cfg_const.key_enable_mask & BIT(i))
                && ((app_cfg_const.key_high_active_mask & BIT(i)) == 0))
            {
                pin_num = GPIO_GetNum(app_cfg_const.key_pinmux[i]);
                if (pin_num <= GPIO31)
                {
                    gpioa_pin |= GPIO_GetPin(app_cfg_const.key_pinmux[i]);
                }
                else
                {
                    gpiob_pin |= GPIO_GetPin(app_cfg_const.key_pinmux[i]);
                }
            }
        }
        gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
        gpio_param.GPIO_ITCmd = ENABLE;
        gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
        gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
        gpio_param.GPIO_DebounceTime = GPIO_DETECT_DEBOUNCE_TIME;
        if (gpioa_pin != 0)
        {
            RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
            gpio_param.GPIO_PinBit = gpioa_pin;
            GPIOx_Init(GPIOA, &gpio_param);
        }
        if (gpiob_pin != 0)
        {
            RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);
            gpio_param.GPIO_PinBit = gpiob_pin;
            GPIOx_Init(GPIOB, &gpio_param);
        }

        /* Enable Interrupt (Peripheral, CPU NVIC) */
        for (i = 0; i < MAX_KEY_NUM; i++)
        {
            if (app_cfg_const.key_enable_mask & BIT(i))
            {
                if (app_cfg_const.key_high_active_mask & BIT(i))
                {
                    Pad_WakeupPolarityValue(app_cfg_const.key_pinmux[i], PAD_WAKEUP_POL_HIGH);
                }
                else
                {
                    Pad_WakeupPolarityValue(app_cfg_const.key_pinmux[i], PAD_WAKEUP_POL_LOW);
                }
                pin_num = GPIO_GetNum(app_cfg_const.key_pinmux[i]);
                GPIOx_MaskINTConfig(pin_num <= GPIO31 ? GPIOA : GPIOB, GPIO_GetPin(app_cfg_const.key_pinmux[i]),
                                    DISABLE);
                gpio_init_irq(pin_num);
                GPIOx_INTConfig(pin_num <= GPIO31 ? GPIOA : GPIOB, GPIO_GetPin(app_cfg_const.key_pinmux[i]),
                                ENABLE);
            }

        }
    }
}

ISR_TEXT_SECTION
void key_gpio_intr_handler(uint8_t key_mask, uint8_t gpio_index, uint8_t key_index)
{
    uint8_t key_status;
    uint8_t key_status_update_fg = 0;
    T_IO_MSG button_msg;
    uint8_t gpio_num = 0;/* specified GPIO pin number*/
    GPIO_TypeDef *GPIOx;
    gpio_num = GPIO_GetNum(gpio_index);
    GPIOx = gpio_num <= GPIO31 ? GPIOA : GPIOB;
    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);
    app_dlps_pad_wake_up_polarity_invert(gpio_index);

    key_status = gpio_read_input_level(gpio_index);

    APP_PRINT_TRACE3("key_gpio_intr_handler: key_mask =0x%x, gpio_index =%d, key_status %d", key_mask,
                     gpio_index, key_status);

    /* Disable GPIO interrupt */
    GPIOx_INTConfig(GPIOx, GPIO_GetPin(gpio_index), DISABLE);
    GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(gpio_index), ENABLE);
    GPIOx_ClearINTPendingBit(GPIOx, GPIO_GetPin(gpio_index));

    button_msg.type = IO_MSG_TYPE_GPIO;
    button_msg.subtype = IO_MSG_GPIO_KEY;

    if (key_mask & app_cfg_const.key_high_active_mask) // high active
    {
        if (key_status) //Button pressed
        {
            GPIOx->INTPOLARITY &= ~(GPIO_GetPin(gpio_index)); //Polarity Low
            if (p_gpio_data->key_press[key_index] == KEY_RELEASE)
            {
                button_msg.u.param = (key_mask << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_PRESS;
            }
        }
        else //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
            GPIOx->INTPOLARITY |= GPIO_GetPin(gpio_index); //Polarity High
            if (p_gpio_data->key_press[key_index] == KEY_PRESS)
            {
                button_msg.u.param = (key_mask << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_RELEASE;
            }
        }
    }
    else // low active
    {
        if (key_status) //Button released
        {
            /* Change GPIO Interrupt Polarity, Enable Interrupt */
            GPIOx->INTPOLARITY &= ~(GPIO_GetPin(gpio_index)); //Polarity Low
            if (p_gpio_data->key_press[key_index] == KEY_PRESS)
            {
                button_msg.u.param = (key_mask << 8) | KEY_RELEASE;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_RELEASE;
            }
        }
        else //Button pressed
        {
            GPIOx->INTPOLARITY |= GPIO_GetPin(gpio_index); //Polarity High
            if (p_gpio_data->key_press[key_index] == KEY_RELEASE)
            {
                button_msg.u.param = (key_mask << 8) | KEY_PRESS;
                key_status_update_fg = 1;
                p_gpio_data->key_press[key_index] = KEY_PRESS;
            }
        }
    }

    /* Send MSG to APP task */
    if (key_status_update_fg)
    {
        if (app_io_send_msg(&button_msg) == false)
        {
            APP_PRINT_ERROR0("key_gpio_intr_handler: Send key msg error");
        }
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    }

    /* Enable GPIO interrupt */
    GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(gpio_index), DISABLE);
    GPIOx_INTConfig(GPIOx, GPIO_GetPin(gpio_index), ENABLE);
}

ISR_TEXT_SECTION
void key0_gpio_intr_handler(void)
{
    uint8_t gpio_num = 0;/* specified GPIO pin number*/
    GPIO_TypeDef *GPIOx;
    gpio_num = GPIO_GetNum(app_cfg_const.key_pinmux[0]);
    GPIOx = gpio_num <= GPIO31 ? GPIOA : GPIOB;

    if (!app_cfg_const.mfb_replace_key0)
    {
        APP_PRINT_TRACE0("key0_gpio_intr_handler");
    }
    else
    {
        APP_PRINT_TRACE0("MFB_intr_handler == key0_gpio_intr_handler");
    }

    /* Control of entering DLPS */
#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
    if (app_cfg_const.gsensor_support && app_cfg_const.enable_mfb_pin_as_gsensor_interrupt_pin)
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_MFB_KEY);
    }
    else
#endif
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);
    }

    if (app_cfg_const.key_enable_mask & KEY0_MASK)
    {
        p_gpio_data->key_status[0] = gpio_read_input_level(app_cfg_const.key_pinmux[0]);
        /* Disable GPIO interrupt */
        GPIOx_INTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]), DISABLE);
        GPIOx_MaskINTConfig(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]), ENABLE);
        GPIOx_ClearINTPendingBit(GPIOx, GPIO_GetPin(app_cfg_const.key_pinmux[0]));
        app_dlps_pad_wake_up_polarity_invert(app_cfg_const.key_pinmux[0]);
    }
    else
    {
        p_gpio_data->key_status[0] = mfb_get_level();
    }

    /* Delay 50ms to read the GPIO status */
    key0_debounce_initial_flag = 0;

#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
    hw_timer_start(key_gpio_hw_timer_handle);
#else
    T_IO_MSG button_msg;

    button_msg.type = IO_MSG_TYPE_GPIO;
    button_msg.subtype = IO_MSG_GPIO_KEY0;

    if (app_io_send_msg(&button_msg) == false)
    {
        APP_PRINT_ERROR0("key0_gpio_intr_handler: Send key msg error");
    }
#endif
}

bool app_key_check_power_button_pressed(void)
{
    if (app_cfg_const.mfb_replace_key0)
    {
        return !((bool)mfb_get_level());
    }
    else
    {
        return !((bool)(app_cfg_const.key_high_active_mask & BIT0) ^ ((bool)gpio_read_input_level(
                                                                          app_cfg_const.key_pinmux[0])));
    }
}
