/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "trace.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "gap_timer.h"
#include "ringtone.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_dlps.h"
#include "app_io_msg.h"
#include "app_gpio.h"
#include "app_mmi.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_adp.h"
#include "section.h"
#include "app_bt_policy_api.h"
#include "app_charger.h"
#include "app_audio_policy.h"
#include "app_sensor.h"
#include "board.h"
#include "test_mode.h"

#if F_APP_LOCAL_PLAYBACK_SUPPORT
#include "audio_fs.h"
#include "app_playback.h"
#endif
#include "system_status_api.h"
#include "sysm.h"
#if GFPS_FEATURE_SUPPORT
#include "gfps.h"
#include "app_gfps.h"
#include "app_gfps_battery.h"
#endif
#if F_APP_GPIO_ONOFF_SUPPORT
#include "app_gpio_on_off.h"
#endif
#include "app_led.h"
#include "bt_avrcp.h"
#include "audio_volume.h"
#include "app_multilink.h"
#include "app_bond.h"
#include "app_bt_policy_int.h"
#include "rtl876x_pinmux.h"
#include "app_ota_tooling.h"
#include "app_wdg.h"

#include "app_cmd.h"
#include "app_relay.h"
#include "app_ota.h"
#include "audio_type.h"
#include "os_sched.h"
#include "app_auto_power_off.h"
#include "app_io_output.h"
#include "app_io_msg.h"
#include "vector.h"
#include "adapter.h"
#include "stdlib.h"
#include "app_adp.h"
#include "app_bud_loc.h"
#include "hw_tim.h"
#include "hal_adp.h"

#if F_APP_ADC_SUPPORT
#include "app_adc.h"
#endif

#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#endif
/********************************************************
vectors must follow BB2
*********************************************************/

#if F_APP_AVP_INIT_SUPPORT
#include "pm.h"
#endif
#if F_APP_ONE_WIRE_UART_SUPPORT
#include "app_one_wire_uart.h"
#endif

/** @defgroup  APP_ADP  App Adp
  * @brief  Tim implementation demo code
  * @{
  */
/**/

/*============================================================================*
 *                              Variables
 *============================================================================*/
///@cond

/** @defgroup _Exported_Variables  adp Variables
    * @{
    */
static void app_adp_cmd_of_chargerbox_handler(void);
static uint32_t app_adp_get_guard_bit_tim(void);
static void app_adp_handle_pri_close_case(void);
#if (F_APP_AVP_INIT_SUPPORT == 1)
void (*app_adp_close_case_hook)(void) = NULL;
void (*app_adp_enter_pairing_hook)(void) = NULL;
void (*app_adp_open_case_hook)(void) = NULL;
void (*app_adp_factory_reset_hook)(void) = NULL;
void (*app_adp_dut_mode_hook)(uint8_t) = NULL;
void (*app_adp_battery_change_hook)(void) = NULL;
#endif

static uint8_t app_adp_timer_queue_id = 0;
static void *timer_handle_ota_tooling_delay_start = NULL;
static void *timer_handle_adaptor_stable_plug = NULL;
static void *timer_handle_adaptor_stable_unplug = NULL;
static void *timer_handle_power_off_enter_dut_mode = NULL;
static void *timer_handle_adp_factory_reset_link_dis = NULL;
static void *timer_handle_adp_cmd_protect = NULL;

static uint8_t adaptor_plug_in = ADAPTOR_UNPLUG;
static T_HW_TIMER_HANDLE adp_hw_timer_handle = NULL;
void adp_timer_isr_callback(T_HW_TIMER_HANDLE handle);

typedef enum
{
    APP_TIMER_ADP_IDLE,
    APP_TIMER_ADP_FACTORY_RESET_LINK_DIS_TIMEOUT,
    APP_TIMER_ADP_CLOSE_CASE_TIMEOUT,
    APP_TIMER_ADP_CMD_RECEIVED_TIMEOUT,
    APP_TIMER_ADP_POWER_ON_WAIT_TIMEOUT,
    APP_TIMER_DISALLOW_CHARGER_LED_TIMEOUT,
    APP_TIMER_ADP_POWER_OFF_DISC_PHONE_LINK,
    APP_TIMER_ADP_IN_CASE_TIMEOUT,
    APP_TIMER_ADP_WAIT_ROLESWAP,
    APP_TIMER_CHECK_ADP_CMD_DISABLE_CHARGER,
    APP_TIMER_CHECK_ADP_CMD_CASE_CLOSE,
    APP_TIMER_BOX_ZERO_VOLUME_IGNORE_ADP_OUT,
    APP_TIMER_OTA_TOOLING_DELAY_START,
    APP_TIMER_ADP_PLUG_STABLE,
    APP_TIMER_ADP_UNPLUG_STABLE,
    APP_TIMER_POWE_OFF_ENTER_DUT_MODE,
    APP_TIMER_ADP_CLOSE_CASE_WAIT_RDTP_CONN,
    APP_TIMER_ADP_CMD_PROTECT,
} T_APP_ADP_TIMER;


// support 40 ms/ 20 ms per bit
static uint8_t len_per_bit[2] = {40, 20};

static T_ADP_CMD_PARSE_STRUCT *p_adp_cmd_data = NULL;

#define USB_STR_WAIT_1_TIM_CNT  1
#define USB_STR_WAIT_0_TIM_CNT  0

static bool out_case_power_on_led_en = false;
static bool in_case_is_power_off = false;
static bool adp_ever_pause_cmd = false;

static void *timer_handle_adp_close_case = NULL;
static void *timer_handle_adp_cmd_received = NULL;
static void *timer_handle_adp_power_on_wait = NULL;
static void *timer_handle_disallow_charger_led = NULL;
static void *timer_handle_adp_power_off_disc_phone_link = NULL;
static void *timer_handle_adp_in_case = NULL;
static void *timer_handle_adp_wait_roleswap = NULL;
static void *timer_handle_check_adp_cmd_disable_charger = NULL;
static void *timer_handle_check_adp_cmd_close_case = NULL;
static void *timer_handle_box_zero_volume_ignore_adp_out = NULL;
static void *timer_handle_adp_wait_rdtp_conn = NULL;

static bool pending_handle_adp_close_case = false;
static uint8_t b2b_connected_check_count = 0;
static uint8_t power_on_wait_counts = 0;
static uint8_t box_bat_volume = 0;
static bool disallow_enable_charger = false;
static bool power_on_wait_when_bt_ready = false;

#define ADP_FACTORY_RESET_LINK_DIS_TIMEOUT_MS           1500
#define ADP_FACTORY_RESET_LINK_DIS_HEADSET_BROADCAST_MS 200
#define ADP_CLOSE_CASE_TIMEOUT_MS                       3000
#define ADP_DISABLE_CHARGER_LED_MS                      3000
#define ADP_IN_CASE_AUTO_POWER_OFF_MS                   120000
#define ADP_CLOSE_CASE_TO_DISC_PHONE_LINK_MS            5000
#define ADP_CHECK_CLOSE_CASE_CMD_MS                     2000
#define MAX_TIMES_CHECK_B2B_CONNECTED_STATE             8

#define ADP_PAYLOAD_TYPE1          0x55        // For Normal Factory Reset and Enter Pairing mode
#define ADP_PAYLOAD_TYPE2          0xAA        // For Enter DUT test mode
#define ADP_PAYLOAD_TYPE3          0x46        // For DUT mode cmd, enter BAT off mode
#define ADP_PAYLOAD_TYPE4          0x45        // For Enter shipping mode

/**
 * @brief ADAPTOR DET to APP cmd
 */
typedef enum
{
    AD2B_CMD_NONE                   = 0x00,
    AD2B_CMD_FACTORY_RESET          = 0x01,
    AD2B_CMD_OPEN_CASE              = 0x03,
    AD2B_CMD_CLOSE_CASE             = 0x04,
    AD2B_CMD_ENTER_PAIRING          = 0x06,
    AD2B_CMD_ENTER_DUT_MODE         = 0x08,
    AD2B_CMD_USB_STATE              = 0x0A,
    AD2B_CMD_OTA_TOOLING            = 0x0B,
    AD2B_CMD_CASE_CHARGER           = 0x0D,// case charger battery
    AD2B_CMD_ENTER_MP_MODE          = 0x0E,

    AD2B_CMD_RESV_1                 = 0x3E,// resv for hw reset --> customer MCU use
    AD2B_CMD_RESV_2                 = 0x3F,// resv for hw reset --> customer MCU use
} T_AD2B_CMD_ID;

static uint8_t adp_cmd_exec = AD2B_CMD_NONE, adp_cmd_rec = AD2B_CMD_NONE;
static uint8_t adp_cmd_pending = AD2B_CMD_NONE, adp_payload_pending = 0;
static bool case_batt_pre_updated = false;



static struct
{
    VECTOR cbs_vector;
    uint8_t adaptor_stable_count;
} app_adp_mgr =  {.cbs_vector = NULL, .adaptor_stable_count = 0};

typedef struct
{
    uint32_t bit_data : 1;
    uint32_t tim_delta_value : 31;
} T_ADP_INT_DATA;

/** End of Exported_Variables
    * @}
    */
///@endcond

/*============================================================================*
 *                              Functions
 *============================================================================*/



void app_adp_set_disable_charger_by_box_battery(bool flag)
{
    if (app_cfg_nv.disable_charger_by_box_battery != flag)
    {
        app_cfg_nv.disable_charger_by_box_battery = flag;
        app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);
    }
}

bool app_adp_cb_reg(APP_ADP_CB  cb)
{
    bool ret = false;

    //Add this to guarantee initialization sequence. app_auto_power_off.c initialized before app_adp.c
    if (app_adp_mgr.cbs_vector == NULL)
    {
        app_adp_mgr.cbs_vector = vector_create(0xff);
    }

    ret = vector_add(app_adp_mgr.cbs_vector, cb);

    return ret;
}


bool app_adp_trig_cb(APP_ADP_CB cb, APP_ADP_STATUS *p_status)
{
    cb(*p_status);

    return true;
}




void app_adp_stop_plug(void)
{
    gap_stop_timer(&timer_handle_adaptor_stable_plug);
}


bool app_adp_plug_debounce_timer_started(void)
{
    return ((timer_handle_adaptor_stable_plug != NULL) ? true : false);
}

bool app_adp_unplug_debounce_timer_started(void)
{
    return ((timer_handle_adaptor_stable_unplug != NULL) ? true : false);
}

void app_adp_inbox_wake_up(void)
{
    gap_start_timer(&timer_handle_adaptor_stable_unplug, "adaptor_stable_unplug",
                    app_adp_timer_queue_id,
                    APP_TIMER_ADP_UNPLUG_STABLE, 0, 10);
}


