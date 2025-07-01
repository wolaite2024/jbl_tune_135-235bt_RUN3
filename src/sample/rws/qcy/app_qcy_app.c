#if C_QCY_APP_FEATURE_SUPPORT

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "trace.h"
#include "app_cfg.h"
#include "app_relay.h"
#include "app_main.h"
#include "gap_ext_adv.h"
#include "ble_ext_adv.h"
#include "app_qcy_app.h"
#include "remote.h"
#include "btm.h"
#include "gap_timer.h"
#include "rtl876x.h"
#include "ble_ext_adv.h"
#include "app_bt_policy_api.h"
#include "app_bond.h"
#include "app_charger.h"
#include "audio.h"

#define APP_QCY_IGNORE_BAT_CHANGE_THRESHOLD 3

#define APP_QCY_ADV_NORMAL_INTERVAL     160    //160 * 0.625 = 100 ms
#define APP_QCY_ADV_LONG_INTERVAL       3200   //3200 * 0.625 = 2000 ms

#define QCY_ADV_LEN 28

uint8_t app_qcy_timer_queue_id = 0;
void *timer_handle_app_ctrl_led_blinking = NULL;
void *timer_handle_app_ctrl_reboot = NULL;
static void *timer_handle_qcy_adv_timeout = NULL;

uint8_t qcy_adv_handle;

static bool app_qcy_check_ignore_battery_change(uint8_t new_bat, uint8_t pre_bat)
{
    bool ret = false;

    if (pre_bat != 0)
    {
        if (new_bat >= pre_bat)
        {
            if ((new_bat - pre_bat) <= APP_QCY_IGNORE_BAT_CHANGE_THRESHOLD)
            {
                ret = true;
            }
        }
        else
        {
            if ((pre_bat - new_bat) <= APP_QCY_IGNORE_BAT_CHANGE_THRESHOLD)
            {
                ret = true;
            }
        }
    }

    return ret;
}

