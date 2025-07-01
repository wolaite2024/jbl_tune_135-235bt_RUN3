/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "os_mem.h"
#include "os_timer.h"
#include "os_sync.h"
#include "console.h"
#include "gap.h"
#include "gap_bond_legacy.h"
#include "trace.h"
#include "bt_types.h"
#include "rtl876x_gpio.h"
#include "gap_timer.h"
#include "transmit_service.h"
#include "bt_iap.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_ble_gap.h"
#include "app_ble_service.h"
#include "app_gpio.h"
#include "app_cmd.h"
#include "app_cfg.h"
#include "app_dlps.h"
#include "bt_spp.h"
#include "app_sdp.h"

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
#include "app_one_wire_uart.h"
#endif

#define UART_TX_QUEUE_NO                    16
#define DT_QUEUE_NO                         16

#define DT_STATUS_IDLE                      0
#define DT_STATUS_ACTIVE                    1

typedef enum
{
    APP_TRANSFER_TIMER_LAUNCH_APP,
    APP_TRANSFER_TIMER_UART_ACK_TO,
    APP_TRANSFER_TIMER_UART_WAKE_UP,
    APP_TRANSFER_TIMER_UART_TX_WAKE_UP,
    APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER,
} T_APP_TRANSFER_TIMER;

typedef struct
{
    uint8_t     *pkt_ptr;
    uint16_t    pkt_len;
    uint8_t     active;
    uint8_t     link_idx;
} T_DT_QUEUE;

typedef struct
{
    uint8_t     dt_queue_w_idx;
    uint8_t     dt_queue_r_idx;
    uint8_t     dt_resend_count;
    uint8_t     dt_status;
} T_DT_QUEUE_CTRL;

typedef struct
{
    uint8_t     *packet_ptr;
    uint16_t    packet_len;
    uint8_t     active;
    uint8_t     extra_param;
} T_UART_TX_QUEUE;

static uint8_t app_transfer_timer_queue_id = 0;

static void *timer_handle_launch_app[MAX_BR_LINK_NUM] = {NULL};
static void *timer_handle_uart_ack = NULL;
static void *timer_handle_uart_wake_up = NULL;
static void *timer_handle_tx_wake_up = NULL;
static void *timer_handle_data_transfer = NULL;

static T_DT_QUEUE_CTRL     dt_queue_ctrl;
static T_DT_QUEUE          dt_queue[DT_QUEUE_NO];

static T_UART_TX_QUEUE uart_tx_queue[UART_TX_QUEUE_NO];
static uint8_t uart_tx_rIndex;                 /**<uart transfer packet read index*/
static uint8_t uart_tx_wIndex;                 /**<uart transfer packet write index*/
static uint8_t uart_resend_count;              /**<uart resend count*/

// for CMD_LEGACY_DATA_TRANSFER and CMD_LE_DATA_TRANSFER
static uint8_t *uart_rx_dt_pkt_ptr = NULL;
static uint16_t uart_rx_dt_pkt_len;

void app_transfer_queue_recv_ack_check(uint16_t event_id, uint8_t cmd_path)
{
    uint16_t    tx_queue_id = ((dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[4]) |
                               (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[5] << 8));

    bool move_to_next = (event_id == tx_queue_id) ? true : false;

    app_pop_data_transfer_queue(cmd_path, move_to_next);
}

void app_transfer_queue_reset(uint8_t cmd_path)
{
    if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_LE || cmd_path == CMD_PATH_IAP)
    {
        gap_stop_timer(&timer_handle_data_transfer);

        for (uint8_t idx = 0 ; idx < DT_QUEUE_NO; idx++)
        {
            if (dt_queue[idx].active)
            {
                dt_queue[idx].active = 0;

                if (dt_queue[idx].pkt_ptr != NULL)
                {
                    free(dt_queue[idx].pkt_ptr);
                    dt_queue[idx].pkt_ptr = NULL;
                }
            }
        }

        dt_queue_ctrl.dt_queue_w_idx = 0;
        dt_queue_ctrl.dt_queue_r_idx = 0;
        dt_queue_ctrl.dt_resend_count = 0;
        dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
    }
}