/**
* @brief: Reset bit process data
*
*/
RAM_TEXT_SECTION static void app_adp_cmd_data_parse_reset(bool usb_dis)
{
    APP_PRINT_INFO1("app_adp_cmd_data_parse_reset usb_dis = %d", usb_dis);
    memset(p_adp_cmd_data, 0, sizeof(T_ADP_CMD_PARSE_STRUCT));
    p_adp_cmd_data->cmd_decode.bit_data = 0xFFFFFFFF;
    p_adp_cmd_data->cmd_parity09_cnt = 9;
    p_adp_cmd_data->cmd_parity15_cnt = 15;
    p_adp_cmd_data->guard_bit_tim = app_adp_get_guard_bit_tim();

    if (!usb_dis)
    {
        p_adp_cmd_data->usb_in_cmd_det = true ;
        p_adp_cmd_data->usb_start_wait_en = 1;
        p_adp_cmd_data->usb_cmd_rec = true;
    }
}
/**
* @brief: Process bit data, currently we use odd parity check
*
*   Payload of CMD_9
*   |bit 8  |bit 7  |bit 6  |bit 5  |bit 4  |bit 3  |bit 2  |bit 1  |bit 0  |
*   |START  |                     Data(6bits)               |PARITY |END    |
*
*   Payload of CMD_15
*   |bit 14 |bit 13 |               ~               |bit 2  |bit 1  |bit 0  |
*   |START  |                     Data(12bits)              |PARITY |END    |
*
*/
RAM_TEXT_SECTION static void app_adp_cmd_process(uint32_t n_input)
{
    n_input &= BIT(0);
    uint8_t cmd_guard = 0;

    if (p_adp_cmd_data->cmd_decode.cmdpack15.start_bit)
    {
        p_adp_cmd_data->cmd_parity15_cnt--;
    }

    if (p_adp_cmd_data->cmd_decode.cmdpack09.start_bit)
    {
        p_adp_cmd_data->cmd_parity09_cnt--;
    }

    if (n_input)
    {
        p_adp_cmd_data->cmd_parity15_cnt++;
        p_adp_cmd_data->cmd_parity09_cnt++;
    }

    p_adp_cmd_data->cmd_decode.bit_data <<= 1;
    p_adp_cmd_data->cmd_decode.bit_data |= n_input;

#if (ADP_CMD_DBG == 1)
    APP_PRINT_INFO2("app_adp_cmd_process bitdata %08x, %d", p_adp_cmd_data->cmd_decode.bit_data,
                    n_input);
#endif

    if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_9BITS)
    {

        if ((p_adp_cmd_data->cmd_decode.cmdpack09.start_bit == 0) &&
            (p_adp_cmd_data->cmd_decode.cmdpack09.stop_bit == 0) &&
            ((p_adp_cmd_data->cmd_parity09_cnt & BIT(0)) == 1))
        {
            cmd_guard = (p_adp_cmd_data->cmd_decode.bit_data >> 9) & GUARD_BIT;

            if (cmd_guard == GUARD_BIT)
            {
                p_adp_cmd_data->cmd_out09 = p_adp_cmd_data->cmd_decode.cmdpack09.cmd_data;
                APP_PRINT_INFO1("app_adp_cmd_process p_adp_cmd_data->cmd_out09 %u", p_adp_cmd_data->cmd_out09);
            }
            else
            {
                APP_PRINT_INFO1("app_adp_cmd_process cmd 09 without guard bit %08x",
                                p_adp_cmd_data->cmd_decode.bit_data);
            }
        }
    }
    else if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
    {
        if ((p_adp_cmd_data->cmd_decode.cmdpack15.start_bit == 0) &&
            (p_adp_cmd_data->cmd_decode.cmdpack15.stop_bit == 0) &&
            ((p_adp_cmd_data->cmd_parity15_cnt & BIT(0)) == 1))
        {
            cmd_guard = (p_adp_cmd_data->cmd_decode.bit_data >> 15) & GUARD_BIT;

            if (cmd_guard == GUARD_BIT)
            {
                p_adp_cmd_data->cmd_out15 = p_adp_cmd_data->cmd_decode.cmdpack15.cmd_data;
                APP_PRINT_INFO2("app_adp_cmd_process cmd_out15 0x%x 0x%x",
                                (p_adp_cmd_data->cmd_out15 >> 8) & 0xFF,
                                (p_adp_cmd_data->cmd_out15 >> 0) & 0xFF);
            }
            else
            {
                APP_PRINT_INFO1("app_adp_cmd_process cmd 15 without guard bit %08x",
                                p_adp_cmd_data->cmd_decode.bit_data);
            }
        }
    }
}

RAM_TEXT_SECTION void app_adp_cmd_send(void)
{
    T_IO_MSG adp_msg;

    if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_9BITS)
    {
        if (p_adp_cmd_data->cmd_out09 > AD2B_CMD_NONE)
        {
            adp_msg.type = IO_MSG_TYPE_GPIO;
            adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_DAT;
            adp_msg.u.param = p_adp_cmd_data->cmd_out09;

            app_adp_cmd_data_parse_reset(1);

            if (!app_io_send_msg(&adp_msg))
            {
                APP_PRINT_ERROR0("app_adp_cmd_send: Send msg error");
            }
        }
    }
    else if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
    {
        if (((p_adp_cmd_data->cmd_out15 >> 8) & 0xFF) > AD2B_CMD_NONE)
        {
            adp_msg.type = IO_MSG_TYPE_GPIO;
            adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_DAT;
            adp_msg.u.param = p_adp_cmd_data->cmd_out15;
            /* should not clear flag when second usb in comman come otherwise cmd cannot be exe*/
            if (((p_adp_cmd_data->cmd_out15 >> 8) & 0xFF) != AD2B_CMD_USB_STATE)
            {
                app_adp_cmd_data_parse_reset(1);
            }
            else
            {
                p_adp_cmd_data->usb_cmd_rec = true; /* code */
            }


            if (!app_io_send_msg(&adp_msg))
            {
                APP_PRINT_ERROR0("app_adp_cmd_send: Send msg error");
            }
        }
    }

    p_adp_cmd_data->cmd_out09 = 0;
    p_adp_cmd_data->cmd_out15 = 0;
}

/**
* @brief: get guard bit time
*
*/
static uint32_t app_adp_get_guard_bit_tim(void)
{
    uint32_t temp_guard_tim;

    if (app_cfg_const.smart_charger_box_bit_length == 1)
    {
        temp_guard_tim = GUART_BIT_TIM - GUART_BIT_TIM_TOLERANCE / 2;
    }
    else
    {
        if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_9BITS)
        {
            temp_guard_tim = GUART_BIT_TIM - GUART_BIT_TIM_TOLERANCE / 2;
        }
        else
        {
            temp_guard_tim = 2 * GUART_BIT_TIM - GUART_BIT_TIM_TOLERANCE;
        }
    }

#if (ADP_CMD_DBG == 1)
    APP_PRINT_INFO1("app_adp_get_guard_bit_tim: temp_guard_tim %d", temp_guard_tim);
#endif

    return temp_guard_tim;
}

/**
 * @brief contains the initialization of adaptor det timer and  ADP_DET_IRQn settings.
 *
 */
static void app_adp_timer_init(void)
{
    adp_hw_timer_handle = hw_timer_create("app_adp_cmd", PERIOD_ADP_TIM, false,
                                          adp_timer_isr_callback);
    if (adp_hw_timer_handle == NULL)
    {
        APP_PRINT_ERROR0("adp cmd HW timer create failed !");
        return;
    }
    app_adp_cmd_data_parse_reset(1);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
}

static void app_adp_int_init(void)
{

    if (p_adp_cmd_data == NULL)
    {
        p_adp_cmd_data = (T_ADP_CMD_PARSE_STRUCT *)calloc(1, sizeof(T_ADP_CMD_PARSE_STRUCT));
    }
    else
    {
        memset(p_adp_cmd_data, 0, sizeof(T_ADP_CMD_PARSE_STRUCT));
    }
    adp_update_isr_cb(ADP_DETECT_5V, (P_ADP_ISR_CBACK)app_adp_cmd_of_chargerbox_handler);
}

void app_adp_set_cmd_received(uint8_t cmd_received)
{
}

uint8_t app_adp_cmd_received(void)
{
    APP_PRINT_INFO1("app_adp_cmd_received: adp_cmd_rec 0x%04x", adp_cmd_rec);
    if ((adp_cmd_rec == AD2B_CMD_NONE) || (adp_cmd_rec == AD2B_CMD_CASE_CHARGER))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void app_adp_case_battery_update_reset(void)
{
    case_batt_pre_updated = false;
}

static void app_adp_case_battery(uint8_t battery)
{
    static uint8_t case_batt_pre = 0, case_batt = 0;

#if (F_APP_AVP_INIT_SUPPORT == 1)
    static uint8_t case_status = 0;
#endif

    if (app_adp_case_battery_check(&battery, &case_batt))
    {
        if (case_batt_pre_updated)
        {
            if (abs((case_batt & 0x7f) - case_batt_pre) >= 10)//remove debounce
            {
                return;
            }
        }

        app_db.case_battery = case_batt;
        app_cfg_nv.case_battery = app_db.case_battery;
        case_batt_pre = case_batt & 0x7f;
        case_batt_pre_updated = true;

#if (F_APP_AVP_INIT_SUPPORT == 1)
        if (app_adp_battery_change_hook)
        {
            if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY) && (case_status != (case_batt >> 7)))
            {
                case_status = case_batt >> 7;
                app_adp_battery_change_hook();
            }
        }
#endif
        app_relay_async_single(APP_MODULE_TYPE_CHARGER, APP_CHARGER_EVENT_BATT_CASE_LEVEL);

        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            app_report_rws_bud_info();
        }
    }
}

bool app_adp_case_battery_check(uint8_t *bat_in, uint8_t *bat_out)
{
    bool is_valid = false;

    APP_PRINT_TRACE2("app_adp_case_battery_check: bat_in 0x%02x, bat_out 0x%02x",
                     *bat_in, *bat_out);

    if ((*bat_in & 0x7f) > 100)//invalid battery level
    {
        *bat_out = *bat_in & 0x80;
    }
    else
    {
        *bat_out = *bat_in;
        is_valid = true;
    }

    return is_valid;
}

static void app_adp_cmd_power_on_handle(void)
{
    app_db.power_on_by_cmd = true;
    app_db.power_off_by_cmd = false;
    pending_handle_adp_close_case = false;

    app_db.local_in_case = app_device_is_in_the_box();
    app_db.local_loc = app_sensor_bud_loc_detect();

    app_cfg_nv.power_off_cause_cmd = 0;
    in_case_is_power_off = false;

    if (app_db.device_state == APP_DEVICE_STATE_OFF)
    {
        out_case_power_on_led_en = false;
        gap_stop_timer(&timer_handle_adp_close_case);//stop here, don't do power off
        gap_stop_timer(&timer_handle_adp_power_off_disc_phone_link);
        app_mmi_handle_action(MMI_DEV_POWER_ON);
    }
    else
    {
        if (app_cfg_const.enable_power_off_immediately_when_close_case == false)
        {
            app_adp_linkback_b2s_when_open_case();
        }
        else
        {
            gap_stop_timer(&timer_handle_adp_close_case);//stop here, don't do power off
        }
    }

    app_bud_loc_evt_handle(APP_BUD_LOC_EVENT_CASE_OPEN_CASE, 0, 0);
}

