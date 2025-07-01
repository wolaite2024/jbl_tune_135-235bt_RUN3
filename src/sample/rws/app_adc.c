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
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#if F_APP_ADC_SUPPORT
#include "trace.h"
#include "app_adc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_adc.h"
#include "adc_manager.h"
#include "board.h"
#include "os_sync.h"
#include "system_status_api.h"
#include "app_report.h"
#include "app_cfg.h"
#include "gap_timer.h"
#include "app_dlps.h"
#include "app_msg.h"
#include "app_adp.h"
#include "app_main.h"
#include "app_io_msg.h"
#include "app_charger.h"
#include "adapter.h"

uint8_t application_key_adc_channel_index = 0x0;
bool is_adc_mgr_ever_registered = false;

static uint8_t target_io_pin;
static uint8_t cmd_path;
static uint8_t app_idx;

static bool adc_adp_voltage_init = false;
static uint16_t adp_voltage = 0;
static uint8_t adc_channel_adp_voltage = 0;

static uint8_t app_adc_timer_queue_id = 0;
static void *timer_handle_adc_read_adp_voltage = NULL;

#define APP_TIMER_ADC_READ_ADP_VOLTAGE   0
#define ADC_READ_ADP_VOLTAGE_TIMEOUT_MS  1000
/***********************************************************
AON must use system_status_api, follow BB2
************************************************************/
/** @defgroup  APP_ADC_API Audio ADC
    * @brief app process implementation for audio sample project
    * @{
    */

/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/** @defgroup APP_ADC_Exported_Functions Audio ADC Functions
    * @brief
    * @{
    */
void app_adc_set_cmd_info(uint8_t path, uint8_t idx)
{
    cmd_path = path;
    app_idx = idx;
}

bool app_adc_enable_read_adc_io(uint8_t input_pin)
{
    if (!IS_ADC_CHANNEL(input_pin))
    {
        APP_PRINT_INFO0("app_adc_enable_read_adc_io: invalid ADC IO");
        return false;
    }

    target_io_pin = input_pin;

    if (is_adc_mgr_ever_registered)
    {
        APP_PRINT_INFO1("app_adc_enable_read_adc_io: ever registered %d",
                        application_key_adc_channel_index);
        adc_mgr_enable_req(application_key_adc_channel_index);
        return true;
    }

    Pad_Config(input_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PowerOrShutDownValue(input_pin, 0);

    sys_hall_set_rglx_auxadc(input_pin);

    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x1;
    ADC_InitStruct.schIndex[0] = EXT_SINGLE_ENDED(input_pin);

    if (!adc_mgr_register_req(&ADC_InitStruct,
                              (adc_callback_function_t)app_adc_interrupt_handler,
                              &application_key_adc_channel_index))
    {
        APP_PRINT_INFO0("app_adc_enable_read_adc_io: ADC Register Request Fail");
        return false;
    }
    else
    {
        is_adc_mgr_ever_registered = true;
    }

    adc_mgr_enable_req(application_key_adc_channel_index);

    APP_PRINT_TRACE1("app_adc_enable_read_adc_io: %d", application_key_adc_channel_index);

    return true;
}

void app_adc_interrupt_handler(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data;
    uint16_t final_value[2];
    uint8_t evt_param[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    adc_mgr_read_data_req(application_key_adc_channel_index, &adc_data, 0x0001);

    final_value[0]  = ADC_GetHighBypassRes(adc_data,
                                           EXT_SINGLE_ENDED(target_io_pin));
    evt_param[0] = (uint8_t) final_value[0];
    evt_param[1] = (uint8_t)(final_value[0] >> 8);
    APP_PRINT_TRACE4("app_adc_interrupt_handler: int_status: %d, adc_data = 0x%x, Final result = 0x%02X %02X",
                     int_status, adc_data,
                     evt_param[1], evt_param[0]);

    app_report_event(cmd_path, EVENT_REPORT_PAD_VOLTAGE, app_idx, evt_param, sizeof(evt_param));
}

static void app_adc_adp_voltage_read_callback(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data;
    uint16_t sched_bit_map = 0x0001;
    bool ret = true;

    adc_mgr_read_data_req(adc_channel_adp_voltage, &adc_data, sched_bit_map);

    adp_voltage = ADC_GetRes(adc_data, INTERNAL_VADPIN_MODE);

    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_ADP_VOLTAGE;

    if (!app_io_send_msg(&adp_msg))
    {
        ret = false;
    }

    APP_PRINT_INFO3("app_adc_adp_voltage_read_callback: %d, %d", adp_voltage, adc_data,
                    ret);
}

static void app_adc_adp_voltage_init(void)
{
    APP_PRINT_INFO1("app_adc_adp_voltage_init: %d", adc_adp_voltage_init);

    if (adc_adp_voltage_init)
    {
        /* already init */
        return;
    }

    ADC_InitTypeDef ADC_InitStruct;

    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x0001;
    ADC_InitStruct.schIndex[0] = INTERNAL_VADPIN_MODE;

    if (!adc_mgr_register_req(&ADC_InitStruct,
                              (adc_callback_function_t)app_adc_adp_voltage_read_callback,
                              &adc_channel_adp_voltage))
    {
        APP_PRINT_ERROR0("app_adc_adp_voltage_init: adc_mgr_register_req failed");
        return;
    }

    adc_adp_voltage_init = true;
}

static void app_adc_adp_voltage_deinit(void)
{
    APP_PRINT_INFO2("app_adc_adp_voltage_deinit: %d, %d", adc_adp_voltage_init,
                    adc_channel_adp_voltage);

    if (adc_adp_voltage_init)
    {
        adc_mgr_free_chann(adc_channel_adp_voltage);
    }
}

static void app_adc_adp_voltage_read(void)
{
    APP_PRINT_INFO1("app_adc_adp_voltage_read: %d", adc_adp_voltage_init);

    if (adc_adp_voltage_init)
    {
        adc_mgr_enable_req(adc_channel_adp_voltage);
    }
}

void app_adc_start_monitor_adp_voltage(void)
{
    APP_PRINT_TRACE1("app_adc_start_monitor_adp_voltage: %d", ADP_DET_status);

    if (ADP_DET_status == ADP_DET_IN)
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_ADP_VOLTAGE);
        app_adc_adp_voltage_init();

        gap_start_timer(&timer_handle_adc_read_adp_voltage,
                        "adc_read_adp_voltage",
                        app_adc_timer_queue_id,
                        APP_TIMER_ADC_READ_ADP_VOLTAGE,
                        0,
                        ADC_READ_ADP_VOLTAGE_TIMEOUT_MS);
    }
}