static bool app_qcy_adv_get(uint8_t *data, uint8_t len, bool set_in_box_flag)
{
    bool ret = true;

    uint8_t qcy_data_template[QCY_ADV_LEN] =
    {
        0x1B, //Length
        0xFF, //Manufacturer data
        0x1C, 0x52, //Company ID
        0x4D, 0xC1, //Model ID
        0xFF, //CRC
        0x00, 0x00, //Earbuds status
        0x00, //Left bud battery information
        0x00, //Right bud battery information
        0x00, //chargr box battey information
        0x00, 0x00, 0x00, //SRC's LAP
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Primiary's BT Addr
        0x00, //Adv counter
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00 //Secondary's BT Addr
    };

    memset(data, 0, sizeof(len));
    memcpy(data, qcy_data_template, sizeof(qcy_data_template));

    APP_PRINT_TRACE4("app_qcy_adv_get: company %02x %02x model %02x %02x", app_cfg_const.company_id[0],
                     app_cfg_const.company_id[1], app_cfg_const.uuid[0],
                     app_cfg_const.uuid[1]);

    /* byte 2~3 : company id */
    if (!(app_cfg_const.company_id[0] == 0 && app_cfg_const.company_id[1] == 0))
    {
        data[2] = app_cfg_const.company_id[1];
        data[3] = app_cfg_const.company_id[0];
    }

    /* byte 4~5 : model id */
    if (!(app_cfg_const.uuid[0] == 0 && app_cfg_const.uuid[1] == 0))
    {
        data[4] = app_cfg_const.uuid[1];
        data[5] = app_cfg_const.uuid[0];
    }

    /* byte 6: crc to be calculated at last (sum of byte7 to byte 14 and get LSB) */

    /* byte 7: bit 0 -> location: left (1) or right (0);
               bit 1 -> color: black (1) white (0);
               bit 2~3 -> reserved
               bit 4 -> left discoverable (1) non discoverable (0)
               bit 5 -> right discoverable (1) non discoverable (0)
               bit 6 -> left connectable (1) non connectable (0)
               bit 7 -> right connectable (1) non connectable (0)

               Bit 7~4 are not used now, set to 0.
    */
    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        data[7] |= BIT(0);
    }
    data[7] |= BIT(1); /* assume color black */

    /* byte 8: bit 0 -> reserve
               bit 1 -> case open (1) close (0)
               bit 2 -> phone connected (1) not connected (0)
               bit 3 -> phone record existed (1) not existed (0)
               bit 4 -> mac encrypt (1) not encrypt (0)
               bit 5 -> bud out box (1) in box (0)
               bit 6 -> rws connected (1) not connected (0)
               bit 7 -> reserve
    */

    /* open case must also be out box; close case must also be in box (these two settings must be together; comment from qcy) */
    if (set_in_box_flag)
    {
        data[8] |= BIT(1);
        data[8] |= BIT(5);
    }

    /* phone connected */
    if (app_bt_policy_get_b2s_connected_num() != 0)
    {
        data[8] |= BIT(2);
    }

    /* phone record */
    if (app_is_b2s_link_record_exist())
    {
        data[8] |= BIT(3);
    }

    /* rws connected */
    if (app_bt_policy_get_b2b_connected())
    {
        data[8] |= BIT(6);
    }


    /* get system battery info */
    bool left_charging, right_charging, case_charging;
    uint8_t left_bat, right_bat, case_bat;

    app_qcy_get_bat_info(&left_charging, &left_bat, &right_charging, &right_bat, &case_charging,
                         &case_bat);

    /* byte 9: bit 0~6 left battery
               bit 7 charging (1) not charging (0)
    */
    if (left_charging)
    {
        data[9] |= BIT(7);
    }
    data[9] |= left_bat;

    /* byte 10: bit 0~6 right battery
                bit 7 charging (1) not charging (0)
    */
    if (right_charging)
    {
        data[10] |= BIT(7);
    }
    data[10] |= right_bat;

    /* byte 11: bit 0~6 box battery
                bit 7 charging (1) not charging (0)
    */
    if (case_charging)
    {
        data[11] |= BIT(7);
    }
    data[11] |= case_bat;

    /* byte 12~14: phone mac LAP (00: e0: 4c: 1A: 1B: 1C) (addr[5] addr[4] addr[3] addr[2] addr[1] addr[0])
                   LAP (1A 1B 1C) (addr[2] addr[1] addr[0])
    */

    /* byte 6: crc to be calculated at last (sum of byte7 to byte 14 and get LSB) */
    uint16_t CRC;
    CRC = data[7] + data[8] + data[9] + data[10] +
          data[11] + data[12] + data[13] + data[14];
    data[6] = (CRC & 0xFF);

    /* byte 15~20: primay bud addr (00: e0: 4c: 1a: 1b: 1c) (addr[5] addr[4] addr[3] addr[2] addr[1] addr[0])
                   byte 15: addr[4] (e0) (LSB of NAP)
                   byte 16: addr[5] (00) (MSB of NAP)
                   byte 17: addr[3] (4c) (UAP)
                   byte 18~20 (LAP)
                   byte 18: addr[0] (1c)
                   byte 19: addr[1] (1b)
                   byte 20: addr[2] (1a)
    */
    data[15] = app_cfg_nv.bud_local_addr[4];
    data[16] = app_cfg_nv.bud_local_addr[5];
    data[17] = app_cfg_nv.bud_local_addr[3];
    data[18] = app_cfg_nv.bud_local_addr[0];
    data[19] = app_cfg_nv.bud_local_addr[1];
    data[20] = app_cfg_nv.bud_local_addr[2];

    /* byte 21: adv cnt (both bud's cnt should be sync) */
    data[21] = app_cfg_nv.qcy_adv_cnt++;

    /* byte 22~27: secondary bud's addr (00 : e0: 4c: 2a: 2b: 2c) (addr[5] addr[4] addr[3] addr[2] addr[1] addr[0])
                   byte 22: addr[4] (e0) (LSB of NAP)
                   byte 23: addr[5] (00) (MSB of NAP)
                   byte 24: addr[3] (4c) (UAP)
                   byte 25~27 (LAP)
                   byte 25: addr[0] (2c)
                   byte 26: addr[1] (2b)
                   byte 27: addr[2] (2a)
    */
    data[22] = app_cfg_nv.bud_peer_addr[4];
    data[23] = app_cfg_nv.bud_peer_addr[5];
    data[24] = app_cfg_nv.bud_peer_addr[3];
    data[25] = app_cfg_nv.bud_peer_addr[0];
    data[26] = app_cfg_nv.bud_peer_addr[1];
    data[27] = app_cfg_nv.bud_peer_addr[2];

    APP_PRINT_TRACE3("app_qcy_adv_get: ret %d set_in_box %d adv %b",
                     ret, set_in_box_flag, TRACE_BINARY(len, data));

    return ret;
}

