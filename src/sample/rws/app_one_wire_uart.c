/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include <stdlib.h>
#include <string.h>
#include "rtl876x_pinmux.h"
#include "rtl876x_uart.h"
#include "platform_utils.h"
#include "os_sched.h"
#include "os_mem.h"
#include "gap_timer.h"
#include "app_one_wire_uart.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_mmi.h"
#include "app_bt_policy_api.h"
#include "app_audio_policy.h"
#include "app_adp.h"
#include "app_bond.h"
#include "ble_ext_adv.h"
#include "audio_probe.h"
#include "app_roleswap.h"
#include "bt_bond.h"
#include "pm.h"
#include "sysm.h"

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)

typedef enum
{
    APP_TIMER_ONE_WIRE_ONE_WIRE_TIMEOUT,
    APP_TIMER_ONE_WIRE_DELAY_WDG_RESET,
    APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT,
} T_APP_ONE_WIRE_TIMER;

T_ONE_WIRE_UART_STATE one_wire_state = ONE_WIRE_STATE_STOPPED;
static T_ONE_WIRE_UART_TYPE cur_uart_type = ONE_WIRE_UART_RX;

static void *timer_handle_one_wire_aging_test_tone_repeat = NULL;
static void *timer_handle_one_wire_timeout = NULL;
static uint8_t app_one_wire_timer_queue_id = 0;

static uint8_t aging_test_adv_handle = 0xff;
T_ONE_WIRE_AGING_TEST_STATE one_wire_aging_test_state = ONE_WIRE_AGING_TEST_STATE_STOPED;

static void app_one_wire_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_one_wire_timeout_cb: timer_id %d, timer chann %d ", timer_id,
                     timer_chann);
    switch (timer_id)
    {
    case APP_TIMER_ONE_WIRE_ONE_WIRE_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_one_wire_timeout);
            //app_one_wire_deinit();
        }
        break;

    case APP_TIMER_ONE_WIRE_DELAY_WDG_RESET:
        {
            gap_stop_timer(&timer_handle_one_wire_timeout);
            WDG_SystemReset(RESET_ALL);
        }
        break;

    case APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT:
        {
            gap_stop_timer(&timer_handle_one_wire_aging_test_tone_repeat);

            app_audio_tone_type_play(TONE_AUDIO_EQ_9, false, false);

            gap_start_timer(&timer_handle_one_wire_aging_test_tone_repeat, "one_wire_aging_test_tone_repeat",
                            app_one_wire_timer_queue_id, APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT,
                            0, 1000);
        }
        break;
    }
}

void app_one_wire_timeout_start(void)
{
    gap_stop_timer(&timer_handle_one_wire_timeout);
    gap_start_timer(&timer_handle_one_wire_timeout, "one_wire_timeout",
                    app_one_wire_timer_queue_id, APP_TIMER_ONE_WIRE_ONE_WIRE_TIMEOUT, 0, 250);
}

