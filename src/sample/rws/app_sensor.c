
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "board.h"
#include "app_main.h"
#include "app_sensor.h"
#include "app_roleswap.h"
#include "app_cfg.h"
#include "app_bud_loc.h"
#include "app_relay.h"
#include "mfb_api.h"
#include "app_mmi.h"
#include "app_audio_policy.h"
#if (F_APP_SENSOR_SUPPORT == 1)
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "platform_utils.h"
#include "hw_tim.h"
#include "wdg.h"
#include "gap_timer.h"
#include "rtl876x_i2c.h"
#include "app_mmi.h"
#include "app_gpio.h"
#include "app_io_msg.h"
#include "app_dlps.h"
#include "app_key_process.h"
#include "app_key_gpio.h"
#include "section.h"
#include "app_audio_policy.h"
#include "sysm.h"
#include "app_bud_loc.h"
#include "app_cmd.h"
#include "app_report.h"
#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
#include "app_sensor_px318j.h"
#endif
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1) || (F_APP_SENSOR_JSA1227_SUPPORT == 1)
#include "app_sensor_jsa.h"
#endif
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
#include "app_sensor_iqs773_873.h"
#include "app_vendor_iqs773_873.h"
#endif
#if (F_APP_HEARABLE_SUPPORT == 1)
#include "app_hearable.h"
#endif
#endif
#if F_APP_SENSOR_CAP_TOUCH_SUPPORT
#include "app_cap_touch.h"
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_sensor_bud_loc_single_hook)(void) = NULL;
#endif
static void app_sensor_bud_changed(uint8_t event, bool from_remote, uint8_t para);

// for CMD_PX318J_CALIBRATION
#define PX_CALIBRATION_NOISE_FLOOR               1
#define PX_CALIBRATION_IN_EAR_THRESHOLD          2
#define PX_CALIBRATION_OUT_EAR_THRESHOLD         3
#define PX_PX318J_PARA                           4
#define PX_PX318J_WRITE_PARA                     5

#if (F_APP_SENSOR_SUPPORT == 1)
T_SENSOR_LD_DATA       sensor_ld_data;                       /**<record sensor variable */
static T_GSENSOR_SL         gsensor_sl;                      /**<record vendor click variable */
static void *timer_handle_gsensor_click_detect = NULL;

static void *timer_handle_sensor_ld_debounce = NULL;
static void *timer_handle_sensor_ld_io_debounce = NULL;
static void *timer_handle_sensor_in_ear_from_case = NULL;

static uint8_t app_gpio_sensor_timer_queue_id = 0;
//static bool enter_sniff_mode = false;
static bool ld_sensor_after_reset = false;
static bool loc_in_air_from_ear = false;
static bool loc_in_ear_from_case = false;
static bool is_light_sensor_enabled = false;

static bool i2c_device_fail = false;

#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
static T_HW_TIMER_HANDLE sensor_hw_timer_handle = NULL;
uint32_t sendor_ld_hx_sw_cycle = SENSOR_LD_POLLING_TIME_ACTIVE;
static bool ld_sensor_started = false;
static void *timer_handle_sensor_ld_polling = NULL;
static void *timer_handle_sensor_ld_start = NULL;
#endif

#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
#define SC7A20_AS_LS_Z_MIN   (app_cfg_const.gsensor_click_sensitivity * (-10)) //default: 21 ,unit: -10
#define SC7A20_AS_LS_Z_MAX   (app_cfg_const.gsensor_click_threshold * (10)) //default: 20 ,unit: 10

#define SC7A20_AS_LS_MAX_PAUSE_COUNT  2
#define SC7A20_AS_LS_MAX_WEAR_COUNT  2


#define SC7A20_AS_LS_POLLING_TIME     500 //ms

static bool sc7a20_as_ls_started = false;
static void *timer_handle_sc7a20_as_ls_polling = NULL;
static uint8_t sc7a20_as_ls_wear_state = SENSOR_LD_NONE;

static void sc7a20_as_ls_enable(void);
#endif

typedef enum
{
    APP_IO_TIMER_SENSOR_LD_POLLING,
    APP_IO_TIMER_SENSOR_LD_START,
    APP_IO_TIMER_SENSOR_LD_DEBOUNCE,
    APP_IO_TIMER_SENSOR_IN_EAR_FROM_CASE,
    APP_IO_TIMER_SENSOR_LD_IO_DEBOUNCE,
    APP_IO_TIMER_GSENSOR_CLICK_DETECT,
    APP_IO_TIMER_SC7A20_AS_LS_POLLING,
} T_APP_GPIO_SENSOR_TIMER;

static void app_gpio_sensor_timeout_cb(uint8_t timer_id, uint16_t timer_chann);

/*============================================================================*
 *                              Public Functions
 *============================================================================*/

/**
    * @brief  app sensor hw timer isr callback.
    *         Disable line in timer peripheral and clear line in timer interrupt mask.
    *         Read line in pinmux polarity and send IO_MSG_TYPE_GPIO message to app task.
    *         Clear line in GPIO interrupt mask and enable line in GPIO interrupt.
    * @param  void
    * @return void
    */
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
ISR_TEXT_SECTION void sensor_timer_isr_callback(T_HW_TIMER_HANDLE handle)
{
    if (app_cfg_const.sensor_support)
    {
        if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX)
        {
            switch (sensor_ld_data.ld_state)
            {
            case LD_STATE_CHECK_IN_EAR:
                {
                    uint8_t status;

                    //Read detect result twice to make sure stable
                    status = gpio_read_input_level(app_cfg_const.sensor_result_pinmux);
                    if (status == gpio_read_input_level(app_cfg_const.sensor_result_pinmux))
                    {
                        sensor_ld_data.ld_state = LD_STATE_FILTER_HIGH_LIGHT;
                        sensor_ld_data.cur_status = status;

                        //Pull high light detect pin to disable light detect
                        Pad_OutputControlValue(app_cfg_const.sensor_detect_pinmux, PAD_OUT_HIGH);
                        //Wait 150us to filter out high light interference
                        hw_timer_restart(sensor_hw_timer_handle, SENSOR_LD_DETECT_TIME);
                    }
                    else
                    {
                        //Detect result unstable: bypass and polling again
                        //Pull high light detect pin to disable light detect
                        Pad_OutputControlValue(app_cfg_const.sensor_detect_pinmux, PAD_OUT_HIGH);
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_SENSOR);
                    }
                }
                break;

            case LD_STATE_FILTER_HIGH_LIGHT:
                {
                    //If detect result is high (in-ear) when light detect is disabled,
                    //this is treated as high light interference and the detect result should be filter out.

                    uint8_t status;
                    uint8_t stable_count = SENSOR_LD_STABLE_COUNT;

                    //Read detect result twice to make sure stable
                    status = gpio_read_input_level(app_cfg_const.sensor_result_pinmux);
                    if (status == gpio_read_input_level(app_cfg_const.sensor_result_pinmux))
                    {
                        //simulate High light interference when in ear
                        //if (sensor_ld_data.pre_status == SENSOR_LD_IN_EAR) status = SENSOR_LD_IN_EAR;

                        /*
                          in ear  -> HH or LL -> out ear
                          out ear -> HL       -> in ear
                        */
                        if ((sensor_ld_data.pre_status == SENSOR_LD_IN_EAR &&
                             sensor_ld_data.cur_status == status) ||
                            (sensor_ld_data.pre_status == SENSOR_LD_OUT_EAR &&
                             sensor_ld_data.cur_status == SENSOR_LD_IN_EAR &&
                             status == SENSOR_LD_OUT_EAR))
                        {
                            sensor_ld_data.stable_count++;

                            //speed up check cycle
                            sendor_ld_hx_sw_cycle = SENSOR_LD_POLLING_SPEED_UP;

                            //out ear stable count
                            if (sensor_ld_data.pre_status == SENSOR_LD_IN_EAR)
                            {
                                stable_count = SENSOR_LD_STABLE_OUT_EAR_COUNT;
                            }
                        }
                        else if (sensor_ld_data.pre_status == SENSOR_LD_NONE)
                        {
                            sensor_ld_data.pre_status = SENSOR_LD_OUT_EAR;
                        }
                        else
                        {
                            sensor_ld_data.stable_count = 0;
                            sendor_ld_hx_sw_cycle = SENSOR_LD_POLLING_TIME_ACTIVE;
                        }

                        //Detect twice (600ms) to make sure sensor stable
                        if (sensor_ld_data.stable_count >= stable_count)
                        {
                            T_IO_MSG gpio_msg;

                            gpio_msg.type = IO_MSG_TYPE_GPIO;
                            gpio_msg.subtype = IO_MSG_GPIO_SENSOR_LD;
                            gpio_msg.u.param = !(sensor_ld_data.pre_status);

                            if (app_io_send_msg(&gpio_msg) == false)
                            {
                                APP_PRINT_ERROR0("sensor_timer_isr_callback: SENSOR_LD_TIMER Send msg error");
                            }
                            else
                            {
                                sensor_ld_data.stable_count = 0;
                                sensor_ld_data.pre_status = gpio_msg.u.param;
                            }
                            sendor_ld_hx_sw_cycle = SENSOR_LD_POLLING_TIME_ACTIVE;
                        }
                    }
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_SENSOR);
                }
                break;
            }
        }
    }
}
#endif