void app_adc_stop_monitor_adp_voltage(void)
{
    APP_PRINT_INFO0("app_adc_stop_monitor_adp_voltage");

    app_dlps_enable(APP_DLPS_ENTER_CHECK_ADP_VOLTAGE);
    app_adc_adp_voltage_deinit();
    gap_stop_timer(&timer_handle_adc_read_adp_voltage);
}

void app_adc_handle_adp_voltage_msg(void)
{
    APP_PRINT_INFO3("app_adc_handle_adp_voltage_msg: %d, %d, %d", ADP_DET_status, adp_voltage,
                    app_db.device_state);

    if (ADP_DET_status == ADP_DET_IN)
    {
        if ((adp_voltage < 4400) && (app_db.device_state == APP_DEVICE_STATE_OFF))
        {
            app_adc_stop_monitor_adp_voltage();

            app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_ADP_VOLTAGE);
        }
        else
        {
            app_adp_smart_box_charger_control(true, CHARGER_CONTROL_BY_ADP_VOLTAGE);
        }
    }
}

static void app_adc_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case APP_TIMER_ADC_READ_ADP_VOLTAGE:
        {
            if (timer_handle_adc_read_adp_voltage != NULL)
            {
                app_adc_adp_voltage_read();
                gap_start_timer(&timer_handle_adc_read_adp_voltage,
                                "adc_read_adp_voltage",
                                app_adc_timer_queue_id,
                                APP_TIMER_ADC_READ_ADP_VOLTAGE,
                                0,
                                5 * ADC_READ_ADP_VOLTAGE_TIMEOUT_MS);
            }
        }
        break;

    default:
        break;
    }
}

void app_adc_init(void)
{
    if (app_cfg_const.charger_control_by_mcu)
    {
        gap_reg_timer_cb(app_adc_timeout_cb, &app_adc_timer_queue_id);
    }
}
#endif
/** @} */ /* End of group APP_ADC_Exported_Functions */
/** @} */ /* End of group APP_ADC_API */