void app_one_wire_uart_switch_pinmiux(T_ONE_WIRE_UART_TYPE type)
{
    APP_PRINT_TRACE2("app_one_wire_uart_switch_pinmiux: %d -> %d", cur_uart_type,
                     type);

    if (cur_uart_type == type)
    {
        return;
    }
    else
    {
        cur_uart_type = type;
    }

    if (type == ONE_WIRE_UART_TX)
    {
        Pinmux_Config(app_cfg_const.one_wire_uart_data_pinmux, UART0_TX);
        Pad_Config(app_cfg_const.one_wire_uart_data_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        Pad_PullConfigValue(app_cfg_const.one_wire_uart_data_pinmux, PAD_WEAKLY_PULL);

        UART_INTConfig(UART0, UART_INT_RD_AVA | UART_INT_IDLE, DISABLE);
    }
    else
    {
        Pinmux_Config(app_cfg_const.one_wire_uart_data_pinmux, UART0_RX);
        Pad_Config(app_cfg_const.one_wire_uart_data_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        Pad_PullConfigValue(app_cfg_const.one_wire_uart_data_pinmux, PAD_WEAKLY_PULL);

        UART_INTConfig(UART0, UART_INT_RD_AVA | UART_INT_IDLE, ENABLE);
    }
}

void app_one_wire_start_aging_test(void)
{
    one_wire_aging_test_state = ONE_WIRE_AGING_TEST_STATE_TESTING;

    /* Start BLE aging test adv with minumum adv interval */
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop =
        LE_EXT_ADV_LEGACY_ADV_NON_SCAN_NON_CONN_UNDIRECTED;
    uint16_t adv_interval_min = 0x20;
    uint16_t adv_interval_max = 0x20;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    ble_ext_adv_mgr_init_adv_params(&aging_test_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 0, NULL,
                                    0, NULL, NULL);
    ble_ext_adv_mgr_change_adv_tx_power(aging_test_adv_handle, 10);

    if (ble_ext_adv_mgr_get_adv_state(aging_test_adv_handle) == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        ble_ext_adv_mgr_enable(aging_test_adv_handle, 0);
    }

    /* Push VP with maximum volume */
    voice_prompt_volume_set(app_cfg_const.voice_prompt_volume_max);

    app_audio_tone_type_play(TONE_AUDIO_EQ_9, false, false);

    gap_start_timer(&timer_handle_one_wire_aging_test_tone_repeat, "one_wire_aging_test_tone_repeat",
                    app_one_wire_timer_queue_id, APP_TIMER_ONE_WIRE_AGING_TEST_TONE_REPEAT,
                    0, 1000);

    /* Enable microphone bias*/
    audio_probe_codec_micbias_set(ENABLE);
}

void app_one_wire_stop_aging_test(bool aging_test_done)
{
    one_wire_aging_test_state = ONE_WIRE_AGING_TEST_STATE_STOPED;

    if (aging_test_done)
    {
        app_cfg_nv.one_wire_aging_test_done = true;

        app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);
    }

    /* Stop BLE aging test adv*/
    ble_ext_adv_mgr_disable(aging_test_adv_handle, 0);

    /* Reset to default tone/VP volume */
    gap_stop_timer(&timer_handle_one_wire_aging_test_tone_repeat);

    app_audio_tone_type_stop();

    voice_prompt_volume_set(app_cfg_const.voice_prompt_volume_default);

    /* Disable microphone bias*/
    audio_probe_codec_micbias_set(DISABLE);

    app_device_reboot(1000, RESET_ALL);
}

T_ONE_WIRE_AGING_TEST_STATE app_one_wire_get_aging_test_state(void)
{
    return one_wire_aging_test_state;
}

void app_one_wire_enter_shipping_mode(void)
{
    app_cfg_nv.need_set_lps_mode = 1;

    power_mode_set(POWER_POWEROFF_MODE);
    app_cfg_nv.ota_parking_lps_mode = OTA_TOOLING_SHIPPING_5V_WAKEUP_ONLY;

    app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);

    // clear phone record
    app_bond_clear_non_rws_keys();

    // power off
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);

        gap_start_timer(&timer_handle_one_wire_timeout, "one_wire_delay_wdg_reset",
                        app_one_wire_timer_queue_id, APP_TIMER_ONE_WIRE_DELAY_WDG_RESET, NULL, 6000);
    }
}

void app_one_wire_reset_to_uninitial_state(void)
{
    bt_bond_clear();

    memset(app_cfg_nv.bud_peer_addr, 0, 6);
    memset(app_cfg_nv.bud_peer_factory_addr, 0, 6);
    app_cfg_nv.factory_reset_done = 0;

    app_cfg_reset();

    // for auto enter pairing after reboot
    app_cfg_nv.one_wire_reset_to_uninitial_state = 1;
    app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);

    // for auto power on after reboot
    app_cfg_nv.auto_power_on_after_factory_reset = 1;
    app_cfg_store(&app_cfg_nv.offset_is_dut_test_mode, 1);

    app_device_reboot(2000, RESET_ALL);
}


void app_one_wire_start_force_engage(uint8_t *target_bud_addr)
{
    memcpy(app_cfg_nv.bud_peer_factory_addr, target_bud_addr, 6);

    if (app_db.device_state != APP_DEVICE_STATE_OFF)
    {
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }

    app_bond_clear_non_rws_keys();
    app_device_factory_reset();

    // for auto power on after reboot
    app_cfg_nv.auto_power_on_after_factory_reset = 1;

    app_cfg_nv.one_wire_start_force_engage = 1;

    // save all config to protect
    app_cfg_store_all();

    app_device_reboot(2000, RESET_ALL);
}

void app_one_wire_data_uart_handler(void)
{
    uint32_t int_status = 0;
    int_status = UART_GetIID(UART0);

    APP_PRINT_TRACE2("app_one_wire_data_uart_handler: int_status %d, TSR_EMPTY %d", int_status,
                     UART_GetFlagState(UART0, UART_FLAG_THR_TSR_EMPTY));

    /* disable UART_INT_FIFO_EMPTY interrupt first */
    UART_INTConfig(UART0, UART_INT_FIFO_EMPTY, DISABLE);

    if (UART_GetFlagState(UART0, UART_FLAG_RX_IDLE) == SET)
    {
        /* uart RX idle */
    }
    else if (int_status == UART_INT_ID_TX_EMPTY)
    {
        /* switch to uart Rx when Tx empty */
        UART_INTConfig(UART0, UART_INT_IDLE, DISABLE);
        if (UART_GetFlagState(UART0, UART_FLAG_THR_TSR_EMPTY) == SET)
        {
            app_one_wire_uart_switch_pinmiux(ONE_WIRE_UART_RX);
        }
        UART_INTConfig(UART0, UART_INT_IDLE, ENABLE);

        /* Wait Tx empty then deinit one-wire uart if need to deinit */
        if (one_wire_state == ONE_WIRE_STATE_STOPPING)
        {
            //app_one_wire_deinit();
        }
    }
}

