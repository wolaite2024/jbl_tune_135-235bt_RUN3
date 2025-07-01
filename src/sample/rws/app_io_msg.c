/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "os_sync.h"
#include "os_msg.h"
#include "trace.h"
#include "app_main.h"
#include "gap_timer.h"

#include "app_charger.h"
#include "app_gpio.h"
#include "app_key_gpio.h"
#include "app_key_process.h"
#include "app_ble_gap.h"
#include "app_cfg.h"
#include "app_transfer.h"
#include "app_console_msg.h"

#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
#include "app_sensor.h"
#if F_APP_NFC_SUPPORT
#include "app_nfc.h"
#endif
#include "app_adp.h"

#if GFPS_FEATURE_SUPPORT
#include "app_ecc.h"
#endif

#include "app_line_in.h"
#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
#include "app_sensor_iqs773_873.h"
#endif
#if F_APP_LOCAL_PLAYBACK_SUPPORT
#include "app_playback_update_file.h"
#endif
#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#endif
#if F_APP_ADC_SUPPORT
#include "app_adc.h"
#endif
#if BISTO_FEATURE_SUPPORT
#include "app_bisto_msg.h"
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif

#if (F_APP_QDECODE_SUPPORT == 1)
#include "app_qdec.h"
#endif

#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_device.h"
#endif

#if APP_CAP_TOUCH_SUPPORT
#include "app_cap_touch.h"
#endif

#if F_APP_SS_SUPPORT
#include "app_ss_cmd.h"
#endif
#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
#include "app_ext_charger.h"
#include "app_harman_adc.h"
#endif

#if HARMAN_USB_CONNECTOR_PROTECT
#include "app_harman_usb_connector_protect.h"
#endif

void (*app_rtk_charger_box_dat_msg_hook)(T_IO_MSG *) = NULL;


RAM_TEXT_SECTION bool  app_io_send_msg(T_IO_MSG *io_msg)
{
    T_EVENT_TYPE  event;
    bool ret = false;
    event = EVENT_IO_TO_APP;

    if (os_msg_send(audio_io_queue_handle, io_msg, 0) == true)
    {
        ret = os_msg_send(audio_evt_queue_handle, &event, 0);
    }

    return ret;
}