void app_qcy_get_bat_info(bool *left_charging, uint8_t *left_bat,
                          bool *right_charging, uint8_t *right_bat,
                          bool *case_charging, uint8_t *case_bat)
{
    uint8_t left_tmp, right_tmp, case_tmp;
    static uint8_t pre_left_bat, pre_right_bat, pre_case_bat;

    app_charger_get_battery_info(left_charging, &left_tmp,
                                 right_charging, &right_tmp,
                                 case_charging, &case_tmp);

    bool ignore_left_bat_change = app_qcy_check_ignore_battery_change(left_tmp, pre_left_bat);
    bool ignore_right_bat_change = app_qcy_check_ignore_battery_change(right_tmp, pre_right_bat);
    bool ignore_case_bat_change = app_qcy_check_ignore_battery_change(case_tmp, pre_case_bat);

    /* prevent battery ping-ping effect when adp in/out */
    if (ignore_left_bat_change == false)
    {
        pre_left_bat = left_tmp;
    }
    if (ignore_right_bat_change == false)
    {
        pre_right_bat = right_tmp;
    }
    if (ignore_case_bat_change == false)
    {
        pre_case_bat = case_tmp;
    }

    *left_bat = pre_left_bat;
    *right_bat = pre_right_bat;
    *case_bat = pre_case_bat;

    APP_PRINT_TRACE6("app_qcy_get_bat_info: left %d(%d) right %d(%d) case %d(%d)",
                     *left_bat, ignore_left_bat_change,
                     *right_bat, ignore_right_bat_change,
                     *case_bat, ignore_case_bat_change);

}

/* set_out_box_flag:
*  false -> when close case and both buds in box to indicate both buds in box
*  (comment from QCY)
*  timeout: the timeout value to stop adv
*/
void app_qcy_adv_start(bool set_out_box_flag, T_QCY_APP_ADV_TIMEOUT timeout)
{
    uint8_t qcy_adv[QCY_ADV_LEN] = {0};
    uint32_t timeout_value = 0;

    if (timeout == APP_QCY_ADV_LONG_TIMEOUT)
    {
        timeout_value = 120000; // 2 mins
    }
    else if (timeout == APP_QCY_ADV_SHORT_TIMEOUT)
    {
        timeout_value = 30000; // 30 secs
    }
    else if (timeout == APP_QCY_ADV_NO_RESTART_TIMER)
    {
        if (timer_handle_qcy_adv_timeout == NULL)
        {
            /* no adv is playing; just return */
            return;
        }
    }

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.device_state == APP_DEVICE_STATE_ON) &&
        (app_db.qcy_app_db.qcy_le_connected == false))
    {
        uint16_t adv_interval;
        bool get_adv = app_qcy_adv_get(qcy_adv, sizeof(qcy_adv), set_out_box_flag);

        APP_PRINT_TRACE4("app_qcy_adv_start: get_adv %d out_box %d adv_interval %d timeout %d",
                         get_adv, set_out_box_flag, app_db.qcy_app_db.qcy_adv_interval, timeout);

        if (get_adv == false)
        {
            return;
        }

        if (timeout_value)
        {
            gap_start_timer(&timer_handle_qcy_adv_timeout, "qcy_adv_timeout",
                            app_qcy_timer_queue_id, QCY_APP_ADV_TIMEOUT,
                            0, timeout_value);
        }

        if (app_db.qcy_app_db.qcy_adv_interval == 0)
        {
            adv_interval = APP_QCY_ADV_NORMAL_INTERVAL;
        }
        else
        {
            adv_interval = APP_QCY_ADV_LONG_INTERVAL;
        }

        ble_ext_adv_mgr_change_adv_interval(qcy_adv_handle, adv_interval);
        ble_ext_adv_mgr_set_adv_data(qcy_adv_handle, sizeof(qcy_adv), qcy_adv);

        ble_ext_adv_mgr_enable(qcy_adv_handle, 0);
    }
}

/* stop_timer: true -> stop adv timer;
*              false -> no stop adv timer (ex: just want to change the running adv interval)
*/
void app_qcy_adv_stop(bool stop_timer)
{
    APP_PRINT_TRACE1("app_qcy_adv_stop: stop_timer %d", stop_timer);

    if (stop_timer)
    {
        gap_stop_timer(&timer_handle_qcy_adv_timeout);
    }

    ble_ext_adv_mgr_disable(qcy_adv_handle, 0);
}

bool app_qcy_adv_is_advertising(void)
{
    return timer_handle_qcy_adv_timeout ? true : false;
}

void app_qcy_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause, uint16_t disc_cause)
{
    APP_PRINT_TRACE3("app_qcy_le_disconnect_cb: conn_id %d, local_disc_cause %d, disc_cause 0x%x",
                     conn_id, local_disc_cause, disc_cause);

    app_db.qcy_app_db.qcy_le_connected = false;

    /* still consider if we need to restart adv when le disconnected */
#if 0
    app_qcy_adv_start(true, APP_QCY_ADV_LONG_TIMEOUT);
#endif
}