void app_pop_data_transfer_queue(uint8_t cmd_path, bool next_flag)
{
    T_UART_TX_QUEUE *p_queue;
    uint8_t     app_idx;
    uint8_t     *pkt_ptr;
    uint16_t    pkt_len;
    uint16_t    event_id;

    APP_PRINT_TRACE2("app_pop_data_transfer_queue: cmd_path %d, next_flag %d", cmd_path, next_flag);

    if (CMD_PATH_UART == cmd_path)
    {
        T_UART_TX_QUEUE *tx_queue;
        gap_stop_timer(&timer_handle_uart_ack);

        if (next_flag)
        {
            tx_queue = &(uart_tx_queue[uart_tx_rIndex]);
            if (tx_queue->active)
            {
                uart_resend_count = 0;
                event_id = ((tx_queue->packet_ptr[3]) | (tx_queue->packet_ptr[4] << 8));

                if (event_id == EVENT_LEGACY_DATA_TRANSFER)
                {
                    uint8_t type;

                    type = tx_queue->packet_ptr[6];
                    if ((type == PKT_TYPE_SINGLE) || (type == PKT_TYPE_END))
                    {
                        uint8_t app_index;
                        uint8_t local_server_chann =  RFC_SPP_CHANN_NUM;


#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                        if (app_db.remote_is_8753bau)
                        {
                            local_server_chann = RFC_SPP_DONGLE_CHANN_NUM;
                        }
#endif
                        app_index = tx_queue->packet_ptr[5];
                        if (app_db.br_link[app_index].rtk_vendor_spp_active)
                        {
                            local_server_chann = RFC_RTK_VENDOR_CHANN_NUM;
                        }
                        bt_spp_credits_give(app_db.br_link[app_index].bd_addr, local_server_chann, tx_queue->extra_param);
                    }
                }
                free(tx_queue->packet_ptr);
                tx_queue->active = 0;
                uart_tx_rIndex++;
                if (uart_tx_rIndex >= UART_TX_QUEUE_NO)
                {
                    uart_tx_rIndex = 0;
                }
            }
        }

        p_queue = &(uart_tx_queue[uart_tx_rIndex]);
        if (p_queue->active)
        {
            //app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_TX);
            if (app_cfg_const.enable_tx_wake_up)
            {
                gpio_write_output_level(app_cfg_const.tx_wake_up_pinmux, 0);
                gap_start_timer(&timer_handle_tx_wake_up, "uart_tx_wake_up",
                                app_transfer_timer_queue_id, APP_TRANSFER_TIMER_UART_TX_WAKE_UP, 0, 10);
            }
            else
            {
#if F_APP_CONSOLE_SUPPORT
                console_write(p_queue->packet_ptr, p_queue->packet_len);
#endif

                event_id = ((p_queue->packet_ptr[4]) | (p_queue->packet_ptr[5] << 8));
                if (((event_id == EVENT_ACK) && (app_cfg_const.report_uart_event_only_once == 0)) ||
                    app_cfg_const.report_uart_event_only_once
#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
                    || (app_cfg_const.one_wire_uart_support && one_wire_state == ONE_WIRE_STATE_IN_ONE_WIRE)
#endif
                   )
                {
                    app_pop_data_transfer_queue(CMD_PATH_UART, true);
                }
                else
                {
                    //wait ack or timeout
                    gap_start_timer(&timer_handle_uart_ack, "uart_ack", app_transfer_timer_queue_id,
                                    APP_TRANSFER_TIMER_UART_ACK_TO, event_id, 800);
                }
            }
        }
        else
        {
            if (app_cfg_const.enable_tx_wake_up)
            {
                gpio_write_output_level(app_cfg_const.tx_wake_up_pinmux, 1);
            }
        }
    }
    else
    {
        if (next_flag == true)
        {
            gap_stop_timer(&timer_handle_data_transfer);
            if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active)
            {
                dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;

                if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr != NULL)
                {
                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                }

                dt_queue_ctrl.dt_queue_r_idx++;
                if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                {
                    dt_queue_ctrl.dt_queue_r_idx = 0;
                }
            }

            dt_queue_ctrl.dt_resend_count = 0;
            dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        }

        app_idx = dt_queue[dt_queue_ctrl.dt_queue_r_idx].link_idx;
        pkt_ptr = dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr;
        pkt_len = dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_len;
        event_id = ((dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[4]) |
                    (dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[5] << 8));

        APP_PRINT_INFO7("app_pop_data_transfer_queue: dt_status %d, active %d, connected_profile 0x%x, rfc_credit %d, pkt_len:%d, event_id:0x%x, r idx:%d",
                        dt_queue_ctrl.dt_status,
                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].active,
                        app_db.br_link[app_idx].connected_profile,
                        app_db.br_link[app_idx].rfc_credit,
                        pkt_len, event_id, dt_queue_ctrl.dt_queue_r_idx);

        APP_PRINT_INFO2("app_pop_data_transfer_queue: iap credit %d, session_status %d",
                        app_db.br_link[app_idx].iap.credit, app_db.br_link[app_idx].iap.rtk.session_status);


        if (dt_queue_ctrl.dt_status == DT_STATUS_IDLE)
        {
            if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_IAP)
            {
                if (app_db.br_link[app_idx].connected_profile & IAP_PROFILE_MASK)
                {
                    if (app_db.br_link[app_idx].iap.rtk.session_status == DATA_SESSION_OPEN)
                    {
                        if (bt_iap_data_send(app_db.br_link[app_idx].bd_addr, app_db.br_link[app_idx].iap.rtk.session_id,
                                             pkt_ptr,
                                             pkt_len) == true)
                        {
                            if (app_cfg_const.enable_embedded_cmd)
                            {
                                if (event_id == EVENT_ACK)
                                {
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                                    dt_queue_ctrl.dt_queue_r_idx++;
                                    if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                                    {
                                        dt_queue_ctrl.dt_queue_r_idx = 0;
                                    }

                                    gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                                    app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 1);
                                }
                                else
                                {
                                    dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                                    gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                                    app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x01, 2000);
                                }
                            }
                        }
                        else
                        {
                            gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                            app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 100);
                        }
                    }
                    else if (app_db.br_link[app_idx].iap.rtk.session_status == DATA_SESSION_CLOSE)
                    {
                        if (app_db.br_link[app_idx].iap.authen_flag)
                        {

                            char boundle_id[] = "com.realtek.EADemo2";
                            T_BT_IAP_APP_LAUNCH_METHOD method = BT_IAP_APP_LAUNCH_WITH_USER_ALERT;
                            bt_iap_app_launch(app_db.br_link[app_idx].bd_addr, boundle_id, sizeof(boundle_id), method);
                            app_db.br_link[app_idx].iap.rtk.session_status = DATA_SESSION_LAUNCH;

                            gap_start_timer(&(timer_handle_launch_app[app_idx]), "launch app",
                                            app_transfer_timer_queue_id, APP_TRANSFER_TIMER_LAUNCH_APP, app_idx, 60000);
                        }
                        else
                        {
                            free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                            dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                            dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                            dt_queue_ctrl.dt_queue_r_idx++;
                            if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                            {
                                dt_queue_ctrl.dt_queue_r_idx = 0;
                            }
                            gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                            app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 1);
                        }
                    }
                }
            }
            else if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_SPP)
            {
                if (app_db.br_link[app_idx].connected_profile & SPP_PROFILE_MASK)
                {
                    if (app_db.br_link[app_idx].rfc_credit)
                    {
                        uint8_t local_server_chann =  RFC_SPP_CHANN_NUM;

                        if (app_db.br_link[app_idx].rtk_vendor_spp_active)
                        {
                            local_server_chann = RFC_RTK_VENDOR_CHANN_NUM;
                        }

#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                        if (app_db.remote_is_8753bau)
                        {
                            local_server_chann = RFC_SPP_DONGLE_CHANN_NUM;
                        }
#endif
                        APP_PRINT_INFO4("app_pop_data_transfer_queue: local_server_chann %d dt_w %d dt_r %d tx_id %d",
                                        local_server_chann,
                                        dt_queue_ctrl.dt_queue_r_idx,
                                        dt_queue_ctrl.dt_queue_w_idx,
                                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr[1]);

                        if (bt_spp_data_send(app_db.br_link[app_idx].bd_addr, local_server_chann,
                                             pkt_ptr, pkt_len, false) == true)
                        {
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                            if (app_db.remote_is_8753bau)
                            {
                                app_db.br_link[app_idx].rfc_credit --;
                            }
#endif

                            if (app_cfg_const.enable_embedded_cmd)
                            {
                                if ((event_id == EVENT_ACK)
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                                    || (app_db.remote_is_8753bau)
#endif
                                   )
                                {
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                                    dt_queue_ctrl.dt_queue_r_idx++;
                                    if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                                    {
                                        dt_queue_ctrl.dt_queue_r_idx = 0;
                                    }

                                    if (app_cfg_const.enable_dsp_capture_data_by_spp == true)
                                    {
                                        app_pop_data_transfer_queue(CMD_PATH_SPP, true);
                                    }
                                    else
                                    {
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
                                        if (app_db.remote_is_8753bau)
                                        {
                                            //do nothing.
                                        }
                                        else
#endif
                                        {
                                            gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                                            app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 1);
                                        }
                                    }
                                }
                                else
                                {
                                    if (app_cfg_const.enable_dsp_capture_data_by_spp == true)
                                    {
#if 0
                                        APP_PRINT_ERROR2("dsp_capture_send_data_decode, id = %x, pkt_len = %x",
                                                         pkt_ptr[13] << 8 | pkt_ptr[12],
                                                         pkt_len);
#endif
                                        app_pop_data_transfer_queue(CMD_PATH_SPP, true);
                                    }
                                    else
                                    {
                                        dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                                        gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                                        app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x01, 2000);
                                    }
                                }
                            }
                        }
                        else
                        {
                            APP_PRINT_TRACE1("send spp fail, app_idx =%d", app_idx);
                            if (app_cfg_const.enable_dsp_capture_data_by_spp == true)
                            {
                                app_pop_data_transfer_queue(CMD_PATH_SPP, true);
                            }
                            else
                            {
                                gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                                app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 100);
                            }
                        }
                    }
                    else
                    {
                        free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                        dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                        dt_queue_ctrl.dt_queue_r_idx++;
                        if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                        {
                            dt_queue_ctrl.dt_queue_r_idx = 0;
                        }
                        dt_queue_ctrl.dt_resend_count = 0;
                        gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                        app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 1);
                    }
                }
            }
            else if (dt_queue[dt_queue_ctrl.dt_queue_r_idx].active == CMD_PATH_LE)
            {
                if (app_db.le_link[app_idx].state == LE_LINK_STATE_CONNECTED)
                {
#if HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT
                    if (app_db.le_link[app_idx].transmit_srv_tx_enable_fg == TX_ENABLE_READY)
#else
                    if (app_db.le_link[app_idx].transmit_srv_tx_enable_fg == TX_ENABLE_CCCD_BIT)
#endif
                    {
                        if (transmit_srv_tx_data(app_db.le_link[app_idx].conn_id, pkt_len, pkt_ptr) == true)
                        {
                            dt_queue_ctrl.dt_status = DT_STATUS_ACTIVE;
                            if (app_cfg_const.enable_embedded_cmd)
                            {
#if F_APP_HARMAN_FEATURE_SUPPORT
                                app_pop_data_transfer_queue(CMD_PATH_LE, true);
#else
                                if (event_id != EVENT_ACK)
                                {
                                    gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                                    app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x01, 2000);
                                }
#endif
                            }
                        }
                        else
                        {
                            gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                            app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 100);
                        }
                    }
                    else
                    {
                        gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                        app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 1000);
                    }
                }
                else
                {
                    free(dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr);
                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].pkt_ptr = NULL;
                    dt_queue[dt_queue_ctrl.dt_queue_r_idx].active = 0;
                    dt_queue_ctrl.dt_queue_r_idx++;
                    if (dt_queue_ctrl.dt_queue_r_idx == DT_QUEUE_NO)
                    {
                        dt_queue_ctrl.dt_queue_r_idx = 0;
                    }
                    dt_queue_ctrl.dt_resend_count = 0;
                    //set timer to pop queue
                    gap_start_timer(&timer_handle_data_transfer, "app_data_trans",
                                    app_transfer_timer_queue_id, APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER, 0x00, 1);
                }
            }
        }
    }
}