void app_adp_clear_pending(void)
{
    adp_cmd_pending = AD2B_CMD_NONE;
    adp_payload_pending = 0;
}

void app_adp_smart_box_charger_control(bool enable, T_CHARGER_CONTROL_TYPE type)
{
    APP_PRINT_INFO3("app_adp_smart_box_charger_control: enable %d, type %d, disallow_enable_charger %d",
                    enable, type, disallow_enable_charger);

    if (disallow_enable_charger && enable)
    {
        return;
    }

    if (enable)
    {
        /* enable CHARGER_AUTO_ENABLE to enable charger interrupt again */
        sys_hall_charger_auto_enable(true, type, false);

        Pad_WakeUpCmd(ADP_MODE, POL_HIGH, ENABLE);
    }
    else
    {
        /* disable CHARGER_AUTO_ENABLE first to prevent interrupt to enable charger again */
        sys_hall_charger_auto_enable(false, type, false);

        Pad_WakeUpCmd(ADP_MODE, POL_LOW, ENABLE);
    }
}

#if F_APP_LOCAL_PLAYBACK_SUPPORT
void app_adp_usb_start_handle(void)
{
    set_cpu_sleep_en(0);
    app_dlps_stop_power_down_wdg_timer();
    app_sd_card_power_down_disable(APP_SD_POWER_DOWN_ENTER_CHECK_USB_MSC);
    app_sd_card_power_on(); //??
    usb_start(NULL);
    app_dlps_disable(APP_DLPS_ENTER_CHECK_USB);
}

void app_adp_usb_stop_handle(void)
{
    usb_stop(NULL);
    audio_fs_update(NULL);
    set_cpu_sleep_en(1);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_USB);
    /*for customer MIFO work around sd en pin*/
    app_sd_card_power_down_enable(APP_SD_POWER_DOWN_ENTER_CHECK_USB_MSC);
    app_dlps_start_power_down_wdg_timer();
}
#endif

void app_adp_delay_charger_enable(void)
{
    bool enable_thermal_detection = false;
    uint8_t type = (uint8_t)CHARGER_CONTROL_BY_BOX_BATTERY;
    if (sys_hall_charger_auto_enable(true, type, true))
    {
        enable_thermal_detection = true;
    }

    if (app_cfg_const.smart_charger_control && enable_thermal_detection)
    {
        app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_BOX_BATTERY);

        disallow_enable_charger = true;
        gap_start_timer(&timer_handle_check_adp_cmd_disable_charger,
                        "check_charger_cmd_disable_charger",
                        app_adp_timer_queue_id, APP_TIMER_CHECK_ADP_CMD_DISABLE_CHARGER,
                        0, 1000);
    }
}

static void app_adp_delay_power_on_when_bt_ready(void)
{
    gap_start_timer(&timer_handle_adp_power_on_wait, "adp_power_on_wait",
                    app_adp_timer_queue_id,
                    APP_TIMER_ADP_POWER_ON_WAIT_TIMEOUT,
                    0, 100);
}

void app_adp_close_case_cmd_check(void)
{
    if ((app_db.local_loc != BUD_LOC_IN_CASE) || (timer_handle_adp_close_case != NULL) ||
        (app_db.device_state == APP_DEVICE_STATE_OFF))
    {
        return;
    }

    gap_stop_timer(&timer_handle_check_adp_cmd_close_case);
    gap_start_timer(&timer_handle_check_adp_cmd_close_case,
                    "check_adp_cmd_close",
                    app_adp_timer_queue_id, APP_TIMER_CHECK_ADP_CMD_CASE_CLOSE,
                    0, ADP_CHECK_CLOSE_CASE_CMD_MS);
}

void app_adp_open_case_cmd_check(void)
{
    gap_stop_timer(&timer_handle_check_adp_cmd_close_case);
}

static void app_adp_cmd_open_case_start(void)
{
    APP_PRINT_INFO0("app_adp_cmd_open_case_start");

    gap_stop_timer(&timer_handle_adp_in_case);
    gap_stop_timer(&timer_handle_adp_power_on_wait);
    gap_stop_timer(&timer_handle_adp_wait_rdtp_conn);

    b2b_connected_check_count = 0;
    app_cfg_nv.power_on_cause_cmd = 0;

    if (app_db.bt_is_ready)
    {
        if ((app_db.device_state == APP_DEVICE_STATE_OFF_ING) || app_mmi_reboot_check_timer_started())
        {
            app_cfg_nv.power_on_cause_cmd = 1;
            app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);
        }
        else
        {
            app_adp_cmd_power_on_handle();
        }
    }
    else
    {
        power_on_wait_when_bt_ready = true;
        power_on_wait_counts = 0;
        adp_cmd_exec = AD2B_CMD_NONE; //rec the next power on cmd
        app_adp_delay_power_on_when_bt_ready();
    }
}

static void app_adp_cmd_close_case_start(void)
{
    uint32_t time_to_power_off;

    APP_PRINT_INFO0("app_adp_cmd_close_case_start");

    gap_stop_timer(&timer_handle_check_adp_cmd_close_case);
    gap_stop_timer(&timer_handle_adp_in_case);

    if (app_cfg_nv.power_on_cause_cmd)
    {

        app_cfg_nv.power_on_cause_cmd = false;
        app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);
    }

    if (app_db.device_state == APP_DEVICE_STATE_OFF)
    {
        in_case_is_power_off = true;

        app_cfg_nv.power_off_cause_cmd = 1;
        app_cfg_store(&app_cfg_nv.remote_loc, 4);

        APP_PRINT_INFO0("app_adp_cmd_close_case_start: already power off,wdt rst later");
    }
    else
    {
        power_on_wait_when_bt_ready = false;

        if (app_cfg_const.enable_power_off_immediately_when_close_case == false)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_bt_policy_stop();
            }

            gap_stop_timer(&timer_handle_adp_power_off_disc_phone_link);
            gap_start_timer(&timer_handle_adp_power_off_disc_phone_link, "adp_power_off_disc_phone_link",
                            app_adp_timer_queue_id,
                            APP_TIMER_ADP_POWER_OFF_DISC_PHONE_LINK,
                            0, ADP_CLOSE_CASE_TO_DISC_PHONE_LINK_MS);

            time_to_power_off = ADP_CLOSE_CASE_TIMEOUT_MS * 10;
        }
        else
        {
            time_to_power_off = ADP_CLOSE_CASE_TIMEOUT_MS;
        }

        gap_stop_timer(&timer_handle_adp_close_case);
        gap_start_timer(&timer_handle_adp_close_case, "adp_close_case",
                        app_adp_timer_queue_id,
                        APP_TIMER_ADP_CLOSE_CASE_TIMEOUT,
                        0, time_to_power_off);

        app_bud_loc_evt_handle(APP_BUD_LOC_EVENT_CASE_CLOSE_CASE, 0, 0);
    }
}

static void app_adp_other_feature_handle_when_close_case(void)
{
    if (app_db.device_state != APP_DEVICE_STATE_OFF)
    {
#if GFPS_FEATURE_SUPPORT
        if (extend_app_cfg_const.gfps_support)
        {
            app_gfps_handle_case_status(false);
        }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
        if (app_adp_close_case_hook)
        {
            app_adp_close_case_hook();
        }
#endif
    }
}

/* load when app_cfg_load init (this flag has only one credit) */
void app_adp_load_adp_high_wake_up(void)
{
    APP_PRINT_TRACE1("app_adp_load_adp_high_wake_up: %d",
                     app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset);

    if (app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset)
    {
        app_db.adp_high_wake_up_for_zero_box_bat_vol = true;

        if (app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset > 0)
        {
            app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset--;
        }
        app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);
    }
}

void app_adp_smart_box_update_battery_lv(uint8_t payload, bool report_flag)
{
    box_bat_volume = (payload & 0x7F);
    app_cfg_nv.case_battery = payload;

    if (report_flag)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            app_report_rws_bud_info();
        }
        else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (app_db.remote_loc != BUD_LOC_IN_CASE)
            {
                app_db.need_sync_case_battery_to_pri = true;
            }
        }
    }

    if (app_cfg_const.smart_charger_control)
    {
        if (box_bat_volume <= app_cfg_const.smart_charger_disable_threshold)
        {
            if (app_db.device_state != APP_DEVICE_STATE_OFF)
            {
                /* this flag will be applied after sw reset after power off in order to disable charger */
                app_cfg_nv.report_box_bat_lv_again_after_sw_reset = true;
            }

            app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_BOX_BATTERY);

            app_adp_set_disable_charger_by_box_battery(true);

            if (box_bat_volume == 0)
            {
                /* means receive box 0% when bud still power on */
                if (timer_handle_check_adp_cmd_close_case != NULL &&
                    (app_db.device_state != APP_DEVICE_STATE_OFF))
                {
                    /* role swap will be triggerd when get close case cmd or in box */
                    gap_stop_timer(&timer_handle_adp_power_off_disc_phone_link);
                    gap_stop_timer(&timer_handle_check_adp_cmd_close_case);

                    app_adp_cmd_close_case_start();
                    app_adp_other_feature_handle_when_close_case();
                }
                else
                {
                    /* restart power_down_wdg protect timer */
                    app_dlps_start_power_down_wdg_timer();

                    /* when get box bat vol 0 %; start a 5sec timer to change to adp high wake up */
                    app_dlps_disable(APP_DLPS_ENTER_CHECK_BOX_BAT);
                    gap_start_timer(&timer_handle_box_zero_volume_ignore_adp_out,
                                    "box_zero_volume_ignore_adp_out", app_adp_timer_queue_id,
                                    APP_TIMER_BOX_ZERO_VOLUME_IGNORE_ADP_OUT, 0, 5000);
                }
            }
        }
        else if (box_bat_volume >= app_cfg_const.smart_charger_enable_threshold)
        {
            app_cfg_nv.report_box_bat_lv_again_after_sw_reset = false;
            app_adp_smart_box_charger_control(true, CHARGER_CONTROL_BY_BOX_BATTERY);

            app_adp_set_disable_charger_by_box_battery(false);
        }

        if (box_bat_volume == 0)
        {
            app_db.adp_high_wake_up_for_zero_box_bat_vol = true;
        }
        else
        {
            app_db.adp_high_wake_up_for_zero_box_bat_vol = false;
        }

        if (app_cfg_const.enable_external_mcu_reset)
        {
            /* save to flash when external mcu reset is enabled; and this flag only works for next boot up */
            if (box_bat_volume == 0)
            {
                /* has 2 credits; one for sw rst and the other for external mcu hw rst */
                app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset = 2;
            }
            else
            {
                app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset = 0;
            }
            app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);
        }
    }

    app_cfg_store(&app_cfg_nv.case_battery, 4);
    app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);

    APP_PRINT_INFO2("app_adp_smart_box_update_battery_lv: box_bat_volume %d, case_battery %d",
                    box_bat_volume, app_cfg_nv.case_battery);
}

