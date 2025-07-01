#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
#include "app_dongle_spp.h"
#include "app_dongle_record.h"
#include "app_mmi.h"
#include "app_main.h"
#include "bt_spp.h"
#include "btm.h"
#include "trace.h"
#include "app_report.h"
#include "app_roleswap.h"
#include "remote.h"
#include "app_relay.h"
#include "app_multilink.h"
#include "app_cfg.h"
#include "app_transfer.h"
#include "app_listening_mode.h"
#include "app_audio_passthrough.h"
#include "app_hfp.h"
#include "app_audio_policy.h"
#include "sysm.h"
#include "gap_legacy.h"
#include "app_bt_sniffing.h"
#include "app_roleswap_control.h"

static uint8_t mic_data_idx = 0;

static const uint8_t dongle_service_class_uuid128[16] =
{
    0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0x5A, 0x01, 0x00, 0x00
};

static bool is_dongle_spp_conn = false;

void app_dongle_set_ext_eir(void)
{
    uint8_t p_eir[9];
    p_eir[0] = 8; /* length */
    p_eir[1] = 0xFF;
    p_eir[2] = 0x5D;
    p_eir[3] = 0x00;
    p_eir[4] = 0x08;
    /* bit0: 0- stereo headset 1- TWS headset
       bit1: 1- support LowLatency with RTL8753BAU
             0- not support LowLatency with RTL8753BAU
    */
    p_eir[5] = (remote_session_role_get() == REMOTE_SESSION_ROLE_SINGLE) ? 0x02 : 0x03;
    /*
         bit 3~0: 0: sbc frame nums in each avdtp packet depend on RTL8753BAU
    */
    p_eir[6] = 0x0;

    //Set pairing ID
    p_eir[7] = (app_cfg_const.rws_custom_uuid >> 8) & 0xFF;
    p_eir[8] = app_cfg_const.rws_custom_uuid & 0xFF;

    legacy_set_ext_eir(&p_eir[0], 9);
}

void app_dongle_handle_gaming_mode_cmd(uint8_t action)
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_mmi_handle_action(action);
    }
    else
    {
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_GAMING_MODE_REQUEST);
        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                              0, false);
    }
}

void app_dongle_sync_is_mic_enable_status(void)
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_DONGLE_IS_ENABLE_MIC);
    }
}

void app_dongle_control_apt(uint8_t action)
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_mmi_handle_action(action);
    }
    else
    {
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_DONGLE_IS_DISABLE_APT);
        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                              0, false);
    }
}

bool app_dongle_get_spp_conn_flag(void)
{
    return is_dongle_spp_conn;
}