bool app_push_data_transfer_queue(uint8_t cmd_path, uint8_t *data, uint16_t data_len,
                                  uint8_t extra_param)
{
    APP_PRINT_TRACE4("app_push_data_transfer_queue: cmd_path %d, data_len %d w_idx %d r_idx %d",
                     cmd_path,
                     data_len,
                     dt_queue_ctrl.dt_queue_w_idx,
                     dt_queue_ctrl.dt_queue_r_idx);

    if (CMD_PATH_UART == cmd_path)
    {
        if (uart_tx_queue[uart_tx_wIndex].active == 0)
        {
            uart_tx_queue[uart_tx_wIndex].active = 1;
            uart_tx_queue[uart_tx_wIndex].packet_ptr = data;
            uart_tx_queue[uart_tx_wIndex].packet_len = data_len;
            uart_tx_queue[uart_tx_wIndex].extra_param = extra_param;
            uart_tx_wIndex++;
            if (uart_tx_wIndex >= UART_TX_QUEUE_NO)
            {
                uart_tx_wIndex = 0;
            }

            app_pop_data_transfer_queue(cmd_path, false);
            return true;
        }
        else
        {
            APP_PRINT_TRACE0("app_push_data_transfer_queue: uart_tx_queue full");
            return false;
        }
    }
    else
    {
        if (dt_queue[dt_queue_ctrl.dt_queue_w_idx].active == 0)
        {
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].active = cmd_path;
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].link_idx = extra_param;
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_ptr = data;
            dt_queue[dt_queue_ctrl.dt_queue_w_idx].pkt_len = data_len;
            dt_queue_ctrl.dt_queue_w_idx++;
            if (dt_queue_ctrl.dt_queue_w_idx == DT_QUEUE_NO)
            {
                dt_queue_ctrl.dt_queue_w_idx = 0;
            }


            uint8_t idx = extra_param;
            uint8_t *bd_addr = app_db.br_link[idx].bd_addr;
            APP_PRINT_TRACE2("app_push_data_transfer_queue: idx %d, bd_addr %s", idx, TRACE_BDADDR(bd_addr));

            app_pop_data_transfer_queue(cmd_path, false);
            return true;
        }
        else
        {
            APP_PRINT_TRACE1("app_push_data_transfer_queue: dt_queue[%d] full", dt_queue_ctrl.dt_queue_w_idx);
            return false;
        }
    }
}