static void app_adp_cmd_exec(uint8_t cmd, uint8_t payload)
{
    adp_cmd_exec = cmd; //current executing cmd

    APP_PRINT_INFO4("app_adp_cmd_exec: adp_cmd_exec 0x%x payload %d device_state %d bt_is_ready %d",
                    adp_cmd_exec, payload, app_db.device_state, app_db.bt_is_ready);

    if (cmd == AD2B_CMD_OPEN_CASE)
    {
        app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_OPEN_CASE);
    }

    if ((cmd == AD2B_CMD_OPEN_CASE) || (cmd == AD2B_CMD_CASE_CHARGER))
    {
        if ((payload & 0x7F) > 100)//invalid battery level
        {
            payload = (payload & 0x80) | 0x50;
        }
        app_adp_smart_box_update_battery_lv(payload, true);
    }

    if (cmd != AD2B_CMD_CASE_CHARGER)
    {
        app_auto_power_off_enable(AUTO_POWER_OFF_MASK_IN_BOX, app_cfg_const.timer_auto_power_off);
    }

    if (app_cfg_const.enable_rtk_charging_box)
    {
        switch (cmd)
        {
        case AD2B_CMD_FACTORY_RESET:
            {
                uint16_t link_dis_delay = 0;

                if (app_cfg_nv.adp_factory_reset_power_on != 0)
                {
                    APP_PRINT_INFO0("app_adp_handle_msg power on by factory reset,ignore");
                    break;
                }

#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_adp_factory_reset_hook)
                {
                    app_adp_factory_reset_hook();
                }
#endif

                if (app_cfg_const.enable_broadcasting_headset != 0)
                {
                    link_dis_delay = ADP_FACTORY_RESET_LINK_DIS_HEADSET_BROADCAST_MS;
                }
#if (BROADCAST_BY_RTK_CHARGER_BOX == 1)
                else
                {
                    link_dis_delay = ADP_FACTORY_RESET_LINK_DIS_TIMEOUT_MS;
                }
#endif

                if (payload == ADP_PAYLOAD_TYPE1)
                {
                    app_adp_factory_reset_link_dis(link_dis_delay);
                }
                else if (payload == ADP_PAYLOAD_TYPE2)
                {
                    app_cfg_nv.adp_factory_reset_power_on = 1;
                    app_mmi_handle_action(MMI_DEV_PHONE_RECORD_RESET);
                }
            }
            break;

        case AD2B_CMD_OPEN_CASE:
            {
                if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
                {
                    app_adp_case_battery(payload);
                }

#if LOCAL_PLAYBACK_FEATURE_SUPPORT
                usb_stop(NULL);
                audio_fs_update(NULL);
                app_dlps_enable(APP_DLPS_ENTER_CHECK_USB);
#endif

                app_adp_cmd_open_case_start();
#if GFPS_FEATURE_SUPPORT
                if (extend_app_cfg_const.gfps_support)
                {
                    app_gfps_handle_case_status(true);
                }
#endif

#if (F_APP_AVP_INIT_SUPPORT == 1)
                if (app_db.bt_is_ready && !app_cfg_nv.power_on_cause_cmd)
                {
                    if (app_adp_open_case_hook)
                    {
                        app_adp_open_case_hook();
                    }
                }
#endif
            }
            break;

        case AD2B_CMD_CLOSE_CASE:
            {
                app_adp_cmd_close_case_start();
                app_adp_other_feature_handle_when_close_case();
            }
            break;

        case AD2B_CMD_ENTER_PAIRING:
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY &&
                    app_cfg_nv.first_engaged)
                {
                    if (app_db.device_state == APP_DEVICE_STATE_OFF)
                    {
                        sys_mgr_power_on();
                    }
                }
                else
                {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                    if (app_adp_enter_pairing_hook)
                    {
                        app_adp_enter_pairing_hook();
                    }
#else
                    app_mmi_handle_action(MMI_DEV_FORCE_ENTER_PAIRING_MODE);
#endif
                }
            }
            break;

        case AD2B_CMD_ENTER_DUT_MODE:
            {
                if (payload == ADP_PAYLOAD_TYPE2)
                {
                    if (app_db.device_state == APP_DEVICE_STATE_ON)
                    {
                        app_mmi_handle_action(MMI_DUT_TEST_MODE);
                    }
                    else
                    {
                        app_cfg_nv.trigger_dut_mode_from_power_off = 1;
                        app_cfg_store(&app_cfg_nv.eq_idx_gaming_mode_record, 4);

                        app_mmi_handle_action(MMI_DEV_ENTER_PAIRING_MODE);
                        gap_start_timer(&timer_handle_power_off_enter_dut_mode, "power_off_enter_dut_mode",
                                        app_adp_timer_queue_id,
                                        APP_TIMER_POWE_OFF_ENTER_DUT_MODE, 0, POWER_OFF_ENTER_DUT_MODE_INTERVAL);
                    }
                }
                else if (payload == ADP_PAYLOAD_TYPE3)
                {
                    led_change_mode(LED_MODE_ENTER_PCBA_SHIPPING_MODE, true, false);
                }
#if (F_APP_AVP_INIT_SUPPORT == 1)
                else if (payload == ADP_PAYLOAD_TYPE4)
                {
                    power_mode_set(POWER_POWEROFF_MODE);
                }
                else
                {
                    if (app_adp_dut_mode_hook)
                    {
                        app_adp_dut_mode_hook(payload);
                    }
                }
#endif
            }
            break;

        case AD2B_CMD_USB_STATE:
            {
                APP_PRINT_INFO2("app_adp_handle_msg +++++ AD2B_CMD_USB(in=0x0a05) = 0x%x usb_in_cmd_det_already =%d",
                                (cmd << 8) | payload, p_adp_cmd_data->usb_in_cmd_det);
                if (payload == ADP_USB_IN_PLAYLOAD) //usb_in
                {
                    if ((p_adp_cmd_data->usb_in_cmd_det == true) ||
                        (p_adp_cmd_data->tim_cnt_finish == true)) //second detect usb in command and wait adp timer finished
                    {
                        APP_PRINT_INFO0("app_adp_handle_msg  USB START"); // second usb in command get usb start
                        /*start usb clear flag*/
                        app_adp_cmd_data_parse_reset(1);
                        app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);

                        p_adp_cmd_data->usb_started = true;

#if F_APP_LOCAL_PLAYBACK_SUPPORT //& F_APP_ERWS_SUPPORT
                        if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE))
                        {
                            app_adp_usb_start_handle();
                        }
#endif
                    }
                    else
                    {
                        APP_PRINT_INFO0("first get usb USB in cmd");
                        /*first usb cmd get clear flag*/
                        app_adp_cmd_data_parse_reset(0);

                    }
                }
                else
                {
                    APP_PRINT_INFO0("app_adp_handle_msg  USB stop");
#if F_APP_LOCAL_PLAYBACK_SUPPORT //& F_APP_ERWS_SUPPORT
                    if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE))
                    {
                        p_adp_cmd_data->usb_started = false;
                        app_adp_usb_stop_handle();
                    }
#endif
                }

                //@lemon add usb start code here,payload 0x05:usb in   0x70:usb out
            }
            break;

        case AD2B_CMD_CASE_CHARGER:
            {
                if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
                {
                    app_adp_case_battery(payload);
#if GFPS_FEATURE_SUPPORT
                    if (extend_app_cfg_const.gfps_support || (app_cfg_const.supported_profile_mask & GFPS_PROFILE_MASK))
                    {
                        app_gfps_battery_info_report(GFPS_BATTERY_REPORT_CASE_BATTERY_CHANGE);
                    }
#endif
                }
            }
            break;

        case AD2B_CMD_ENTER_MP_MODE:
            {
                switch_into_hci_mode();
            }
            break;

        default:
            break;
        }
    }

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    if (cmd == AD2B_CMD_OTA_TOOLING)
    {
        app_adp_cmd_ota_tooling(payload);
    }
#endif
}

void app_adp_pending_cmd_exec(void)
{
    if (adp_cmd_pending != AD2B_CMD_NONE)
    {
        APP_PRINT_INFO2("app_adp_pending_cmd_exec: cmd 0x%x payload 0x%d", adp_cmd_pending,
                        adp_payload_pending);
        app_adp_cmd_exec(adp_cmd_pending, adp_payload_pending);
        app_adp_clear_pending();
    }
}

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
void app_adp_special_cmd_handle(uint8_t jig_subcmd, uint8_t jig_dongle_id)
{
    APP_PRINT_INFO2("app_adp_special_cmd_handle: jig_subcmd 0x%x, jig_dongle_id 0x%x", jig_subcmd,
                    jig_dongle_id);

    // Do not auto power while OTA tooling mode
    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_OTA_TOOLING);

    adp_cmd_exec = AD2B_CMD_OTA_TOOLING;
    gap_start_timer(&timer_handle_adp_cmd_received, "adp_cmd_received",
                    app_adp_timer_queue_id,
                    APP_TIMER_ADP_CMD_RECEIVED_TIMEOUT,
                    0, 3000);

    switch (jig_subcmd)
    {
    case APP_ADP_SPECIAL_CMD_CREATE_SPP_CON:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
            {
                app_bt_policy_enter_ota_mode(true);
                app_ota_adv_start(app_db.jig_dongle_id, 30);
            }
            else
            {
                app_bt_policy_start_ota_shaking();
            }
        }
        break;

    case APP_ADP_SPECIAL_CMD_OTA:
    case APP_ADP_SPECIAL_CMD_FACTORY_MODE:
        {
            app_bt_policy_enter_ota_mode(true);
            app_ota_adv_start(app_db.jig_dongle_id, 30);
        }
        break;

    case APP_ADP_SPECIAL_CMD_RWS_FORCE_ENGAGE:
        {
            // TODO
        }
        break;

    default:
        break;
    }
}
#endif

/**
    * @brief  App handle rx data message from peripherals of adp_det.
    * @param  io_driver_msg_recv The T_IO_MSG from peripherals.
    * @return void
    */
