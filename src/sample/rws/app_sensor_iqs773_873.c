#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
#include "trace.h"
#include "board.h"
#include "app_key_process.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "section.h"
#include "app_gpio.h"
#include "platform_utils.h"
#include "app_io_msg.h"
#include "app_sensor_iqs773_873.h"
#include "app_vendor_iqs773_873.h"

/**
    * @brief  PSensor iqs773 Detect GPIO interrupt will be handle in this function.
    *         Disable GPIO interrupt and send IO_GPIO_MSG_TYPE message to app task.
    *         Then enable GPIO interrupt.
    * @param  void
    * @return void
    */
ISR_TEXT_SECTION void app_psensor_iqs773_873_intr_handler(void)
{
    uint8_t pinmux = app_cfg_const.gsensor_int_pinmux;
    T_IO_MSG io_msg;

    //Don't print log in ISR_TEXT_SECTION
    //APP_PRINT_TRACE0("psensor_intr_handler");
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    /* Disable GPIO interrupt */
    gpio_int_config(pinmux, DISABLE);
    gpio_mask_int_config(pinmux, ENABLE);
    gpio_clear_int_pending(pinmux);
    app_dlps_pad_wake_up_polarity_invert(pinmux);
    io_msg.type = IO_MSG_TYPE_GPIO;
    io_msg.subtype = IO_MSG_GPIO_PSENSOR;

    if (app_io_send_msg(&io_msg) == false)
    {
        APP_PRINT_ERROR0("psensor_intr_handler: error");
    }

    /* Enable GPIO interrupt */
    gpio_mask_int_config(pinmux, DISABLE);
    gpio_int_config(pinmux, ENABLE);
}

/**
    * @brief  check psensor iqs773 i2c event when receive io msg from interrupt
    * @param  void
    * @return void
    */
void app_psensor_iqs_key_handle(T_PSENSOR_I2C_EVENT event)
{
    T_IO_MSG button_msg; //key click

    APP_PRINT_TRACE2("psensor_check_i2c_event: %d 0x%02x", app_cfg_const.psensor_vendor,
                     event);

    if (event & PSENSOR_I2C_EVENT_PRESS)
    {
        //Psensor is treated as key0
        button_msg.u.param = (BIT0 << 8) | KEY_PRESS;
        app_key_handle_msg(&button_msg);
    }
    else if (event & PSENSOR_I2C_EVENT_RELEASE)
    {
        //Psensor is treated as key0
        button_msg.u.param = (BIT0 << 8) | KEY_RELEASE;
        app_key_handle_msg(&button_msg);
    }
}

void app_psensor_iqs773_check_i2c_event(void)
{
    T_PSENSOR_I2C_EVENT event;

    event = i2c_iqs773_read_event();

    app_psensor_iqs_key_handle(event);

    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
}

void app_psensor_iqs873_check_i2c_event(void)
{
    T_PSENSOR_I2C_EVENT event;
    T_IO_MSG gpio_msg;   //in-out ear

    // Disable Interrupt
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    platform_delay_us(300);
    event = i2c_iqs873_read_event();

    // Enable Interrupt
    platform_delay_us(300);
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);

    app_psensor_iqs_key_handle(event);

    gpio_msg.u.param = SENSOR_LD_NONE;

    if (event & PSENSOR_I2C_EVENT_IN_EAR)
    {
        //Psensor is treated as key0
        gpio_msg.type = IO_MSG_TYPE_GPIO;
        gpio_msg.subtype = IO_MSG_GPIO_SENSOR_LD;
        gpio_msg.u.param = SENSOR_LD_IN_EAR;
    }
    else if (event & PSENSOR_I2C_EVENT_OUT_EAR)
    {
        //Psensor is treated as key0
        gpio_msg.type = IO_MSG_TYPE_GPIO;
        gpio_msg.subtype = IO_MSG_GPIO_SENSOR_LD;
        gpio_msg.u.param = SENSOR_LD_OUT_EAR;
    }

    if (gpio_msg.u.param != SENSOR_LD_NONE)
    {
        if (app_io_send_msg(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("app_psensor_iqs873_check_i2c_event: Send msg error");
        }
    }

    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
}

void app_sensor_iqs873_enable(void)
{
    // Disable Interrupt
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    platform_delay_us(300);

    i2c_iqs873_wear_touch_thr(0x10);

    // Enable Interrupt
    platform_delay_us(300);
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
}

void app_sensor_iqs873_disable(void)
{
    // Disable Interrupt
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    platform_delay_us(300);

    i2c_iqs873_wear_touch_thr(0xFF);

    // Enable Interrupt
    platform_delay_us(300);
    gpio_clear_int_pending(app_cfg_const.gsensor_int_pinmux);
    gpio_int_config(app_cfg_const.gsensor_int_pinmux, ENABLE);
    gpio_mask_int_config(app_cfg_const.gsensor_int_pinmux, DISABLE);
}

#endif