static void app_dongle_spp_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            if (param->spp_conn_cmpl.local_server_chann != RFC_SPP_DONGLE_CHANN_NUM)
            {
                return;
            }

            p_link = app_find_br_link(param->spp_credit_rcvd.bd_addr);

            if (p_link != NULL)
            {
                p_link->rfc_spp_credit = param->spp_conn_cmpl.link_credit;
                p_link->rfc_spp_frame_size = param->spp_conn_cmpl.frame_size;
                is_dongle_spp_conn = true;
                app_dongle_updata_mic_data_idx(true);

                APP_PRINT_TRACE2("app_dongle_spp_cback: param->bt_spp_conn_cmpl.link_credit %d param->bt_spp_conn_cmpl.frame_size %d",
                                 p_link->rfc_spp_credit, p_link->rfc_spp_frame_size);
            }
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        {
            if (param->spp_credit_rcvd.local_server_chann != RFC_SPP_DONGLE_CHANN_NUM)
            {
                return;
            }

            p_link = app_find_br_link(param->spp_credit_rcvd.bd_addr);

            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_dongle_spp_cback: no acl link found");
                return;
            }

            p_link->rfc_credit = param->spp_credit_rcvd.link_credit;
        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            if (param->spp_data_ind.local_server_chann != RFC_SPP_DONGLE_CHANN_NUM)
            {
                return;
            }

            uint8_t     *p_data;
            uint16_t    len;
            uint8_t     app_idx;
            uint16_t    data_len;

            p_link = app_find_br_link(param->spp_data_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_dongle_spp_cback: no acl link found");
                return;
            }
            app_idx = p_link->id;
            p_data = param->spp_data_ind.data;
            len = param->spp_data_ind.len;
            data_len = len;

            bt_spp_credits_give(app_db.br_link[app_idx].bd_addr, param->spp_data_ind.local_server_chann,
                                1);

            if (app_cfg_const.enable_embedded_cmd)
            {
                if (data_len > 4)
                {
                    uint8_t payload_len = p_data[2];

                    // header check
                    if ((p_data[0] == VOICE_OVER_SPP_START_BIT) && (p_data[payload_len + 3] == VOICE_OVER_SPP_STOP_BIT))
                    {
                        APP_PRINT_INFO6("app_dongle_spp_cback %02x, %02x, %02x, %02x, %02x, %02x,",
                                        p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5]);

                        if (p_data[1] == VOICE_OVER_SPP_CMD)
                        {
                            if (p_data[3] == VOICE_OVER_SPP_CMD_SET_GAMING_MOE)
                            {
                                APP_PRINT_TRACE3("VOICE_OVER_SPP_CMD_SET_GAMING_MOE: %02x, %02x, %02x",
                                                 app_db.ignore_bau_first_gaming_cmd, app_db.restore_gaming_mode, app_db.gaming_mode);

                                uint8_t action = MMI_DEV_GAMING_MODE_SWITCH;

                                if (app_db.ignore_bau_first_gaming_cmd)
                                {
                                    app_db.ignore_bau_first_gaming_cmd = false;

                                    if (app_db.restore_gaming_mode && (app_db.gaming_mode == 0))
                                    {
                                        //Restore gaming mode to on.
                                        app_db.gaming_mode_request_is_received = false;
                                        app_dongle_handle_gaming_mode_cmd(action);
                                    }

                                    app_db.restore_gaming_mode = false;
                                    break;
                                }

                                if ((p_data[4] == 1) && (app_db.gaming_mode == 0))
                                {
                                    // enable gaming mode
                                    app_db.gaming_mode_request_is_received = true;
                                    app_dongle_handle_gaming_mode_cmd(action);
                                }
                                else if ((p_data[4] == 0) && (app_db.gaming_mode == 1))
                                {
                                    //disable gaming mode
                                    app_db.gaming_mode_request_is_received = true;
                                    app_dongle_handle_gaming_mode_cmd(action);
                                }
                            }
                            else if (p_data[3] == VOICE_OVER_SPP_CMD_REQ_OPEN_MIC)
                            {
#if F_APP_APT_SUPPORT
                                uint8_t action = MMI_NULL;
#endif

                                if (p_data[4] == 1)
                                {
                                    app_db.dongle_is_enable_mic = true;
                                    app_transfer_queue_reset(CMD_PATH_SPP);

#if F_APP_APT_SUPPORT
                                    if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                                    {
                                        action = MMI_AUDIO_APT;
                                        app_db.dongle_is_disable_apt = true;
                                        app_dongle_control_apt(action);
                                    }
#endif

                                    app_dongle_sync_is_mic_enable_status();
                                    app_dongle_start_recording(app_db.br_link[app_idx].bd_addr);
                                    app_bt_sniffing_bau_record_set_nack_num();
                                }
                                else
                                {
                                    app_db.dongle_is_enable_mic = false;
                                    app_dongle_stop_recording(app_db.br_link[app_idx].bd_addr);
                                    app_dongle_sync_is_mic_enable_status();
                                    app_bt_sniffing_bau_record_set_nack_num();
#if F_APP_APT_SUPPORT
                                    if (app_db.dongle_is_disable_apt &&
                                        (!app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state)))
                                    {
                                        action = MMI_AUDIO_APT;
                                        app_db.dongle_is_disable_apt = false;
                                        app_dongle_control_apt(action);
                                    }
#endif
                                }
                            }
                        }
                    }
                }
            }
        }
        break;

    case BT_EVENT_SPP_CONN_IND:
        {
            if (param->spp_conn_ind.local_server_chann != RFC_SPP_DONGLE_CHANN_NUM)
            {
                return;
            }

            bt_spp_connect_cfm(param->spp_conn_ind.bd_addr, param->spp_conn_ind.local_server_chann, true,
                               param->spp_conn_ind.frame_size, 7);
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            if (param->spp_disconn_cmpl.local_server_chann != RFC_SPP_DONGLE_CHANN_NUM)
            {
                return;
            }

            is_dongle_spp_conn = false;
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true && (event_type != BT_EVENT_SPP_CREDIT_RCVD))
    {
        APP_PRINT_INFO1("app_dongle_spp_cback: event_type 0x%04x", event_type);
    }
}