void app_adp_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint16_t param = 0;
    uint8_t pay_load = 0;
    bool is_valid_cmd = false;
    T_BT_DEVICE_MODE radio_mode = app_bt_policy_get_radio_mode();
    /* Handle adp_dat to app cmd here. */
    param =  io_driver_msg_recv->u.param;

    if (param <= AD2B_CMD_NONE)
    {
        APP_PRINT_WARN1("app_adp_handle_msg: rec invalid cmd 0x%x", param);
        goto invalid_adp_cmd_handle;
    }

    if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
    {
        adp_cmd_rec = param >> 8;
        pay_load = param & 0xff;
    }
    else
    {
        adp_cmd_rec = param;
        pay_load = 0;
    }

    if (adp_cmd_rec == 0xFF)
    {
        goto invalid_adp_cmd_handle;
    }

    if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
    {
        if ((adp_cmd_rec == AD2B_CMD_FACTORY_RESET) || (adp_cmd_rec == AD2B_CMD_ENTER_PAIRING))
        {
            if (pay_load != ADP_PAYLOAD_TYPE1 && pay_load != ADP_PAYLOAD_TYPE2)
            {
                goto invalid_adp_cmd_handle;
            }
        }

        if (adp_cmd_rec == AD2B_CMD_ENTER_DUT_MODE)
        {
            if ((pay_load != ADP_PAYLOAD_TYPE2) && (pay_load != ADP_PAYLOAD_TYPE3)
#if (F_APP_AVP_INIT_SUPPORT == 1)
                && (pay_load != ADP_PAYLOAD_AVP_OPEN) && (pay_load != ADP_PAYLOAD_AVP_CLOSE)
                && (pay_load != ADP_PAYLOAD_TYPE4)
#endif
               )
            {
                goto invalid_adp_cmd_handle;
            }
        }
    }

    APP_PRINT_INFO8("app_adp_handle_msg: adp_cmd_rec 0x%0x, pay_load 0x%0x adp_cmd_exec 0x%0x adp_cmd_pending 0x%0x device_state %d radio_mode %d local_loc %d adp_factory_reset_power_on %d",
                    adp_cmd_rec, pay_load, adp_cmd_exec, adp_cmd_pending, app_db.device_state, radio_mode,
                    app_db.local_loc,
                    app_cfg_nv.adp_factory_reset_power_on);

    if (app_cfg_nv.adp_factory_reset_power_on != 0)
    {
        APP_PRINT_INFO1("app_adp_handle_msg %d", app_cfg_nv.power_off_cause_cmd);
        if (!app_cfg_nv.power_off_cause_cmd && (adp_cmd_exec == AD2B_CMD_CLOSE_CASE))
        {
            //Disable power on when received power off cmd during factory reset
            app_cfg_nv.power_off_cause_cmd = 1;
            app_cfg_store(&app_cfg_nv.remote_loc, 4);
            return;
        }
    }

    if (adp_cmd_exec == adp_cmd_rec)//same cmd,ignore
    {
        if ((adp_cmd_rec != AD2B_CMD_USB_STATE) &&
            (adp_cmd_rec != AD2B_CMD_CASE_CHARGER)) //need to exec many times
        {
            return;
        }
    }

    /*firstly,check inbox status*/
    if (app_db.local_loc != BUD_LOC_IN_CASE)
    {
        app_db.local_in_case = true;
        app_db.local_loc = app_sensor_bud_loc_detect();

        if (app_db.local_loc == BUD_LOC_IN_CASE)
        {
            //do nothing
            APP_PRINT_TRACE0("app_adp_handle_msg: change from outbox to in box");
        }

        app_sensor_bud_loc_sync();
    }

    /*receive valid adp cmd here*/
    is_valid_cmd = true;

    if (app_cfg_const.disallow_charging_led_before_power_off == 0)
    {
        app_db.disallow_charging_led = 1;
        gap_stop_timer(&timer_handle_disallow_charger_led);
        gap_start_timer(&timer_handle_disallow_charger_led, "disable_charger_led",
                        app_adp_timer_queue_id,
                        APP_TIMER_DISALLOW_CHARGER_LED_TIMEOUT,
                        0, ADP_DISABLE_CHARGER_LED_MS);
        APP_PRINT_INFO1("app_adp_handle_msg: disallow_charging_led = %d", app_db.disallow_charging_led);
    }

    gap_stop_timer(&timer_handle_adp_cmd_received);
    gap_start_timer(&timer_handle_adp_cmd_received, "adp_cmd_received",
                    app_adp_timer_queue_id,
                    APP_TIMER_ADP_CMD_RECEIVED_TIMEOUT,
                    0, 4000);

    if ((app_db.local_loc != BUD_LOC_IN_CASE) && (adp_cmd_rec != AD2B_CMD_OPEN_CASE))
    {
        bool pending_cmd = false;
#if F_APP_GPIO_ONOFF_SUPPORT
        /* this should not happen: set cmd as pending and exe after plug*/
        if ((app_cfg_const.box_detect_method == GPIO_DETECT) && app_device_is_in_the_box() &&
            app_gpio_detect_onoff_debounce_timer_started())
        {
            pending_cmd = true;
        }
        else
#endif
            if ((app_cfg_const.box_detect_method == ADAPTOR_DETECT) &&
                app_adp_plug_debounce_timer_started())
            {
                pending_cmd = true;
            }

        if (pending_cmd)
        {
            adp_cmd_pending = adp_cmd_rec;
            adp_payload_pending = pay_load;
        }
        else
        {
            app_adp_clear_pending();
        }

        if (p_adp_cmd_data->usb_cmd_rec == false)
        {
            APP_PRINT_TRACE0("usb_cmd_rec ==false return ");
            return;
        }
    }

    /*exe cmd if arrive here*/
    app_adp_cmd_exec(adp_cmd_rec, pay_load);

invalid_adp_cmd_handle:
    if (!is_valid_cmd)
    {
        adp_cmd_rec = AD2B_CMD_NONE;
    }
}

static void app_adp_power_off(void)
{
    if (in_case_is_power_off)
    {
        in_case_is_power_off = false;
        APP_PRINT_TRACE0("app_adp_power_off: already power off,ignore");
        return;
    }
    app_db.power_on_by_cmd = false;
    app_db.power_off_by_cmd = true;

    app_cfg_nv.power_on_cause_cmd = 0;
    app_cfg_nv.power_off_cause_cmd = 1;

    uint8_t b2s_num = app_bt_policy_get_b2s_connected_num();

    APP_PRINT_TRACE5("app_adp_power_off: bud_role %d case closed %d b2b %d remote_loc %d b2s %d",
                     app_cfg_nv.bud_role, app_db.remote_case_closed, app_db.remote_session_state, app_db.remote_loc,
                     b2s_num);

    bool roleswap_triggered = app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_CLOSE_CASE_POWER_OFF);

    if (roleswap_triggered == false)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }
}