bool app_transfer_check_active(uint8_t cmd_path)
{
    if (CMD_PATH_UART == cmd_path)
    {
        if (uart_tx_queue[uart_tx_wIndex].active == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        if (dt_queue[dt_queue_ctrl.dt_queue_w_idx].active == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}

void app_transfer_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_RX);
    uint32_t active_time = (console_get_mode() == CONSOLE_MODE_STRING) ? 30000 : 2000;
    gap_start_timer(&timer_handle_uart_wake_up, "uart_wake_up",
                    app_transfer_timer_queue_id, APP_TRANSFER_TIMER_UART_WAKE_UP, 0, active_time);
}

static void app_transfer_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_transfer_timeout_cb: timer_id %d, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {

    case APP_TRANSFER_TIMER_LAUNCH_APP:
        gap_stop_timer(&(timer_handle_launch_app[timer_chann]));
        if (app_db.br_link[timer_chann].iap.rtk.session_status == DATA_SESSION_LAUNCH)
        {
            APP_PRINT_TRACE2("app_transfer_timeout_cb: session_status = DATA_SESSION_CLOSE", timer_id,
                             timer_chann);
            app_db.br_link[timer_chann].iap.rtk.session_status = DATA_SESSION_CLOSE;
        }
        break;

    case APP_TRANSFER_TIMER_UART_ACK_TO:
        gap_stop_timer(&timer_handle_uart_ack);
#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
        if (app_cfg_const.one_wire_uart_support)
        {
            /* don't retry ack and switch to UART Rx*/
            uart_resend_count = app_cfg_const.dt_resend_num;
            app_one_wire_uart_switch_pinmiux(ONE_WIRE_UART_RX);
        }
#endif
        uart_resend_count++;
        if (uart_resend_count > app_cfg_const.dt_resend_num)
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, true);
        }
        else
        {
            app_pop_data_transfer_queue(CMD_PATH_UART, false);
        }
        break;

    case APP_TRANSFER_TIMER_UART_WAKE_UP:
        gap_stop_timer(&timer_handle_uart_wake_up);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_RX);
        break;

    case APP_TRANSFER_TIMER_UART_TX_WAKE_UP:
        gap_stop_timer(&timer_handle_tx_wake_up);
        app_pop_data_transfer_queue(CMD_PATH_UART, false);
        break;

    case APP_TRANSFER_TIMER_CHECK_DATA_TRANSFER:
        gap_stop_timer(&timer_handle_data_transfer);
        dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
        if (timer_chann == 0x01) //ack timeout
        {
            dt_queue_ctrl.dt_resend_count++;
            //dt_queue_ctrl.dt_status = DT_STATUS_IDLE;
            if (dt_queue_ctrl.dt_resend_count >= app_cfg_const.dt_resend_num)
            {
                app_pop_data_transfer_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, true);
            }
            else
            {
                app_pop_data_transfer_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, false);
            }
        }
        else
        {
            app_pop_data_transfer_queue(dt_queue[dt_queue_ctrl.dt_queue_r_idx].active, false);
        }
        break;

    default:
        break;
    }
}