void app_dongle_gaming_mode_request(bool status)
{
    uint8_t buf[6];
    uint8_t app_idx = app_get_active_a2dp_idx();

    buf[0] = VOICE_OVER_SPP_START_BIT;
    buf[1] = VOICE_OVER_SPP_CMD;
    buf[2] = VOICE_OVER_SPP_PAYLOAD_LEHGTH;
    buf[3] = VOICE_OVER_SPP_CMD_REQUEST_GAMING_MOE;
    buf[4] = status;
    buf[5] = VOICE_OVER_SPP_STOP_BIT;
    bt_spp_data_send(app_db.br_link[app_idx].bd_addr, RFC_SPP_DONGLE_CHANN_NUM, &buf[0], 6, false);
}

void app_dongle_updata_mic_data_idx(bool is_reset)
{
    if (is_reset)
    {
        mic_data_idx = 0;
    }
    else
    {
        mic_data_idx++;
        if (mic_data_idx > 0x3F)
        {
            mic_data_idx = 0;
        }
    }
}

void app_dongle_mic_data_report(void *data, uint16_t required_len)
{
    uint8_t buf[70];
    uint8_t app_idx = app_get_active_a2dp_idx();

    buf[0] = VOICE_OVER_SPP_START_BIT;
    buf[1] = VOICE_OVER_SPP_DATA | (mic_data_idx << 2);
    buf[2] = required_len;
    memcpy(&buf[3], data, required_len);
    buf[3 + required_len] = VOICE_OVER_SPP_STOP_BIT;

    app_dongle_updata_mic_data_idx(false);

    if (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE)
    {
        app_report_raw_data(CMD_PATH_SPP, app_idx, &buf[0], required_len + 4);
    }
}

static void app_dongle_spp_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {
            if (app_cfg_const.output_ind3_link_mic_toggle)
            {
                app_mmi_handle_action(MMI_OUTPUT_INDICATION3_TOGGLE);
            }
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            uint8_t app_idx = app_get_active_a2dp_idx();

            if (app_db.dongle_is_enable_mic)
            {
                app_mmi_handle_action(MMI_DEV_MIC_MUTE);
                app_dongle_stop_recording(app_db.br_link[app_idx].bd_addr);
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
        APP_PRINT_INFO1("app_dongle_spp_dm_cback: event_type 0x%04x", event_type);
    }
}

bool app_dongle_spp_init(void)
{
    APP_PRINT_TRACE0("app_dongle_spp_init");

    bt_spp_service_register((uint8_t *)dongle_service_class_uuid128, RFC_SPP_DONGLE_CHANN_NUM);
    sys_mgr_cback_register(app_dongle_spp_dm_cback);
    bt_mgr_cback_register(app_dongle_spp_cback);

    return true;
}

#endif