static void app_adp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            T_BT_ROLESWAP_STATUS status = param->remote_roleswap_status.status;
            APP_PRINT_TRACE2("app_adp_bt_cback: roleswap status %d pending handle close case %d",
                             status, pending_handle_adp_close_case);

            if ((status == BT_ROLESWAP_STATUS_SUCCESS) || (status == BT_ROLESWAP_STATUS_FAIL))
            {
                if (app_db.local_loc == BUD_LOC_IN_CASE)
                {
                    app_audio_spk_mute_unmute(true);
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (memcpy(param->acl_conn_disconn.bd_addr, app_cfg_nv.bud_peer_addr, 6) == 0)
            {
                //b2b link disconnected
                b2b_connected_check_count = 0;
                gap_stop_timer(&timer_handle_adp_wait_rdtp_conn);
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
        APP_PRINT_INFO1("app_adp_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_adp_handle_pri_close_case(void)
{
    if (app_bt_policy_get_b2b_connected())
    {
        //this scenario is rdtp linking with b2b acl already exist
        if (b2b_connected_check_count <= MAX_TIMES_CHECK_B2B_CONNECTED_STATE)
        {
            b2b_connected_check_count++;
            gap_start_timer(&timer_handle_adp_wait_rdtp_conn, "wait_rdtp_conn", app_adp_timer_queue_id,
                            APP_TIMER_ADP_CLOSE_CASE_WAIT_RDTP_CONN, 0, 50);
        }
        else
        {
            b2b_connected_check_count = 0;
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
    }
    else
    {
        //b2b disconnected
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }
}

static void app_adp_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_OFF:
        {
            gap_stop_timer(&timer_handle_adp_in_case);
            gap_stop_timer(&timer_handle_adp_wait_roleswap);
            out_case_power_on_led_en = false;
            power_on_wait_when_bt_ready = false;
            app_db.local_in_ear = false;

            if (app_cfg_const.disallow_charging_led_before_power_off == 1)
            {
                app_db.disallow_charging_led = 0;
                APP_PRINT_INFO1("app_adp_dm_cback: disallow_charging_led = %d", app_db.disallow_charging_led);
            }

            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_adp_case_battery_check(&app_db.case_battery, &app_cfg_nv.case_battery);
            }
        }
        break;

    case SYS_EVENT_POWER_ON:
        {
            app_cfg_nv.power_off_cause_cmd = 0;
            case_batt_pre_updated = false;
            app_db.local_in_ear = false;
            in_case_is_power_off = false;

            app_db.adp_high_wake_up_for_zero_box_bat_vol = false;
            if (app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset)
            {
                app_cfg_nv.adp_high_wake_up_credit_for_external_mcu_reset = 0;
                app_cfg_store(&app_cfg_nv.offset_smart_chargerbox, 1);
            }

            if (app_cfg_const.disallow_charging_led_before_power_off == 1)
            {
                app_db.disallow_charging_led = 1;
                app_dlps_enable(APP_DLPS_ENTER_CHECK_CHARGER);
            }

#if (F_APP_AVP_INIT_SUPPORT == 1)
            //rsv for avp
#else
            if (app_db.local_in_case)
            {
                app_audio_spk_mute_unmute(true);
            }
            else
            {
                app_db.local_loc = app_sensor_bud_loc_detect();
                app_sensor_bud_loc_sync();
            }
#endif

            if (app_cfg_const.enable_rtk_charging_box)
            {
                app_adp_case_battery_check(&app_cfg_nv.case_battery, &app_db.case_battery);
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
        APP_PRINT_INFO1("app_adp_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_adp_rtk_in_case_start_timer(void)
{
    APP_PRINT_INFO1("app_adp_rtk_in_case_start_timer: app_db.device_state  %d", app_db.device_state);
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        gap_stop_timer(&timer_handle_adp_in_case);
        gap_start_timer(&timer_handle_adp_in_case, "adp_in_case",
                        app_adp_timer_queue_id, APP_TIMER_ADP_IN_CASE_TIMEOUT,
                        0, ADP_IN_CASE_AUTO_POWER_OFF_MS);
    }
}

void app_adp_rtk_stop_in_case_timer(void)
{
    gap_stop_timer(&timer_handle_adp_in_case);
}

bool app_adp_rtk_in_case_timer_started(void)
{
    return ((timer_handle_adp_in_case != NULL) ? true : false);
}

void app_adp_rtk_out_case_blink_power_on_led(bool led_on)
{
    APP_PRINT_INFO2("app_adp_rtk_out_case_blink_power_on_led: device_state %d, led_on %d",
                    app_db.device_state, led_on);
    if (led_on)
    {
        if (out_case_power_on_led_en)
        {
            out_case_power_on_led_en = false;
            led_change_mode(LED_MODE_POWER_ON, true, false);
        }
    }
    else
    {
        if (app_db.device_state == APP_DEVICE_STATE_ON)
        {
            out_case_power_on_led_en = true;
        }
    }
}

void app_adp_set_ever_pause(bool ever_set)
{
    adp_ever_pause_cmd = ever_set;
}

void app_adp_avrcp_status_handle(void)
{
    uint8_t app_idx = app_get_active_a2dp_idx();

    APP_PRINT_INFO5("app_adp_avrcp_status_handle: %d,%d,%d,%d,%d", app_cfg_nv.bud_role,
                    app_db.local_loc, app_db.remote_loc,
                    adp_ever_pause_cmd, app_db.br_link[app_idx].avrcp_play_status);

    if (((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
         (app_db.local_loc == BUD_LOC_IN_CASE) && (app_db.remote_loc == BUD_LOC_IN_CASE)) ||
        ((app_db.local_loc == BUD_LOC_IN_CASE) &&
         (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)))
    {
        if (app_db.br_link[app_idx].connected_profile & AVRCP_PROFILE_MASK)
        {
            if (app_db.br_link[app_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                if (adp_ever_pause_cmd == false)
                {
                    adp_ever_pause_cmd = true;
                    app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
                }
            }
        }
    }
}

void app_adp_linkback_b2s_when_open_case(void)
{
    APP_PRINT_INFO0("app_adp_linkback_b2s_when_open_case start");

    gap_stop_timer(&timer_handle_adp_close_case);
    gap_stop_timer(&timer_handle_adp_power_off_disc_phone_link);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_bt_policy_restore();
    }
}

void app_adp_stop_power_off_disc_phone_link_timer(void)
{
    gap_stop_timer(&timer_handle_adp_power_off_disc_phone_link);
}

void app_adp_stop_close_case_timer(void)
{
    gap_stop_timer(&timer_handle_adp_close_case);
}

void app_adp_stop_check_disable_charger_timer(void)
{
    gap_stop_timer(&timer_handle_check_adp_cmd_disable_charger);
}

void app_adp_power_on_when_out_case(void)
{
    APP_PRINT_INFO2("app_adp_power_on_when_out_case: app_db.device_state %d, power_on_wait_when_bt_ready %d",
                    app_db.device_state, power_on_wait_when_bt_ready);

    if (!app_cfg_nv.is_dut_test_mode && (power_on_wait_when_bt_ready == false))
    {
        app_mmi_handle_action(MMI_DEV_POWER_ON);
    }

    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
}

void app_adp_cmd_enter_pcba_shipping_mode(void)
{
    if (app_cfg_const.pcba_shipping_mode_pinmux != 0xFF)
    {
        Pad_OutputControlValue(app_cfg_const.pcba_shipping_mode_pinmux, PAD_OUT_HIGH);
    }
}

static bool ota_tooling_function_check(uint8_t payload)
{
    bool rtn = true;
    uint8_t jig_op_code = payload >> 5 & 0x07;
    uint8_t jig_id = payload & 0x1f;
    if ((jig_op_code == APP_ADP_SPECIAL_CMD_NULL) ||
        (jig_op_code == APP_ADP_SPECIAL_CMD_RSV_6) ||
        (jig_op_code == APP_ADP_SPECIAL_CMD_RSV_7))
    {
        rtn = false;
    }

    APP_PRINT_TRACE4("ota_tooling_function_check %d, %d, %d, %d", rtn, payload, jig_op_code, jig_id);
    return rtn;
}

void app_adp_cmd_ota_tooling_nv_store(void)
{
    // keep current status and restart
    // app_cfg_nv.ota_tooling_start = 1;
    app_cfg_nv.ota_tooling_start = 1;
    app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

    app_cfg_nv.jig_subcmd = app_db.jig_subcmd;
    app_cfg_nv.jig_dongle_id = app_db.jig_dongle_id;
    app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
}

static void ota_tooling_function_start(void)
{
    APP_PRINT_INFO2("ota_tooling_function_start %d, %d", app_db.bt_is_ready, app_db.jig_subcmd);

    if (app_db.jig_subcmd == APP_ADP_SPECIAL_CMD_RWS_FORCE_ENGAGE)
    {
        app_mmi_handle_action(MMI_DEV_TOOLING_FACTORY_RESET);
    }
    else if (app_db.jig_subcmd == APP_ADP_SPECIAL_CMD_OTA)
    {
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
        {
            app_relay_async_single(APP_MODULE_TYPE_OTA, APP_REMOTE_MSG_OTA_TOOLING_POWER_OFF);
        }
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }

    app_db.ota_tooling_start = 1;
    app_mmi_modify_reboot_check_times(REBOOT_CHECK_OTA_TOOLING_MAX_TIMES);
    app_mmi_reboot_check_timer_start(2000);
}

void app_adp_cmd_ota_tooling(uint8_t payload)
{
    bool is_valid_op_code = ota_tooling_function_check(payload);
    if (is_valid_op_code == false)
    {
        // get invalid op code of jig, skip it!
        return ;
    }

    if (app_db.executing_charger_box_special_cmd == 0)
    {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
        app_cmd_stop_ota_parking_power_off();
#endif
        app_db.executing_charger_box_special_cmd = app_cfg_nv.jig_subcmd;
        app_db.jig_subcmd = payload >> 5 & 0x07;
        app_db.jig_dongle_id = payload & 0x1f;

        app_dlps_stop_power_down_wdg_timer();

        if (app_db.bt_is_ready)
        {
            ota_tooling_function_start();
        }
        else
        {
            gap_start_timer(&timer_handle_ota_tooling_delay_start, "ota_tooling_delay_start",
                            app_adp_timer_queue_id, APP_TIMER_OTA_TOOLING_DELAY_START, 0, 200);
        }
    }
}

void app_adp_factory_reset_link_dis(uint16_t delay)
{
    gap_start_timer(&timer_handle_adp_factory_reset_link_dis, "adp_facotory_reset_link_dis",
                    app_adp_timer_queue_id,
                    APP_TIMER_ADP_FACTORY_RESET_LINK_DIS_TIMEOUT,
                    0, delay);
}

static void app_adp_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_adp_timeout_cb: timer_id 0x%02x, timer_id %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
    case APP_TIMER_ADP_CLOSE_CASE_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_adp_in_case);
            gap_stop_timer(&timer_handle_adp_close_case);

#if (F_APP_AVP_INIT_SUPPORT == 1)
            if (app_charger_box_stop_hook)
            {
                app_charger_box_stop_hook();
            }
#endif

            app_db.power_off_cause = POWER_OFF_CAUSE_ADP_CLOSE_CASE_TIMEOUT;
            app_adp_power_off();
        }
        break;

    case APP_TIMER_ADP_CMD_RECEIVED_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_adp_cmd_received);
            adp_cmd_rec = AD2B_CMD_NONE;
            adp_cmd_exec = AD2B_CMD_NONE;
            app_adp_clear_pending();
        }
        break;

    case APP_TIMER_ADP_POWER_ON_WAIT_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_adp_power_on_wait);
            power_on_wait_counts++;
            /*wait for power off state*/
            if (((app_db.device_state == APP_DEVICE_STATE_OFF) && app_db.bt_is_ready) ||
                (power_on_wait_counts > 20))
            {
                power_on_wait_counts = 0;

                if (power_on_wait_when_bt_ready)
                {
                    power_on_wait_when_bt_ready = false;
                    app_adp_cmd_power_on_handle();
#if (F_APP_AVP_INIT_SUPPORT == 1)
                    if (app_adp_open_case_hook)
                    {
                        app_adp_open_case_hook();
                    }
#endif
                }

                app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
                app_dlps_enable(APP_DLPS_ENTER_CHECK_ADAPTOR);
            }
            else
            {
                app_adp_delay_power_on_when_bt_ready();
            }
        }
        break;

    case APP_TIMER_DISALLOW_CHARGER_LED_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_disallow_charger_led);
            app_db.disallow_charging_led = 0;
            APP_PRINT_INFO1("app_adp_timeout_cb: disallow_charging_led = %d", app_db.disallow_charging_led);
        }
        break;

    case APP_TIMER_ADP_IN_CASE_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_adp_in_case);
            app_db.power_off_cause = POWER_OFF_CAUSE_ADP_IN_CASE_TIMEOUT;

            bool power_off_directly = true;

#if (F_APP_ERWS_SUPPORT == 1)
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                if (app_db.local_loc == BUD_LOC_IN_CASE && app_db.remote_loc == BUD_LOC_IN_CASE)
                {
                    /* when in case timeout and both buds in case; both buds need power off */
                    app_roleswap_sync_poweroff();
                    power_off_directly = false;
                }
                else
                {
                    if (app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_IN_CASE_TIMEOUT_TO_POWER_OFF))
                    {
                        /* power off after role swap */
                        power_off_directly = false;
                    }
                }
            }
