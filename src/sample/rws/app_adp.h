/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_ADP_H_
#define _APP_ADP_H_

#include "app_msg.h"
#include <stdbool.h>
#include "app_msg.h"
#include "app_charger.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_ADP_COMMAND APP Adaptor Command
  * @brief APP Adaptor Command
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup AUDIO_SMART_ADP_COMMAND_Exported_Macros Audio smart adp command Macros
    * @{
    */

/*timer counter must be a multiple of 40ms  MAX_INACCURACY is max inaccuracy */
#define ADP_CMD_DBG                         0

/******************************************************************************/
/*40Mhz 40 devide, set 1ms period=1000 */
//#define PERIOD_ADP_TIM                  400000// 400 000=400ms
#define PERIOD_ADP_TIM                      2000000 // 2s
#define GUART_BIT_TIM                       320000 //320ms
#define GUART_BIT_TIM_TOLERANCE             50000
#define MAX_INACCURACY                      9000

//tim period = TEST_TIM_PERIOD * (clock_source/divider_value)
#define TEST_TIM_PERIOD                     (400000)

#define PULSE_LAST_TIME                     1500

/*usb in out */
#define ADP_USB_IN_PLAYLOAD                 0x05
#define ADP_USB_OUT                         0x0a70
#define ADP_USB_IN_PRAR                     0x0a05

// Guard BIT, now we just use(6 bits), Spec define 8 bits.
#define GUARD_BIT                           0x3F

#define MAX_BIT_CNT                         32

#define ADP_PAYLOAD_AVP_OPEN       0x55        // avp open
#define ADP_PAYLOAD_AVP_CLOSE      0x65        // avp close

/******************************************************************************/
/** End of AUDIO_SMART_ADP_COMMAND_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup AUDIO_SMART_ADP_COMMAND_Exported_Types Audio smart adp command Types
    * @{
    */

typedef enum
{
    APP_ADP_STATUS_PLUGING          = 0,
    APP_ADP_STATUS_UNPLUGING        = 1,
    APP_ADP_STATUS_PLUG_STABLE      = 2,
    APP_ADP_STATUS_UNPLUG_STABLE    = 3,
} APP_ADP_STATUS;



typedef enum _CHARGER_BOX_CMD_SET_
{
    CHARGER_BOX_CMD_SET_9BITS = 0,
    CHARGER_BOX_CMD_SET_15BITS = 1,
    CHARGER_BOX_CMD_SET_RSV1,
    CHARGER_BOX_CMD_SET_RSV2
} CHARGER_BOX_CMD_SET;

typedef struct
{
    uint32_t stop_bit: 1;
    uint32_t parity_bit: 1;
    uint32_t cmd_data: 6;
    uint32_t start_bit: 1;
    uint32_t reserved_bit: 23;
} T_APP_ADP_CMDPACK09;

typedef struct
{
    uint32_t stop_bit: 1;
    uint32_t parity_bit: 1;
    uint32_t cmd_data: 12;
    uint32_t start_bit: 1;
    uint32_t reserved_bit: 1;
} T_APP_ADP_CMDPACK15;

typedef union
{
    uint32_t bit_data;

    T_APP_ADP_CMDPACK09 cmdpack09;
    T_APP_ADP_CMDPACK15 cmdpack15;
} T_APP_ADP_CMD_DECODE;

typedef struct _T_ADP_CMD_PARSE_STRUCT
{
    uint8_t tim_triggle : 1;
    uint8_t tim_cnt_finish : 1;
    uint8_t usb_start_wait_en : 1;
    uint8_t usb_in_cmd_det : 1;
    uint8_t usb_started : 1;
    uint8_t usb_cmd_rec : 1;
    uint8_t rev3 : 1;
    uint8_t usb_start_wait_tim_cnt;
    uint32_t tim_prev_value;
    uint32_t guard_bit_tim ;
    uint32_t cmd_parity09_cnt;
    uint32_t cmd_parity15_cnt;
    uint32_t cmd_out09;
    uint32_t cmd_out15;
    T_APP_ADP_CMD_DECODE cmd_decode;
} T_ADP_CMD_PARSE_STRUCT;


/**  @brief CHARGERBOX_SPECIAL_COMMAND type */
typedef enum
{
    APP_ADP_SPECIAL_CMD_NULL                = 0x00, // This CMD should not be used.
    APP_ADP_SPECIAL_CMD_CREATE_SPP_CON      = 0x01, // Create SPP connection
    APP_ADP_SPECIAL_CMD_FACTORY_MODE        = 0x02,
    APP_ADP_SPECIAL_CMD_RWS_STATUS_CHECK    = 0x03, // Get RWS engaged status
    APP_ADP_SPECIAL_CMD_OTA                 = 0x04, // Special CMD for OTA
    APP_ADP_SPECIAL_CMD_RWS_FORCE_ENGAGE    = 0x05, // Force RWS re-engaged with the same dongle ID
    APP_ADP_SPECIAL_CMD_RSV_6               = 0x06,
    APP_ADP_SPECIAL_CMD_RSV_7               = 0x07,
} T_APP_ADP_SPECIAL_CMD;

typedef enum
{
    OTA_TOOLING_POWER_DONE                  = 0,
    OTA_TOOLING_SHIPPING_BOTH               = 1, /*Wake up by both 5V or MFB*/
    OTA_TOOLING_SHIPPING_5V_WAKEUP_ONLY     = 2, /*Wake up by both 5V */
    OTA_TOOLING_SHIPPING_MFB_WAKEUP_ONLY    = 3, /*Wake up by both MFB*/
} T_OTA_DLPS_SETTING;


typedef void (*APP_ADP_CB)(APP_ADP_STATUS status);

#if (F_APP_AVP_INIT_SUPPORT == 1)
extern void (*app_adp_close_case_hook)(void);
extern void (*app_adp_enter_pairing_hook)(void);
extern void (*app_adp_open_case_hook)(void);
extern void (*app_adp_factory_reset_hook)(void);
extern void (*app_cfg_reset_hook)(void);
extern void (*app_adp_dut_mode_hook)(uint8_t);
extern void (*app_adp_battery_change_hook)(void);
#endif

/******************************************************************************/

/**
    * @brief  adp detect init
    * @param  void
    * @return void
    */
void app_adp_init(void);

/**
    * @brief  adp cmd received
    * @param  void
    * @return ture or false for cmd
    */
uint8_t app_adp_cmd_received(void);

/**
 * @brief set cmd received.
 * @param  cmd received
 * @return void
 */
void app_adp_set_cmd_received(uint8_t cmd_received);

/**
    * @brief  adp pending cmd clear
    * @param  void
    * @return void
    */
void app_adp_clear_pending(void);

#if F_APP_LOCAL_PLAYBACK_SUPPORT
/**
    * @brief  adp usb start
    * @param  void
    * @return void
    */
void app_adp_usb_start_handle(void);
/**
    * @brief  adp usb stop
    * @param  void
    * @return void
    */
void app_adp_usb_stop_handle(void);
#endif

/**
    * @brief  adp pending cmd execute
    * @param  void
    * @return void
    */
void app_adp_pending_cmd_exec(void);
/**
    * @brief  adp specia cmd execute
    * @param  void
    * @return void
    */
void app_adp_special_cmd_handle(uint8_t jig_subcmd, uint8_t jig_dongle_id);

/**
    * @brief  adp msg handle
    * @param  msg
    * @return void
    */
void app_adp_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  adp in case start timer
    * @param  void
    * @return void
    */
void app_adp_rtk_in_case_start_timer(void);

/**
    * @brief  adp in case timer started
    * @param  void
    * @return void
    */
bool app_adp_rtk_in_case_timer_started(void);

/**
    * @brief  stop in case timer
    * @param  void
    * @return void
    */
void app_adp_rtk_stop_in_case_timer(void);

/**
    * @brief  display power on led for rtk out case
    * @param  led_on whether need to display power on led
    * @return void
    */
void app_adp_rtk_out_case_blink_power_on_led(bool led_on);

/**
    * @brief  set ever pause
    * @param  ever_set
    * @return void
    */
void app_adp_set_ever_pause(bool ever_set);

/**
    * @brief  avrcp status handle: music will be paused
    *         when rws not connect but bud inbox or rws both bud inbox
    * @param  void
    * @return void
    */
void app_adp_avrcp_status_handle(void);

/**
    * @brief  linkback b2s when open case
    * @param  void
    * @return void
    */
void app_adp_linkback_b2s_when_open_case(void);

/**
    * @brief  stop power off disc phone link timer
    * @param  void
    * @return void
    */
void app_adp_stop_power_off_disc_phone_link_timer(void);

/**
    * @brief  stop close case timer
    * @param  void
    * @return void
    */
void app_adp_stop_close_case_timer(void);

/**
    * @brief  battery valid check
    * @param  battery in/out
    * @return is valid battery
    */
bool app_adp_case_battery_check(uint8_t *bat_in, uint8_t *bat_out);

/**
    * @brief  reset case battery updated flag
    * @param  void
    * @return void
    */
void app_adp_case_battery_update_reset(void);

/**
    * @brief  smart box charger control
    * @param  enable whether enable charger
    * @param  type charger tpye
    * @return void
    */
void app_adp_smart_box_charger_control(bool enable, T_CHARGER_CONTROL_TYPE type);

/**
    * @brief  battery valid check
    * @param  void
    * @return void
    */
void app_adp_delay_charger_enable(void);

/**
    * @brief  delay power on when out case
    * @param  void
    * @return void
    */
void app_adp_power_on_when_out_case(void);

/**
    * @brief  open case cmd check
    * @param  void
    * @return void
    */
void app_adp_open_case_cmd_check(void);

/**
    * @brief  close case cmd check
    * @param  void
    * @return void
    */
void app_adp_close_case_cmd_check(void);

void app_adp_load_adp_high_wake_up(void);

void app_adp_smart_box_update_battery_lv(uint8_t payload, bool report_flag);


/**
    * @brief  check whether adaptor is pluged or not.
    * @param  void
    * @return @ref T_ADAPTOR_STATUS.
    */
uint8_t app_adp_get_plug_state(void);

/**
    * @brief  set adaptor to plug or unplug.
    * @param  plug_state plug or unplug
    * @return none.
    */
void app_adp_set_plug_state(uint8_t plug_state);


void app_adp_plug_handle(T_IO_MSG *io_driver_msg_recv);


void app_adp_unplug_handle(T_IO_MSG *io_driver_msg_recv);

void app_adp_wdg_rst_stop_timer(void);

void app_adp_int_handle(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  check adaptor plug/unplug state
    * @param  void
    * @return none
    */
void app_adp_detect(void);

/**
    * @brief  stop adaptor plug timer
    * @param  void
    * @return none
    */
void app_adp_stop_plug(void);


bool app_adp_cb_reg(APP_ADP_CB  cb);

/**
    * @brief  stop close case timer
    * @param  void
    * @return void
    */
void app_adp_stop_check_disable_charger_timer(void);

/**

    * @brief  inbox wake up
    * @param  void
    * @return none
    */
void app_adp_inbox_wake_up(void);

/**
    * @brief  factory rst link disconnect delay
    * @param  delay time
    * @return none
    */
void app_adp_factory_reset_link_dis(uint16_t delay);

/**
    * @brief  check charger plug debounce timer.
    *
    * @param  void
    * @return timer started if true
    */
bool app_adp_plug_debounce_timer_started(void);

/**
    * @brief  check charger unplug debounce timer.
    *
    * @param  void
    * @return timer started if true
    */
bool app_adp_unplug_debounce_timer_started(void);

/**
    * @brief  ota tooling.
    *
    * @param  payload
    * @return void
    */
void app_adp_cmd_ota_tooling(uint8_t payload);

void app_adp_cmd_ota_tooling_nv_store(void);

/**
    * @brief  bud change handle
    * @param  event
    * @param  from_remote
    * @param  para
    * @return none
    */
void app_adp_bud_change_handle(uint8_t event, bool from_remote, uint8_t para);

/**
    * @brief  cmd protect
    * @param  void
    * @return none
    */
void app_adp_cmd_protect(void);

/**
    * @brief enter pcba shipping mode
    * @return none
    */
void app_adp_cmd_enter_pcba_shipping_mode(void);

void app_adp_set_disable_charger_by_box_battery(bool flag);

/******************************************************************************/
/** @} */ /* End of group APP_ADP_Exported_Functions */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_ADP_DAT_H_ */