void app_transfer_init(void)
{
    app_db.external_mcu_mtu = 256;

    gap_reg_timer_cb(app_transfer_timeout_cb, &app_transfer_timer_queue_id);
}

static void app_transfer_bt_data(uint8_t *cmd_ptr, uint8_t cmd_path, uint8_t app_idx,
                                 uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    uint16_t total_len;
    uint16_t pkt_len;
    uint8_t  idx;
    uint8_t  pkt_type;
    uint8_t  *pkt_ptr;

    idx        = cmd_ptr[2];
    pkt_type   = cmd_ptr[3];
    total_len  = (cmd_ptr[4] | (cmd_ptr[5] << 8));
    pkt_len    = (cmd_ptr[6] | (cmd_ptr[7] << 8));
    pkt_ptr    = &cmd_ptr[8];

    if (((cmd_id == CMD_LEGACY_DATA_TRANSFER) &&
         ((app_db.br_link[idx].connected_profile & SPP_PROFILE_MASK) ||
          (app_db.br_link[idx].connected_profile & IAP_PROFILE_MASK))) ||
        ((cmd_id == CMD_LE_DATA_TRANSFER) && (app_db.le_link[idx].state == LE_LINK_STATE_CONNECTED)))
    {
        if (cmd_path == CMD_PATH_UART)
        {
            if (pkt_len)
            {
                if ((pkt_type == PKT_TYPE_SINGLE) || (pkt_type == PKT_TYPE_START))
                {
                    if ((cmd_id == CMD_LEGACY_DATA_TRANSFER) &&
                        (((app_db.br_link[idx].connected_profile & SPP_PROFILE_MASK) &&
                          (!app_transfer_check_active(CMD_PATH_SPP))) ||
                         (((app_db.br_link[idx].connected_profile & IAP_PROFILE_MASK) &&
                           (!app_transfer_check_active(CMD_PATH_IAP))))))
                    {
                        if (app_db.br_link[idx].uart_rx_dt_pkt_ptr)
                        {
                            free(app_db.br_link[idx].uart_rx_dt_pkt_ptr);
                        }

                        app_db.br_link[idx].uart_rx_dt_pkt_ptr = malloc(total_len);
                        memcpy(app_db.br_link[idx].uart_rx_dt_pkt_ptr, pkt_ptr, pkt_len);
                        app_db.br_link[idx].uart_rx_dt_pkt_len = pkt_len;
                    }
                    else if ((cmd_id == CMD_LE_DATA_TRANSFER) && (!app_transfer_check_active(CMD_PATH_LE)))
                    {
                        if (uart_rx_dt_pkt_ptr)
                        {
                            free(uart_rx_dt_pkt_ptr);
                        }

                        uart_rx_dt_pkt_ptr = malloc(total_len);
                        memcpy(uart_rx_dt_pkt_ptr, pkt_ptr, pkt_len);
                        uart_rx_dt_pkt_len = pkt_len;
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    }
                }
                else
                {
                    if ((cmd_id == CMD_LEGACY_DATA_TRANSFER) && (app_db.br_link[idx].uart_rx_dt_pkt_ptr))
                    {
                        uint8_t *temp_ptr;

                        temp_ptr = app_db.br_link[idx].uart_rx_dt_pkt_ptr +
                                   app_db.br_link[idx].uart_rx_dt_pkt_len;
                        memcpy(temp_ptr, pkt_ptr, pkt_len);
                        app_db.br_link[idx].uart_rx_dt_pkt_len += pkt_len;
                    }
                    else if ((cmd_id == CMD_LE_DATA_TRANSFER) && uart_rx_dt_pkt_ptr)
                    {
                        uint8_t *temp_ptr;

                        temp_ptr = uart_rx_dt_pkt_ptr + uart_rx_dt_pkt_len;
                        memcpy(temp_ptr, pkt_ptr, pkt_len);
                        uart_rx_dt_pkt_len += pkt_len;
                    }
                    else//maybe start packet been lost
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }
                }

                if ((pkt_type == PKT_TYPE_SINGLE) || (pkt_type == PKT_TYPE_END))
                {
                    if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
                    {
                        if (cmd_id == CMD_LEGACY_DATA_TRANSFER)
                        {
                            if (app_db.br_link[idx].connected_profile & SPP_PROFILE_MASK)
                            {
                                app_report_raw_data(CMD_PATH_SPP, app_idx, app_db.br_link[idx].uart_rx_dt_pkt_ptr,
                                                    app_db.br_link[idx].uart_rx_dt_pkt_len);

                                free(app_db.br_link[idx].uart_rx_dt_pkt_ptr);
                                app_db.br_link[idx].uart_rx_dt_pkt_ptr = NULL;

                                if (app_transfer_check_active(CMD_PATH_SPP))
                                {
                                    ack_pkt[2] = CMD_SET_STATUS_BUSY;
                                    app_db.br_link[idx].resume_fg = 0x01;
                                }
                            }
                            else if (app_db.br_link[idx].connected_profile & IAP_PROFILE_MASK)
                            {
                                app_report_raw_data(CMD_PATH_IAP, app_idx, app_db.br_link[idx].uart_rx_dt_pkt_ptr,
                                                    app_db.br_link[idx].uart_rx_dt_pkt_len);

                                free(app_db.br_link[idx].uart_rx_dt_pkt_ptr);
                                app_db.br_link[idx].uart_rx_dt_pkt_ptr = NULL;

                                if (app_transfer_check_active(CMD_PATH_IAP))
                                {
                                    ack_pkt[2] = CMD_SET_STATUS_BUSY;
                                    app_db.br_link[idx].resume_fg = 0x01;
                                }
                            }
                        }
                        else if (cmd_id == CMD_LE_DATA_TRANSFER)
                        {
                            app_report_raw_data(CMD_PATH_LE, app_idx, uart_rx_dt_pkt_ptr, uart_rx_dt_pkt_len);

                            free(uart_rx_dt_pkt_ptr);
                            uart_rx_dt_pkt_ptr = NULL;

                            if (app_transfer_check_active(CMD_PATH_LE))
                            {
                                ack_pkt[2] = CMD_SET_STATUS_BUSY;
                            }
                        }
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
        }
    }
    else
    {
        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
    }

    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
}

void app_transfer_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                             uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_LEGACY_DATA_TRANSFER:
    case CMD_LE_DATA_TRANSFER:
        {
            app_transfer_bt_data(&cmd_ptr[0], cmd_path, app_idx, &ack_pkt[0]);
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