/**
    * @brief  Sensor Light detect GPIO initial.
    *         Include APB peripheral clock config, GPIO parameter config and
    *         GPIO interrupt mark config.
    * @param  void
    * @return void
    */
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
void app_sensor_ld_init(void)
{
    GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */

    gpio_periphclk_config(app_cfg_const.sensor_detect_pinmux, (FunctionalState)ENABLE);

    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(app_cfg_const.sensor_detect_pinmux);
    gpio_param.GPIO_Mode = GPIO_Mode_OUT;
    gpio_param_config(app_cfg_const.sensor_detect_pinmux, &gpio_param);

    gpio_periphclk_config(app_cfg_const.sensor_result_pinmux, (FunctionalState)ENABLE);

    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(app_cfg_const.sensor_result_pinmux);
    gpio_param_config(app_cfg_const.sensor_result_pinmux, &gpio_param);

    sensor_hw_timer_handle = hw_timer_create("app_sensor", SENSOR_LD_DETECT_TIME, false,
                                             sensor_timer_isr_callback);
    if (sensor_hw_timer_handle == NULL)
    {
        APP_PRINT_ERROR0("Could not create sensor timer, check hw timer usage");
    }

    memset(&sensor_ld_data, 0, sizeof(T_SENSOR_LD_DATA));
}
#endif

void app_io_sensor_ld_init(void)
{
    APP_PRINT_TRACE1("app_io_sensor_ld_init: sensor_detect_pinmux 0x%x",
                     app_cfg_const.sensor_detect_pinmux);
    GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */

    gpio_periphclk_config(app_cfg_const.sensor_detect_pinmux, (FunctionalState)ENABLE);

    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(app_cfg_const.sensor_detect_pinmux);
    gpio_param.GPIO_ITCmd = ENABLE;
    gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    gpio_param.GPIO_DebounceTime = GPIO_DETECT_DEBOUNCE_TIME;

    gpio_param_config(app_cfg_const.sensor_detect_pinmux, &gpio_param);
    gpio_init_irq(GPIO_GetNum(app_cfg_const.sensor_detect_pinmux));
}

static uint8_t app_ld_sensor_io_status(void)
{
    uint8_t in_ear_status;
    uint8_t pin_status = gpio_read_input_level(app_cfg_const.sensor_detect_pinmux);

    if (app_cfg_const.iosensor_active)
    {
        in_ear_status = pin_status ? 1 : 0;
    }
    else
    {
        in_ear_status = pin_status ? 0 : 1;
    }
    return in_ear_status;
}

static bool app_sensor_is_play_in_ear_tone(void)
{
    bool play_in_ear_tone = false;

#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        play_in_ear_tone = true;
    }
    else
    {
        if (app_cfg_const.play_in_ear_tone_when_any_bud_in_ear)
        {
            play_in_ear_tone = true;
        }
        else
        {
            if (app_db.remote_loc != BUD_LOC_IN_EAR)
            {
                play_in_ear_tone = true;
            }
        }
    }
#endif

    return play_in_ear_tone;
}

/**
    * @brief  Sensor Light detect state reset and start detect.
    * @param  void
    * @return void
    */
static void sensor_ld_reset(void)
{
    if (app_cfg_const.sensor_support) //sensor_support
    {
        if (0)
        {
            /* for feature define; do nothing */
        }
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX) //sensor_vendor
        {
            sensor_ld_data.ld_state = LD_STATE_POLLING;

#if 0 //TODO: Sniff mode uses different detect interval
            //TODO:check if enter_sniff_mode is ture
            if (enter_sniff_mode == true)
            {
                gap_start_timer(&timer_handle_sensor_ld_polling, "sensor_ld_polling_sniff",
                                app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SENSOR_LD_POLLING,
                                0, SENSOR_LD_POLLING_TIME_SNIFF);
            }
            else
#endif
            {
                gap_start_timer(&timer_handle_sensor_ld_polling, "sensor_ld_polling_active",
                                app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SENSOR_LD_POLLING,
                                0, sendor_ld_hx_sw_cycle);
            }
        }
#endif
#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
        else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_SC7A20)
        {
            gap_start_timer(&timer_handle_sc7a20_as_ls_polling, "sc7a20_as_ls_polling_active",
                            app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SC7A20_AS_LS_POLLING,
                            0, SC7A20_AS_LS_POLLING_TIME);
        }
#endif
    }
}

/**
    * @brief  Sensor Light detect stop.
    * @param  void
    * @return void
    */
void sensor_ld_stop(void)
{
    APP_PRINT_TRACE1("sensor_ld_stop: is_light_sensor_enabled %d", is_light_sensor_enabled);

    if (!is_light_sensor_enabled)
    {
        /* already disabled */
        return;
    }

    is_light_sensor_enabled = false;

    if (0)
    {
        /* for feature define; do nothing */
    }
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX)
    {
        ld_sensor_started = false;
        gap_stop_timer(&timer_handle_sensor_ld_start);
        gap_stop_timer(&timer_handle_sensor_ld_polling);
        memset(&sensor_ld_data, 0, sizeof(T_SENSOR_LD_DATA));
        //Pull high light detect pin to disable light detect
        Pad_OutputControlValue(app_cfg_const.sensor_detect_pinmux, PAD_OUT_HIGH);
        hw_timer_stop(sensor_hw_timer_handle);
    }
#endif
#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_SC7A20)
    {
        sc7a20_as_ls_started = false;
        gap_stop_timer(&timer_handle_sc7a20_as_ls_polling);
        gsensor_vendor_sl_disable();
    }
#endif
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1) || (F_APP_SENSOR_JSA1227_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225 ||
             app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
    {
        app_sensor_jsa_disable();
    }
#endif
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IO)
    {
        gpio_int_config(app_cfg_const.sensor_detect_pinmux, DISABLE);
        gpio_mask_int_config(app_cfg_const.sensor_detect_pinmux, ENABLE);
        gpio_clear_int_pending(app_cfg_const.sensor_detect_pinmux);
    }
#if (F_APP_SENSOR_CAP_TOUCH_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_CAP_TOUCH)
    {
        app_cap_touch_set_ld_det(false);
    }
#endif
#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)
    {
        app_sensor_px318j_disable();
    }
#endif
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IQS873)
    {
        app_sensor_iqs873_disable();
    }
#endif
}