#endif

            if (power_off_directly)
            {
                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
        }
        break;

    case APP_TIMER_ADP_WAIT_ROLESWAP:
        {
            gap_stop_timer(&timer_handle_adp_wait_roleswap);
            app_mmi_handle_action(MMI_DEV_POWER_OFF);//roleswap failed,direct power off
        }
        break;

    case APP_TIMER_ADP_POWER_OFF_DISC_PHONE_LINK:
        {
            uint8_t i = 0;
            uint32_t plan_profs = 0;

            gap_stop_timer(&timer_handle_adp_power_off_disc_phone_link);

            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
            {
                if (app_db.remote_case_closed || (app_db.remote_loc == BUD_LOC_IN_CASE))
                {
                    //When both buds in case, src link will be disconnected 5 sec after close case.
                    for (i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_check_b2s_link_by_id(i))
                        {
                            plan_profs = (app_db.br_link[i].connected_profile & (~RDTP_PROFILE_MASK));
                            app_bt_policy_disconnect(app_db.br_link[i].bd_addr, plan_profs);
                        }
                    }
                }
            }
        }
        break;

    case APP_TIMER_CHECK_ADP_CMD_DISABLE_CHARGER:
        {
            gap_stop_timer(&timer_handle_check_adp_cmd_disable_charger);

            disallow_enable_charger = false;

            if ((app_db.local_loc == BUD_LOC_IN_CASE) &&
                ((app_cfg_nv.case_battery & 0x7F) > app_cfg_const.smart_charger_disable_threshold))
            {
                app_adp_smart_box_charger_control(true, CHARGER_CONTROL_BY_BOX_BATTERY);
            }
        }
        break;

    case APP_TIMER_CHECK_ADP_CMD_CASE_CLOSE:
        {
            gap_stop_timer(&timer_handle_check_adp_cmd_close_case);

            if (app_db.local_loc == BUD_LOC_IN_CASE)
            {
                app_adp_cmd_close_case_start();
                app_adp_other_feature_handle_when_close_case();
            }
        }
        break;

    case APP_TIMER_BOX_ZERO_VOLUME_IGNORE_ADP_OUT:
        {
            gap_stop_timer(&timer_handle_box_zero_volume_ignore_adp_out);

            /* change to adp high wake up before enter power down mode */
            Pad_WakeUpCmd(ADP_MODE, POL_HIGH, ENABLE);

            app_dlps_enable(APP_DLPS_ENTER_CHECK_BOX_BAT);
        }
        break;

    case APP_TIMER_ADP_CLOSE_CASE_WAIT_RDTP_CONN:
        {
            gap_stop_timer(&timer_handle_adp_wait_rdtp_conn);

            app_adp_handle_pri_close_case();
        }
        break;

    case APP_TIMER_ADP_FACTORY_RESET_LINK_DIS_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_adp_factory_reset_link_dis);

            app_cfg_nv.adp_factory_reset_power_on = 1;
            app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

            app_mmi_handle_action(MMI_DEV_FACTORY_RESET);
        }
        break;

    case APP_TIMER_OTA_TOOLING_DELAY_START:
        {
            gap_stop_timer(&timer_handle_ota_tooling_delay_start);
            if (app_db.bt_is_ready)
            {
                ota_tooling_function_start();
            }
            else
            {
                gap_start_timer(&timer_handle_ota_tooling_delay_start, "ota_tooling_delay_start",
                                app_adp_timer_queue_id, APP_TIMER_OTA_TOOLING_DELAY_START, 0, 200);
            }
        }
        break;

    case APP_TIMER_ADP_PLUG_STABLE:
        gap_stop_timer(&timer_handle_adaptor_stable_plug);
        if (app_adp_get_plug_state() == ADAPTOR_PLUG)
        {
            APP_ADP_STATUS status = APP_ADP_STATUS_PLUG_STABLE;
            vector_mapping(app_adp_mgr.cbs_vector, (V_MAPPER)app_adp_trig_cb, &status);
        }
        break;

    case APP_TIMER_ADP_UNPLUG_STABLE:
        gap_stop_timer(&timer_handle_adaptor_stable_unplug);
        if (app_adp_get_plug_state() == ADAPTOR_UNPLUG)
        {
            APP_ADP_STATUS status = APP_ADP_STATUS_UNPLUG_STABLE;
            vector_mapping(app_adp_mgr.cbs_vector, (V_MAPPER)app_adp_trig_cb, &status);
        }
        break;

    case APP_TIMER_POWE_OFF_ENTER_DUT_MODE:
        {
            gap_stop_timer(&timer_handle_power_off_enter_dut_mode);
            app_mmi_handle_action(MMI_DUT_TEST_MODE);
        }
        break;

    case APP_TIMER_ADP_CMD_PROTECT:
        {
            gap_stop_timer(&timer_handle_adp_cmd_protect);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
        }
        break;

    default:
        break;
    }
}

#if F_APP_ERWS_SUPPORT
void app_adp_bud_change_handle(uint8_t event, bool from_remote, uint8_t para)
{
    static T_BUD_LOCATION local_loc_pre = BUD_LOC_UNKNOWN;
    static T_BUD_LOCATION remote_loc_pre = BUD_LOC_UNKNOWN;

    bool local_in_case = false, local_out_case = false, remote_in_case = false, remote_out_case = false;

    if ((local_loc_pre != BUD_LOC_IN_CASE) && (app_db.local_loc == BUD_LOC_IN_CASE))
    {
        local_in_case = true;
    }

    if ((remote_loc_pre != BUD_LOC_IN_CASE) && (app_db.remote_loc == BUD_LOC_IN_CASE))
    {
        remote_in_case = true;
    }

    if ((local_loc_pre == BUD_LOC_IN_CASE) && (app_db.local_loc == BUD_LOC_IN_AIR))
    {
        local_out_case = true;
    }

    if ((remote_loc_pre == BUD_LOC_IN_CASE) && (app_db.remote_loc == BUD_LOC_IN_AIR))
    {
        remote_out_case = true;
    }

    APP_PRINT_INFO6("app_adp_bud_change_handle: event %02x, loc (%d->%d), remote_loc (%d->%d), from_remote %d",
                    event, local_loc_pre, app_db.local_loc,
                    remote_loc_pre, app_db.remote_loc, from_remote);

    if ((app_db.local_loc != BUD_LOC_IN_CASE) || (app_db.remote_loc != BUD_LOC_IN_CASE))
    {
        app_adp_set_ever_pause(false);
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if ((app_db.local_loc == BUD_LOC_IN_CASE) && (app_db.remote_loc == BUD_LOC_IN_CASE))
        {
            app_adp_case_battery_update_reset();
        }

#if (F_APP_AVP_INIT_SUPPORT == 1)
//rsv for avp
#else
        app_adp_avrcp_status_handle();
#endif
    }

    if (!from_remote)
    {
        if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
        {
            if (local_in_case)
            {
                if (app_cfg_const.do_not_auto_power_off_when_case_not_close == false)
                {
                    if (local_loc_pre != BUD_LOC_UNKNOWN)
                    {
                        app_adp_rtk_in_case_start_timer();
                    }
                }

                app_adp_rtk_out_case_blink_power_on_led(false);

                if (app_cfg_const.smart_charger_control)
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_report_rws_bud_info();
                    }

                    app_adp_set_disable_charger_by_box_battery(false);

                    app_adp_smart_box_charger_control(true, CHARGER_CONTROL_BY_BOX_BATTERY);

                    if (app_cfg_nv.report_box_bat_lv_again_after_sw_reset)
                    {
                        /* clear this delay close charger flag if bud out box */
                        app_cfg_nv.report_box_bat_lv_again_after_sw_reset = false;
                        app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
                    }
                }
            }
        }
        else if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
        {
            if (local_out_case)
            {
                if (app_cfg_const.smart_charger_control)
                {
                    app_adp_stop_check_disable_charger_timer();
                    app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_BOX_BATTERY);
                }

                if (app_adp_rtk_in_case_timer_started())
                {
                    app_adp_rtk_stop_in_case_timer();
                }

                app_adp_rtk_out_case_blink_power_on_led(true);

                if (app_db.device_state == APP_DEVICE_STATE_ON)
                {
                    if (app_cfg_const.enable_power_off_immediately_when_close_case == false)
                    {
                        app_adp_linkback_b2s_when_open_case();

                        app_bud_loc_evt_handle(APP_BUD_LOC_EVENT_CASE_OPEN_CASE, 0, 0);
                    }
                    else
                    {
                        app_adp_stop_close_case_timer();
                    }
                }
            }
        }
    }
    else
    {
        if (event == APP_BUD_LOC_EVENT_CASE_IN_CASE)
        {
            if (remote_in_case)
            {
            }
        }
        else if (event == APP_BUD_LOC_EVENT_CASE_OUT_CASE)
        {
            if (remote_out_case)
            {

            }
        }
    }

    local_loc_pre = (T_BUD_LOCATION)app_db.local_loc;
    remote_loc_pre = (T_BUD_LOCATION)app_db.remote_loc;
}
#endif

/**
 * @brief ADP DET TIMx interrupt handler function.
 *
 */
RAM_TEXT_SECTION void adp_timer_isr_callback(T_HW_TIMER_HANDLE handle)
{

#if F_APP_LOCAL_PLAYBACK_SUPPORT //& F_APP_ERWS_SUPPORT
    if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE))
    {
        if (p_adp_cmd_data->usb_start_wait_en == 1)
        {
            // second usb in command not get  start usb
            APP_PRINT_INFO0("APP_ADP_DET_TIMER_HANDLER: USB START");
            p_adp_cmd_data->usb_start_wait_en = 0;
            p_adp_cmd_data->tim_cnt_finish = true;
            T_IO_MSG adp_msg;
            adp_msg.type = IO_MSG_TYPE_GPIO;
            adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_DAT;
            adp_msg.u.param = ADP_USB_IN_PRAR;

            if (!app_io_send_msg(&adp_msg))
            {
                APP_PRINT_ERROR0("[ADP_CMD]APP_ADP_DET_TIMER_HANDLER: Send msg error");
            }
            hw_timer_restart(adp_hw_timer_handle, PERIOD_ADP_TIM);
            return;
        }
    }
#endif
    app_adp_cmd_data_parse_reset(1);
}

RAM_TEXT_SECTION void app_adp_cmd_protect_send_msg(void)
{
    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_SMARTBOX_COMMAND_PROTECT;
    adp_msg.u.param = 0;

    if (!app_io_send_msg(&adp_msg))
    {
        APP_PRINT_ERROR0("app_adp_cmd_protect_send_msg: Send msg error");
    }
}

void app_adp_cmd_protect(void)
{
    gap_start_timer(&timer_handle_adp_cmd_protect, "cmd_protect",
                    app_adp_timer_queue_id, APP_TIMER_ADP_CMD_PROTECT, 0, 6000);
}

/**
 * @brief ADP DET peripheral interrupt handler function.
 *
 */
RAM_TEXT_SECTION  static void app_adp_cmd_of_chargerbox_handler(void)
{
    // This is an protection, to avoid cmd receive failed.
    if (!(app_dlps_get_dlps_bitmap() & APP_DLPS_ENTER_CHECK_CMD_PROTECT))
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
        app_adp_cmd_protect_send_msg();
    }


    if (p_adp_cmd_data == NULL)
    {
        return;
    }

    uint32_t tim_current_value;
    T_ADP_INT_DATA adp_data;

    adp_data.bit_data = adp_get_level(ADP_DETECT_5V);

    if (p_adp_cmd_data->tim_triggle == false)
    {
        if (adp_data.bit_data == 0)
        {
            if (p_adp_cmd_data->usb_in_cmd_det == true)
            {
                app_adp_cmd_data_parse_reset(0);
            }
            else
            {
                app_adp_cmd_data_parse_reset(1); /* code */
            }
            app_dlps_disable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);

            p_adp_cmd_data->tim_triggle = true;
            hw_timer_restart(adp_hw_timer_handle, PERIOD_ADP_TIM);
            p_adp_cmd_data->tim_prev_value = PERIOD_ADP_TIM;
            APP_PRINT_INFO0("app_adp_cmd_of_chargerbox_handler: adp timer start");
            return;
        }
        else
        {
            if (p_adp_cmd_data->tim_triggle == false)
            {
                app_adp_cmd_data_parse_reset(1);
                app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
                APP_PRINT_INFO0("app_adp_cmd_of_chargerbox_handler: tim_triggle == false not valid start signal");
                return;
            }
        }
    }

    hw_timer_get_current_count(adp_hw_timer_handle, &tim_current_value);
    adp_data.tim_delta_value = p_adp_cmd_data->tim_prev_value - tim_current_value;
    p_adp_cmd_data->tim_prev_value = tim_current_value;

#if (ADP_CMD_DBG == 1)
    APP_PRINT_INFO4("app_adp_cmd_of_chargerbox_handler: read [%d], timer = %d->%d, delatT = %d",
                    adp_data.bit_data,
                    p_adp_cmd_data->tim_prev_value,
                    tim_current_value,
                    adp_data.tim_delta_value);