static void app_qcy_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    APP_PRINT_TRACE1("app_qcy_adv_callback: type %d", cb_type);

    switch (cb_type)
    {
    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            app_reg_le_link_disc_cb(cb_data.p_ble_conn_info->conn_id, app_qcy_le_disconnect_cb);

            app_db.qcy_app_db.qcy_le_connected = true;

            app_qcy_adv_stop(true);
        }
        break;

    default:
        break;
    }

}

void app_qcy_adv_init()
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval = APP_QCY_ADV_NORMAL_INTERVAL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_RANDOM;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    bool gen_new_le_addr = false;
    uint8_t factory_addr[6] = {0};

    /* only factory role pri will generate le addr (and it will be sync to sec when first engage) */
    if ((app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_cfg_nv.le_random_addr[5] & 0xC0) != 0xC0)
    {
        gen_new_le_addr = true;
        le_gen_rand_addr(GAP_RAND_ADDR_STATIC, app_cfg_nv.le_random_addr);

        /* hash to a new random addr (byte 5 must be & 0xc0 ) */
        gap_get_param(GAP_PARAM_BD_ADDR, factory_addr);
        app_cfg_nv.le_random_addr[0] += factory_addr[0];
        app_cfg_nv.le_random_addr[1] += factory_addr[1];
        app_cfg_nv.le_random_addr[2] += factory_addr[2];
        app_cfg_nv.le_random_addr[3] += factory_addr[3];
        app_cfg_nv.le_random_addr[4] += factory_addr[4];
    }

    ble_ext_adv_mgr_init_adv_params(&qcy_adv_handle,
                                    adv_event_prop,
                                    adv_interval,
                                    adv_interval,
                                    own_address_type,
                                    peer_address_type,
                                    peer_address,
                                    filter_policy,
                                    0, NULL, 0, NULL, app_cfg_nv.le_random_addr);

    ble_ext_adv_mgr_register_callback(app_qcy_adv_callback, qcy_adv_handle);

    APP_PRINT_TRACE4("app_qcy_adv_init: handle %d addr %s gen_new_le_addr %d factory addr %s",
                     qcy_adv_handle,
                     TRACE_BDADDR(app_cfg_nv.le_random_addr), gen_new_le_addr, TRACE_BDADDR(factory_addr));

}


static void app_qcy_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = false;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                APP_PRINT_TRACE7("send le random addr: %s %02x %02x %02x %02x %02x %02x",
                                 TRACE_BDADDR(app_cfg_nv.le_random_addr),
                                 app_cfg_nv.le_random_addr[0], app_cfg_nv.le_random_addr[1],
                                 app_cfg_nv.le_random_addr[2], app_cfg_nv.le_random_addr[3],
                                 app_cfg_nv.le_random_addr[4], app_cfg_nv.le_random_addr[5]);

                /* le_random_addr is generated in factory role pri */
                remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_LE_RANDOM_ADDR_SYNC,
                                       app_cfg_nv.le_random_addr, sizeof(app_cfg_nv.le_random_addr), false);
            }

            app_qcy_info_sync(true);

            handle = true;
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_CHANGED:
    case BT_EVENT_AVRCP_VOLUME_UP:
    case BT_EVENT_AVRCP_VOLUME_DOWN:
    case BT_EVENT_SPP_CONN_CMPL:
    case BT_EVENT_A2DP_DISCONN_CMPL:
    case BT_EVENT_A2DP_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_qcy_adv_start(true, APP_QCY_ADV_LONG_TIMEOUT);
            }

            handle = true;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE1("app_qcy_bt_cback: event %04x", event_type);
    }

}

void app_qcy_info_sync(bool play_adv_after_get_info)
{
    T_QCY_APP_RELAY_MSG msg;

    memset(&msg, 0, sizeof(msg));

    msg.play_adv_after_get_info = play_adv_after_get_info;
    msg.adv_cnt = app_cfg_nv.qcy_adv_cnt;
    msg.batt_level = app_db.local_batt_level;
    msg.charger_state = app_db.local_charger_state;
    msg.bud_location = app_db.local_loc;
    msg.qcy_sleep_mode = app_db.qcy_app_db.qcy_sleep_mode;
    memcpy(&msg.key_info, &app_cfg_nv.qcy_key_info, sizeof(T_QCY_KEY_INFO));

    remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_QCY_APP_INFO_SYNC, &msg,
                           sizeof(T_QCY_APP_RELAY_MSG), false);

    APP_PRINT_TRACE1("app_qcy_info_sync: payload %b", TRACE_BINARY(sizeof(msg), (uint8_t *)&msg));
}