static void app_sensor_bud_loc_detect_sync(void)
{
    uint8_t local_loc_pre = app_db.local_loc;
    app_db.local_loc = app_sensor_bud_loc_detect();
    if (local_loc_pre != app_db.local_loc)
    {
        app_sensor_bud_loc_sync();
    }

    APP_PRINT_TRACE2("app_sensor_bud_loc_detect_sync: local_loc_pre %d local_loc %d ", local_loc_pre,
                     app_db.local_loc);
}

/**
    * @brief  Sensor Light detect start.
    * @param  void
    * @return void
    */
void sensor_ld_start(void)
{
    APP_PRINT_TRACE1("sensor_ld_start: is_light_sensor_enabled %d", is_light_sensor_enabled);

    if (is_light_sensor_enabled)
    {
        /* already enabled */
        return;
    }

    is_light_sensor_enabled = true;

    app_bud_loc_cause_action_flag_set(false);
    ld_sensor_after_reset = false;

    if (0)
    {
        /* for feature define; do nothing */
    }
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX)
    {
        ld_sensor_started = true;
        sensor_ld_data.pre_status = SENSOR_LD_NONE;//send for the first time
        gap_start_timer(&timer_handle_sensor_ld_start, "sensor_ld_start",
                        app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SENSOR_LD_START,
                        0, 500);
    }
#endif
#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_SC7A20)
    {
        sc7a20_as_ls_started = true;
        sc7a20_as_ls_wear_state = SENSOR_LD_NONE;
        sc7a20_as_ls_enable();
        gap_start_timer(&timer_handle_sc7a20_as_ls_polling, "sc7a20_as_ls_polling_active",
                        app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SC7A20_AS_LS_POLLING,
                        0, SC7A20_AS_LS_POLLING_TIME);
    }
#endif
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1) || (F_APP_SENSOR_JSA1227_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225 ||
             app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
    {
        app_sensor_jsa_enable();
    }
#endif
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IO)
    {
        //first detect in out ear
        uint8_t status = app_ld_sensor_io_status();
        app_db.local_in_ear = (status == SENSOR_LD_IN_EAR) ? true : false;

        app_sensor_bud_loc_detect_sync();

        uint8_t gpio_level = gpio_read_input_level(app_cfg_const.sensor_detect_pinmux);

        if (gpio_level)
        {
            gpio_intpolarity_config(app_cfg_const.sensor_detect_pinmux,
                                    GPIO_INT_POLARITY_ACTIVE_LOW);
        }
        else
        {
            gpio_intpolarity_config(app_cfg_const.sensor_detect_pinmux,
                                    GPIO_INT_POLARITY_ACTIVE_HIGH);
        }

        //enable int
        gpio_mask_int_config(app_cfg_const.sensor_detect_pinmux, DISABLE);
        gpio_int_config(app_cfg_const.sensor_detect_pinmux, ENABLE);
    }
#if (F_APP_SENSOR_CAP_TOUCH_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_CAP_TOUCH)
    {
        app_cap_touch_set_ld_det(true);
    }
#endif
#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)
    {
        app_sensor_px318j_enable();
    }
#endif
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_IQS873)
    {
        app_sensor_iqs873_enable();
    }
#endif
}

void app_sensor_ld_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    if (!app_cfg_nv.light_sensor_enable)
    {
        return;
    }

    uint8_t status = io_driver_msg_recv->u.param;

#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
    if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX)
    {
        app_gpio_sensor_timeout_cb(APP_IO_TIMER_SENSOR_LD_DEBOUNCE, status);
    }
    else
#endif
    {
        gap_stop_timer(&timer_handle_sensor_ld_debounce);
        gap_start_timer(&timer_handle_sensor_ld_debounce, "sensor_ld_detect debounce",
                        app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SENSOR_LD_DEBOUNCE, status, 500);
    }
}

void app_sensor_ld_io_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    if (!app_cfg_nv.light_sensor_enable)
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        return;
    }

    gap_start_timer(&timer_handle_sensor_ld_io_debounce, "sensor_ld_io_detect debounce",
                    app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SENSOR_LD_IO_DEBOUNCE,
                    0, 500);
}

/**
    * @brief  IOSensor Detect GPIO interrupt will be handle in this function.
    *         First disable app enter dlps mode and read current GPIO input data bit.
    *         Disable GPIO interrupt and send IO_GPIO_MSG_TYPE message to app task.
    *         Then enable GPIO interrupt.
    * @param  void
    * @return void
    */
ISR_TEXT_SECTION void sensor_ld_io_int_gpio_intr_handler(void)
{
    T_IO_MSG gpio_msg;
    uint8_t pinmux = app_cfg_const.sensor_detect_pinmux;
    uint8_t gpio_level = gpio_read_input_level(pinmux);
    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    app_dlps_pad_wake_up_polarity_invert(pinmux);

    /* Disable GPIO interrupt */
    gpio_int_config(pinmux, DISABLE);
    gpio_mask_int_config(pinmux, ENABLE);
    gpio_clear_int_pending(pinmux);
    /* Change GPIO Interrupt Polarity */
    if (gpio_level)
    {
        gpio_intpolarity_config(pinmux, GPIO_INT_POLARITY_ACTIVE_LOW); //Polarity Low
    }
    else
    {
        gpio_intpolarity_config(pinmux, GPIO_INT_POLARITY_ACTIVE_HIGH); //Polarity High
    }

    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_SENSOR_LD_IO_DETECT;

    if (app_io_send_msg(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("sensor_ld_io_int_gpio_intr_handler: send msg err");
    }

    /* Enable GPIO interrupt */
    gpio_mask_int_config(pinmux, DISABLE);
    gpio_int_config(pinmux, ENABLE);
}

/**
    * @brief  gsensor I2C init.
    *         Initialize I2C peripheral
    * @param  addr: gsensor i2c slave adress
    * @return void
    */
void sensor_i2c_init(uint8_t addr)
{
    I2C_InitTypeDef i2c_param;

    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);

    I2C_StructInit(&i2c_param);

#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
    if (app_cfg_const.psensor_support &&
        ((app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS773) ||
         (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS873)) &&
        addr == I2C_AZQ_ADDR)
    {
        i2c_param.I2C_ClockSpeed = 200000;
    }
#endif

#if (F_APP_SENSOR_JSA1227_SUPPORT == 1)
    if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227 &&
        addr == SENSOR_ADDR_JSA1227)
    {
        i2c_param.I2C_ClockSpeed = 100000;
    }
#endif

#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
    if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX &&
        addr == PX318J_ID)
    {
        i2c_param.I2C_ClockSpeed = 100000;
    }
#endif

    i2c_param.I2C_DeviveMode = I2C_DeviveMode_Master;
    i2c_param.I2C_AddressMode = I2C_AddressMode_7BIT;
    i2c_param.I2C_SlaveAddress = addr;
    i2c_param.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C0, &i2c_param);
    I2C_Cmd(I2C0, ENABLE);
}

/**
    * @brief  Gsensor INT GPIO initial.
    *         Include APB peripheral clock config, GPIO parameter config and
    *         GPIO interrupt mark config. Enable GPIO interrupt.gg
    * @param  void
    * @return void
    */
void sensor_int_gpio_init(uint8_t pinmux)
{
    GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */
    gpio_periphclk_config(pinmux, (FunctionalState)ENABLE);

    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(pinmux);
    gpio_param.GPIO_ITCmd = ENABLE;
    gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_DISABLE;
    gpio_param_config(pinmux, &gpio_param);

    /* Enable Interrupt (Peripheral, CPU NVIC) */
    {
        gpio_mask_int_config(pinmux, DISABLE);
        gpio_init_irq(GPIO_GetNum(pinmux));
        gpio_int_config(pinmux, ENABLE);
    }
}

/**
    * @brief  GPIO interrupt will be handle in this function.
    *         First disable app enter dlps mode and read current GPIO input data bit.
    *         Disable GPIO interrupt and send IO_GPIO_MSG_TYPE message to app task.
    *         Then enable GPIO interrupt.
    * @param  void
    * @return void
    */