#endif

    if ((adp_data.tim_delta_value >= p_adp_cmd_data->guard_bit_tim) &&
        (adp_data.bit_data == 0)) //guard_bit_got
    {
        p_adp_cmd_data->tim_triggle = true;
        hw_timer_restart(adp_hw_timer_handle, PERIOD_ADP_TIM);
        p_adp_cmd_data->tim_prev_value = PERIOD_ADP_TIM;
    }

    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_ADP_INT;
    memcpy(&adp_msg.u.param, &adp_data, sizeof(adp_msg.u.param));

    if (!app_io_send_msg(&adp_msg))
    {
        APP_PRINT_ERROR0("app_adp_cmd_of_chargerbox_handler: Send msg error");
    }

}

#if (XM_CHARGERBOX_SUPPORT == 0)
void app_adp_int_handle(T_IO_MSG *io_driver_msg_recv)
{
    T_ADP_INT_DATA adp_data;
    uint8_t bit_cnt = 0;
    uint8_t bit_idx = 0;

    memcpy(&adp_data, &io_driver_msg_recv->u.param, sizeof(adp_data));

    app_dlps_disable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);

    if (adp_data.tim_delta_value <= (14 * 1000)) // pules 10ms  15bit 9bit plus is 10ms
    {
        hw_timer_stop(adp_hw_timer_handle);
        app_adp_cmd_data_parse_reset(1);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
    }
    else
    {
        bit_cnt = (adp_data.tim_delta_value + MAX_INACCURACY) /
                  (len_per_bit[app_cfg_const.smart_charger_box_bit_length] * 1000);

        if (bit_cnt >= MAX_BIT_CNT)
        {
            bit_cnt = 16;
        }
        for (bit_idx = 0; bit_idx < bit_cnt; bit_idx++)
        {
            app_adp_cmd_process(!adp_data.bit_data);
            app_adp_cmd_send();
        }
    }

    APP_PRINT_INFO4("app_adp_int_handle: push [%d] x %d, bit_data %d, tim_delta_value %d",
                    (~adp_data.bit_data) & 0x01,
                    bit_cnt,
                    adp_data.bit_data,
                    adp_data.tim_delta_value);
}
#endif

uint8_t app_adp_get_plug_state(void)
{
    return adaptor_plug_in;
}


void app_adp_set_plug_state(uint8_t plug_state)
{
    adaptor_plug_in = plug_state;
    APP_PRINT_TRACE1("app_adp_set_plug_state: adaptor_plug_in %d", adaptor_plug_in);
}


void app_adp_detect(void)
{
    app_cfg_nv.adaptor_changed = 0;

    while (ADP_DET_status == ADP_DET_ING)
    {
        os_delay(10);
    }

    if (ADP_DET_status == ADP_DET_IN)
    {
        adaptor_plug_in = ADAPTOR_PLUG;

        if (!app_cfg_nv.adaptor_is_plugged)
        {
            app_cfg_nv.adaptor_is_plugged = 1;
            app_cfg_nv.adaptor_changed = 1;
        }
    }
    else if (ADP_DET_status == ADP_DET_OUT)
    {
        adaptor_plug_in = ADAPTOR_UNPLUG;

        if (app_cfg_nv.adaptor_is_plugged)
        {
            app_cfg_nv.adaptor_is_plugged = 0;
            app_cfg_nv.adaptor_changed = 1;
        }
    }

    if (app_cfg_nv.adaptor_changed)
    {
        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
        app_cfg_store(&app_cfg_nv.tone_volume_out_level, 8);
    }

    APP_PRINT_TRACE2("app_adp_detect: ADP_DET_status %d, adaptor_plug_in %d", ADP_DET_status,
                     adaptor_plug_in);
}

RAM_TEXT_SECTION static void app_adp_in_send_msg(void)
{
    T_IO_MSG adp_msg;

    app_cfg_nv.adaptor_changed = 0;


    if (!app_cfg_nv.adaptor_is_plugged)
    {
        app_cfg_nv.adaptor_is_plugged = 1;
        app_cfg_nv.adaptor_changed = 1;
        adaptor_plug_in = ADAPTOR_PLUG;

        adp_msg.type = IO_MSG_TYPE_GPIO;
        adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_PLUG;
        adp_msg.u.param = ADAPTOR_PLUG;

        if (!app_io_send_msg(&adp_msg))
        {
            APP_PRINT_ERROR0("audio_app_adp_in_handler: Send msg error");
        }
    }

    APP_PRINT_TRACE2("app_adp_in_send_msg: adaptor_is_plugged %d, adaptor_plug_in %d",
                     app_cfg_nv.adaptor_is_plugged, adaptor_plug_in);
}

RAM_TEXT_SECTION static void app_adp_out_send_msg(void)
{
    T_IO_MSG adp_msg;

    app_cfg_nv.adaptor_changed = 0;

    if (app_cfg_nv.adaptor_is_plugged)
    {
        app_cfg_nv.adaptor_is_plugged = 0;
        adaptor_plug_in = ADAPTOR_UNPLUG;

        adp_msg.type = IO_MSG_TYPE_GPIO;
        adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_UNPLUG;
        adp_msg.u.param = ADAPTOR_UNPLUG;

        if (!app_io_send_msg(&adp_msg))
        {
            APP_PRINT_ERROR0("audio_app_adp_out_handler: Send msg error");
        }
    }

    APP_PRINT_TRACE2("app_adp_out_send_msg: adaptor_is_plugged %d, adaptor_plug_in %d",
                     app_cfg_nv.adaptor_is_plugged, adaptor_plug_in);
}

static void  adp_plug_cback(T_ADP_PLUG_EVENT plug_event, void *user_data)
{
    if (plug_event == ADP_EVENT_PLUG_IN)
    {
        app_adp_in_send_msg();
    }
    else
    {
        app_adp_out_send_msg();
    }
}

static void app_adp_interrupt_interposition(void)
{
    adp_register_state_change_cb(ADP_DETECT_5V, (P_ADP_PLUG_CBACK)adp_plug_cback, NULL);
}


void app_adp_plug_handle(T_IO_MSG *io_driver_msg_recv)
{
    APP_PRINT_TRACE0("app_adp_plug_handle");

#if F_APP_IO_OUTPUT_SUPPORT
    if (app_cfg_const.enable_power_supply_adp_in)
    {
        app_io_output_power_supply(true);
    }
#endif

    app_adp_set_disable_charger_by_box_battery(false);

    if (app_cfg_const.charger_control_by_mcu)
    {
#if F_APP_ADC_SUPPORT
        app_adc_start_monitor_adp_voltage();
#endif
        {
            app_adp_smart_box_charger_control(true, CHARGER_CONTROL_BY_ADP_VOLTAGE);
        }
    }

    {
        uint8_t stable_interval = (app_cfg_const.enable_rtk_charging_box == 0) ?
                                  CHARGERBOX_ADAPTOR_DETECT_TIMER :
                                  SMART_CHARGERBOX_ADAPTOR_DETECT_TIMER;

        gap_stop_timer(&timer_handle_adaptor_stable_unplug);
        gap_start_timer(&timer_handle_adaptor_stable_plug, "adaptor_stable_plug",
                        app_adp_timer_queue_id,
                        APP_TIMER_ADP_PLUG_STABLE, 0, stable_interval);
    }

#if F_APP_LOCAL_PLAYBACK_SUPPORT //& (!F_APP_ERWS_SUPPORT)
    if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        if (app_db.device_state == APP_DEVICE_STATE_OFF)
        {
            app_adp_usb_start_handle();
        }
        else
        {
            if (app_cfg_nv.adaptor_is_plugged)
            {
                app_cfg_nv.usb_need_start = 1;
                app_cfg_store(&app_cfg_nv.pin_code[6], 4);
            }
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
    }
#endif
#if F_APP_USB_AUDIO_SUPPORT
    app_usb_charger_plug_handle_msg(io_driver_msg_recv);
#endif


    APP_ADP_STATUS status = APP_ADP_STATUS_PLUGING;
    vector_mapping(app_adp_mgr.cbs_vector, (V_MAPPER)app_adp_trig_cb, &status);
}



void app_adp_unplug_handle(T_IO_MSG *io_driver_msg_recv)
{
    APP_PRINT_TRACE0("app_adp_unplug_handle");

#if F_APP_IO_OUTPUT_SUPPORT
    if (app_cfg_const.enable_power_supply_adp_in)
    {
        if (app_db.device_state != APP_DEVICE_STATE_ON)
        {
            app_io_output_power_supply(false);
        }
    }
#endif

    if (app_cfg_const.charger_control_by_mcu)
    {
#if F_APP_ADC_SUPPORT
        app_adc_stop_monitor_adp_voltage();
#endif
        {
            app_adp_smart_box_charger_control(false, CHARGER_CONTROL_BY_ADP_VOLTAGE);
        }
    }

    {
        uint8_t stable_interval = (app_cfg_const.enable_rtk_charging_box == 0) ?
                                  CHARGERBOX_ADAPTOR_DETECT_TIMER :
                                  SMART_CHARGERBOX_ADAPTOR_DETECT_TIMER;

        gap_stop_timer(&timer_handle_adaptor_stable_plug);

        gap_start_timer(&timer_handle_adaptor_stable_unplug, "adaptor_stable_unplug",
                        app_adp_timer_queue_id,
                        APP_TIMER_ADP_UNPLUG_STABLE, 0, stable_interval);
    }

#if F_APP_LOCAL_PLAYBACK_SUPPORT //& (!F_APP_ERWS_SUPPORT)
    if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        app_adp_usb_stop_handle();
    }
#endif

#if F_APP_USB_AUDIO_SUPPORT
    app_usb_charger_unplug_handle_msg(io_driver_msg_recv);
#endif

    APP_ADP_STATUS status = APP_ADP_STATUS_UNPLUGING;
    vector_mapping(app_adp_mgr.cbs_vector, (V_MAPPER)app_adp_trig_cb, &status);
}


void app_adp_init(void)
{
    app_adp_interrupt_interposition();

    Pad_WakeUpCmd(ADP_MODE, POL_HIGH, ENABLE);

    if (app_adp_mgr.cbs_vector == NULL)
    {
        app_adp_mgr.cbs_vector = vector_create(0xff);
    }

    gap_reg_timer_cb(app_adp_timeout_cb, &app_adp_timer_queue_id);

    if (app_cfg_const.enable_rtk_charging_box)
    {
        app_adp_int_init();
        app_adp_timer_init();
        bt_mgr_cback_register(app_adp_bt_cback);
        sys_mgr_cback_register(app_adp_dm_cback);
    }
    else
    {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
        app_adp_int_init();
        app_adp_timer_init();

        // Since support rtk charging box is disabled
        // We should set those value.
        app_cfg_const.smart_charger_box_cmd_set = CHARGER_BOX_CMD_SET_15BITS;
        app_cfg_const.smart_charger_box_bit_length = 1;   // 1: 20ms, 0 40ms
#endif
    }
}