void app_io_handle_msg(T_IO_MSG io_driver_msg_recv)
{
    uint16_t msgtype = io_driver_msg_recv.type;

    switch (msgtype)
    {
    case IO_MSG_TYPE_GPIO:
        {
            switch (io_driver_msg_recv.subtype)
            {
            case IO_MSG_GPIO_KEY:
                app_key_handle_msg(&io_driver_msg_recv);
                break;

            case IO_MSG_GPIO_UART_WAKE_UP:
                app_transfer_handle_msg(&io_driver_msg_recv);
                break;

            case IO_MSG_GPIO_CHARGER:
                app_charger_handle_msg(&io_driver_msg_recv);
                break;

#if F_APP_LOCAL_PLAYBACK_SUPPORT
            case IO_MSG_GPIO_PLAYBACK_TRANS_FILE_ACK:
                app_playback_trans_write_file_ack_handle_msg(&io_driver_msg_recv);
                break;
#endif

            case IO_MSG_GPIO_ADAPTOR_PLUG:
                app_adp_plug_handle(&io_driver_msg_recv);
                break;

            case IO_MSG_GPIO_ADAPTOR_UNPLUG:
                app_adp_unplug_handle(&io_driver_msg_recv);

                break;
#if F_APP_GPIO_ONOFF_SUPPORT
            case IO_MSG_GPIO_CHARGERBOX_DETECT:
                if (app_cfg_const.box_detect_method == GPIO_DETECT)
                {
                    app_gpio_detect_onoff_handle_msg(&io_driver_msg_recv);
                }
                break;
#endif

#if F_APP_NFC_SUPPORT
            case IO_MSG_GPIO_NFC:
                app_gpio_nfc_handle_msg(&io_driver_msg_recv);
                break;
#endif

#if F_APP_LOSS_BATTERY_PROTECT
            case IO_MSG_GPIO_LOSS_BATTERY_IO_DETECT:
                {
                    app_device_loss_battery_gpio_detect_handle_msg(&io_driver_msg_recv);
                }
                break;
#endif

#if (F_APP_SENSOR_SUPPORT == 1)
            case IO_MSG_GPIO_SENSOR_LD:
                {
                    app_sensor_ld_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_SENSOR_LD_IO_DETECT:
                {
                    app_sensor_ld_io_handle_msg(&io_driver_msg_recv);
                }
                break;

#if (F_APP_SENSOR_SL7A20_SUPPORT == 1)
            case IO_MSG_GPIO_GSENSOR:
                app_gsensor_handle_msg(&io_driver_msg_recv);
                break;
#endif

#if (F_APP_SENSOR_IQS773_873_SUPPORT == 1)
            case IO_MSG_GPIO_PSENSOR:
                {
                    if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS773)
                    {
                        app_psensor_iqs773_check_i2c_event();
                    }
                    else if (app_cfg_const.psensor_vendor == PSENSOR_VENDOR_IQS873)
                    {
                        app_psensor_iqs873_check_i2c_event();
                    }
                }
                break;
#endif
#endif

            case IO_MSG_GPIO_ADAPTOR_DAT:
                {
                    app_adp_delay_charger_enable();
                    app_adp_handle_msg(&io_driver_msg_recv);
                }
                break;


#if F_APP_LINEIN_SUPPORT
            case IO_MSG_GPIO_LINE_IN:
                app_line_in_detect_msg_handler(&io_driver_msg_recv);
                break;
#endif

#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
            case IO_MSG_GPIO_SLIDE_SWITCH_0:
            case IO_MSG_GPIO_SLIDE_SWITCH_1:
                app_slide_switch_handle_msg(&io_driver_msg_recv);
                break;
#endif

            case IO_MSG_GPIO_KEY0:
                {
#if HARMAN_HW_TIMER_REPLACE_OS_TIMER
#else
                    gap_start_timer(&timer_handle_key0_debounce, "key0_debounce",
                                    key_gpio_timer_queue_id,
                                    APP_IO_TIMER_KEY0_DEBOUNCE, 0, GPIO_DETECT_DEBOUNCE_TIME);
#endif
                }
                break;

#if (F_APP_QDECODE_SUPPORT == 1)
            case IO_MSG_GPIO_QDEC:
                {
                    app_qdec_msg_handler(&io_driver_msg_recv);
                }
                break;
#endif

            case IO_MSG_GPIO_SMARTBOX_COMMAND_PROTECT:
                {
                    app_adp_cmd_protect();
                }
                break;

            case IO_MSG_GPIO_ADP_INT:
                {
                    app_adp_int_handle(&io_driver_msg_recv);
                }
                break;

#if F_APP_EXT_CHARGER_FEATURE_SUPPORT
            case IO_MSG_GPIO_EXT_CHARGER_STATE:
                {
                    app_ext_charger_handle_io_msg(io_driver_msg_recv);
                }
                break;
#endif

#if HARMAN_NTC_DETECT_PROTECT || HARMAN_DISCHARGER_NTC_DETECT_PROTECT
            case IO_MSG_GPIO_EXT_CHARGER_ADC_VALUE:
                {
                    app_harman_adc_msg_handle();
                }
                break;
#endif

#if HARMAN_USB_CONNECTOR_PROTECT
            case IO_MSG_GPIO_USB_CONNECTOR_ADC_VALUE:
                {
                    app_harman_usb_connector_adc_update();
                }
                break;
#endif

            default:
                break;
            }
        }
        break;

    case IO_MSG_TYPE_BT_STATUS:
        app_ble_handle_gap_msg(&io_driver_msg_recv);
        break;
#if F_APP_CONSOLE_SUPPORT
    case IO_MSG_TYPE_CONSOLE:
        app_console_handle_msg(io_driver_msg_recv);
        break;
#endif
#if GFPS_FEATURE_SUPPORT
    case IO_MSG_TYPE_ECC:
        app_ecc_handle_msg();
        break;
#endif

#if F_APP_ADC_SUPPORT
    case IO_MSG_TYPE_ADP_VOLTAGE:
        app_adc_handle_adp_voltage_msg();
        break;
#endif

#if BISTO_FEATURE_SUPPORT
    case IO_MSG_TYPE_BISTO:
        app_bisto_msg_handle(io_driver_msg_recv);
        break;
#endif

#if F_APP_USB_AUDIO_SUPPORT
    case IO_MSG_TYPE_USB_UAC:
        {
            extern void app_usb_audio_handle_msg(T_IO_MSG * msg);
            app_usb_audio_handle_msg(&io_driver_msg_recv);
            break;
        }
    case IO_MSG_TYPE_USB_HID:
        {
//            extern void app_usb_hid_handle_msg(T_IO_MSG * msg);
//            app_usb_hid_handle_msg(&io_driver_msg_recv);
            break;
        }
#endif

#if APP_CAP_TOUCH_SUPPORT
    case IO_MSG_TYPE_CAP_TOUCH:
        {
            app_cap_touch_msg_handle((T_IO_MSG_CAP_TOUCH)io_driver_msg_recv.subtype);
        }
        break;
#endif

#if F_APP_TUYA_SUPPORT
    case IO_MSG_TYPE_TUYA:
        rtk_tuya_handle_event_msg(io_driver_msg_recv);
        break;
#endif

#if F_APP_SS_SUPPORT
    case IO_MSG_TYPE_SS:
        app_ss_handle_msg(&io_driver_msg_recv);
        break;
#endif

    default:
        break;
    }
}