ISR_TEXT_SECTION void app_sensor_sl_int_gpio_intr_handler(void)
{
    uint8_t int_status;
    T_IO_MSG gpio_msg;

    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    int_status = gpio_read_input_level(app_cfg_const.gsensor_int_pinmux);

    /* Disable GPIO interrupt */
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);

    app_dlps_pad_wake_up_polarity_invert(app_cfg_const.gsensor_int_pinmux);

    /* Change GPIO Interrupt Polarity */
    if (int_status == GSENSOR_INT_RELEASED)
    {
        gpio_intpolarity_config(app_cfg_const.gsensor_int_pinmux,
                                GPIO_INT_POLARITY_ACTIVE_LOW); //Polarity Low
        gpio_msg.u.param = 1;

        if ((gsensor_sl.click_status == 0) &&
            (timer_handle_gsensor_click_detect == NULL))
        {
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
    }
    else
    {
        gpio_intpolarity_config(app_cfg_const.gsensor_int_pinmux,
                                GPIO_INT_POLARITY_ACTIVE_HIGH); //Polarity High
        gpio_msg.u.param = 0;

        //Only handle click press (high -> low), not handle release
        gpio_msg.type = IO_MSG_TYPE_GPIO;
        gpio_msg.subtype = IO_MSG_GPIO_GSENSOR;

        if (app_io_send_msg(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("gsensor_int_gpio_intr_handler: Send msg error");
        }
    }

    /* Enable GPIO interrupt */
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
}

void app_sensor_sl_mfb_intr_handler(void)
{
    T_IO_MSG gpio_msg;
    uint8_t key_status;

    key_status = mfb_get_level();

    /* Change GPIO Interrupt Polarity */
    if (key_status == GSENSOR_INT_RELEASED)
    {
        if ((gsensor_sl.click_status == 0) &&
            (timer_handle_gsensor_click_detect == NULL))
        {
            app_dlps_enable(APP_DLPS_ENTER_CHECK_MFB_KEY);
        }
    }
    else
    {
        //Only handle click press (high -> low), not handle release
        gpio_msg.type = IO_MSG_TYPE_GPIO;
        gpio_msg.subtype = IO_MSG_GPIO_GSENSOR;

        if (app_io_send_msg(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("sensor_int_gpio_intr_handler_click: Send msg error");
        }
    }

    mfb_irq_enable();
    return;
}

ISR_TEXT_SECTION I2C_Status app_sensor_i2c_read(uint8_t slave_addr, uint8_t reg_addr,
                                                uint8_t *p_read_buf,
                                                uint16_t read_len)
{
    I2C_Status status;

    if (slave_addr != I2C0->IC_TAR)
    {
        I2C_Cmd(I2C0, DISABLE);
        I2C_SetSlaveAddress(I2C0, slave_addr);
        I2C_Cmd(I2C0, ENABLE);
    }

    status = I2C_RepeatRead(I2C0, &reg_addr, 1, p_read_buf, read_len);

    return status;
}

ISR_TEXT_SECTION I2C_Status app_sensor_i2c_read_8(uint8_t slave_addr, uint8_t reg_addr,
                                                  uint8_t *read_buf)
{
    return app_sensor_i2c_read(slave_addr, reg_addr, read_buf, 1);
}

I2C_Status app_sensor_i2c_read_16(uint8_t slave_addr, uint8_t reg_addr, uint8_t *read_buf)
{
    return app_sensor_i2c_read(slave_addr, reg_addr, read_buf, 2);
}

I2C_Status app_sensor_i2c_read_32(uint8_t slave_addr, uint8_t reg_addr, uint8_t *read_buf)
{
    return app_sensor_i2c_read(slave_addr, reg_addr, read_buf, 4);
}

ISR_TEXT_SECTION I2C_Status app_sensor_i2c_write(uint8_t slave_addr, uint8_t *write_buf,
                                                 uint16_t write_len)
{
    I2C_Status status;

    if (slave_addr != I2C0->IC_TAR)
    {
        I2C_Cmd(I2C0, DISABLE);
        I2C_SetSlaveAddress(I2C0, slave_addr);
        I2C_Cmd(I2C0, ENABLE);
    }

    status = I2C_MasterWrite(I2C0, write_buf, write_len);

    return status;
}

ISR_TEXT_SECTION I2C_Status app_sensor_i2c_write_8(uint8_t slave_addr, uint8_t reg_addr,
                                                   uint8_t reg_value)
{
    uint8_t write_buf[2];
    I2C_Status status;

    write_buf[0] = reg_addr;
    write_buf[1] = reg_value;

    status = app_sensor_i2c_write(slave_addr, write_buf, 2);

    return status;
}

I2C_Status app_sensor_i2c_write_16(uint8_t slave_addr, uint8_t reg_addr, uint16_t reg_value)
{
    uint8_t write_buf[3];
    I2C_Status status;

    write_buf[0] = reg_addr;
    write_buf[1] = reg_value & 0x00FF;
    write_buf[2] = reg_value >> 8;

    status = app_sensor_i2c_write(slave_addr, write_buf, 3);

    return status;
}

I2C_Status app_sensor_i2c_write_32(uint8_t slave_addr, uint8_t reg_addr, uint32_t reg_value)
{
    uint8_t write_buf[5];
    I2C_Status status;

    write_buf[0] = reg_addr;
    write_buf[1] = reg_value & 0x00FF;
    write_buf[2] = (reg_value & 0xFF00) >> 8;
    write_buf[3] = (reg_value & 0xFF0000) >> 16;
    write_buf[4] = (reg_value & 0xFF000000) >> 24;

    status = app_sensor_i2c_write(slave_addr, write_buf, 5);

    return status;
}

bool app_sensor_is_i2c_device_fail(void)
{
    return i2c_device_fail;
}

void app_sensor_set_i2c_device_fail(bool status)
{
    i2c_device_fail = status;
}

#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
static void app_sensor_sent_in_out_ear_event(uint8_t event)
{
    if (event != SENSOR_LD_NONE)
    {
        T_IO_MSG gpio_msg;   //in-out ear

        gpio_msg.type = IO_MSG_TYPE_GPIO;
        gpio_msg.subtype = IO_MSG_GPIO_SENSOR_LD;
        gpio_msg.u.param = event;

        if (app_io_send_msg(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("app_sensor_sent_in_out_ear_event: Send msg error");
        }
    }

    APP_PRINT_TRACE1("app_sensor_sent_in_out_ear_event: wear state %d", event);
}

static void sc7a20_as_ls_enable(void)
{
    if (app_cfg_const.sensor_support)
    {
        //clock reinit firstly
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1,
                               0x97); //config to 1.25KHz first
        //clock set
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1,
                               0x27); //Clock: 10Hz = 100ms unit
    }
}

void app_sensor_sc7a20_as_ls_init(void)
{
    uint8_t status = 0;

    app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_WHO_AM_I, &status);
    if (status == 0x11) //sensor exist
    {
        sc7a20_as_ls_enable();
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG4,
                               0x08); //2G
    }
    else
    {
        app_sensor_set_i2c_device_fail(true);
    }

    APP_PRINT_TRACE1("app_sensor_sc7a20_as_ls_init: status 0x%02x", status);
}

static bool sc7a20_as_ls_drv_read_xyz_value(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t data[6], st = 0;

    app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x27, &st);
    if ((st & 0x0f) == 0x0f)
    {
        app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x28, &data[0]);
        app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x29, &data[1]);
        app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x2a, &data[2]);
        app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x2b, &data[3]);
        app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x2c, &data[4]);
        app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, 0x2d, &data[5]);

        *x = (int16_t)(data[1] << 8 | data[0]);
        *y = (int16_t)(data[3] << 8 | data[2]);
        *z = (int16_t)(data[5] << 8 | data[4]);

        *x >>= 6;
        *y >>= 6;
        *z >>= 6;

        return 1;
    }

    return 0;
}