static void app_qcy_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_qcy_timeout_cb: timer_id 0x%02x, timer_chann %d", timer_id,
                     timer_chann);

    switch (timer_id)
    {
    case QCY_APP_ADV_TIMEOUT:
        {
            gap_stop_timer(&timer_handle_qcy_adv_timeout);
            app_qcy_adv_stop(true);
        }
        break;

    case QCY_APP_TIMER_LED_BLINKING:
        {
            gap_stop_timer(&timer_handle_app_ctrl_led_blinking);
            app_qcy_led_blinking(LED_STOP_BLINKING);
        }
        break;

    case QCY_APP_TIMER_REBOOT:
        {
            gap_stop_timer(&timer_handle_app_ctrl_reboot);
            WDG_SystemReset(RESET_ALL);
        }
        break;

    default:
        {
        }
        break;
    }
}

void app_qcy_handle_in_out_box_adv(void)
{
    static uint8_t pre_loc = BUD_LOC_UNKNOWN;
    static uint8_t pre_remote_loc = BUD_LOC_UNKNOWN;

    if ((app_db.device_state == APP_DEVICE_STATE_ON) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
        (app_db.local_loc != pre_loc || app_db.remote_loc != pre_remote_loc))
    {
        app_qcy_adv_start(true, APP_QCY_ADV_LONG_TIMEOUT);
    }

    APP_PRINT_TRACE4("app_qcy_handle_in_out_box_adv: pre_loc %d pre_remote_loc %d loc %d remote_loc %d",
                     pre_loc, pre_remote_loc, app_db.local_loc, app_db.remote_loc);

    pre_loc = app_db.local_loc;
    pre_remote_loc = app_db.remote_loc;
}

void app_qcy_handle_role_swap(T_REMOTE_SESSION_ROLE role, T_BTM_ROLESWAP_STATUS status)
{
    APP_PRINT_TRACE2("app_qcy_handle_role_swap: role %d status %02x", role, status);

    if (status == BTM_ROLESWAP_STATUS_START_IND)
    {
        app_qcy_adv_stop(true);
    }
    else if (status == BTM_ROLESWAP_STATUS_SUCCESS ||
             status == BTM_ROLESWAP_STATUS_TERMINATED ||
             status == BTM_ROLESWAP_STATUS_FAIL)
    {
        if (role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (timer_handle_qcy_adv_timeout)
            {
                /* ask peer bud continue to play qcy's adv */
                app_qcy_info_sync(true);
            }

            app_qcy_adv_stop(true);
        }
    }
}

static void app_qcy_audio_policy_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;

    if (event_type == AUDIO_EVENT_TRACK_STATE_CHANGED)
    {
        T_AUDIO_STREAM_TYPE stream_type;
        T_AUDIO_TRACK_STATE state;
        static T_AUDIO_TRACK_STATE pre_state;

        if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
        {
            return;
        }

        state = param->track_state_changed.state;

        APP_PRINT_TRACE3("app_qcy_audio_policy_cback: stream_type %d state (%d->%d)", stream_type,
                         pre_state, state);

        /* start to play a2dp */
        if (state == AUDIO_TRACK_STATE_STARTED || state == AUDIO_TRACK_STATE_RESTARTED)
        {
            if (timer_handle_qcy_adv_timeout)
            {
                app_qcy_adv_stop(false);

                /* increase qcy adv's interval */
                app_db.qcy_app_db.qcy_adv_interval = 1;

                app_qcy_adv_start(true, APP_QCY_ADV_NO_RESTART_TIMER);
            }
        }
        /* stop a2dp */
        else if (state == AUDIO_TRACK_STATE_STOPPED || state == AUDIO_TRACK_STATE_PAUSED)
        {
            if (timer_handle_qcy_adv_timeout)
            {
                app_qcy_adv_stop(false);

                /* back to normal interval */
                app_db.qcy_app_db.qcy_adv_interval = 0;

                app_qcy_adv_start(true, APP_QCY_ADV_NO_RESTART_TIMER);
            }
        }

        pre_state = state;
    }
}

void app_qcy_init()
{
    bt_mgr_cback_register(app_qcy_bt_cback);

    audio_mgr_cback_register(app_qcy_audio_policy_cback);

    gap_reg_timer_cb(app_qcy_timeout_cb, &app_qcy_timer_queue_id);
}

#endif
