

/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
  ***************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#if F_APP_NFC_SUPPORT
#include "trace.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "wdg.h"
#include "gap_timer.h"
#include "app_mmi.h"
#include "sysm.h"
#include "app_cfg.h"
#include "app_gpio.h"
#include "app_io_msg.h"
#include "app_dlps.h"
#include "app_nfc.h"
#include "app_audio_policy.h"
#if F_APP_BUZZER_SUPPORT
#include "app_buzzer.h"
#endif
#include "section.h"

static void *timer_handle_gpio_nfc = NULL;
static void *timer_handle_nfc_multi_link_switch = NULL;
static uint8_t app_gpio_nfc_timer_queue_id = 0;
T_NFC_DATA nfc_data;                       /**<record nfc variable */


typedef enum
{
    APP_IO_TIMER_GPIO_NFC_STABLE,
    APP_IO_TIMER_GPIO_NFC_MULTI_LINK_SWITCH,
} T_APP_GPIO_NFC_TIMER;

static void app_gpio_nfc_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_gpio_nfc_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
    case APP_IO_TIMER_GPIO_NFC_STABLE:
        {
            gap_stop_timer(&timer_handle_gpio_nfc);

            if (nfc_data.nfc_detected == 1)
            {
                nfc_data.nfc_stable_count++;
                if (nfc_data.nfc_stable_count > app_cfg_const.nfc_stable_interval)
                {
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
#if F_APP_BUZZER_SUPPORT
                    buzzer_set_mode(ALARM_BUZZER);
#endif
                    app_audio_tone_type_play(TONE_ALARM, false, false);
                    app_mmi_handle_action(MMI_DEV_NFC_DETECT);
                    nfc_data.nfc_stable_count = 0;
                }
                else //Polling NFC stable
                {
                    gap_start_timer(&timer_handle_gpio_nfc, "nfc_stable", app_gpio_nfc_timer_queue_id,
                                    APP_IO_TIMER_GPIO_NFC_STABLE, 0, NFC_STABLE_TIMER_UNIT_MS);
                }
            }
            else
            {
                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                nfc_data.nfc_stable_count = 0;
            }
        }
        break;

    case APP_IO_TIMER_GPIO_NFC_MULTI_LINK_SWITCH:
        {
            gap_stop_timer(&timer_handle_nfc_multi_link_switch);
            nfc_data.nfc_multi_link_switch = 0;
        }
        break;

    default:
        break;
    }
}


void app_gpio_nfc_board_init(void)
{
    uint8_t pin_num = app_cfg_const.nfc_pinmux;
    Pinmux_Config(pin_num, DWGPIO);
    Pad_Config(pin_num, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}


/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/**
    * @brief  NFC GPIO initial.
    *         Include APB peripheral clock config, NFC GPIO parameter config and
    *         NFC GPIO interrupt mark config. Enable NFC GPIO interrupt.
    * @param  void
    * @return void
    */
void app_nfc_init(void)
{
    GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */

    GPIO_StructInit(&gpio_param);

    gpio_param.GPIO_PinBit = GPIO_GetPin(app_cfg_const.nfc_pinmux);
    gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
    gpio_param.GPIO_ITCmd = ENABLE;
    gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
    gpio_param.GPIO_DebounceTime = GPIO_INT_DEBOUNCE_DISABLE;

    gpio_periphclk_config(app_cfg_const.nfc_pinmux, ENABLE);
    gpio_param_config(app_cfg_const.nfc_pinmux, &gpio_param);

    /* Enable Interrupt (Peripheral, CPU NVIC) */
    {
        gpio_mask_int_config(app_cfg_const.nfc_pinmux, DISABLE);
        gpio_init_irq(GPIO_GetNum(app_cfg_const.nfc_pinmux));
        gpio_int_config(app_cfg_const.nfc_pinmux, ENABLE);
    }

    /*register gpio nfc timer callback queue,device manager callback*/
    gap_reg_timer_cb(app_gpio_nfc_timeout_cb, &app_gpio_nfc_timer_queue_id);
}

void app_gpio_nfc_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    nfc_data.nfc_detected = io_driver_msg_recv->u.param;
    gap_start_timer(&timer_handle_gpio_nfc, "nfc_stable", app_gpio_nfc_timer_queue_id,
                    APP_IO_TIMER_GPIO_NFC_STABLE, 0, NFC_STABLE_TIMER_UNIT_MS);
}

void app_nfc_multi_link_switch_trigger(void)
{
    gap_stop_timer(&timer_handle_nfc_multi_link_switch);
    nfc_data.nfc_multi_link_switch |= (NFC_MULTI_LINK_SWITCH_A2DP | NFC_MULTI_LINK_SWITCH_HF);
    gap_start_timer(&timer_handle_nfc_multi_link_switch, "nfc_switch", app_gpio_nfc_timer_queue_id,
                    APP_IO_TIMER_GPIO_NFC_MULTI_LINK_SWITCH, 0, NFC_TRIGGER_TIMER_UNIT_MS);
}

/**
    * @brief  NFC GPIO interrupt will be handle in this function.
    *         First disable app enter dlps mode and read current NFC GPIO input data bit.
    *         Disable NFC GPIO interrupt and send IO_MSG_TYPE_GPIO message to app task.
    *         Then enable NFC GPIO interrupt.
    * @param  void
    * @return void
    */
ISR_TEXT_SECTION
void nfc_gpio_intr_handler(void)
{
    uint8_t gpio_status;
    T_IO_MSG gpio_msg;

    APP_PRINT_INFO0("nfc_gpio_intr_handler");
    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);
    gpio_status = gpio_read_input_level(app_cfg_const.nfc_pinmux);

    /* Disable GPIO interrupt */
    gpio_int_config(app_cfg_const.nfc_pinmux, DISABLE);
    gpio_mask_int_config(app_cfg_const.nfc_pinmux, ENABLE);
    gpio_clear_int_pending(app_cfg_const.nfc_pinmux);

    /* Change GPIO Interrupt Polarity */
    if (gpio_status) //GPIO detected and Polarity low
    {
        gpio_intpolarity_config(app_cfg_const.nfc_pinmux, GPIO_INT_POLARITY_ACTIVE_LOW);
        gpio_msg.u.param = 1;
    }
    else //GPIO released and Polarity High
    {
        gpio_intpolarity_config(app_cfg_const.nfc_pinmux, GPIO_INT_POLARITY_ACTIVE_HIGH);
        gpio_msg.u.param = 0;
    }

    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_NFC;

    if (app_io_send_msg(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("nfc_gpio_intr_handler: Send NFC msg error");
    }

    /* Enable GPIO interrupt */
    gpio_mask_int_config(app_cfg_const.nfc_pinmux, DISABLE);
    gpio_int_config(app_cfg_const.nfc_pinmux, ENABLE);
}

#endif