static uint8_t sc7a20_as_ls_is_freedots_unwear(void)
{
    int16_t x, y, z;
    bool r;
    static uint8_t  pause_count = 0, wear_count = 0;

    r = sc7a20_as_ls_drv_read_xyz_value(&x, &y, &z);
    if (r)
    {
        if ((z < SC7A20_AS_LS_Z_MIN) || (z > SC7A20_AS_LS_Z_MAX))
        {
            if (pause_count <= SC7A20_AS_LS_MAX_PAUSE_COUNT)
            {
                pause_count++;
            }
            wear_count = 0;
        }
        else
        {
            pause_count = 0;
            if (wear_count <= SC7A20_AS_LS_MAX_WEAR_COUNT)
            {
                wear_count++;
            }
        }

        if (pause_count > SC7A20_AS_LS_MAX_PAUSE_COUNT)
        {
            if (sc7a20_as_ls_wear_state == SENSOR_LD_IN_EAR || sc7a20_as_ls_wear_state == SENSOR_LD_NONE)
            {
                sc7a20_as_ls_wear_state = SENSOR_LD_OUT_EAR;
                app_sensor_sent_in_out_ear_event(SENSOR_LD_OUT_EAR);
            }
        }
        else if (wear_count > SC7A20_AS_LS_MAX_WEAR_COUNT)
        {
            if (sc7a20_as_ls_wear_state == SENSOR_LD_OUT_EAR || sc7a20_as_ls_wear_state == SENSOR_LD_NONE)
            {
                sc7a20_as_ls_wear_state = SENSOR_LD_IN_EAR;
                app_sensor_sent_in_out_ear_event(SENSOR_LD_IN_EAR);
            }
        }
    }

    app_dlps_enable(APP_DLPS_ENTER_CHECK_SENSOR);

    return sc7a20_as_ls_wear_state;
}
#endif

#if (F_APP_SENSOR_SL7A20_SUPPORT == 1) || (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
/**
    * @brief  Gsensor vendor function: disable detect.
    * @param  void
    * @return void
    */
void gsensor_vendor_sl_disable(void)
{
    APP_PRINT_TRACE0("gsensor_vendor_sl_disable");
    /* Disable ODR clock */
    app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1, 0x0F);
}
#endif

#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
/**
    * @brief  Gsensor vendor function: init procedure.
    * @param  void
    * @return void
    */
void gsensor_vendor_sl_init(void)
{
    uint8_t status = 0;

    app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_WHO_AM_I, &status);
    if (status == 0x11) //sensor exist
    {
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1,
                               0x0F); //Clock: 400Hz = 2.5ms unit
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG2, 0x0C);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG3,
                               0x80); //Enable INT1
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG4, 0x90);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG5, 0x40);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_FIFO_CFG, 0x80);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CLICK_CTRL, 0x15);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CLICK_THS,
                               app_cfg_const.gsensor_click_sensitivity);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_TIME_LIMIT, 0x05);
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_TIME_LATENCY,
                               0x0C); //INT duration. unit: 2.5ms
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG6,
                               0x02); //INT low active
    }
    else
    {
        APP_PRINT_ERROR0("gsensor_vendor_sl_init: gsensor not exist");
    }
}

/**
    * @brief  Gsensor vendor function: enable detect.
    * @param  void
    * @return void
    */
static void gsensor_vendor_sl_enable(void)
{
    APP_PRINT_TRACE0("gsensor_vendor_sl_enable");

    if (app_cfg_const.gsensor_support)
    {
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1,
                               0x97); //config to 1.25KHz first
#if 0
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1,
                               0x77); //Clock: 400Hz = 2.5ms unit
#endif
        app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG1,
                               0x67); //Clock: 200Hz = 5ms unit
    }
}

static int32_t gsensor_vendor_sl_click_sqrt(uint32_t sqrt_data)
{
    uint32_t sqrt_low, sqrt_up, sqrt_mid;
    uint8_t sqrt_num = 0;

    sqrt_low = 0;
    sqrt_up = sqrt_data;
    sqrt_mid = (sqrt_up + sqrt_low) / 2;

    while (sqrt_num < 200)
    {
        if ((sqrt_mid * sqrt_mid) > sqrt_data)
        {
            sqrt_up = sqrt_mid;
        }
        else
        {
            sqrt_low = sqrt_mid;
        }

        if ((sqrt_up - sqrt_low) == 1)
        {
            if (((sqrt_up * sqrt_up) - sqrt_data) > (sqrt_data - (sqrt_low * sqrt_low)))
            {
                return sqrt_low;
            }
            else
            {
                return sqrt_up;
            }
        }

        sqrt_mid = (sqrt_up + sqrt_low) / 2;
        sqrt_num++;
    }

    return 0;
}

/**
    * @brief  Gsensor vendor function.
    * @param  th1: click threshold
    * @param  th2: noise threshold
    * @return true: click detect, false: click not detect
    */
static bool gsensor_vendor_sl_click_read(uint8_t th1, uint8_t th2)
{
    uint8_t i, j, k;
    uint8_t fifo_len;
    uint32_t fifo_data_tmp;
    uint32_t fifo_data_xyz[32];
    uint8_t click_result = 0;
    uint8_t click_num = 0;
    uint32_t click_sum = 0;
    uint8_t data_tmp[6];
    int8_t data[6];

    app_sensor_i2c_read_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_FIFO_SRC, &fifo_len);

    if ((fifo_len & 0x40) == 0x40)
    {
        fifo_len = 32;
    }
    else
    {
        fifo_len &= 0x1F;
    }

    for (i = 0; i < fifo_len; i++)
    {
        app_sensor_i2c_read(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_FIFO_DATA, data_tmp, 6);

        data[1] = (int8_t)data_tmp[1];
        data[3] = (int8_t)data_tmp[3];
        data[5] = (int8_t)data_tmp[5];

        fifo_data_tmp = (data[1] * data[1]) + (data[3] * data[3]) + (data[5] * data[5]);
        fifo_data_tmp = gsensor_vendor_sl_click_sqrt(fifo_data_tmp);
        fifo_data_xyz[i] = fifo_data_tmp;
    }

    k = 0;
    for (i = 1; i < fifo_len - 1; i++)
    {
        if ((fifo_data_xyz[i + 1] > th1) && (fifo_data_xyz[i - 1] < 30))
        {
            if (click_num == 0)
            {
                click_sum = 0; //first peak
                for (j = 0; j < i - 1; j++)
                {
                    if (fifo_data_xyz[j] > fifo_data_xyz[j + 1])
                    {
                        click_sum += (fifo_data_xyz[j] - fifo_data_xyz[j + 1]);
                    }
                    else
                    {
                        click_sum += (fifo_data_xyz[j + 1] - fifo_data_xyz[j]);
                    }
                }

                if (click_sum > th2)
                {
                    gsensor_sl.pp_num++;
                    break;
                }

                k = i;
            }
            //NOT used currently(copy from sample code)
            /*
            else
            {
                k = i; //sencond peak
            }
            */
        }

        if (k != 0)
        {
            if ((fifo_data_xyz[i - 1] - fifo_data_xyz[i + 1]) > (th1 - 10))
            {
                if ((i - k) < 5)
                {
                    click_num = 1;
                    break;
                }
            }
        }
    }

    if (click_num == 1)
    {
        click_result = 1;
    }
    else
    {
        click_result = 0;
    }

    return click_result;
}

/**
    * @brief  Gsensor vendor function: Check if click success detected, executed in click INT
    * @param  void
    * @return true: click detect, false: click not detect
    */