void app_one_wire_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                             uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_FORCE_ENGAGE:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            app_one_wire_start_force_engage(&cmd_ptr[2]);
        }
        break;

    case CMD_AGING_TEST_CONTROL:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            uint8_t action = cmd_ptr[2];
            uint8_t temp_buff[2] = {0};

            switch (action)
            {
            case CMD_AGING_TEST_ACTION_CHECK_STATE:
                {
                    /* do nothing just report status */
                }
                break;

            case CMD_AGING_TEST_ACTION_START:
                {
                    if (app_cfg_nv.one_wire_aging_test_done == false)
                    {
                        app_one_wire_start_aging_test();
                    }
                }
                break;

            case CMD_AGING_TEST_ACTION_STOP:
                {
                    if (app_one_wire_get_aging_test_state() == ONE_WIRE_AGING_TEST_STATE_TESTING)
                    {
                        app_one_wire_stop_aging_test(false);
                    }
                }
                break;

            case CMD_AGING_TEST_ACTION_DONE:
                {
                    app_one_wire_stop_aging_test(true);
                }
                break;
            }

            temp_buff[0] = app_cfg_nv.one_wire_aging_test_done;
            temp_buff[1] = app_one_wire_get_aging_test_state();

            app_report_event(CMD_PATH_UART, EVENT_AGING_TEST_CONTROL, 0, temp_buff, sizeof(temp_buff));
        }
        break;

    default:
        {
            ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
    }
}

static void app_one_wire_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            app_cfg_nv.one_wire_start_force_engage = 0;
            app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_one_wire_bt_cback: event_type 0x%04x", event_type);
    }

}

static void app_one_wire_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            if (app_cfg_nv.one_wire_reset_to_uninitial_state)
            {
                app_bt_policy_enter_pairing_mode(false, true);
                app_cfg_nv.one_wire_reset_to_uninitial_state = 0;
                app_cfg_store(&app_cfg_nv.offset_one_wire_aging_test_done, 1);
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_one_wire_dm_cback: event_type 0x%04x", event_type);
    }

}

void app_one_wire_init(void)
{
    if (one_wire_state != ONE_WIRE_STATE_STOPPED)
    {
        return;
    }

    one_wire_state = ONE_WIRE_STATE_IN_ONE_WIRE;

    APP_PRINT_TRACE0("app_one_wire_init");

    bt_mgr_cback_register(app_one_wire_bt_cback);
    sys_mgr_cback_register(app_one_wire_dm_cback);

    cur_uart_type = ONE_WIRE_UART_RX;
    Pinmux_Config(app_cfg_const.one_wire_uart_data_pinmux, UART0_RX);
    Pad_Config(app_cfg_const.one_wire_uart_data_pinmux,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PullConfigValue(app_cfg_const.one_wire_uart_data_pinmux, PAD_WEAKLY_PULL);

    // uart_enable
    UART_INTConfig(UART0, UART_INT_RD_AVA | UART_INT_IDLE, ENABLE);

    gap_reg_timer_cb(app_one_wire_timeout_cb, &app_one_wire_timer_queue_id);

    platform_delay_ms(5);
    if (app_cfg_const.one_wire_uart_gpio_support)
    {
        Pad_Config(app_cfg_const.one_wire_uart_gpio_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, ONE_WIRE_GPIO_COM_PULL, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
    //app_onewire_timeout_start();
}

void app_one_wire_deinit(void)
{
    if (one_wire_state == ONE_WIRE_STATE_STOPPED)
    {
        return;
    }

    one_wire_state = ONE_WIRE_STATE_STOPPED;

    gap_stop_timer(&timer_handle_one_wire_timeout);

    if (app_cfg_const.one_wire_uart_gpio_support)
    {
        Pad_Config(app_cfg_const.one_wire_uart_gpio_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, ONE_WIRE_GPIO_5V_PULL, PAD_OUT_DISABLE, PAD_OUT_LOW);
        platform_delay_ms(10);

        Pad_Config(app_cfg_const.one_wire_uart_gpio_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, ONE_WIRE_GPIO_COM_PULL, PAD_OUT_DISABLE, PAD_OUT_LOW);
        platform_delay_ms(10);

        Pad_Config(app_cfg_const.one_wire_uart_gpio_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, ONE_WIRE_GPIO_5V_PULL, PAD_OUT_DISABLE, PAD_OUT_LOW);
        platform_delay_ms(10);
    }

    APP_PRINT_TRACE0("app_one_wire_deinit");

    Pinmux_Config(app_cfg_const.one_wire_uart_data_pinmux, IDLE_MODE);
    Pad_Config(app_cfg_const.one_wire_uart_data_pinmux,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    // uart_disable
    UART_INTConfig(UART0, UART_INT_RD_AVA | UART_INT_IDLE, DISABLE);

    if (app_cfg_const.one_wire_uart_gpio_support)
    {
        Pad_Config(app_cfg_const.one_wire_uart_gpio_pinmux,
                   PAD_SW_MODE, PAD_IS_PWRON, ONE_WIRE_GPIO_5V_PULL, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
}
#endif