bool app_gsensor_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    bool click_status;

    //Disable click INT1
    app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG3, 0x00);

    click_status = gsensor_vendor_sl_click_read(app_cfg_const.gsensor_click_threshold,
                                                app_cfg_const.gsensor_noise_threshold);

    if (click_status == true)
    {
        if (timer_handle_gsensor_click_detect == NULL)
        {
            gap_start_timer(&timer_handle_gsensor_click_detect, "gsensor_click_detect",
                            app_gpio_sensor_timer_queue_id, APP_IO_TIMER_GSENSOR_CLICK_DETECT,
                            0, GSENSOR_CLICK_DETECT_TIME);

            //clear click timer cnt value
            gsensor_sl.click_timer_cnt = 0;
            gsensor_sl.click_timer_total_cnt = 0;
            gsensor_sl.click_final_cnt = 0;
        }

        gsensor_sl.click_status = 1;
    }
    else
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    }

    //Enable click INT1
    app_sensor_i2c_write_8(GSENSOR_I2C_SLAVE_ADDR_SILAN, GSENSOR_VENDOR_REG_SL_CTRL_REG3, 0x80);
    return click_status;
}

/**
    * @brief  Gsensor vendor function.
    * @param  fun_flag: clear pp_num
    * @return pp_num
    */
static uint8_t gsensor_vendor_sl_get_click_pp_cnt(uint8_t fun_flag)
{
    if (fun_flag == 0)
    {
        gsensor_sl.pp_num = 0;
    }

    return gsensor_sl.pp_num;
}

/**
    * @brief  Gsensor vendor function: Calculate detected click count, excuted in click timer
    * @param  void
    * @return detected click count
    */
static uint8_t gsensor_vendor_sl_click_status(void)
{
    uint8_t click_pp_num = app_cfg_const.gsensor_click_pp_num;
    uint8_t click_max_num = app_cfg_const.gsensor_click_max_num;
    uint8_t click_e_cnt = 0;

    gsensor_sl.click_timer_cnt++;

    if ((gsensor_sl.click_timer_cnt < click_pp_num) &&
        (gsensor_sl.click_status == 1))
    {
        gsensor_sl.click_status = 0;
        gsensor_sl.click_timer_total_cnt += gsensor_sl.click_timer_cnt;
        gsensor_sl.click_timer_cnt = 0;
        gsensor_sl.click_final_cnt++;
    }

    click_e_cnt = gsensor_vendor_sl_get_click_pp_cnt(1);
    if ((((gsensor_sl.click_timer_cnt >= click_pp_num) ||
          (gsensor_sl.click_timer_total_cnt >= click_max_num)) && (click_e_cnt < 1)) ||
        ((gsensor_sl.click_timer_cnt >= click_pp_num) && (click_e_cnt > 0)))
    {
        app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        gap_stop_timer(&timer_handle_gsensor_click_detect);
        gsensor_sl.click_timer_cnt = 0;
        gsensor_sl.click_timer_total_cnt = 0;

        if (0)//(click_e_cnt > 0)
        {
            click_e_cnt = gsensor_vendor_sl_get_click_pp_cnt(0);
            return 0;
        }
        else
        {
            return gsensor_sl.click_final_cnt;
        }
    }
    else
    {
        gap_start_timer(&timer_handle_gsensor_click_detect, "gsensor_click_detect",
                        app_gpio_sensor_timer_queue_id, APP_IO_TIMER_GSENSOR_CLICK_DETECT,
                        0, GSENSOR_CLICK_DETECT_TIME);
    }

    return 0;
}

void app_gsensor_init(void)
{
    sensor_i2c_init(GSENSOR_I2C_SLAVE_ADDR_SILAN);

    if (!app_cfg_const.enable_mfb_pin_as_gsensor_interrupt_pin)
    {
        sensor_int_gpio_init(app_cfg_const.gsensor_int_pinmux);
    }

    gsensor_vendor_sl_init();
}
#endif

void app_int_psensor_init(void)
{
    GPIO_InitTypeDef gpio_param; /* Define GPIO parameter structure */

    gpio_periphclk_config(app_cfg_const.gsensor_int_pinmux, (FunctionalState)ENABLE);

    GPIO_StructInit(&gpio_param);
    gpio_param.GPIO_PinBit = GPIO_GetPin(app_cfg_const.gsensor_int_pinmux);
    gpio_param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    gpio_param.GPIO_ITCmd = ENABLE;
    gpio_param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    gpio_param.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_ENABLE;
    gpio_param.GPIO_DebounceTime = GPIO_DETECT_DEBOUNCE_TIME;

    gpio_param_config(app_cfg_const.gsensor_int_pinmux, &gpio_param);

    /* Enable Interrupt (Peripheral, CPU NVIC) */
    {
        gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
        gpio_init_irq(GPIO_GetNum(app_cfg_const.gsensor_int_pinmux));
        gpio_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    }
}

static void app_gpio_sensor_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_gpio_sensor_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
    case APP_IO_TIMER_SENSOR_LD_POLLING:
        {
            gap_stop_timer(&timer_handle_sensor_ld_polling);

            if (ld_sensor_started)
            {
                app_dlps_disable(APP_DLPS_ENTER_CHECK_SENSOR);
                sensor_ld_reset();
                sensor_ld_data.ld_state = LD_STATE_CHECK_IN_EAR;
                //Pull low light detect pin to enable light detect
                Pad_OutputControlValue(app_cfg_const.sensor_detect_pinmux, PAD_OUT_LOW);
                //Wait 150us for detect result stable
                hw_timer_start(sensor_hw_timer_handle);
            }
        }
        break;

    case APP_IO_TIMER_SENSOR_LD_START:
        {
            gap_stop_timer(&timer_handle_sensor_ld_start);
            sensor_ld_reset();
        }
        break;
#endif

    case APP_IO_TIMER_SENSOR_LD_DEBOUNCE:
        {
            uint8_t status = timer_chann;
            gap_stop_timer(&timer_handle_sensor_ld_debounce);

            app_db.local_in_ear = (status == SENSOR_LD_IN_EAR) ? true : false;

#if (F_APP_SENSOR_HX3001_SUPPORT == 1)
            if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_HX)
            {
                app_bud_loc_cause_action_flag_set(true);
            }
#endif
#if (F_APP_SENSOR_CAP_TOUCH_SUPPORT == 1)
            if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_CAP_TOUCH)
            {
                app_bud_loc_cause_action_flag_set(true);
            }
#endif
            {
                /*after system rst,the first ld position is not action cause*/
                if (ld_sensor_after_reset)
                {
                    app_bud_loc_cause_action_flag_set(true);
                }
                else
                {
                    app_bud_loc_cause_action_flag_set(false);
                    ld_sensor_after_reset = true;
                }
            }

            app_sensor_bud_loc_detect_sync();

            if ((app_db.local_loc == BUD_LOC_IN_EAR) && app_sensor_is_play_in_ear_tone())
            {
                if (app_cfg_const.play_in_ear_tone_regardless_of_phone_connection ||
                    (app_db.b2s_connected_num > 0))
                {
                    app_audio_tone_type_play(TONE_IN_EAR_DETECTION, false, false);
                }
            }

            APP_PRINT_TRACE1("app_gpio_sensor_timeout_cb: ld sensor loc %d", status);
        }
        break;

    case APP_IO_TIMER_SENSOR_LD_IO_DEBOUNCE:
        {
            uint8_t status = app_ld_sensor_io_status();
            gap_stop_timer(&timer_handle_sensor_ld_io_debounce);

            app_db.local_in_ear = (status == SENSOR_LD_IN_EAR) ? true : false;

            app_bud_loc_cause_action_flag_set(true);
            app_sensor_bud_loc_detect_sync();

            if ((app_db.local_loc == BUD_LOC_IN_EAR) && app_sensor_is_play_in_ear_tone())
            {
                if (app_cfg_const.play_in_ear_tone_regardless_of_phone_connection ||
                    (app_db.b2s_connected_num > 0))
                {
                    app_audio_tone_type_play(TONE_IN_EAR_DETECTION, false, false);
                }
            }

            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
        break;

    case APP_IO_TIMER_SENSOR_IN_EAR_FROM_CASE:
        {
            if (timer_handle_sensor_in_ear_from_case != NULL)
            {
                gap_stop_timer(&timer_handle_sensor_in_ear_from_case);
                if (loc_in_ear_from_case)
                {
                    if (app_sensor_is_play_in_ear_tone())
                    {
                        if (app_cfg_const.play_in_ear_tone_regardless_of_phone_connection ||
                            (app_db.b2s_connected_num > 0))
                        {
                            app_audio_tone_type_play(TONE_IN_EAR_DETECTION, false, false);
                        }
                    }
                    loc_in_ear_from_case = false;
                    app_sensor_bud_changed(APP_BUD_LOC_EVENT_SENSOR_IN_EAR, 0, true);
                }
            }
        }
        break;

#if (F_APP_SENSOR_SC7A20_AS_LS_SUPPORT == 1)
    case APP_IO_TIMER_SC7A20_AS_LS_POLLING:
        {
            gap_stop_timer(&timer_handle_sc7a20_as_ls_polling);

            if (sc7a20_as_ls_started)
            {
                app_dlps_disable(APP_DLPS_ENTER_CHECK_SENSOR);
                sensor_ld_reset();
                sc7a20_as_ls_is_freedots_unwear();
            }
        }
        break;
#endif

#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
    case APP_IO_TIMER_GSENSOR_CLICK_DETECT:
        {
            uint8_t click_cnt;

            click_cnt = gsensor_vendor_sl_click_status();
            APP_PRINT_TRACE1("app_gpio_sensor_timeout_cb: click_cnt %d", click_cnt);
            if (click_cnt == 1)
            {
                app_key_single_click(KEY0_MASK);
            }
            else if (click_cnt >= 2)
            {
                if (click_cnt >= 7)
                {
                    click_cnt = 7; //Max 7-click
                }
                app_key_hybrid_multi_clicks(KEY0_MASK, click_cnt);
            }
        }
        break;
#endif

    default:
        break;
    }
}

static void app_sensor_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            if (app_cfg_const.output_indication_support == 1 &&
                app_cfg_const.enable_output_power_supply == 1)
            {
                if (app_cfg_const.enable_output_ind1_high_active == true)
                {
                    if (app_cfg_const.sensor_support && app_cfg_const.chip_supply_power_to_light_sensor_digital_VDD)
                    {
                        Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
                                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
                        Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
                                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
                        Pad_PullConfigValue(app_cfg_const.i2c_0_dat_pinmux, PAD_STRONG_PULL);
                        Pad_PullConfigValue(app_cfg_const.i2c_0_clk_pinmux, PAD_STRONG_PULL);

                        /* must have delay at least 30 ms after power on reset before soft reboot */
                        platform_delay_ms(50);

#if (F_APP_SENSOR_PX318J_SUPPORT == 1)
                        if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_PX)
                        {
                            sensor_i2c_init(PX318J_ID);
                            app_sensor_px318j_init();
                            sensor_int_gpio_init(app_cfg_const.sensor_detect_pinmux);
                        }
#endif
#if (F_APP_SENSOR_JSA1225_SUPPORT == 1)
                        if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225)
                        {
                            sensor_i2c_init(SENSOR_ADDR_JSA);
                            app_sensor_jsa1225_init();
                            sensor_int_gpio_init(app_cfg_const.sensor_detect_pinmux);
                        }
#endif
#if (F_APP_SENSOR_JSA1227_SUPPORT == 1)
                        if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
                        {
                            sensor_i2c_init(SENSOR_ADDR_JSA1227);
                            app_sensor_jsa1227_init();
                            sensor_int_gpio_init(app_cfg_const.sensor_detect_pinmux);
                        }
#endif
                    }
                }
            }

            if (LIGHT_SENSOR_ENABLED)
            {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                sensor_ld_start();
#else
                app_sensor_control(app_cfg_nv.light_sensor_enable, false, true);
#endif
            }

#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
            if (app_cfg_const.gsensor_support)
            {
                gsensor_vendor_sl_enable();
            }
#endif
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            if (app_cfg_const.sensor_support)
            {
                sensor_ld_stop();

                app_dlps_enable(APP_DLPS_ENTER_CHECK_SENSOR_CALIB);
            }
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_sensor_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_sensor_init(void)
{
    sys_mgr_cback_register(app_sensor_dm_cback);
    gap_reg_timer_cb(app_gpio_sensor_timeout_cb, &app_gpio_sensor_timer_queue_id);
}

static void app_sensor_clear_sensor_related_data(void)
{
    app_db.detect_suspend_by_out_ear = false;
    app_db.in_ear_recover_a2dp = 0;
}

void app_sensor_control(uint8_t new_state, bool is_push_tone, bool is_during_power_on)
{
    if (app_cfg_const.sensor_support)
    {
        if ((app_cfg_nv.light_sensor_enable != new_state) || (is_during_power_on))
        {
            app_cfg_nv.light_sensor_enable = new_state;

            if (app_cfg_nv.light_sensor_enable) /* enable light sensor */
            {
                sensor_ld_start();
            }
            else/* disable light sensor */
            {
                sensor_ld_stop();
                app_sensor_clear_sensor_related_data();
            }

            if (is_push_tone)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    if (app_cfg_nv.light_sensor_enable)
                    {
                        app_audio_tone_type_play(TONE_LIGHT_SENSOR_ON, false, true);
                    }
                    else
                    {
                        app_audio_tone_type_play(TONE_LIGHT_SENSOR_OFF, false, true);
                    }
                    app_relay_async_single(APP_MODULE_TYPE_BUD_LOC, APP_BUD_LOC_EVENT_SENSOR_LD_CONFIG);
                }
            }
        }
    }
}
#endif

static void app_sensor_bud_changed(uint8_t event, bool from_remote, uint8_t para)
{
#if (F_APP_ERWS_SUPPORT == 1)
    app_bud_loc_evt_handle(event, from_remote, para);
#else
#if (F_APP_AVP_INIT_SUPPORT == 1)
    if (app_sensor_bud_loc_single_hook)
    {
        app_sensor_bud_loc_single_hook();
    }
#else
    if (event == APP_BUD_LOC_EVENT_SENSOR_IN_EAR)
    {
        app_audio_in_ear_handle();
    }
    else if (event == APP_BUD_LOC_EVENT_SENSOR_OUT_EAR)
    {
        app_audio_out_ear_handle();
    }
    app_db.local_loc_pre = app_db.local_loc;
    app_db.remote_loc_pre = app_db.remote_loc;
#endif

    if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE && app_cfg_const.enable_inbox_power_off)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }
#endif
}

void app_sensor_bud_loc_sync(void)
{
#if (F_APP_SENSOR_SUPPORT == 1)
    APP_PRINT_TRACE2("app_sensor_bud_loc_sync: ld_sensor_cause_action %d local_loc %d",
                     app_bud_loc_cause_action_flag_get(), app_db.local_loc);
#endif

    switch (app_db.local_loc)
    {
    case BUD_LOC_IN_CASE:
        {
            app_sensor_bud_changed(APP_BUD_LOC_EVENT_CASE_IN_CASE, 0, 0);
        }
        break;

    case BUD_LOC_IN_AIR:
        {
            {
#if (F_APP_SENSOR_SUPPORT == 1)
                if (loc_in_air_from_ear)
                {
                    app_sensor_bud_changed(APP_BUD_LOC_EVENT_SENSOR_OUT_EAR, 0, app_bud_loc_cause_action_flag_get());
                }
                else
#endif
                {
                    app_sensor_bud_changed(APP_BUD_LOC_EVENT_CASE_OUT_CASE, 0, 0);
                }
            }
        }
        break;

#if (F_APP_SENSOR_SUPPORT == 1)
    case BUD_LOC_IN_EAR:
        {
            if (loc_in_ear_from_case)
            {
                app_sensor_bud_changed(APP_BUD_LOC_EVENT_CASE_OUT_CASE, 0, 0);
                gap_start_timer(&timer_handle_sensor_in_ear_from_case, "in_ear_from_case",
                                app_gpio_sensor_timer_queue_id, APP_IO_TIMER_SENSOR_IN_EAR_FROM_CASE,
                                0, 500);
            }
            else
            {
                app_sensor_bud_changed(APP_BUD_LOC_EVENT_SENSOR_IN_EAR, 0, app_bud_loc_cause_action_flag_get());
            }
        }
        break;
#endif

    default:
        break;
    }

#if (F_APP_SENSOR_SUPPORT == 1)
    app_bud_loc_cause_action_flag_set(false);//clear action flag after sync
#endif
}

uint8_t app_sensor_bud_loc_detect(void)
{
    uint8_t local_loc;
#if (F_APP_SENSOR_SUPPORT == 1)
    uint8_t local_loc_pre = app_db.local_loc;
    loc_in_ear_from_case = false;
#endif

    if (app_db.local_in_case)
    {
        local_loc = BUD_LOC_IN_CASE;
    }
    else
    {
        if (app_db.local_in_ear)
        {
            local_loc = BUD_LOC_IN_EAR;
#if (F_APP_SENSOR_SUPPORT == 1)
            loc_in_ear_from_case = (local_loc_pre == BUD_LOC_IN_CASE) ? true : false;
#endif
        }
        else
        {
            local_loc = BUD_LOC_IN_AIR;
#if (F_APP_SENSOR_SUPPORT == 1)
            loc_in_air_from_ear = (local_loc_pre == BUD_LOC_IN_EAR) ? true : false;
#endif
        }
    }

#if (F_APP_SENSOR_SUPPORT == 1)
    gap_stop_timer(&timer_handle_sensor_in_ear_from_case);
    APP_PRINT_TRACE7("app_sensor_bud_loc_detect: local_loc %d remote_loc %d local_in_case %d local_in_ear %d ld_sensor_cause_action %d loc_in_air_from_ear %d loc_in_ear_from_case %d",
                     local_loc, app_db.remote_loc, app_db.local_in_case, app_db.local_in_ear,
                     app_bud_loc_cause_action_flag_get(),
                     loc_in_air_from_ear, loc_in_ear_from_case);
#else
    APP_PRINT_TRACE4("app_sensor_bud_loc_detect: local_loc %d remote_loc %d local_in_case %d local_in_ear %d",
                     local_loc, app_db.remote_loc, app_db.local_in_case, app_db.local_in_ear);
#endif

    return local_loc;
}

void app_sensor_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                           uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
#if (F_APP_SENSOR_PX318J_SUPPORT)
    case CMD_PX318J_CALIBRATION:
        {
            switch (cmd_ptr[2])
            {
            case PX_CALIBRATION_NOISE_FLOOR:
                {
                    uint8_t evt_data[6] = {0};

                    evt_data[0] = PX_CALIBRATION_NOISE_FLOOR;
                    evt_data[1] = app_sensor_px318j_auto_dac(&evt_data[2]);

                    if (evt_data[1] != PX318J_CAL_SUCCESS)
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_report_event(cmd_path, EVENT_PX318J_CALIBRATION_REPORT, app_idx, evt_data, 6);
                        break;
                    }

                    evt_data[1] |= app_sensor_px318j_read_ps_data_after_noise_floor_cal(&evt_data[4]);

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_PX318J_CALIBRATION_REPORT, app_idx, evt_data, sizeof(evt_data));
                }
                break;

            case PX_CALIBRATION_IN_EAR_THRESHOLD:
                {
                    uint8_t evt_data[4] = {0};

                    evt_data[0] = PX_CALIBRATION_IN_EAR_THRESHOLD;
                    evt_data[1] = app_sensor_px318j_threshold_cal(IN_EAR_THRESH, &evt_data[2]);

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_PX318J_CALIBRATION_REPORT, app_idx, evt_data, sizeof(evt_data));
                }
                break;

            case PX_CALIBRATION_OUT_EAR_THRESHOLD:
                {
                    uint8_t evt_data[4] = {0};

                    evt_data[0] = PX_CALIBRATION_OUT_EAR_THRESHOLD;
                    evt_data[1] = app_sensor_px318j_threshold_cal(OUT_EAR_THRESH, &evt_data[2]);

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_PX318J_CALIBRATION_REPORT, app_idx, evt_data, sizeof(evt_data));
                }
                break;

            case PX_PX318J_PARA:
                {
                    uint8_t evt_data[13] = {0};

                    evt_data[0] = PX_PX318J_PARA;

                    app_sensor_px318j_get_para(&evt_data[1]);
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_PX318J_CALIBRATION_REPORT, app_idx, evt_data, sizeof(evt_data));
                }
                break;

            case PX_PX318J_WRITE_PARA:
                {
                    uint8_t evt_data[2] = {0};

                    evt_data[0] = PX_PX318J_WRITE_PARA;
                    evt_data[1] = PX318J_CAL_SUCCESS;

                    app_sensor_px318j_write_data(cmd_ptr[3], &cmd_ptr[4]);

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_PX318J_CALIBRATION_REPORT, app_idx, evt_data, sizeof(evt_data));
                }
                break;
            }
        }
        break;
#endif

#if (F_APP_SENSOR_JSA1225_SUPPORT) || (F_APP_SENSOR_JSA1227_SUPPORT)
    case CMD_JSA_CALIBRATION:
        {
            /* disable dlps when doing JSA calibration */
            app_dlps_disable(APP_DLPS_ENTER_CHECK_SENSOR_CALIB);

            uint8_t evt_data[9];
            uint8_t evt_len = 0;

            switch (cmd_ptr[2])
            {
            case SENSOR_JSA_CAL_XTALK:
            case SENSOR_JSA_LOW_THRES:
            case SENSOR_JSA_HIGH_THRES:
            case SENSOR_JSA_READ_PS_DATA:
                {
                    uint16_t cal_result = 0;
                    T_JSA_CAL_RETURN result = JSA_CAL_SUCCESS;

                    if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1225)
                    {
                        result = app_sensor_jsa1225_calibration(cmd_ptr[2], &cal_result);
                    }
                    else if (app_cfg_const.sensor_vendor == SENSOR_LD_VENDOR_JSA1227)
                    {
                        result = app_sensor_jsa1227_calibration(cmd_ptr[2], &cal_result);
                    }

                    evt_data[0] = result;
                    evt_data[1] = cal_result & 0x00FF;
                    evt_data[2] = (cal_result & 0xFF00) >> 8;
                    evt_data[3] = (uint8_t)(((double)cal_result / SENSOR_VAL_JSA_PS_DATA_MAX) * 100);
                    evt_len = 4;
                }
                break;

            case SENSOR_JSA_WRITE_INT_TIME:
            case SENSOR_JSA_WRITE_PS_GAIN:
            case SENSOR_JSA_WRITE_PS_PULSE:
            case SENSOR_JSA_WRITE_LOW_THRES:
            case SENSOR_JSA_WRITE_HIGH_THRES:
                {
                    if (cmd_len - 2 == 2 || cmd_len - 2 == 3)
                    {
                        app_sensor_jsa_write_data(cmd_ptr[2], &cmd_ptr[3]);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    }
                }
                break;

            case SENSOR_JSA_READ_CAL_DATA:
                {
                    app_sensor_jsa_read_cal_data(evt_data);
                    evt_len = 9;
                }
                break;

            default:
                break;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE && evt_len > 0)
            {
                app_report_event(cmd_path, EVENT_JSA_CAL_RESULT, app_idx, evt_data, evt_len);
            }
        }
        break;
#endif

    default:
        break;
    }
}
bool app_sensor_ld_is_light_sensor_enabled(void)
{
#if (F_APP_SENSOR_SUPPORT == 1)
    return is_light_sensor_enabled;
#else
    return false;
#endif
}
