/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "console.h"
#include "gap_scan.h"
#include "gap_timer.h"
#include "os_mem.h"
#include "ancs_client.h"
#include "gap.h"
#include "gap_legacy.h"
#include "gap_bond_legacy.h"
#include "gap_lib_common.h"
#include "cfg_item_api.h"
#include "audio.h"
#include "audio_probe.h"
#include "audio_volume.h"
#include "board.h"
#include "led_module.h"
#include "bt_hfp.h"
#include "bt_iap.h"
#include "btm.h"
#include "bt_bond.h"
#include "bt_types.h"
#include "bt_pbap.h"
#include "remote.h"
#include "stdlib.h"
#include "patch_header_check.h"
#include "fmc_api.h"
#include "test_mode.h"
#include "rtl876x_pinmux.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_audio_policy.h"
#include "app_transfer.h"
#include "app_report.h"
#include "app_ble_gap.h"
#include "app_ble_tts.h"
#include "app_bt_policy_api.h"
#include "app_led.h"
#include "app_mmi.h"
#include "app_cfg.h"
#include "app_ota.h"
#include "app_eq.h"
#include "app_relay.h"
#include "app_roleswap.h"
#include "app_multilink.h"
#include "app_hfp.h"
#include "app_linkback.h"
#include "app_bond.h"
#include "app_sensor.h"
#include "app_dlps.h"
#include "app_key_process.h"
#include "app_bt_policy_int.h"
#include "app_test.h"
#include "app_dsp_cfg.h"
#include "app_device.h"
#include "app_wdg.h"
#include "system_status_api.h"
#include "app_ble_rand_addr_mgr.h"
#include "app_ble_common_adv.h"
#if F_APP_CLI_BINARY_MP_SUPPORT
#include "mp_test.h"
#endif
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif
#if F_APP_ADC_SUPPORT
#include "app_adc.h"
#endif
#if F_APP_SPP_CAPTURE_DSP_DATA
#include "app_sniff_mode.h"
#include "pm.h"
#endif
#if F_APP_BRIGHTNESS_SUPPORT
#include "app_audio_passthrough_brightness.h"
#endif
#if F_APP_AVP_INIT_SUPPORT
#include "app_avp.h"
#endif
#if F_APP_PBAP_CMD_SUPPORT
#include "app_pbap.h"
#endif
#if F_APP_DUAL_AUDIO_EFFECT
#include "app_dual_audio_effect.h"
#endif
#if F_APP_ONE_WIRE_UART_SUPPORT
#include "app_one_wire_uart.h"
#endif

/* BBPro2 specialized feature */
#if F_APP_LOCAL_PLAYBACK_SUPPORT
#include "app_playback_update_file.h"
#endif
#if F_APP_HEARABLE_SUPPORT
#include "ftl.h"
#include "app_hearable.h"
#endif

#if APP_CAP_TOUCH_SUPPORT
#include "app_cap_touch.h"
#endif
#if GFPS_SASS_SUPPORT
#include "app_sass_policy.h"
#endif

// end of BBPro2 specialized feature

#if F_APP_SS_SUPPORT
#include "app_ss_cmd.h"
#endif

#if HARMAN_SPP_CMD_SUPPORT
#include "app_harman_license.h"
#endif

/* Define application support status */
#define SNK_SUPPORT_GET_SET_LE_NAME                 1
#define SNK_SUPPORT_GET_SET_BR_NAME                 1
#define SNK_SUPPORT_GET_SET_VP_LANGUAGE             1
#define SNK_SUPPORT_GET_BATTERY_LEVEL               1
#define SNK_SUPPORT_GET_SET_KEY_REMAP               F_APP_KEY_EXTEND_FEATURE

//for CMD_INFO_REQ
#define CMD_INFO_STATUS_VALID       0x00
#define CMD_INFO_STATUS_ERROR       0x01
#define CMD_SUPPORT_VER_CHECK_LEN   3

//for CMD_LINE_IN_CTRL
#define CFG_LINE_IN_STOP            0x00
#define CFG_LINE_IN_START           0x01

//for CMD_GET_FLASH_DATA and EVENT_REPORT_FLASH_DATA
#define START_TRANS                 0x00
#define CONTINUE_TRANS              0x01
#define SUPPORT_IMAGE_TYPE          0x02

#define TRANS_DATA_INFO             0x00
#define CONTINUE_TRANS_DATA         0x01
#define END_TRANS_DATA              0x02
#define SUPPORT_IMAGE_TYPE_INFO     0x03

#define SYSTEM_CONFIGS              0x00
#define ROM_PATCH_IMAGE             0x01
#define APP_IMAGE                   0x02
#define DSP_SYSTEM_IMAGE            0x03
#define DSP_APP_IMAGE               0x04
#define FTL_DATA                    0x05
#define ANC_IMAGE                   0x06
#define LOG_PARTITION               0x07
#define CORE_DUMP_PARTITION         0x08

#define FLASH_ALL                   0xFF
#define ALL_DUMP_IMAGE_MASK         ((0x01 << SYSTEM_CONFIGS) | (0x01 << ROM_PATCH_IMAGE) | (0x01 << APP_IMAGE) \
                                     | (0x01 << DSP_SYSTEM_IMAGE) | (0x01 << DSP_APP_IMAGE) \
                                     | (0x01 << FTL_DATA) |(0x01 << CORE_DUMP_PARTITION))

/* BBPro2 specialized feature */
//for CMD_DSP_DEBUG_SIGNAL_IN
#define CFG_SET_DSP_DEBUG_SIGNAL_IN     0x71
#define DSP_DEBUG_SIGNAL_IN_PAYLOAD_LEN 16
// end of BBPro2 specialized feature

#define CLEAR_BOND_INFO_SUCCESS     0x00
#define CLEAR_BOND_INFO_FAIL        0x01

#define F_APP_SUPPORT_SASS_SPP_CMD 1

typedef enum
{
    SASS_PREEMPTIVE_FEATURE_BIT_SET,
    SASS_PREEMPTIVE_FEATURE_BIT_GET,
    SASS_LINK_SWITCH,
    SASS_SWITCH_BACK,
    SASS_MULTILINK_ON_OFF,
} T_SASS_CMD;

#if F_APP_DEVICE_CMD_SUPPORT
static bool enable_auto_reject_conn_req_flag = false;
static bool enable_auto_accept_conn_req_flag = true;
static bool enable_report_attr_info_flag = false;
#endif

#if F_APP_SPP_CAPTURE_DSP_DATA
static uint8_t dsp_capture_data_master_retry = 0;
static uint8_t *dsp_spp_capture_cmd_ptr;
static uint8_t dsp_spp_capture_cmd_ptr_len;
static uint8_t dsp_spp_capture_app_idx;
static uint8_t dsp_capture_data_state = 0;

T_AUDIO_TRACK_HANDLE dsp_spp_capture_audio_track_handle;
#endif

static uint8_t uart_rx_seqn = 0; // uart receive sequence number
static uint8_t dlps_status = SET_DLPS_ENABLE;

static T_SRC_SUPPORT_VER_FORMAT src_support_version_br_link[MAX_BR_LINK_NUM];
static T_SRC_SUPPORT_VER_FORMAT src_support_version_le_link[MAX_BLE_LINK_NUM];
static T_SRC_SUPPORT_VER_FORMAT src_support_version_uart;

T_FLASH_DATA flash_data;

//for CMD_GET_CFG_SETTING
typedef enum
{
    CFG_GET_LE_NAME                  = 0x00,
    CFG_GET_LEGACY_NAME              = 0x01,
    CFG_GET_IC_NAME                  = 0x02,
    CFG_GET_COMPANY_ID_AND_MODEL_ID  = 0x03,
    CFG_GET_MAX                      = 0x04,
} T_CMD_GET_CFG_TYPE;

// for get FW version type
typedef enum
{
    GET_PRIMARY_FW_VERSION           = 0x00,
    GET_SECONDARY_FW_VERSION         = 0x01,
    GET_PRIMARY_OTA_FW_VERSION       = 0x02,
    GET_SECONDARY_OTA_FW_VERSION     = 0x03,
} T_CMD_GET_FW_VER_TYPE;

typedef enum
{
    APP_TIMER_SWITCH_TO_HCI_DOWNLOAD_MODE,
    APP_TIMER_ENTER_DUT_FROM_SPP_WAIT_ACK,
    APP_TIMER_OTA_JIG_DELAY_POWER_OFF,
    APP_TIMER_OTA_JIG_DELAY_WDG_RESET,
    APP_TIMER_OTA_JIG_DLPS_ENABLE,
    APP_TIMER_DSP_SPP_CAPTRUE_CHECK_LINK,
    APP_TIMER_IO_PIN_PULL_HIGH,
    APP_TIMER_STOP_PERIODIC_INQUIRY,
} T_APP_CMD_TIMER;

typedef enum
{
    MP_HCI_CMD_AUTO_XTAL_K          = 0x0600,
    MP_HCI_CMD_XTAL_VALUE           = 0x0601
} T_MP_HCI_CMD;

static uint8_t app_cmd_timer_queue_id = 0;
static void *timer_handle_switch_to_hci_mode = NULL;
static void *timer_handle_enter_dut_from_spp_wait_ack = NULL;
static void *timer_handle_io_pin_pull_high = NULL;

#if F_APP_SPP_CAPTURE_DSP_DATA
static void *timer_handle_dsp_spp_captrue_check_link = NULL;
#endif

#if F_APP_OTA_TOOLING_SUPPORT
static void *timer_handle_ota_parking_power_off = NULL;
static void *timer_handle_ota_parking_wdg_reset = NULL;
static void *timer_handle_ota_parking_dlps_enable = NULL;
#endif

#if F_APP_DEVICE_CMD_SUPPORT
static void *timer_handle_stop_periodic_inquiry = NULL;
#endif

static void app_cmd_handle_remote_cmd(uint16_t msg, void *buf, uint8_t len);

#if F_APP_DEVICE_CMD_SUPPORT
bool app_cmd_get_auto_reject_conn_req_flag(void)
{
    return enable_auto_reject_conn_req_flag;
}

bool app_cmd_get_auto_accept_conn_req_flag(void)
{
    return enable_auto_accept_conn_req_flag;
}

bool app_cmd_get_report_attr_info_flag(void)
{
    return enable_report_attr_info_flag;
}

void app_cmd_set_report_attr_info_flag(bool flag)
{
    enable_report_attr_info_flag = flag;
}
#endif

#if F_APP_SPP_CAPTURE_DSP_DATA
uint8_t app_cmd_dsp_capture_data_state(void)
{
    return dsp_capture_data_state;
}

bool app_cmd_spp_capture_executing_check(void)
{
    return (dsp_capture_data_state & (DSP_CAPTURE_DATA_LOG_EXECUTING | DSP_CAPTURE_RAW_DATA_EXECUTING))
           ? true : false;
}

#if F_APP_TEST_SUPPORT
bool app_cmd_spp_capture_audio_dsp_ctrl_send_handler(uint8_t *cmd_ptr, uint16_t cmd_len,
                                                     uint8_t cmd_path, uint8_t app_idx, uint8_t *ack_pkt, bool send_flag)
{
    bool send_cmd_flag = send_flag;
    uint32_t actual_mhz;
    uint16_t h2d_cmd_id = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));
    APP_PRINT_TRACE3("CMD_AUDIO_DSP_CTRL_SEND %x %x %x", cmd_ptr[2], h2d_cmd_id,
                     app_cfg_const.enable_dsp_capture_data_by_spp);

    //for capture dsp data handle stop cmd
    if (app_cfg_const.enable_dsp_capture_data_by_spp)
    {
        if (cmd_ptr[2] == H2D_CMD_DSP_DAC_ADC_DATA_TO_MCU)
        {
            uint8_t i = 10; //begin of para 1
            uint8_t end = 0;
            dsp_capture_data_state &= ~(DSP_CAPTURE_DATA_START_MASK | DSP_CAPTURE_DATA_SWAP_TO_MASTER);

            if (cmd_ptr[4] == VENDOR_SPP_CAPTURE_DSP_LOG)
            {
                end = 13;//end of para 2
            }
            else if (cmd_ptr[4] == VENDOR_SPP_CAPTURE_DSP_RWA_DATA)
            {
                end = 17; //end of para 3
            }
            APP_PRINT_TRACE1("CMD_AUDIO_DSP_CTRL_SEND %b", TRACE_BINARY(end, cmd_ptr));

            while (i <= end)
            {
                if (cmd_ptr[i] != 0)
                {
                    dsp_capture_data_state |= DSP_CAPTURE_DATA_START_MASK;
                    break;
                }
                i++;
            }

            if (dsp_capture_data_state & DSP_CAPTURE_DATA_START_MASK)
            {
                memset(&(g_vendor_spp_data), 0, sizeof(T_VENDOR_SPP_DATA));
                g_vendor_spp_data.spp_ptr = malloc(VENDOR_SPP_ALLOCATE_SIZE);
                g_temp_pkt = malloc(VENDOR_SPP_MAX_SIZE);
                send_cmd_flag = false;
                dsp_capture_data_state |= DSP_CAPTURE_DATA_SWAP_TO_MASTER;
                dsp_capture_data_state |= (cmd_ptr[4] == VENDOR_SPP_CAPTURE_DSP_LOG) ?
                                          DSP_CAPTURE_DATA_LOG_EXECUTING : DSP_CAPTURE_RAW_DATA_EXECUTING;
                app_dlps_disable(APP_DLPS_ENTER_CHECK_SPP_CAPTURE);
                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_SPP_CAPTURE);
                legacy_vendor_data_rate_set(0); // force 3M not support yet, 0 : 2M/3M

                bt_acl_pkt_type_set(app_db.br_link[app_idx].bd_addr, BT_ACL_PKT_TYPE_3M);
                dsp_capture_data_state &= ~DSP_CAPTURE_DATA_START_MASK;
                dsp_spp_capture_cmd_ptr = os_mem_alloc(RAM_TYPE_DATA_OFF, cmd_len + 2);
                dsp_spp_capture_cmd_ptr_len = cmd_len - 2;
                memcpy(dsp_spp_capture_cmd_ptr, &cmd_ptr[2], dsp_spp_capture_cmd_ptr_len);

                pm_cpu_freq_set(100, &actual_mhz);
                bt_link_qos_set(app_db.br_link[app_idx].bd_addr, BT_QOS_TYPE_GUARANTEED, 6);
                app_test_dsp_capture_data_set_param(app_idx);
                app_bt_policy_abandon_engage();

                if (app_hfp_get_call_status() ||
                    ((app_db.br_link[app_idx].acl_link_role == BT_LINK_ROLE_MASTER)))
                {
                    // send H2D_SPPCAPTURE_SET cmd
                    audio_probe_dsp_send(dsp_spp_capture_cmd_ptr, dsp_spp_capture_cmd_ptr_len);
                    free(dsp_spp_capture_cmd_ptr);
                }
                else
                {
                    bt_link_role_switch(app_db.br_link[app_idx].bd_addr, true);
                }
            }
            else
            {
                // reset to default
                dsp_capture_data_state &= ~((cmd_ptr[4] == VENDOR_SPP_CAPTURE_DSP_LOG) ?
                                            DSP_CAPTURE_DATA_LOG_EXECUTING : DSP_CAPTURE_RAW_DATA_EXECUTING);

                if ((dsp_capture_data_state & (DSP_CAPTURE_DATA_LOG_EXECUTING | DSP_CAPTURE_RAW_DATA_EXECUTING)) ==
                    0)
                {
                    app_dlps_enable(APP_DLPS_ENTER_CHECK_SPP_CAPTURE);
                    app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_SPP_CAPTURE);
                    bt_acl_pkt_type_set(app_db.br_link[app_idx].bd_addr, BT_ACL_PKT_TYPE_2M);
                }

                pm_cpu_freq_set(40, &actual_mhz);
                bt_link_role_switch(app_db.br_link[app_idx].bd_addr, false);
                bt_link_qos_set(app_db.br_link[app_idx].bd_addr, BT_QOS_TYPE_GUARANTEED, 40);
            }
        }
        else if (h2d_cmd_id == H2D_SPPCAPTURE_SET)
        {
            uint32_t plan_profs;
            uint32_t bond_flag;
            dsp_spp_capture_app_idx = app_idx;

            if (cmd_ptr[6] == CHANGE_MODE_TO_SCO)
            {
                send_cmd_flag = false;
                dsp_spp_capture_cmd_ptr = os_mem_alloc(RAM_TYPE_DATA_OFF, cmd_len + 2);
                dsp_spp_capture_cmd_ptr_len = cmd_len - 2;
                memcpy(dsp_spp_capture_cmd_ptr, &cmd_ptr[2], dsp_spp_capture_cmd_ptr_len);
                dsp_capture_data_state |= DSP_CAPTURE_DATA_CHANGE_MODE_TO_SCO_MASK;
                plan_profs = (app_db.br_link[app_idx].connected_profile & (~RDTP_PROFILE_MASK) &
                              (~SPP_PROFILE_MASK));

                if (plan_profs)
                {
                    app_bt_policy_disconnect(app_db.br_link[app_idx].bd_addr, plan_profs);
                }

                gap_start_timer(&timer_handle_dsp_spp_captrue_check_link, "dsp_spp_captrue_check_link",
                                app_cmd_timer_queue_id,
                                APP_TIMER_DSP_SPP_CAPTRUE_CHECK_LINK, app_idx, 1500);
            }
            else if (cmd_ptr[6] == CHANGE_MODE_EXIST)
            {
                if (dsp_spp_capture_audio_track_handle != NULL)
                {
                    audio_track_release(dsp_spp_capture_audio_track_handle);
                }
                dsp_capture_data_state &= ~DSP_CAPTURE_DATA_CHANGE_MODE_TO_SCO_MASK;

                bt_bond_flag_get(app_db.br_link[app_idx].bd_addr, &bond_flag);
                if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
                {
                    plan_profs = get_profs_by_bond_flag(bond_flag);
                    app_bt_policy_default_connect(app_db.br_link[app_idx].bd_addr, plan_profs, false);
                }
            }
        }
    }

    return send_cmd_flag;
}
#endif
#endif

static void app_cmd_bt_event_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
#if (F_APP_SPP_CAPTURE_DSP_DATA == 1)
    T_BT_EVENT_PARAM *param = event_buf;
#endif
    bool handle = true;

    switch (event_type)
    {
#if F_APP_SPP_CAPTURE_DSP_DATA
    case BT_EVENT_ACL_ROLE_MASTER:
        {
            if (dsp_capture_data_state & DSP_CAPTURE_DATA_SWAP_TO_MASTER)
            {
                audio_probe_dsp_send(dsp_spp_capture_cmd_ptr, dsp_spp_capture_cmd_ptr_len);
                dsp_capture_data_state |= DSP_CAPTURE_DATA_START_MASK;
                free(dsp_spp_capture_cmd_ptr);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SWITCH_FAIL:
        {
            if (dsp_capture_data_state & DSP_CAPTURE_DATA_SWAP_TO_MASTER)
            {
                if (dsp_capture_data_master_retry < 3)
                {
                    bt_link_role_switch(param->acl_role_switch_fail.bd_addr, true);
                    dsp_capture_data_master_retry++;
                }
                else
                {
                    audio_probe_dsp_send(dsp_spp_capture_cmd_ptr, dsp_spp_capture_cmd_ptr_len);
                    dsp_capture_data_state |= DSP_CAPTURE_DATA_START_MASK;
                    dsp_capture_data_state &= ~DSP_CAPTURE_DATA_SWAP_TO_MASTER;
                    dsp_capture_data_master_retry = 0;
                    free(dsp_spp_capture_cmd_ptr);
                }
            }
        }
        break;
#endif

    case BT_EVENT_ACL_CONN_IND:
        {
            //Stop periodic inquiry when connecting
#if F_APP_DEVICE_CMD_SUPPORT
            gap_stop_timer(&timer_handle_stop_periodic_inquiry);
#endif
            bt_periodic_inquiry_stop();
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_cmd_bt_cback: event_type 0x%04x", event_type);
    }
}

static void app_cmd_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
#if F_APP_SPP_CAPTURE_DSP_DATA
    T_AUDIO_EVENT_PARAM *param = event_buf;
#endif
    bool handle = true;

    switch (event_type)
    {
#if F_APP_SPP_CAPTURE_DSP_DATA
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            if (param->track_state_changed.handle == dsp_spp_capture_audio_track_handle)
            {
                APP_PRINT_ERROR1("track_state, %x", param->track_state_changed.state);

                if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED)
                {
                    uint8_t send_pkt_num = 5;
                    uint8_t seq_num;
                    uint8_t p_audio_buf[4] = {0, 0, 0, 0};
                    uint8_t buf[4] = {0, 0, 0, 0};

                    for (seq_num = 0; seq_num < send_pkt_num; seq_num++)
                    {
                        uint16_t written_len;
                        audio_track_write(dsp_spp_capture_audio_track_handle,
                                          0,//              timestamp,
                                          seq_num,
                                          AUDIO_STREAM_STATUS_CORRECT,
                                          1,//            frame_num,
                                          (uint8_t *)p_audio_buf,
                                          sizeof(p_audio_buf),
                                          &written_len);
                    }

                    dsp_capture_data_state |= DSP_CAPTURE_DATA_ENTER_SCO_MODE_MASK;

                    // send H2D_SPPCAPTURE_SET cmd
                    audio_probe_dsp_send(dsp_spp_capture_cmd_ptr, dsp_spp_capture_cmd_ptr_len);
                    buf[0] = 1;
                    app_report_event(CMD_PATH_SPP, EVENT_AUDIO_DSP_CTRL_INFO, dsp_spp_capture_app_idx, buf, 4);
                    free(dsp_spp_capture_cmd_ptr);
                }

                if (param->track_state_changed.state == AUDIO_TRACK_STATE_RELEASED)
                {
                    dsp_spp_capture_audio_track_handle = NULL;
                    dsp_capture_data_state &= ~DSP_CAPTURE_DATA_ENTER_SCO_MODE_MASK;
                }
            }
        }
        break;

    case AUDIO_EVENT_TRACK_DATA_IND:
        {
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (param->track_state_changed.handle == dsp_spp_capture_audio_track_handle)
                {
                    T_APP_BR_LINK *p_link;
                    p_link = app_find_br_link(app_db.br_link[dsp_spp_capture_app_idx].bd_addr);
                    if (p_link == NULL)
                    {
                        break;
                    }

                    APP_PRINT_TRACE1("app_cmd_audio_cback: data ind len %u", param->track_data_ind.len);
                    uint32_t timestamp;
                    uint16_t seq_num;
                    uint8_t frame_num;
                    uint16_t read_len;
                    T_AUDIO_STREAM_STATUS status;
                    uint8_t *buf;

                    buf = malloc(param->track_data_ind.len);

                    if (buf == NULL)
                    {
                        return;
                    }

                    if (audio_track_read(dsp_spp_capture_audio_track_handle,
                                         &timestamp,
                                         &seq_num,
                                         &status,
                                         &frame_num,
                                         buf,
                                         param->track_data_ind.len,
                                         &read_len) == true)
                    {
                        if (p_link->duplicate_fst_sco_data)
                        {
                            p_link->duplicate_fst_sco_data = false;
                            bt_sco_data_send(app_db.br_link[dsp_spp_capture_app_idx].bd_addr, seq_num - 1, buf, read_len);
                        }
                        bt_sco_data_send(app_db.br_link[dsp_spp_capture_app_idx].bd_addr, seq_num, buf, read_len);
                    }

                    free(buf);
                }
            }
        }
        break;
#endif

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_cmd_audio_cback: event_type 0x%04x", event_type);
    }
}

static void app_cmd_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case APP_TIMER_SWITCH_TO_HCI_DOWNLOAD_MODE:
        {
            if (timer_handle_switch_to_hci_mode != NULL)
            {
                gap_stop_timer(&timer_handle_switch_to_hci_mode);
                sys_hall_set_hci_download_mode(true);
                set_hci_mode_flag(true);
                app_wdg_reset(RESET_ALL_EXCEPT_AON);
            }
        }
        break;

    case APP_TIMER_ENTER_DUT_FROM_SPP_WAIT_ACK:
        {
            if (timer_handle_enter_dut_from_spp_wait_ack != NULL)
            {
                gap_stop_timer(&timer_handle_enter_dut_from_spp_wait_ack);
                app_mmi_handle_action(MMI_ENTER_DUT_FROM_SPP);
            }
        }
        break;

#if (F_APP_SPP_CAPTURE_DSP_DATA == 1)
    case APP_TIMER_DSP_SPP_CAPTRUE_CHECK_LINK:
        {
            if (timer_handle_dsp_spp_captrue_check_link != NULL)
            {
                gap_stop_timer(&timer_handle_dsp_spp_captrue_check_link);

                if ((app_db.br_link[timer_chann].connected_profile & (~RDTP_PROFILE_MASK) & (~SPP_PROFILE_MASK)))
                {
                    gap_start_timer(&timer_handle_dsp_spp_captrue_check_link, "dsp_spp_captrue_check_link",
                                    app_cmd_timer_queue_id,
                                    APP_TIMER_DSP_SPP_CAPTRUE_CHECK_LINK, timer_chann, 1500);
                }
                else
                {
                    T_AUDIO_FORMAT_INFO format_info;

                    format_info.type = AUDIO_FORMAT_TYPE_MSBC;
                    format_info.attr.msbc.chann_mode = AUDIO_SBC_CHANNEL_MODE_MONO;
                    format_info.attr.msbc.sample_rate = 16000;
                    format_info.attr.msbc.bitpool = 26;
                    format_info.attr.msbc.allocation_method = 0;
                    format_info.attr.msbc.subband_num = 8;
                    format_info.attr.msbc.block_length = 15;

                    if (dsp_spp_capture_audio_track_handle)
                    {
                        audio_track_release(dsp_spp_capture_audio_track_handle);
                    }

                    dsp_spp_capture_audio_track_handle = audio_track_create(AUDIO_STREAM_TYPE_VOICE,
                                                                            AUDIO_STREAM_MODE_NORMAL,
                                                                            AUDIO_STREAM_USAGE_LOCAL,
                                                                            format_info,
                                                                            app_cfg_const.voice_out_volume_default,
                                                                            app_cfg_const.voice_volume_in_default,
                                                                            AUDIO_DEVICE_OUT_DEFAULT | AUDIO_DEVICE_IN_DEFAULT,
                                                                            NULL,
                                                                            NULL);

                    audio_track_latency_set(dsp_spp_capture_audio_track_handle, 15, false);
                    audio_track_start(dsp_spp_capture_audio_track_handle);
                }
            }
        }
        break;
#endif

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    case APP_TIMER_OTA_JIG_DELAY_POWER_OFF:
        {
            if (timer_handle_ota_parking_power_off != NULL)
            {
                gap_stop_timer(&timer_handle_ota_parking_power_off);

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    app_relay_async_single(APP_MODULE_TYPE_OTA, APP_REMOTE_MSG_OTA_PARKING);
                }
                else
                {
                    app_cmd_ota_tooling_parking();
                }
            }
        }
        break;

    case APP_TIMER_OTA_JIG_DELAY_WDG_RESET:
        {
            if (timer_handle_ota_parking_wdg_reset != NULL)
            {
                gap_stop_timer(&timer_handle_ota_parking_wdg_reset);
                app_wdg_reset(RESET_ALL);
            }
        }
        break;

    case APP_TIMER_OTA_JIG_DLPS_ENABLE:
        {
            if (timer_handle_ota_parking_dlps_enable != NULL)
            {
                gap_stop_timer(&timer_handle_ota_parking_dlps_enable);
                app_dlps_enable(APP_DLPS_ENTER_OTA_TOOLING_PARK);
            }
        }
        break;
#endif

    case APP_TIMER_IO_PIN_PULL_HIGH:
        {
            if (timer_handle_io_pin_pull_high != NULL)
            {
                gap_stop_timer(&timer_handle_io_pin_pull_high);

                uint8_t pin_num = (uint8_t)timer_chann;

                Pad_Config(pin_num, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            }
        }
        break;

#if F_APP_DEVICE_CMD_SUPPORT
    case APP_TIMER_STOP_PERIODIC_INQUIRY:
        {
            if (timer_handle_stop_periodic_inquiry != NULL)
            {
                gap_stop_timer(&timer_handle_stop_periodic_inquiry);
                bt_periodic_inquiry_stop();
            }
        }
        break;
#endif

    default:
        break;
    }
}

uint16_t app_cmd_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    uint8_t eq_ctrl_by_src = app_db.eq_ctrl_by_src;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_SYNC_EQ_CTRL_BY_SRC:
        {
            msg_ptr = (uint8_t *)&eq_ctrl_by_src;
            payload_len = 1;
            skip = false;
        }
        break;

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_CMD, payload_len, msg_ptr, skip, total);
}

static void app_cmd_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_CMD_GET_FW_VERSION:
    case APP_REMOTE_MSG_CMD_REPORT_FW_VERSION:
    case APP_REMOTE_MSG_CMD_GET_OTA_FW_VERSION:
    case APP_REMOTE_MSG_CMD_REPORT_OTA_FW_VERSION:
        {
            app_cmd_handle_remote_cmd(msg_type, buf, len);
        }
        break;

    case APP_REMOTE_MSG_SYNC_EQ_CTRL_BY_SRC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_cmd_update_eq_ctrl(*((uint8_t *)buf), false);
            }
        }
        break;

    /* BBPro2 specialized feature*/
    case APP_REMOTE_MSG_DSP_DEBUG_SIGNAL_IN_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_probe_dsp_send(buf, len);
            }
        }
        break;
    // end of BBPro2 specialized feature

    default:
        break;
    }
}

void app_cmd_init(void)
{
    audio_mgr_cback_register(app_cmd_audio_cback);
    bt_mgr_cback_register(app_cmd_bt_event_cback);
    gap_reg_timer_cb(app_cmd_timeout_cb, &app_cmd_timer_queue_id);
    app_relay_cback_register(app_cmd_relay_cback, app_cmd_parse_cback,
                             APP_MODULE_TYPE_CMD, APP_REMOTE_MSG_CMD_TOTAL);
}

void app_cmd_set_event_broadcast(uint16_t event_id, uint8_t *buf, uint16_t len)
{
    T_APP_BR_LINK *br_link;
    T_APP_LE_LINK *le_link;
    uint8_t        i;

    for (i = 0; i < MAX_BR_LINK_NUM; i ++)
    {
        br_link = &app_db.br_link[i];

        if (br_link->cmd_set_enable == true)
        {
            if (br_link->connected_profile & SPP_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_SPP, event_id, i, buf, len);
            }

            if (br_link->connected_profile & IAP_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_IAP, event_id, i, buf, len);
            }
        }
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        le_link = &app_db.le_link[i];

        if (le_link->state == LE_LINK_STATE_CONNECTED)
        {
            if (le_link->cmd_set_enable == true)
            {
                app_report_event(CMD_PATH_LE, event_id, i, buf, len);
            }
        }
    }
}

void app_read_flash(uint32_t start_addr, uint8_t cmd_path, uint8_t app_idx)
{
    uint32_t start_addr_tmp;
    uint16_t data_send_len;

    data_send_len = 0x200;// in case assert fail
    start_addr_tmp = start_addr;

    if (cmd_path == CMD_PATH_SPP)
    {
        APP_PRINT_TRACE1("app_read_flash: rfc_frame_size %d", app_db.br_link[app_idx].rfc_frame_size);
        if (app_db.br_link[app_idx].rfc_frame_size - 12 < data_send_len)
        {
            data_send_len = app_db.br_link[app_idx].rfc_frame_size - 12;
        }
    }
    else if (cmd_path == CMD_PATH_IAP)
    {
        APP_PRINT_TRACE1("app_read_flash: iap frame_size %d", app_db.br_link[app_idx].iap.frame_size);
        if (app_db.br_link[app_idx].iap.frame_size - 12 < data_send_len)
        {
            data_send_len = app_db.br_link[app_idx].iap.frame_size - 12;
        }
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        APP_PRINT_TRACE1("app_read_flash: mtu_size %d", app_db.le_link[app_idx].mtu_size);
        if (app_db.le_link[app_idx].mtu_size - 15 < data_send_len)
        {
            data_send_len = app_db.le_link[app_idx].mtu_size - 15;
        }
    }

    uint8_t *data = malloc(data_send_len + 6);

    if (data != NULL)
    {
        if (start_addr + data_send_len >= flash_data.flash_data_start_addr + flash_data.flash_data_size)
        {
            data_send_len = flash_data.flash_data_start_addr + flash_data.flash_data_size - start_addr;
            data[0] = END_TRANS_DATA;
        }
        else
        {
            data[0] = CONTINUE_TRANS_DATA;
        }

        data[1] = flash_data.flash_data_type;
        data[2] = (uint8_t)(start_addr_tmp);
        data[3] = (uint8_t)(start_addr_tmp >> 8);
        data[4] = (uint8_t)(start_addr_tmp >> 16);
        data[5] = (uint8_t)(start_addr_tmp >> 24);

        if (fmc_flash_nor_read(start_addr_tmp, &data[6], data_send_len))// read flash data
        {
            app_report_event(cmd_path, EVENT_REPORT_FLASH_DATA, app_idx, data, data_send_len + 6);
        }

        flash_data.flash_data_start_addr_tmp += data_send_len;
        free(data);
    }
}

//T_FLASH_DATA initialization
void app_flash_data_set_param(uint8_t flash_type, uint8_t cmd_path, uint8_t app_idx)
{
    flash_data.flash_data_type = flash_type;
    flash_data.flash_data_start_addr = 0x800000;
    flash_data.flash_data_size = 0x00;

    switch (flash_type)
    {
    case FLASH_ALL:
        {
            flash_data.flash_data_start_addr = 0x800000;
            flash_data.flash_data_size = 0x100000;
        }
        break;

    case SYSTEM_CONFIGS:
        {
            flash_data.flash_data_start_addr = flash_partition_addr_get(PARTITION_FLASH_OCCD);
            flash_data.flash_data_size = flash_partition_size_get(PARTITION_FLASH_OCCD) & 0x00FFFFFF;
        }
        break;

    case ROM_PATCH_IMAGE:
        {
            flash_data.flash_data_start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_MCUPATCH);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_MCUPATCH);
        }
        break;

    case APP_IMAGE:
        {
            flash_data.flash_data_start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_MCUAPP);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_MCUAPP);
        }
        break;

    case DSP_SYSTEM_IMAGE:
        {
            flash_data.flash_data_start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_DSPPATCH);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_DSPPATCH);
        }
        break;

    case DSP_APP_IMAGE:
        {
            flash_data.flash_data_start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_DSPAPP);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_DSPAPP);
        }
        break;

    case FTL_DATA:
        {
            flash_data.flash_data_start_addr = flash_partition_addr_get(PARTITION_FLASH_FTL);
            flash_data.flash_data_size = flash_partition_size_get(PARTITION_FLASH_FTL) & 0x00FFFFFF;
        }
        break;

    case ANC_IMAGE:
        {
            flash_data.flash_data_start_addr = flash_cur_bank_img_header_addr_get(FLASH_IMG_ANC);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_ANC);
        }
        break;

    case LOG_PARTITION:
        {
            //add later;
        }
        break;

    case CORE_DUMP_PARTITION:
        {
            flash_data.flash_data_start_addr = flash_partition_addr_get(PARTITION_FLASH_HARDFAULT_RECORD);
            flash_data.flash_data_size = flash_partition_size_get(PARTITION_FLASH_HARDFAULT_RECORD);
        }
        break;

    default:
        break;
    }

    flash_data.flash_data_start_addr_tmp = flash_data.flash_data_start_addr;

    //report TRANS_DATA_INFO param
    uint8_t paras[10];

    paras[0] = TRANS_DATA_INFO;
    paras[1] = flash_data.flash_data_type;

    paras[2] = (uint8_t)(flash_data.flash_data_size);
    paras[3] = (uint8_t)(flash_data.flash_data_size >> 8);
    paras[4] = (uint8_t)(flash_data.flash_data_size >> 16);
    paras[5] = (uint8_t)(flash_data.flash_data_size >> 24);

    paras[6] = (uint8_t)(flash_data.flash_data_start_addr);
    paras[7] = (uint8_t)(flash_data.flash_data_start_addr >> 8);
    paras[8] = (uint8_t)(flash_data.flash_data_start_addr >> 16);
    paras[9] = (uint8_t)(flash_data.flash_data_start_addr >> 24);

    app_report_event(cmd_path, EVENT_REPORT_FLASH_DATA, app_idx, paras, sizeof(paras));
}

T_SNK_CAPABILITY app_cmd_get_system_capability(void)
{
    T_SNK_CAPABILITY snk_capability;

    memset(&snk_capability, 0, sizeof(T_SNK_CAPABILITY));
    snk_capability.snk_support_get_set_le_name = SNK_SUPPORT_GET_SET_LE_NAME;
    snk_capability.snk_support_get_set_br_name = SNK_SUPPORT_GET_SET_BR_NAME;
    snk_capability.snk_support_get_set_vp_language = SNK_SUPPORT_GET_SET_VP_LANGUAGE;
    snk_capability.snk_support_get_battery_info = SNK_SUPPORT_GET_BATTERY_LEVEL;
    snk_capability.snk_support_ota = true;
#if F_APP_TTS_SUPPORT
    snk_capability.snk_support_tts = app_cfg_const.tts_support;
#else
    snk_capability.snk_support_tts = 0;
#endif

    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        snk_capability.snk_support_change_channel = 1;
        snk_capability.snk_support_get_set_rws_state = 1;
    }
    snk_capability.snk_support_get_set_apt_state = app_cfg_const.normal_apt_support;
    snk_capability.snk_support_get_set_eq_state = true;
    snk_capability.snk_support_get_set_vad_state = false;

#if F_APP_ANC_SUPPORT
    snk_capability.snk_support_get_set_anc_state = (app_anc_get_activated_scenario_cnt() > 0);
    snk_capability.snk_support_get_set_listening_mode_cycle = true;
    snk_capability.snk_support_anc_scenario_choose = (app_anc_get_activated_scenario_cnt() > 1);
#endif
#if F_APP_APT_SUPPORT
    snk_capability.snk_support_get_set_llapt_state = (app_cfg_const.llapt_support &&
                                                      (app_apt_get_llapt_activated_scenario_cnt() > 0));
#endif
    snk_capability.snk_support_ansc = false;
    snk_capability.snk_support_vibrator = false;
    snk_capability.snk_support_gaming_mode = true;

#if F_APP_KEY_EXTEND_FEATURE
    snk_capability.snk_support_key_remap = SNK_SUPPORT_GET_SET_KEY_REMAP;
    snk_capability.snk_support_reset_key_remap = SNK_SUPPORT_GET_SET_KEY_REMAP;
    snk_capability.snk_support_reset_key_map_by_bud = SNK_SUPPORT_GET_SET_KEY_REMAP;

#if F_APP_RWS_KEY_SUPPORT
    snk_capability.snk_support_rws_key_remap = app_key_is_rws_key_setting();
#endif
#endif

    snk_capability.snk_support_gaming_mode_eq = eq_utils_num_get(SPK_SW_EQ, GAMING_MODE) >= 1;
    snk_capability.snk_support_anc_eq = eq_utils_num_get(SPK_SW_EQ, ANC_MODE) >= 1;
    snk_capability.snk_support_multilink_support = app_cfg_const.enable_multi_link;
    snk_capability.snk_support_phone_set_anc_eq = true;
    snk_capability.snk_support_new_report_bud_status_flow = true;

#if F_APP_SENSOR_SUPPORT
    if (app_cfg_const.sensor_support)
    {
        snk_capability.snk_support_ear_detection = true;
    }
#endif

#if F_APP_AVP_INIT_SUPPORT
    snk_capability.snk_support_avp = true;
    snk_capability.snk_support_multilink_support = true;
#else
    snk_capability.snk_support_multilink_support = app_cfg_const.enable_multi_link;
#endif

#if NEW_FORMAT_LISTENING_CMD_REPORT
    snk_capability.snk_support_new_report_listening_status = true;
#endif

#if F_APP_APT_SUPPORT
    if ((app_cfg_const.normal_apt_support) && (eq_utils_num_get(MIC_SW_EQ, APT_MODE) != 0))
    {
        snk_capability.snk_support_apt_eq = true;

        /* RHE related features */
#if F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
        snk_capability.snk_support_apt_eq_adjust_separate = app_cfg_const.rws_apt_eq_adjust_separate;
#endif
    }

#if F_APP_BRIGHTNESS_SUPPORT
    snk_capability.snk_support_llapt_brightness = (app_apt_brightness_get_support_bitmap() != 0x0);
#endif
#if F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
    snk_capability.snk_support_llapt_scenario_choose = (app_apt_get_llapt_activated_scenario_cnt() > 1);
#endif
#if F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
    snk_capability.snk_support_power_on_delay_apply_apt_on = true;
#endif
#if (F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT == 0)
    snk_capability.snk_support_apt_volume_force_adjust_sync = true;
#endif
#endif

#if F_APP_ADJUST_TONE_VOLUME_SUPPORT
    snk_capability.snk_support_tone_volume_adjustment = true;
#endif
// end of RHE related feature

    /* BBPro2 specialized feature */
#if (F_APP_LOCAL_PLAYBACK_SUPPORT == 1)
    snk_capability.snk_support_local_playback = app_cfg_const.local_playback_support;
#endif
#if F_APP_HEARABLE_SUPPORT
    snk_capability.snk_support_HA = 1;
#endif
// end of BBPro2 specialized feature

    return snk_capability;
}

static void app_cmd_get_fw_version(uint8_t *p_data)
{
    uint8_t temp_buff[13];
    T_IMG_HEADER_FORMAT *p_app_header = (T_IMG_HEADER_FORMAT *)flash_cur_bank_img_header_addr_get(
                                            FLASH_IMG_MCUAPP);
    T_IMG_HEADER_FORMAT *p_patch_header = (T_IMG_HEADER_FORMAT *)flash_cur_bank_img_header_addr_get(
                                              FLASH_IMG_MCUPATCH);
    T_PATCH_IMG_VER_FORMAT *p_patch_img_ver = (T_PATCH_IMG_VER_FORMAT *) & (p_patch_header->git_ver);

    T_IMG_HEADER_FORMAT *p_app_ui_header = (T_IMG_HEADER_FORMAT *)flash_cur_bank_img_header_addr_get(
                                               FLASH_IMG_MCUDATA);
    T_APP_UI_IMG_VER_FORMAT *p_app_ui_ver = (T_APP_UI_IMG_VER_FORMAT *) & (p_app_ui_header->git_ver);

    temp_buff[0] = p_app_header->git_ver.sub_version._version_major;
    temp_buff[1] = p_app_header->git_ver.sub_version._version_minor;
    temp_buff[2] = p_app_header->git_ver.sub_version._version_revision;

    // currently 5 bits, must be 0
    temp_buff[3] = 0; // p_app_header->git_ver.sub_version._version_reserve >> 8;
    temp_buff[4] = p_app_header->git_ver.sub_version._version_reserve;

    temp_buff[5] = p_patch_img_ver->ver_major;
    temp_buff[6] = p_patch_img_ver->ver_minor;
    temp_buff[7] = p_patch_img_ver->ver_revision >> 8;
    temp_buff[8] = p_patch_img_ver->ver_revision;

    temp_buff[9] = p_app_ui_ver->ver_reserved;
    temp_buff[10] = p_app_ui_ver->ver_revision;
    temp_buff[11] = p_app_ui_ver->ver_minor;
    temp_buff[12] = p_app_ui_ver->ver_major;

    memcpy((void *)p_data, (void *)&temp_buff, 13);
}

#if F_APP_OTA_TOOLING_SUPPORT
void app_cmd_ota_tooling_parking(void)
{
    APP_PRINT_INFO2("app_cmd_ota_tooling_parking %d, %d", app_cfg_nv.ota_tooling_start,
                    app_db.device_state);

    app_dlps_disable(APP_DLPS_ENTER_OTA_TOOLING_PARK);
    gap_start_timer(&timer_handle_ota_parking_dlps_enable, "ota_parking_dlps_enable",
                    app_cmd_timer_queue_id,
                    APP_TIMER_OTA_JIG_DLPS_ENABLE, NULL, 3500);

    // clear phone record
    app_bond_clear_non_rws_keys();

    // clear dongle info
    app_db.jig_subcmd = 0;
    app_db.jig_dongle_id = 0;
    app_db.ota_tooling_start = 0;

    // remove OTA power on flag
    if (app_cfg_nv.ota_tooling_start)
    {
        app_cfg_nv.ota_tooling_start = 0;
        app_cfg_store(&app_cfg_nv.eq_idx_anc_mode_record, 4);
    }

    // power off
    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        app_db.power_off_cause = POWER_OFF_CAUSE_OTA_TOOL;
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
        gap_start_timer(&timer_handle_ota_parking_wdg_reset, "ota_jig_delay_wdg_reset",
                        app_cmd_timer_queue_id,
                        APP_TIMER_OTA_JIG_DELAY_WDG_RESET, NULL, 3000);
    }
}

void app_cmd_stop_ota_parking_power_off(void)
{
    // avoid timeout to clear ota dongle info
    gap_stop_timer(&timer_handle_ota_parking_power_off);

    // avoid timeout to reset system when receive new dongle command
    gap_stop_timer(&timer_handle_ota_parking_wdg_reset);

    // clear set dlps
    app_cfg_nv.need_set_lps_mode = 0;
}
#endif

bool app_cmd_relay_command_set(uint16_t cmd_id, uint8_t *cmd_ptr, uint16_t cmd_len,
                               T_APP_MODULE_TYPE module_type, uint8_t relay_cmd_id, bool sync)
{
    uint8_t error_code = 0;

    uint8_t *relay_cmd;
    uint16_t total_len;

    relay_cmd = NULL;
    total_len = 5 + cmd_len;
    relay_cmd = (uint8_t *)malloc(total_len);

    if (relay_cmd == NULL)
    {
        error_code = 1;
        goto SKIP;
    }

    /* bypass_cmd             *
     * byte [0,1]  : cmd_id   *
     * byte [2,3]  : cmd_len  *
     * byte [4]    : cmd_path *
     * byte [5-N]  : cmd      */

    relay_cmd[0] = (uint8_t)cmd_id;
    relay_cmd[1] = (uint8_t)(cmd_id >> 8);
    relay_cmd[2] = (uint8_t)cmd_len;
    relay_cmd[3] = (uint8_t)(cmd_len >> 8);


    memcpy(&relay_cmd[5], &cmd_ptr[0], cmd_len);

    if (sync)
    {
        relay_cmd[4] = CMD_PATH_RWS_SYNC;

        if (app_relay_sync_single_with_raw_msg(module_type, relay_cmd_id, relay_cmd, total_len,
                                               REMOTE_TIMER_HIGH_PRECISION, 0, false) == false)
        {
            error_code = 2;
            free(relay_cmd);
            goto SKIP;
        }
    }
    else
    {
        relay_cmd[4] = CMD_PATH_RWS_ASYNC;

        if (app_relay_async_single_with_raw_msg(module_type, relay_cmd_id, relay_cmd, total_len) == false)
        {
            error_code = 3;
            free(relay_cmd);
            goto SKIP;
        }
    }

    free(relay_cmd);
    return true;

SKIP:
    APP_PRINT_INFO2("app_cmd_relay_command_set fail cmd_id = %x, error = %d", cmd_id, error_code);
    return false;
}

bool app_cmd_relay_event(uint16_t event_id, uint8_t *event_ptr, uint16_t event_len,
                         T_APP_MODULE_TYPE module_type, uint8_t relay_event_id)
{
    uint8_t error_code = 0;

    uint16_t total_len;
    uint8_t *report_event;

    total_len = 4 + event_len;

    report_event = (uint8_t *)malloc(total_len);

    if (report_event == NULL)
    {
        error_code = 1;
        goto SKIP;
    }

    /* report
     * byte [0,1] : event_id    *
     * byte [2,3] : report_len  *
     * byte [4-N] : report      */

    report_event[0] = (uint8_t)event_id;
    report_event[1] = (uint8_t)(event_id >> 8);
    report_event[2] = (uint8_t)event_len;
    report_event[3] = (uint8_t)(event_len >> 8);

    memcpy(&report_event[4], &event_ptr[0], event_len);

    if (app_relay_async_single_with_raw_msg(module_type, relay_event_id,
                                            report_event, total_len) == false)
    {
        error_code = 2;
        free(report_event);
        goto SKIP;
    }

    free(report_event);
    return true;

SKIP:
    APP_PRINT_INFO2("app_cmd_relay_event fail cmd_id = %x, error = %d", event_id,
                    error_code);
    return false;
}

static void app_cmd_handle_remote_cmd(uint16_t msg, void *buf, uint8_t len)
{
    switch (msg)
    {
#if (F_APP_OTA_TOOLING_SUPPORT == 1)
    case APP_REMOTE_MSG_CMD_GET_FW_VERSION:
        {
            uint8_t *p_info = (uint8_t *)buf;

            // get secondary's fw version by app_cmd_get_fw_version()
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint8_t data[15] = {0};
                data[0] = p_info[0];    // cmd path
                data[1] = p_info[1];    // app idx

                app_cmd_get_fw_version(&data[2]);
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_CMD, APP_REMOTE_MSG_CMD_REPORT_FW_VERSION,
                                                    data, sizeof(data));
            }
        }
        break;

    case APP_REMOTE_MSG_CMD_REPORT_FW_VERSION:
        {
            uint8_t *p_info = (uint8_t *)buf;
            uint8_t report_data[9];

            memcpy(&report_data[0], &p_info[2], 9);

            // primary return secondary's fw version
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_report_event(p_info[0], EVENT_FW_VERSION, p_info[1], report_data, sizeof(report_data));
            }
        }
        break;

    case APP_REMOTE_MSG_CMD_GET_OTA_FW_VERSION:
        {
            uint8_t *p_info = (uint8_t *)buf;

            // get secondary's fw version by app_ota_get_brief_img_version_for_dongle()
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint8_t data[IMG_LEN_FOR_DONGLE + 3] = {0};
                data[0] = p_info[0];    // cmd path
                data[1] = p_info[1];    // app idx
                data[2] = GET_SECONDARY_OTA_FW_VERSION;
                app_ota_get_brief_img_version_for_dongle(&data[3]);
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_CMD, APP_REMOTE_MSG_CMD_REPORT_OTA_FW_VERSION,
                                                    data, sizeof(data));
            }
        }
        break;

    case APP_REMOTE_MSG_CMD_REPORT_OTA_FW_VERSION:
        {
            uint8_t *p_info = (uint8_t *)buf;
            uint8_t report_data[IMG_LEN_FOR_DONGLE + 1];

            memcpy(&report_data[0], &p_info[2], IMG_LEN_FOR_DONGLE + 1);

            // primary return secondary's fw version
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_report_event(p_info[0], EVENT_FW_VERSION, p_info[1], report_data, sizeof(report_data));
            }
        }
        break;
#endif

    default:
        break;
    }
}

static void app_cmd_rf_xtak_k(uint8_t channel, uint8_t upperbound, uint8_t lowerbound,
                              uint8_t offset)
{
    uint8_t params[6];

    params[0] = 0x06; //Module ID, MODULE_XTAL = 0x06
    params[1] = 0x00; //Subcmd, AUTO_K = 0x00

    /* cmd params */
    params[2] = channel;
    params[3] = upperbound;
    params[4] = lowerbound;
    params[5] = offset;

    gap_vendor_cmd_req(MP_CMD_HCI_OPCODE, sizeof(params), params);
}

static void app_cmd_get_xtak_k_result(void)
{
    uint8_t params[4];

    params[0] = 0x06; //Module ID, MODULE_XTAL = 0x06
    params[1] = 0x01; //Subcmd, XTAL_VALUE = 0x01

    /* cmd params */
    params[2] = 0x0;  //Type, VALUE_GET = 0x0
    params[3] = 0x0;  //Value, this param would be ignored when type is get.

    gap_vendor_cmd_req(MP_CMD_HCI_OPCODE, sizeof(params), params);
}

static void app_cmd_set_xtak_k_result(uint8_t xtal_val)
{
    cfg_update_xtal(xtal_val);
}

void app_cmd_handle_mp_cmd_hci_evt(void *p_gap_vnd_cmd_cb_data)
{
    T_GAP_CB_DATA cb_data;
    uint16_t cmd;

    memcpy(&cb_data, p_gap_vnd_cmd_cb_data, sizeof(T_GAP_CB_DATA));

    /* Byte 0: Module ID, Byte 1: Subcmd*/
    BE_STREAM_TO_UINT16(cmd, cb_data.p_gap_vnd_cmd_cmpl_evt_rsp->p_param);
    cb_data.p_gap_vnd_cmd_cmpl_evt_rsp->param_len -= 2;

    APP_PRINT_TRACE1("app_cmd_handle_mp_cmd_hci_evt: cmd: 0x%X", cmd);

    switch (cmd)
    {
    case MP_HCI_CMD_AUTO_XTAL_K:
        {
            app_cfg_nv.xtal_k_result = cb_data.p_gap_vnd_cmd_cmpl_evt_rsp->cause;
            app_cfg_store(&app_cfg_nv.offset_xtal_k_result, 4);

            if (app_cfg_nv.xtal_k_result == 0x00) //Success
            {
                app_cfg_nv.xtal_k_times++;
                app_cfg_store(&app_cfg_nv.xtal_k_times, 4);
                app_cmd_get_xtak_k_result();
            }

            APP_PRINT_TRACE2("CMD_RF_XTAK_K: result %d times %d", app_cfg_nv.xtal_k_result,
                             app_cfg_nv.xtal_k_times);

            app_mmi_handle_action(MMI_DEV_FORCE_ENTER_PAIRING_MODE);

            app_dlps_enable(APP_DLPS_ENTER_CHECK_RF_XTAL);
        }
        break;

    case MP_HCI_CMD_XTAL_VALUE:
        {
            uint8_t xtal_k_result[3] = {0};
            uint8_t xtal_val;

            BE_STREAM_TO_UINT8(xtal_val, cb_data.p_gap_vnd_cmd_cmpl_evt_rsp->p_param);
            cb_data.p_gap_vnd_cmd_cmpl_evt_rsp->param_len -= 1;

            if (xtal_val == 0xff || app_cfg_nv.xtal_k_times == 0)
            {
                app_cfg_nv.xtal_k_result = 0x07; /* XTAL_K_NOT_DO_YET */
            }

            xtal_k_result[0] = app_cfg_nv.xtal_k_result; /* 0: XTAL_K_SUCCESS */
            xtal_k_result[1] = xtal_val;
            xtal_k_result[2] = app_cfg_nv.xtal_k_times;

            APP_PRINT_TRACE3("CMD_RF_XTAL_K_GET_RESULT: %02x %02x %02x", xtal_k_result[0], xtal_k_result[1],
                             xtal_k_result[2]);

            if (xtal_k_result[0] == 0x00) //Success
            {
                app_cmd_set_xtak_k_result(xtal_val);
            }

            app_report_event_broadcast(EVENT_RF_XTAL_K_GET_RESULT, xtal_k_result, sizeof(xtal_k_result));

            app_dlps_enable(APP_DLPS_ENTER_CHECK_RF_XTAL);
        }
        break;

    default:
        break;
    }

    return;
}


bool app_cmd_get_tool_connect_status(void)
{
    bool tool_connect_status = false;
    T_APP_BR_LINK *br_link;
    T_APP_LE_LINK *le_link;
    uint8_t        i;

    for (i = 0; i < MAX_BR_LINK_NUM; i ++)
    {
        br_link = &app_db.br_link[i];

        if (br_link->connected_profile & (SPP_PROFILE_MASK | IAP_PROFILE_MASK))
        {
            if (br_link->cmd_set_enable == true)
            {
                tool_connect_status = true;
            }
        }
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        le_link = &app_db.le_link[i];
        if (le_link->state == LE_LINK_STATE_CONNECTED)
        {
            if (le_link->cmd_set_enable == true)
            {
                tool_connect_status = true;
            }
        }
    }

    return tool_connect_status;
}

void app_cmd_update_eq_ctrl(uint8_t value, bool is_need_relay)
{
    if (app_db.eq_ctrl_by_src != value)
    {
        app_db.eq_ctrl_by_src = false;

        if (is_need_relay)
        {
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_CMD,
                                                APP_REMOTE_MSG_SYNC_EQ_CTRL_BY_SRC,
                                                &value, sizeof(uint8_t));
        }
    }

    APP_PRINT_TRACE1("app_cmd_update_eq_ctrl: connect_status %d", value);
}

T_SRC_SUPPORT_VER_FORMAT *app_cmd_get_src_version(uint8_t cmd_path, uint8_t app_idx)
{
    T_SRC_SUPPORT_VER_FORMAT *version = NULL;

    if (cmd_path == CMD_PATH_UART)
    {
        version = &src_support_version_uart;
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        version = &src_support_version_le_link[app_idx];
    }
    else if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
    {
        version = &src_support_version_br_link[app_idx];
    }

    return version;
}

bool app_cmd_check_specific_version(T_SRC_SUPPORT_VER_FORMAT *version, uint8_t ver_major,
                                    uint8_t ver_minor)
{
    if (version->cmd_set_ver_major > ver_major ||
        (version->cmd_set_ver_major == ver_minor && version->cmd_set_ver_minor >= ver_minor))
    {
        // SRC support version is compatible.
        return true;
    }

    return false;
}

bool app_cmd_check_src_cmd_version(uint8_t cmd_path, uint8_t app_idx)
{
    T_SRC_SUPPORT_VER_FORMAT *version = app_cmd_get_src_version(cmd_path, app_idx);

    if (version)
    {
        if (app_cmd_check_specific_version(version, CMD_SET_VER_MAJOR, CMD_SET_VER_MINOR))
        {
            // SRC support version is new, which is valid.
            return true;
        }
        else if (version->cmd_set_ver_major == 0 && version->cmd_set_ver_minor == 0)
        {
            // SRC never update support version
            return true;
        }
    }

    return false;
}

bool app_cmd_check_src_eq_spec_version(uint8_t cmd_path, uint8_t app_idx)
{
    T_SRC_SUPPORT_VER_FORMAT *version = app_cmd_get_src_version(cmd_path, app_idx);

    if (version)
    {
        uint8_t dsp_cfg_tool_ver = app_dsp_cfg_get_tool_info_version();
        uint8_t eq_spec_minor = (dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_2) ? EQ_SPEC_VER_MINOR_2 :
                                EQ_SPEC_VER_MINOR_0;

        if (dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_0)
        {
            return false;
        }

        if (version->eq_spec_ver_major > EQ_SPEC_VER_MAJOR ||
            (version->eq_spec_ver_major == EQ_SPEC_VER_MAJOR && version->eq_spec_ver_minor >= eq_spec_minor))
        {
            // SRC support version is new, which is valid.
            return true;
        }
        else if (version->eq_spec_ver_major == 0 && version->eq_spec_ver_minor == 0)
        {
            // SRC never update support version
            return true;
        }
    }

    return false;
}

void app_cmd_bt_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                           uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_BT_READ_PAIRED_RECORD:
        {
            T_APP_LINK_RECORD *link_record;
            uint8_t bond_num = app_b2s_bond_num_get();
            uint8_t temp_buff[bond_num * 8 + 1];

            memset(temp_buff, 0, sizeof(temp_buff));
            link_record = malloc(sizeof(T_APP_LINK_RECORD) * bond_num);
            if (link_record != NULL)
            {
                bond_num = app_get_b2s_link_record(link_record, bond_num);
                temp_buff[0] = bond_num;

                for (uint8_t i = 0; i < bond_num; i++)
                {
                    temp_buff[i * 8 + 1] = link_record[i].priority;
                    temp_buff[i * 8 + 2] = link_record[i].bond_flag;
                    memcpy(&temp_buff[i * 8 + 3], &(link_record[i].bd_addr), 6);
                }
                free(link_record);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                app_report_event(cmd_path, EVENT_REPLY_PAIRED_RECORD, app_idx, temp_buff, sizeof(temp_buff));
            }
        }
        break;

    case CMD_BT_CREATE_CONNECTION:
        {
            //Stop periodic inquiry when connecting
#if F_APP_DEVICE_CMD_SUPPORT
            gap_stop_timer(&timer_handle_stop_periodic_inquiry);
#endif
            bt_periodic_inquiry_stop();

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_bt_policy_default_connect(&cmd_ptr[3], cmd_ptr[2], false);
        }
        break;

    case CMD_BT_DISCONNECT:
        {
            T_APP_BR_LINK *p_link;
            uint8_t bd_addr[6];

            memcpy(bd_addr, &cmd_ptr[2], 6);
            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_disconnect(p_link->bd_addr, cmd_ptr[8]);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_BT_READ_LINK_INFO:
        {
            uint8_t app_index = cmd_ptr[2];

            if ((app_index >= MAX_BR_LINK_NUM) || !app_check_b2s_link_by_id(app_index))
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                uint8_t event_buff[9];

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                event_buff[0] = app_index;
                event_buff[1] = app_db.br_link[app_index].connected_profile;
                event_buff[2] = 0;
                memcpy(&event_buff[3], app_db.br_link[app_index].bd_addr, 6);
                app_report_event(CMD_PATH_UART, EVENT_REPLY_LINK_INFO, 0, &event_buff[0], sizeof(event_buff));
            }
        }
        break;

    case CMD_BT_GET_REMOTE_NAME:
        {
            bt_remote_name_req(&cmd_ptr[2]);
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_BT_IAP_LAUNCH_APP:
        {
            uint8_t app_index = cmd_ptr[2];

            if (app_index < MAX_BR_LINK_NUM)
            {
                if ((app_db.br_link[app_index].connected_profile & IAP_PROFILE_MASK)
                    && (app_db.br_link[app_index].iap.authen_flag))
                {
                    char boundle_id[] = "com.realtek.EADemo2";
                    T_BT_IAP_APP_LAUNCH_METHOD method = BT_IAP_APP_LAUNCH_WITH_USER_ALERT;
                    bt_iap_app_launch(app_db.br_link[app_index].bd_addr, boundle_id, sizeof(boundle_id), method);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_BT_SEND_AT_CMD:
        {
            uint8_t app_index = cmd_ptr[2];

            if (bt_hfp_send_vnd_at_cmd_req(app_db.br_link[app_index].bd_addr, (char *)&cmd_ptr[3]) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_BT_HFP_DIAL_WITH_NUMBER:
        {
            uint8_t app_index = app_hfp_get_active_hf_index();
            char *number = (char *)&cmd_ptr[2];

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }
            else
            {
                if ((app_db.br_link[app_index].app_hf_state == APP_HF_STATE_CONNECTED) &&
                    (app_hfp_get_call_status() == BT_HFP_CALL_IDLE))
                {
                    if (bt_hfp_dial_with_number_req(app_db.br_link[app_index].bd_addr, (const char *)number) == false)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_BD_ADDR:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_GET_BD_ADDR, app_idx, app_db.factory_addr,
                             sizeof(app_db.factory_addr));
        }
        break;

    case CMD_LE_START_ADVERTISING:
        {
            if (cmd_ptr[1] <= 31)
            {
                //app_ble_gap_start_advertising(APP_ADV_PURPOSE_VENDOR, cmd_ptr[0], cmd_ptr[1], &cmd_ptr[2]);
                //fixme later
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_LE_START_SCAN:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            le_scan_start();
        }
        break;

    case CMD_LE_STOP_SCAN:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            le_scan_stop();
        }
        break;

    case CMD_LE_GET_ADDR:
        {
            uint8_t rand_addr[6] = {0};
            app_ble_rand_addr_get(rand_addr);
            if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_LE_PUBLIC_ADDR, app_idx, rand_addr, 6);
            }
        }
        break;

    case CMD_BT_GET_LOCAL_ADDR:
        {
            uint8_t temp_buff[6];
            memcpy(&temp_buff[0], app_cfg_nv.bud_local_addr, 6);
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(CMD_PATH_UART, EVENT_LOCAL_ADDR, 0, temp_buff, sizeof(temp_buff));
        }
        break;

#if F_APP_RSSI_INFO_GET_CMD_SUPPORT
    case CMD_GET_LEGACY_RSSI:
        {
            if (cmd_ptr[2] == LEGACY_RSSI)
            {
                uint8_t *bd_addr = (uint8_t *)(&cmd_ptr[3]);

                if (bt_link_rssi_report(bd_addr, true, 200) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
#endif

    case CMD_BT_BOND_INFO_CLEAR:
        {
            T_APP_BR_LINK *p_link = NULL;
            uint8_t *bd_addr = (uint8_t *)(&cmd_ptr[3]);
            uint8_t temp_buff = CLEAR_BOND_INFO_FAIL;

            p_link = app_find_br_link(bd_addr);
            if (p_link == NULL)
            {
                if (cmd_ptr[2] == 0) //clear BR/EDR bond info
                {
                    if (bt_bond_delete(bd_addr) == true)
                    {
                        temp_buff = CLEAR_BOND_INFO_SUCCESS;
                    }
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(CMD_PATH_UART, EVENT_BT_BOND_INFO_CLEAR, 0, &temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_GET_NUM_OF_CONNECTION:
        {
            uint8_t event_data = app_multi_get_acl_connect_num();

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_REPORT_NUM_OF_CONNECTION, app_idx, &event_data,
                             sizeof(event_data));
        }
        break;

    case CMD_SUPPORT_MULTILINK:
        {
#if F_APP_DUAL_AUDIO_EFFECT
            if (app_cfg_const.enable_multi_link == 0)
#endif
            {
#if (F_APP_AVP_INIT_SUPPORT == 1)
                app_avp_cmd_multilink_on();
#endif
                app_cfg_const.enable_multi_link = 1;
                app_cfg_const.max_legacy_multilink_devices = 2;
                app_bt_policy_set_b2s_connected_num_max(app_cfg_const.max_legacy_multilink_devices);
                app_mmi_handle_action(MMI_DEV_ENTER_PAIRING_MODE);
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    default:
        break;
    }
}

#if F_APP_DEVICE_CMD_SUPPORT
static void app_cmd_device_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                      uint8_t app_idx, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_GET_LOCAL_DEV_STATE:
        {
            uint8_t temp_buff[8];

            if (enable_auto_reject_conn_req_flag)
            {
                temp_buff[0] = ENABLE_AUTO_REJECT_ACL_ACF_REQ;
            }
            else if (enable_auto_accept_conn_req_flag)
            {
                temp_buff[0] = ENABLE_AUTO_ACCEPT_ACL_ACF_REQ;
            }
            else
            {
                temp_buff[0] = FORWARD_ACL_ACF_REQ_TO_HOST;
            }

            temp_buff[1] = app_bt_policy_get_radio_mode();
            temp_buff[2] = app_test_get_sco_state();
            temp_buff[3] = app_audio_is_mic_mute();
            temp_buff[4] = audio_volume_out_get(AUDIO_STREAM_TYPE_VOICE);
            temp_buff[5] = audio_volume_out_get(AUDIO_STREAM_TYPE_PLAYBACK);
            temp_buff[6] = app_hfp_get_call_status();
            temp_buff[7] = app_db.avrcp_play_status;

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_GET_LOCAL_DEV_STATE, app_idx, temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_INQUIRY:
        {
            if ((cmd_ptr[2] == START_INQUIRY) && (cmd_ptr[3] <= MAX_INQUIRY_TIME))
            {
                if (cmd_ptr[4] == NORMAL_INQUIRY)
                {
                    if (legacy_start_inquiry(false, cmd_ptr[3]) != GAP_CAUSE_SUCCESS)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    if (bt_periodic_inquiry_start(false, 3, 2, 1))
                    {
                        gap_start_timer(&timer_handle_stop_periodic_inquiry, "stop_periodic_inquiry",
                                        app_cmd_timer_queue_id, APP_TIMER_STOP_PERIODIC_INQUIRY, app_idx, cmd_ptr[3] * 1280);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
            }
            else if (cmd_ptr[2] == STOP_INQUIRY)
            {
                if (cmd_ptr[4] == NORMAL_INQUIRY)
                {
                    if (legacy_stop_inquiry() != GAP_CAUSE_SUCCESS)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    gap_stop_timer(&timer_handle_stop_periodic_inquiry);

                    if (bt_periodic_inquiry_stop() != true)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_SERVICES_SEARCH:
        {
            uint8_t bd_addr[6];

            memcpy(bd_addr, &cmd_ptr[3], 6);

            if (cmd_ptr[2] == START_SERVICES_SEARCH)
            {
                T_LINKBACK_SEARCH_PARAM search_param;
                if (linkback_profile_search_start(bd_addr, cmd_ptr[9], false, &search_param) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else if (cmd_ptr[2] == STOP_SERVICES_SEARCH)
            {
                if (legacy_stop_sdp_discov(bd_addr) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_SET_PAIRING_CONTROL:
        {
            if (cmd_ptr[2] == FORWARD_ACL_ACF_REQ_TO_HOST)
            {
                enable_auto_accept_conn_req_flag = false;
                enable_auto_reject_conn_req_flag = false;
            }
            else if (cmd_ptr[2] == ENABLE_AUTO_ACCEPT_ACL_ACF_REQ)
            {
                enable_auto_accept_conn_req_flag = true;
                enable_auto_reject_conn_req_flag = false;
            }
            else if (cmd_ptr[2] == ENABLE_AUTO_REJECT_ACL_ACF_REQ)
            {
                enable_auto_accept_conn_req_flag = false;
                enable_auto_reject_conn_req_flag = true;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                if (legacy_cfg_auto_accept_acl(enable_auto_accept_conn_req_flag) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_SET_PIN_CODE:
        {
            if ((cmd_ptr[2] >= 0x01) && (cmd_ptr[2] <= 0x08))
            {
                app_cfg_nv.pin_code_size = cmd_ptr[2];
                memcpy(app_cfg_nv.pin_code, &cmd_ptr[3], cmd_ptr[2]);

                //save to flash after set pin_code
                app_cfg_store(&app_cfg_nv.mic_channel, 12);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_PIN_CODE:
        {
            uint8_t pin_code_size = app_cfg_nv.pin_code_size;
            uint8_t temp_buff[pin_code_size + 1];

            temp_buff[0] = pin_code_size;
            memcpy(&temp_buff[1], app_cfg_nv.pin_code, pin_code_size);

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_REPORT_PIN_CODE, app_idx, temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_PAIR_REPLY:
        {
            uint8_t *bd_addr = app_test_get_acl_conn_ind_bd_addr();

            if (cmd_ptr[2] == ACCEPT_PAIRING_REQ)
            {
                if (legacy_accept_acl_conn_req(bd_addr, GAP_BR_LINK_ROLE_SLAVE) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else if (cmd_ptr[2] == REJECT_PAIRING_REQ)
            {
                if (legacy_reject_acl_conn_req(bd_addr, GAP_ACL_REJECT_LIMITED_RESOURCE) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_SSP_CONFIRMATION:
        {
            uint8_t *bd_addr = app_test_get_user_confirmation_bd_addr();

            if (cmd_ptr[2] == ACCEPT_PAIRING_REQ)
            {
                if (legacy_bond_user_cfm(bd_addr, GAP_CFM_CAUSE_ACCEPT) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else if (cmd_ptr[2] == REJECT_PAIRING_REQ)
            {
                if (legacy_bond_user_cfm(bd_addr, GAP_CFM_CAUSE_REJECT) != GAP_CAUSE_SUCCESS)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_CONNECTED_DEV_ID:
        {
            uint8_t b2s_connected_num = 0;
            uint8_t b2s_connected_id[MAX_BR_LINK_NUM] = {0};

            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_check_b2s_link_by_id(i))
                {
                    b2s_connected_id[b2s_connected_num] = i;
                    b2s_connected_num = b2s_connected_num + 1;
                }
            }

            uint8_t temp_buff[b2s_connected_num + 1];

            temp_buff[0] = b2s_connected_num;
            memcpy(&temp_buff[1], &b2s_connected_id[0], b2s_connected_num);

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_REPORT_CONNECTED_DEV_ID, app_idx, temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_GET_REMOTE_DEV_ATTR_INFO:
        {
            uint8_t app_index = cmd_ptr[2];
            T_LINKBACK_SEARCH_PARAM search_param;
            uint8_t bd_addr[6];
            uint8_t prof = 0;

            memcpy(&bd_addr[0], app_db.br_link[app_index].bd_addr, 6);
            if (cmd_ptr[3] == GET_AVRCP_ATTR_INFO)
            {
                prof = AVRCP_PROFILE_MASK;
            }
            else if (cmd_ptr[3] == GET_PBAP_ATTR_INFO)
            {
                prof = PBAP_PROFILE_MASK;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
            {
                if (linkback_profile_search_start(bd_addr, prof, false, &search_param) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
                else
                {
                    app_cmd_set_report_attr_info_flag(true);
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    default:
        break;
    }
}
#endif

static void app_cmd_general_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                       uint8_t app_idx, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_MMI:
        {
#if F_APP_APT_SUPPORT
            if ((cmd_ptr[3] == MMI_AUDIO_APT) &&
                (app_apt_is_apt_on_state((T_ANC_APT_STATE)(app_db.current_listening_state)) == false) &&
                (app_apt_open_condition_check() == false))
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }
#endif
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (cmd_ptr[3] == MMI_ENTER_DUT_FROM_SPP)
            {
                gap_start_timer(&timer_handle_enter_dut_from_spp_wait_ack, "enter_dut_from_spp_wait_ack",
                                app_cmd_timer_queue_id,
                                APP_TIMER_ENTER_DUT_FROM_SPP_WAIT_ACK, app_idx, 100);
                break;
            }

            if (cmd_ptr[3] == MMI_DEV_POWER_OFF)
            {
                app_db.power_off_cause = POWER_OFF_CAUSE_CMD_SET;
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                //single mode
                app_mmi_handle_action(cmd_ptr[3]);
            }
            else
            {
                if (cmd_ptr[3] == MMI_DEV_FACTORY_RESET)
                {
                    app_mmi_handle_action(cmd_ptr[3]);
                }
                else if ((cmd_ptr[3] == MMI_DEV_SPK_MUTE) || (cmd_ptr[3] == MMI_DEV_SPK_UNMUTE))
                {
                    bool ret = (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ? false : true;

                    app_relay_sync_single(APP_MODULE_TYPE_MMI, cmd_ptr[3], REMOTE_TIMER_HIGH_PRECISION, 0, ret);
                }
                else if (cmd_ptr[3] == MMI_DEV_POWER_OFF)
                {
#if F_APP_ERWS_SUPPORT
                    app_roleswap_poweroff_handle(false);
#else
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
#endif
                }
                else
                {
                    app_relay_async_single(APP_MODULE_TYPE_MMI, cmd_ptr[3]);
                    app_mmi_handle_action(cmd_ptr[3]);
                }
            }
        }
        break;

    case CMD_INFO_REQ:
        {
            uint8_t info_type = cmd_ptr[2];
            uint8_t report_to_phone_len = 6;
            uint8_t buf[report_to_phone_len];

            if (info_type == CMD_SET_INFO_TYPE_VERSION)
            {
                uint8_t dsp_cfg_tool_ver = app_dsp_cfg_get_tool_info_version();

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

                if (cmd_len > CMD_SUPPORT_VER_CHECK_LEN) // update SRC support version
                {
                    T_SRC_SUPPORT_VER_FORMAT *version = app_cmd_get_src_version(cmd_path, app_idx);

                    memcpy(version->version, &cmd_ptr[3], 4);

                    if (!app_cmd_check_src_eq_spec_version(cmd_path, app_idx))
                    {
                        app_cmd_update_eq_ctrl(false, true);
                    }

                    if (!app_cmd_check_src_cmd_version(cmd_path, app_idx))
                    {
                        // version not support
                    }
                }

                buf[0] = info_type;
                buf[1] = CMD_INFO_STATUS_VALID;

                if ((dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_1) ||
                    (dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_2))
                {
                    buf[2] = CMD_SET_VER_MAJOR;
                    buf[3] = CMD_SET_VER_MINOR;
                    buf[4] = EQ_SPEC_VER_MAJOR;
                    buf[5] = (dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_2) ?
                             EQ_SPEC_VER_MINOR_2 : EQ_SPEC_VER_MINOR_0;
                }
                else if (dsp_cfg_tool_ver == DSP_CONFIG_TOOL_DEF_VER_0)
                {
                    // command set version 1.4
                    buf[2] = 0x01;
                    buf[3] = 0x04;
                    report_to_phone_len = 4;
                }
                else
                {
                    report_to_phone_len = 0;
                }

                if (report_to_phone_len > 0)
                {
                    app_report_event(cmd_path, EVENT_INFO_RSP, app_idx, buf, report_to_phone_len);
                }
            }
            else if (info_type == CMD_INFO_GET_CAPABILITY)
            {
                T_SNK_CAPABILITY current_snk_cap;
                uint8_t evt_param[8];

                evt_param[0] = info_type;
                evt_param[1] = CMD_INFO_STATUS_VALID;
                current_snk_cap = app_cmd_get_system_capability();
                memcpy(&evt_param[2], (uint8_t *)&current_snk_cap, sizeof(T_SNK_CAPABILITY));

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_INFO_RSP, app_idx, evt_param, 8);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_SET_CFG:
        {
            uint8_t *device_name;
            uint8_t relay_idx;

            if ((cmd_ptr[2] == CFG_SET_LE_NAME) || (cmd_ptr[2] == CFG_SET_LEGACY_NAME))
            {
                if (cmd_ptr[2] == CFG_SET_LE_NAME)
                {
                    le_set_gap_param(GAP_PARAM_DEVICE_NAME, cmd_ptr[3], &cmd_ptr[4]);
                    device_name = app_cfg_nv.device_name_le;
                    relay_idx = APP_REMOTE_MSG_LE_NAME_SYNC;
                }
                else// if (cmd_ptr[2] == CFG_SET_LEGACY_NAME)
                {
                    bt_local_name_set(&cmd_ptr[4], cmd_ptr[3]);
                    device_name = app_cfg_nv.device_name_legacy;
                    relay_idx = APP_REMOTE_MSG_DEVICE_NAME_SYNC;
                }

                if (device_name)
                {
                    if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP) ||
                        (cmd_path == CMD_PATH_LE) || (cmd_path == CMD_PATH_UART))
                    {
                        uint8_t name_len;

                        name_len = cmd_ptr[3];

                        if (name_len >= GAP_DEVICE_NAME_LEN)
                        {
                            name_len = GAP_DEVICE_NAME_LEN - 1;
                        }
                        memcpy(device_name, &cmd_ptr[4], name_len);
                        device_name[name_len] = 0;
                        app_cfg_store(device_name, 40);

                        if (cmd_ptr[2] == CFG_SET_LE_NAME)
                        {
                            le_common_adv_update_scan_rsp_data();
                        }

                        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
                        {
                            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, relay_idx, &cmd_ptr[4], name_len);
                        }
                    }
                }

            }
            else if ((cmd_ptr[2] == CFG_SET_AUDIO_LATENCY) || (cmd_ptr[2] == CFG_SET_SUPPORT_CODEC))
            {
            }
#if F_APP_AVP_INIT_SUPPORT
            else if ((cmd_ptr[2] == CFG_SET_DURIAN_ID) || (cmd_ptr[2] == CFG_SET_DURIAN_SINGLE_ID))
            {
                app_avp_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
                break;
            }
#endif
            else if (cmd_ptr[2] == CFG_SET_HFP_REPORT_BATT)
            {
                app_db.hfp_report_batt = cmd_ptr[3];
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_CFG_SETTING:
        {
            uint8_t get_type = cmd_ptr[2];

            if (get_type >= CFG_GET_MAX)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if ((get_type == CFG_GET_LE_NAME) || (get_type == CFG_GET_LEGACY_NAME) ||
                (get_type == CFG_GET_IC_NAME))
            {
                uint8_t p_name[40 + 2];
                uint8_t *p_buf;
                uint8_t name_len;

                if (get_type == CFG_GET_LEGACY_NAME)
                {
                    name_len = strlen((const char *)app_cfg_nv.device_name_legacy);
                    p_buf = app_cfg_nv.device_name_legacy;
                }
                else if (get_type == CFG_GET_LE_NAME)
                {
                    name_len = strlen((const char *)app_cfg_nv.device_name_le);
                    p_buf = app_cfg_nv.device_name_le;
                }
                else
                {
                    name_len = strlen((const char *)IC_NAME);
                    p_buf = IC_NAME;
                }

                p_name[0] = get_type;
                p_name[1] = name_len;
                memcpy(&p_name[2], p_buf, name_len);

                app_report_event(cmd_path, EVENT_REPORT_CFG_TYPE, app_idx, &p_name[0], name_len + 2);
            }
#if (F_APP_AVP_INIT_SUPPORT == 0)
            else if (get_type == CFG_GET_COMPANY_ID_AND_MODEL_ID)
            {
                uint8_t buf[5];

                buf[0] = get_type;

                // use little endian method
                buf[1] = app_cfg_const.company_id[0];
                buf[2] = app_cfg_const.company_id[1];
                buf[3] = app_cfg_const.uuid[0];
                buf[4] = app_cfg_const.uuid[1];

                app_report_event(cmd_path, EVENT_REPORT_CFG_TYPE, app_idx, buf, sizeof(buf));
            }
#endif
        }
        break;

    case CMD_INDICATION:
        {
            if (cmd_ptr[2] == 0)//report MAC address of smart phone
            {
                memcpy(app_db.le_link[app_idx].bd_addr, &cmd_ptr[3], 6);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_LANGUAGE_GET:
        {
            uint8_t buf[2];

            buf[0] = app_cfg_nv.voice_prompt_language;
            buf[1] = voice_prompt_supported_languages_get();

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_LANGUAGE_REPORT, app_idx, buf, 2);
        }
        break;

    case CMD_LANGUAGE_SET:
        {
            if (voice_prompt_supported_languages_get() & BIT(cmd_ptr[2]))
            {
                if (voice_prompt_language_set((T_VOICE_PROMPT_LANGUAGE_ID)cmd_ptr[2]) == true)
                {
                    bool need_to_save_to_flash = false;

                    if (cmd_ptr[2] != app_cfg_nv.voice_prompt_language)
                    {
                        need_to_save_to_flash = true;
                    }

                    app_cfg_nv.voice_prompt_language = cmd_ptr[2] ;
                    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_VP_LANGUAGE);

                    if (need_to_save_to_flash)
                    {
                        app_cfg_store(&app_cfg_nv.voice_prompt_language, 1);
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_GET_STATUS:
        {
            /* This command is used to get specific RWS info.
            * Only one status can be reported at one time.
            * Use CMD_GET_BUD_INFO instead to get complete RWS bud info.
            */
#if F_APP_AVP_INIT_SUPPORT
            if ((cmd_ptr[2] == GET_STATUS_AVP_RWS_VER) || (cmd_ptr[2] == GET_STATUS_AVP_MULTILINK_ON_OFF))
            {
                app_avp_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
                break;
            }
#endif
            uint8_t buf[3];
            uint8_t report_len = 2;

            buf[0] = cmd_ptr[2]; //status_index

            switch (cmd_ptr[2])
            {
            case GET_STATUS_RWS_STATE:
                {
                    buf[1] = app_db.remote_session_state;
                }
                break;

            case GET_STATUS_RWS_CHANNEL:
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        buf[1] = (app_cfg_nv.spk_channel << 4) | app_db.remote_spk_channel;
                    }
                    else
                    {
                        buf[1] = (app_cfg_const.solo_speaker_channel << 4);
                    }
                }
                break;

            case GET_STATUS_BATTERY_STATUS:
                {
                    buf[1] = app_db.local_batt_level;
                    buf[2] = app_db.remote_batt_level;
                    report_len = 3;
                }
                break;

#if F_APP_APT_SUPPORT
            case GET_STATUS_APT_STATUS:
                {
                    buf[0] = (app_cfg_const.llapt_support) ? GET_STATUS_LLAPT_STATUS : GET_STATUS_APT_STATUS;

                    if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                    {
                        buf[1] = 1;
                    }
                    else
                    {
                        buf[1] = 0;
                    }
                }
                break;
#endif
            case GET_STATUS_APP_STATE:
                {
                    buf[1] = app_bt_policy_get_state();
                }
                break;

            case GET_STATUS_BUD_ROLE:
                {
                    buf[1] = app_cfg_const.bud_role;
                }
                break;

            case GET_STATUS_VOLUME:
                {
                    T_AUDIO_STREAM_TYPE volume_type;

                    if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
                    {
                        volume_type = AUDIO_STREAM_TYPE_VOICE;
                    }
                    else
                    {
                        volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
                    }

                    buf[1] = audio_volume_out_get(volume_type);
                    buf[2] = audio_volume_out_max_get(volume_type);
                    report_len = 3;
                }
                break;

            case GET_STATUS_RWS_DEFAULT_CHANNEL:
                {
                    buf[1] = (app_cfg_const.couple_speaker_channel << 4) | app_db.remote_default_channel;
                }
                break;

            case GET_STATUS_RWS_BUD_SIDE:
                {
                    buf[1] = app_cfg_const.bud_side;
                }
                break;

#if F_APP_APT_SUPPORT
            case GET_STATUS_RWS_SYNC_APT_VOL:
                {
                    buf[1] = RWS_SYNC_APT_VOLUME;
                }
                break;
#endif

            case GET_STATUS_FACTORY_RESET_STATUS:
                {
                    buf[1] = app_cfg_nv.factory_reset_done;
                }
                break;

            default:
                break;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_REPORT_STATUS, app_idx, buf, report_len);
        }
        break;

    case CMD_GET_BUD_INFO:
        {
            /* This command is used when snk_support_new_report_bud_status_flow is true.
             * Return complete RWS bud info.
             */
            uint8_t buf[6];

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_get_bud_info(&buf[0]);
            app_report_event(cmd_path, EVENT_REPORT_BUD_INFO, app_idx, buf, sizeof(buf));
        }
        break;

    case CMD_GET_FW_VERSION:
        {
            uint8_t report_data[2];
            report_data[0] = cmd_path;
            report_data[1] = app_idx;

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            switch (cmd_ptr[2])
            {
            case GET_PRIMARY_FW_VERSION:
                {
                    uint8_t data[13] = {0};

                    app_cmd_get_fw_version(&data[0]);
                    app_report_event(report_data[0], EVENT_FW_VERSION, report_data[1], data, sizeof(data));
                }
                break;

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
            case GET_SECONDARY_FW_VERSION:
                {
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_CMD, APP_REMOTE_MSG_CMD_GET_FW_VERSION,
                                                        &report_data[0], 2);
                }
                break;

            case GET_PRIMARY_OTA_FW_VERSION:
                {
                    uint8_t data[IMG_LEN_FOR_DONGLE + 1] = {0};
                    data[0] = GET_PRIMARY_OTA_FW_VERSION;

                    app_ota_get_brief_img_version_for_dongle(&data[1]);
                    app_report_event(report_data[0], EVENT_FW_VERSION, report_data[1], data, sizeof(data));
                }
                break;

            case GET_SECONDARY_OTA_FW_VERSION:
                {
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_CMD, APP_REMOTE_MSG_CMD_GET_OTA_FW_VERSION,
                                                        &report_data[0], 2);
                }
                break;
#endif
            }
        }
        break;

    case CMD_WDG_RESET:
        {
            uint8_t wdg_status = 0x00;

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_WDG_RESET, app_idx, &wdg_status, 1);
#if F_APP_ANC_SUPPORT
            app_anc_ini_wdg_reset(cmd_ptr[2]);
#endif
        }
        break;

    case CMD_GET_FLASH_DATA:
        {
            switch (cmd_ptr[2])
            {
            case START_TRANS:
                {
                    if ((0x01 << cmd_ptr[3]) & ALL_DUMP_IMAGE_MASK)
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_flash_data_set_param(cmd_ptr[3], cmd_path, app_idx);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                break;

            case CONTINUE_TRANS:
                {
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_read_flash(flash_data.flash_data_start_addr_tmp, cmd_path, app_idx);
                }
                break;

            case SUPPORT_IMAGE_TYPE:
                {
                    uint8_t paras[5];

                    paras[0] = SUPPORT_IMAGE_TYPE_INFO;
                    paras[1] = (uint8_t)(ALL_DUMP_IMAGE_MASK);
                    paras[2] = (uint8_t)(ALL_DUMP_IMAGE_MASK >> 8);
                    paras[3] = (uint8_t)(ALL_DUMP_IMAGE_MASK >> 16);
                    paras[4] = (uint8_t)(ALL_DUMP_IMAGE_MASK >> 24);

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_REPORT_FLASH_DATA, app_idx, paras, sizeof(paras));
                }
                break;

            default:
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
                break;
            }
        }
        break;

    case CMD_GET_PACKAGE_ID:
        {
            uint8_t temp_buff[2];

            temp_buff[0] = sys_hall_read_chip_id();
            temp_buff[1] = sys_hall_read_package_id();

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_REPORT_PACKAGE_ID, app_idx, temp_buff, 2);
        }
        break;

    case CMD_GET_EAR_DETECTION_STATUS:
        {
            uint8_t status = 0;

            if (LIGHT_SENSOR_ENABLED)
            {
                status = 1;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_EAR_DETECTION_STATUS, app_idx, &status, sizeof(status));
        }
        break;

    case CMD_REG_ACCESS:
        {
            uint32_t addr;
            uint32_t value;
            uint8_t report[5];
            uint32_t *report_value = (uint32_t *)(report + 1);

            memcpy(&addr, &cmd_ptr[4], sizeof(uint32_t));
            memcpy(&value, &cmd_ptr[8], sizeof(uint32_t));

            report[0] = false;

            if (cmd_ptr[2] == REG_ACCESS_READ)
            {
                switch (cmd_ptr[3])
                {
                case REG_ACCESS_TYPE_AON:
                    {
                        *report_value = btaon_fast_read_safe_8b(addr);
                    }
                    break;

                case REG_ACCESS_TYPE_AON2B:
                    {
                        *report_value = btaon_fast_read_safe(addr);
                    }
                    break;

                case REG_ACCESS_TYPE_DIRECT:
                    {
                        *report_value = HAL_READ32(addr, 0);
                    }
                    break;

                default:
                    break;
                }
            }
            else if (cmd_ptr[2] == REG_ACCESS_WRITE)
            {
                switch (cmd_ptr[3])
                {
                case REG_ACCESS_TYPE_AON:
                    {
                        btaon_fast_write_safe_8b(addr, value);
                    }
                    break;

                case REG_ACCESS_TYPE_AON2B:
                    {
                        btaon_fast_write_safe(addr, value);
                    }
                    break;

                case REG_ACCESS_TYPE_DIRECT:
                    {
                        HAL_WRITE32(addr, 0, value);
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                report[0] = true;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_REG_ACCESS, app_idx, report, sizeof(report));
        }
        break;

    default:
        break;
    }
}

static void app_cmd_other_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                     uint8_t app_idx, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_ASSIGN_BUFFER_SIZE:
        {
            app_db.external_mcu_mtu = (cmd_ptr[4] | (cmd_ptr[5] << 8));
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_TONE_GEN:
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_STRING_MODE:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
#if F_APP_CONSOLE_SUPPORT
            console_set_mode(CONSOLE_MODE_STRING);
#endif
        }
        break;

    case CMD_SET_AND_READ_DLPS:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (cmd_ptr[2] == SET_DLPS_DISABLE)
            {
                dlps_status = 0x00;
                app_dlps_disable(APP_DLPS_ENTER_CHECK_CMD);
            }
            else if (cmd_ptr[2] == SET_DLPS_ENABLE)
            {
                dlps_status = 0x01;
                app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD);
            }

            app_report_event(cmd_path, EVENT_REPORT_DLPS_STATUS, app_idx, &dlps_status, 1);
        }
        break;

#if F_APP_BLE_ANCS_CLIENT_SUPPORT
    case CMD_ANCS_REGISTER:
        {
            uint8_t le_index;

            le_index = cmd_ptr[2];

            if (app_db.le_link[le_index].state == LE_LINK_STATE_CONNECTED)
            {
                if (ancs_start_discovery(app_db.le_link[le_index].conn_id) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_ANCS_GET_NOTIFICATION_ATTR:
        {
            uint8_t le_index;
            uint32_t  notification_uid;

            le_index = cmd_ptr[2];
            notification_uid = *((uint32_t *)&cmd_ptr[3]);

            if (app_db.le_link[le_index].state == LE_LINK_STATE_CONNECTED)
            {
                if (ancs_get_notification_attr(app_db.le_link[le_index].conn_id, notification_uid,
                                               &cmd_ptr[8],
                                               cmd_ptr[7]) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
#endif

    case CMD_LED_TEST:
        {
            uint8_t led_index = cmd_ptr[2];
            uint8_t on_off_flag = cmd_ptr[3];
            uint8_t event_buff;

            if (led_index >= LED_NUM)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                break;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (on_off_flag == 0)  /*turn off*/
            {
                led_set_designate_led_on(led_index);
            }
            else
            {
                led_set_designate_led_off(led_index);
            }
            event_buff = 1;
            app_report_event(cmd_path, EVENT_LED_TEST, app_idx, &event_buff, 1);
        }
        break;

    case CMD_SWITCH_TO_HCI_DOWNLOAD_MODE:
        {
            //if uart tx shares the same pin with 3pin gpio, set uart tx pin when receive cmd
            if (app_cfg_const.enable_4pogo_pin)
            {
                if (app_cfg_const.gpio_box_detect_pinmux == app_cfg_const.data_uart_tx_pinmux)
                {
                    Pinmux_Config(app_cfg_const.data_uart_tx_pinmux, UART0_TX);
                    Pad_Config(app_cfg_const.data_uart_tx_pinmux,
                               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
                }
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            gap_start_timer(&timer_handle_switch_to_hci_mode, "switch_to_hci_mode",
                            app_cmd_timer_queue_id,
                            APP_TIMER_SWITCH_TO_HCI_DOWNLOAD_MODE, app_idx, 100);
        }
        break;

#if F_APP_ADC_SUPPORT
    case CMD_GET_PAD_VOLTAGE:
        {
            uint8_t adc_pin = cmd_ptr[2];

            app_adc_set_cmd_info(cmd_path, app_idx);
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            if (!app_adc_enable_read_adc_io(adc_pin))
            {
                uint8_t evt_param[2] = {0xFF, 0xFF};

                app_report_event(cmd_path, EVENT_REPORT_PAD_VOLTAGE, app_idx, evt_param, sizeof(evt_param));
                APP_PRINT_TRACE0("CMD_GET_PAD_VOLTAGE register ADC mgr fail");
            }
        }
        break;
#endif

    case CMD_RF_XTAK_K:
        {
            uint8_t report_status = 0;
            uint8_t rf_channel = cmd_ptr[2];
            uint8_t freq_upperbound = cmd_ptr[3];
            uint8_t freq_lowerbound = cmd_ptr[4];
            uint8_t measure_offset = cmd_ptr[5];

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            report_status = 0;
            app_report_event(cmd_path, EVENT_RF_XTAL_K, app_idx, &report_status, 1);

            app_dlps_disable(APP_DLPS_ENTER_CHECK_RF_XTAL);

            /* start rf xtal auto K flow */
            set_mp_mode_flag(true);

            app_cmd_rf_xtak_k(rf_channel, freq_upperbound, freq_lowerbound, measure_offset);
        }
        break;

    case CMD_RF_XTAL_K_GET_RESULT:
        {
            uint8_t xtal_k_result[3] = {0};

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            app_dlps_disable(APP_DLPS_ENTER_CHECK_RF_XTAL);

            /* start to get xtal cap value */
            set_mp_mode_flag(true);

            app_cmd_get_xtak_k_result();
        }
        break;

#if F_APP_ANC_SUPPORT
    case CMD_HCI:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            uint8_t *anc_tool_cmd_ptr;
            anc_tool_cmd_ptr = malloc(cmd_ptr[4] + 5);
            anc_tool_cmd_ptr[0] = cmd_path;
            anc_tool_cmd_ptr[1] = app_idx;
            memcpy(&anc_tool_cmd_ptr[2], &cmd_ptr[2], cmd_ptr[4] + 3);
            app_anc_handle_anc_tool_cmd(anc_tool_cmd_ptr);
            free(anc_tool_cmd_ptr);
        }
        break;
#endif

#if F_APP_OTA_TOOLING_SUPPORT
    case CMD_OTA_TOOLING_PARKING:
        {
            uint8_t report_status = 0;
            uint8_t dlps_stay_mode = 0;

            app_report_event(CMD_PATH_SPP, EVENT_ACK, app_idx, ack_pkt, 3);
            dlps_stay_mode = cmd_ptr[2];

            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_SET_LPS_SYNC,
                                                &dlps_stay_mode, 1);

            // response to HOST
            app_report_event(cmd_path, EVENT_OTA_TOOLING_PARKING, app_idx, &report_status, 1);

            // delay power off to prevent SPP traffic jam
            gap_start_timer(&timer_handle_ota_parking_power_off, "ota_jig_delay_power_off",
                            app_cmd_timer_queue_id,
                            APP_TIMER_OTA_JIG_DELAY_POWER_OFF, NULL, 2000);
        }
        break;
#endif

    case CMD_MEMORY_DUMP:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_mmi_handle_action(MMI_MEMORY_DUMP);
        }
        break;

    case CMD_MP_TEST:
        {
#if F_APP_CLI_BINARY_MP_SUPPORT
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            mp_test_handle_cmd(app_idx, cmd_path, cmd_ptr[2], cmd_ptr[3], &cmd_ptr[4], cmd_len - 4);
#endif
        }
        break;

    default:
        break;
    }
}

static void app_cmd_customized_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path,
                                          uint8_t app_idx, uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_IO_PIN_PULL_HIGH:
        {
            uint8_t report_status = 0;
            uint8_t pin_num = cmd_ptr[2];

            if (app_bt_policy_get_b2b_connected())
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_SYNC_IO_PIN_PULL_HIGH,
                                                    &pin_num, 1);

                gap_start_timer(&timer_handle_io_pin_pull_high,
                                "io_pin_pull_high", app_cmd_timer_queue_id,
                                APP_TIMER_IO_PIN_PULL_HIGH, pin_num, 500);
            }
            else
            {
                Pad_Config(pin_num, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            }

            app_report_event(cmd_path, EVENT_IO_PIN_PULL_HIGH, app_idx, &report_status, 1);
        }
        break;

    case CMD_ENTER_BAT_OFF_MODE:
        {
            uint8_t report_status = 0;

            led_change_mode(LED_MODE_ENTER_PCBA_SHIPPING_MODE, true, true);

            app_report_event(cmd_path, EVENT_ENTER_BAT_OFF_MODE, app_idx, &report_status, 1);
        }
        break;

#if GFPS_SASS_SUPPORT
#if F_APP_SUPPORT_SASS_SPP_CMD
    case CMD_SASS_FEATURE:
        {
            uint8_t need_ack_flag = true;

            switch (cmd_ptr[2])
            {
            case SASS_PREEMPTIVE_FEATURE_BIT_SET:
                {
                    app_sass_policy_set_switch_preference(cmd_ptr[3]);
                }
                break;
            case SASS_PREEMPTIVE_FEATURE_BIT_GET:
                {
                    uint8_t report_buf[2];
                    report_buf[0] = SASS_PREEMPTIVE_FEATURE_BIT_GET;
                    report_buf[1] = app_sass_policy_get_switch_preference();
                    need_ack_flag = false;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_REPORT_SASS_FEATURE, app_idx, report_buf, sizeof(report_buf));
                }
                break;
            case SASS_LINK_SWITCH:
                {
                    app_sass_policy_switch_active_link(app_db.br_link[app_idx].bd_addr, cmd_ptr[3], cmd_ptr[4]);
                }
                break;

            case SASS_SWITCH_BACK:
                {
                    app_sass_policy_switch_back(cmd_ptr[3]);
                }
                break;
            case SASS_MULTILINK_ON_OFF:
                {
                    app_sass_policy_set_multipoint_state(cmd_ptr[3]);
                }
                break;
            default:
                {
                    ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
                }
                break;
            }

            if (need_ack_flag)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;
#endif
#endif
#if F_APP_DUAL_AUDIO_EFFECT
    case CMD_CUSTOMIZED_SITRON_FEATURE:
        {
            uint8_t need_ack_flag = true;

            switch (cmd_ptr[2])
            {
            case SITRONIX_SECP_INDEX:
                {
                    if (app_db.gaming_mode)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        APP_PRINT_INFO0("app_handle_mmi_message: When headset comes to gaming mode, not support default effect change!");
                        break;
                    }
                    dual_audio_set_report_para(app_idx, cmd_path);
                    if (cmd_ptr[3] <  dual_audio_get_mod_num())
                    {
                        app_cfg_nv.audio_effect_type = cmd_ptr[3] == 0 ?  dual_audio_get_mod_num() - 1 :
                                                       cmd_ptr[3] - 1;
                    }
                    else
                    {
                        app_cfg_nv.audio_effect_type =  dual_audio_get_mod_num() - 1;
                    }

                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_mmi_handle_action(MMI_AUDIO_DUAL_EFFECT_SWITCH);
                    }
                    else
                    {
                        uint8_t action;
                        action = MMI_AUDIO_DUAL_EFFECT_SWITCH;
                        dual_audio_sync_info();
                        app_relay_async_single(APP_MODULE_TYPE_MMI, action);
                        app_mmi_handle_action(action);
                    }
                }
                break;

            case SPP_UPADTE_DUAL_AUDIO_EFFECT:
                {
                    bool stream = false;

                    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                    {
                        if (app_db.br_link[i].streaming_fg)
                        {
                            stream = true;
                        }
                    }
                    if (stream)
                    {
                        uint16_t offset_44K = cmd_ptr[3] | (cmd_ptr[4] << 8);
                        dual_audio_effect_spp_init(offset_44K, app_idx);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    }
                }
                break;

            case SPP_UPDATE_SECP_DATA:
                {
                    need_ack_flag = false;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    dual_audio_spp_update_data(&cmd_ptr[3]);
                }
                break;

            case SITRONIX_SECP_NUM:
                {
                    need_ack_flag = false;
                    dual_audio_set_report_para(app_idx, cmd_path);
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    dual_audio_report_effect();
                }
                break;

            case SITRONIX_SECP_SET_APP_VALUE:
                {
                    uint16_t index;

                    index = cmd_ptr[3] | (cmd_ptr[4] << 8);
                    if (index >= 16)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        break;
                    }
                    need_ack_flag = false;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    dual_audio_set_report_para(app_idx, cmd_path);
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        dual_audio_set_app_key_val((uint8_t) index, (cmd_ptr[5] | (cmd_ptr[6] << 8)));
                        dual_audio_report_app_key(index);
                        dual_audio_app_key_to_dsp();
                    }
                    else
                    {
                        dual_audio_sync_app_key_val(&cmd_ptr[3], true);
                    }
                }
                break;

            case SITRONIX_SECP_GET_APP_VALUE:
                {
                    uint16_t index;

                    index = cmd_ptr[3] | (cmd_ptr[4] << 8);
                    if (index >= 16)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        break;
                    }

                    need_ack_flag = false;
                    dual_audio_set_report_para(app_idx, cmd_path);
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    dual_audio_report_app_key(index);
                }
                break;

            case SITRONIX_SECP_SPP_RESTART_ACK:
                {
                    need_ack_flag = false;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    dual_audio_effect_restart_ack();
                }
                break;

            default:
                {
                    ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
                }
                break;
            }

            if (need_ack_flag)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;
#endif

    case CMD_MIC_MP_VERIFY_BY_HFP:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);

            uint8_t specified_mic;
            uint8_t report_status = 0;

            specified_mic = cmd_ptr[2];

            APP_PRINT_INFO4("CMD_MIC_MP_VERIFY_BY_HFP specified_mic = %x, pri_sel_ori = %x, pri_sel_new = %x, pri_type_new = %x",
                            specified_mic, app_db.mic_mp_verify_pri_sel_ori, cmd_ptr[3], cmd_ptr[4]);

            if (specified_mic)
            {
                if (app_db.mic_mp_verify_pri_sel_ori == APP_MIC_SEL_DISABLE)
                {
                    app_db.mic_mp_verify_pri_sel_ori = audio_probe_get_voice_primary_mic_sel();
                    app_db.mic_mp_verify_pri_type_ori = audio_probe_get_voice_primary_mic_type();
                    app_db.mic_mp_verify_sec_sel_ori = audio_probe_get_voice_secondary_mic_sel();
                    app_db.mic_mp_verify_sec_type_ori = audio_probe_get_voice_secondary_mic_type();
                }

                audio_probe_set_voice_primary_mic(cmd_ptr[3], cmd_ptr[4]);
                auido_probe_set_voice_secondary_mic(APP_MIC_SEL_DISABLE, app_db.mic_mp_verify_sec_type_ori);
            }
            else
            {
                if (app_db.mic_mp_verify_pri_sel_ori != APP_MIC_SEL_DISABLE)
                {
                    audio_probe_set_voice_primary_mic(app_db.mic_mp_verify_pri_sel_ori,
                                                      app_db.mic_mp_verify_pri_type_ori);
                    auido_probe_set_voice_secondary_mic(app_db.mic_mp_verify_sec_sel_ori,
                                                        app_db.mic_mp_verify_sec_type_ori);

                    app_db.mic_mp_verify_pri_sel_ori = APP_MIC_SEL_DISABLE;
                }
            }

            app_report_event(cmd_path, EVENT_MIC_MP_VERIFY_BY_HFP, app_idx, &report_status, 1);
        }
        break;

    default:
        break;
    }
}

void app_handle_cmd_set(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t rx_seqn,
                        uint8_t app_idx)
{
#if (F_APP_ANC_SUPPORT | F_APP_APT_SUPPORT | F_APP_BRIGHTNESS_SUPPORT | (F_APP_LISTENING_MODE_SUPPORT & NEW_FORMAT_LISTENING_CMD_REPORT))
    uint16_t param_len = cmd_len - COMMAND_ID_LENGTH;
#endif
    uint16_t cmd_id;
    uint8_t  ack_pkt[3];

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
    if (app_cfg_const.one_wire_uart_support && one_wire_state == ONE_WIRE_STATE_IN_ONE_WIRE)
    {
        uint8_t  device_id;

        device_id  = cmd_ptr[0];
        cmd_ptr++;

        if (device_id != app_cfg_const.bud_side)
        {
            APP_PRINT_TRACE0("app_handle_cmd_set: cmd not for this bud");
            return;
        }
    }
#endif

    cmd_id     = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    ack_pkt[0] = cmd_ptr[0];
    ack_pkt[1] = cmd_ptr[1];
    ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

    APP_PRINT_TRACE5("app_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u, rx_seqn 0x%02x, data %b",
                     cmd_id, cmd_len, cmd_path, rx_seqn, TRACE_BINARY(cmd_len, cmd_ptr));

    /* check duplicated seq num */
    if ((cmd_id != CMD_ACK) && (rx_seqn != 0))
    {
        uint8_t *check_rx_seqn = NULL;
        uint8_t  report_app_idx = app_idx;

        if (cmd_path == CMD_PATH_UART)
        {
            check_rx_seqn = &uart_rx_seqn;
            report_app_idx = 0;
        }
        else if (cmd_path == CMD_PATH_LE)
        {
            check_rx_seqn = &app_db.le_link[app_idx].rx_cmd_seqn;
        }
        else if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
        {
            check_rx_seqn = &app_db.br_link[app_idx].rx_cmd_seqn;
        }

        if (check_rx_seqn)
        {
            if (*check_rx_seqn == rx_seqn)
            {
                app_report_event(cmd_path, EVENT_ACK, report_app_idx, ack_pkt, 3);
                return;
            }

            *check_rx_seqn = rx_seqn;
        }
    }

    if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
    {
        app_db.br_link[app_idx].cmd_set_enable = true;
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        if (app_db.le_link[app_idx].state == LE_LINK_STATE_CONNECTED)
        {
            app_db.le_link[app_idx].cmd_set_enable = true;
        }
    }

    if (!app_db.eq_ctrl_by_src &&
        app_cmd_check_src_eq_spec_version(cmd_path, app_idx))
    {
        app_cmd_update_eq_ctrl(true, true);
    }

    switch (cmd_id)
    {
    case CMD_ACK:
        {
            if (cmd_path == CMD_PATH_UART)
            {
                app_pop_data_transfer_queue(CMD_PATH_UART, true);
            }
            else if ((cmd_path == CMD_PATH_LE) || (cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
            {
                uint16_t event_id = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));
                uint8_t status = cmd_ptr[4];

                if (!app_cfg_const.enable_dsp_capture_data_by_spp)
                {
                    app_transfer_queue_recv_ack_check(event_id, cmd_path);
                }

                if (event_id == EVENT_AUDIO_EQ_PARAM_REPORT)
                {
                    if (cmd_path == CMD_PATH_LE)
                    {
                        T_APP_LE_LINK *p_link;

                        p_link = &app_db.le_link[app_idx];

                        if (status != CMD_SET_STATUS_COMPLETE)
                        {
                            app_eq_report_terminate_param_report((void *) p_link, cmd_path);
                        }
                        else
                        {
                            app_eq_report_eq_param((void *) p_link, cmd_path);
                        }
                    }
                }
                else if (event_id == EVENT_OTA_ACTIVE_ACK || event_id == EVENT_OTA_ROLESWAP)
                {
                    if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_IAP))
                    {
                        app_ota_cmd_ack_handle(event_id, status);
                    }
                }
#if F_APP_LOCAL_PLAYBACK_SUPPORT
                else if ((event_id == EVENT_PLAYBACK_GET_LIST_DATA) && (status == CMD_SET_STATUS_COMPLETE))
                {
                    if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
                    {
                        app_playback_trans_list_data_handle();
                    }
                }
#endif
            }
        }
        break;

    case CMD_BT_READ_PAIRED_RECORD:
    case CMD_BT_CREATE_CONNECTION:
    case CMD_BT_DISCONNECT:
    case CMD_BT_READ_LINK_INFO:
    case CMD_BT_GET_REMOTE_NAME:
    case CMD_BT_IAP_LAUNCH_APP:
    case CMD_BT_SEND_AT_CMD:
    case CMD_BT_HFP_DIAL_WITH_NUMBER:
    case CMD_GET_BD_ADDR:
    case CMD_LE_START_ADVERTISING:
    case CMD_LE_START_SCAN:
    case CMD_LE_STOP_SCAN:
    case CMD_LE_GET_ADDR:
    case CMD_BT_GET_LOCAL_ADDR:
#if F_APP_RSSI_INFO_GET_CMD_SUPPORT
    case CMD_GET_LEGACY_RSSI:
#endif
    case CMD_BT_BOND_INFO_CLEAR:
    case CMD_GET_NUM_OF_CONNECTION:
    case CMD_SUPPORT_MULTILINK:
        {
            app_cmd_bt_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_LEGACY_DATA_TRANSFER:
    case CMD_LE_DATA_TRANSFER:
        {
            app_transfer_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MMI:
    case CMD_INFO_REQ:
    case CMD_SET_CFG:
    case CMD_GET_CFG_SETTING:
    case CMD_INDICATION:
    case CMD_LANGUAGE_GET:
    case CMD_LANGUAGE_SET:
    case CMD_GET_STATUS:
    case CMD_GET_BUD_INFO:
    case CMD_GET_FW_VERSION:
    case CMD_WDG_RESET:
    case CMD_GET_FLASH_DATA:
    case CMD_GET_PACKAGE_ID:
    case CMD_GET_EAR_DETECTION_STATUS:
    case CMD_REG_ACCESS:
        {
            app_cmd_general_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

#if F_APP_TTS_SUPPORT
    case CMD_TTS:
        {
            app_ble_tts_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    case CMD_SET_VP_VOLUME:
    case CMD_AUDIO_DSP_CTRL_SEND:
    case CMD_AUDIO_CODEC_CTRL_SEND:
    case CMD_SET_VOLUME:
#if F_APP_APT_SUPPORT
    case CMD_SET_APT_VOLUME_OUT_LEVEL:
    case CMD_GET_APT_VOLUME_OUT_LEVEL:
#endif
#if F_APP_ADJUST_TONE_VOLUME_SUPPORT
    case CMD_SET_TONE_VOLUME_LEVEL:
    case CMD_GET_TONE_VOLUME_LEVEL:
#endif
    case CMD_DSP_TOOL_FUNCTION_ADJUSTMENT:
#if F_APP_SIDETONE_SUPPORT
    case CMD_SET_SIDETONE:
#endif
    case CMD_MIC_SWITCH:
    case CMD_DSP_TEST_MODE:
    case CMD_DUAL_MIC_MP_VERIFY:
    case CMD_SOUND_PRESS_CALIBRATION:
    case CMD_GET_LOW_LATENCY_MODE_STATUS:
    case CMD_SET_LOW_LATENCY_LEVEL:
        {
            app_audio_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

#if APP_CAP_TOUCH_SUPPORT
    case CMD_CAP_TOUCH_CTL:
        {
            CTC_PRINT_TRACE1("CMD_CAP_TOUCH_CTL %b", TRACE_BINARY(cmd_len, cmd_ptr));
            app_dlps_disable(APP_DLPS_ENTER_CHECK_CMD);
            //app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_cap_touch_cmd_param_handle(cmd_path, cmd_ptr, app_idx);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD);
        }
        break;
#endif

    case CMD_AUDIO_EQ_QUERY:
    case CMD_AUDIO_EQ_QUERY_PARAM:
    case CMD_AUDIO_EQ_PARAM_SET:
    case CMD_AUDIO_EQ_PARAM_GET:
    case CMD_AUDIO_EQ_INDEX_SET:
    case CMD_AUDIO_EQ_INDEX_GET:
#if F_APP_APT_SUPPORT
    case CMD_APT_EQ_INDEX_SET:
    case CMD_APT_EQ_INDEX_GET:
#endif
        {
            if (!app_cmd_check_src_eq_spec_version(cmd_path, app_idx))
            {
                ack_pkt[2] = CMD_SET_STATUS_VERSION_INCOMPATIBLE;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                return;
            }

            app_eq_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

#if F_APP_DEVICE_CMD_SUPPORT
    case CMD_GET_LOCAL_DEV_STATE:
    case CMD_INQUIRY:
    case CMD_SERVICES_SEARCH:
    case CMD_SET_PAIRING_CONTROL:
    case CMD_SET_PIN_CODE:
    case CMD_GET_PIN_CODE:
    case CMD_PAIR_REPLY:
    case CMD_SSP_CONFIRMATION:
    case CMD_GET_CONNECTED_DEV_ID:
    case CMD_GET_REMOTE_DEV_ATTR_INFO:
        {
            app_cmd_device_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if F_APP_SENSOR_PX318J_SUPPORT
    case CMD_PX318J_CALIBRATION:
        {
            app_sensor_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if (F_APP_SENSOR_JSA1225_SUPPORT || F_APP_SENSOR_JSA1227_SUPPORT)
    case CMD_JSA_CALIBRATION:
        {
            app_sensor_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if F_APP_HFP_CMD_SUPPORT
    case CMD_SEND_DTMF:
    case CMD_GET_SUBSCRIBER_NUM:
    case CMD_GET_OPERATOR:
        {
            app_hfp_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if F_APP_AVRCP_CMD_SUPPORT
    case CMD_AVRCP_LIST_SETTING_ATTR:
    case CMD_AVRCP_LIST_SETTING_VALUE:
    case CMD_AVRCP_GET_CURRENT_VALUE:
    case CMD_AVRCP_SET_VALUE:
    case CMD_AVRCP_ABORT_DATA_TRANSFER:
    case CMD_AVRCP_SET_ABSOLUTE_VOLUME:
    case CMD_AVRCP_GET_PLAY_STATUS:
    case CMD_AVRCP_GET_ELEMENT_ATTR:
        {
            app_avrcp_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    case CMD_OTA_DEV_INFO:
    case CMD_OTA_IMG_VER:
    case CMD_OTA_INACTIVE_BANK_VER:
    case CMD_OTA_START:
    case CMD_OTA_PACKET:
    case CMD_OTA_VALID:
    case CMD_OTA_RESET:
    case CMD_OTA_ACTIVE_RESET:
    case CMD_OTA_BUFFER_CHECK_ENABLE:
    case CMD_OTA_BUFFER_CHECK:
    case CMD_OTA_IMG_INFO:
    case CMD_OTA_SECTION_SIZE:
    case CMD_OTA_DEV_EXTRA_INFO:
    case CMD_OTA_PROTOCOL_TYPE:
    case CMD_OTA_GET_RELEASE_VER:
    case CMD_OTA_COPY_IMG:
    case CMD_OTA_CHECK_SHA256:
    case CMD_OTA_TEST_EN:
    case CMD_OTA_ROLESWAP:
        {
            app_ota_cmd_handle(cmd_path, cmd_len, cmd_ptr, app_idx);
        }
        break;

#if F_APP_KEY_EXTEND_FEATURE
    case CMD_GET_SUPPORTED_MMI_LIST:
    case CMD_GET_SUPPORTED_CLICK_TYPE:
    case CMD_GET_SUPPORTED_CALL_STATUS:
    case CMD_GET_KEY_MMI_MAP:
    case CMD_SET_KEY_MMI_MAP:
    case CMD_RESET_KEY_MMI_MAP:
#if F_APP_RWS_KEY_SUPPORT
    case CMD_GET_RWS_KEY_MMI_MAP:
    case CMD_SET_RWS_KEY_MMI_MAP:
    case CMD_RESET_RWS_KEY_MMI_MAP:
#endif
        {
            ack_pkt[2] = app_key_handle_key_remapping_cmd(cmd_len, cmd_ptr, app_idx, cmd_path);
            if (ack_pkt[2] != CMD_SET_STATUS_COMPLETE)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;
#endif

    case CMD_VENDOR_SEPC:
        {
#if HARMAN_SPP_CMD_SUPPORT
            app_harman_spp_cmd_set_handle(cmd_ptr, cmd_len, cmd_path, app_idx);
#endif
        }
        break;
#if HARMAN_VBAT_ONE_ADC_DETECTION
	case CMD_USER_SINGLE_NTC:
		app_harman_spp_cmd_single_ntc_handle(cmd_ptr, cmd_len, cmd_path, app_idx);
		break;
#endif	

#if F_APP_ANC_SUPPORT
    //for ANC command
    case CMD_ANC_TEST_MODE:
    case CMD_ANC_WRITE_GAIN:
    case CMD_ANC_READ_GAIN:
    case CMD_ANC_BURN_GAIN:
    case CMD_ANC_COMPARE:
    case CMD_ANC_GEN_TONE:
    case CMD_ANC_CONFIG_DATA_LOG:
    case CMD_ANC_READ_DATA_LOG:
    case CMD_ANC_READ_REGISTER:
    case CMD_ANC_WRITE_REGISTER:
    case CMD_ANC_QUERY:
    case CMD_ANC_ENABLE_DISABLE:
    case CMD_ANC_LLAPT_WRITE_GAIN:
    case CMD_ANC_LLAPT_READ_GAIN:
    case CMD_ANC_LLAPT_BURN_GAIN:
    case CMD_ANC_LLAPT_FEATURE_CONTROL:
    case CMD_RAMP_GET_INFO:
    case CMD_RAMP_BURN_PARA_START:
    case CMD_RAMP_BURN_PARA_CONTINUE:
    case CMD_RAMP_BURN_GRP_INFO:
    case CMD_RAMP_MULTI_DEVICE_APPLY:
    case CMD_LISTENING_MODE_CYCLE_SET:
    case CMD_LISTENING_MODE_CYCLE_GET:
    case CMD_ANC_GET_ADSP_INFO:
    case CMD_ANC_SEND_ADSP_PARA_START:
    case CMD_ANC_SEND_ADSP_PARA_CONTINUE:
    case CMD_ANC_ENABLE_DISABLE_ADAPTIVE_ANC:
    case CMD_ANC_SCENARIO_CHOOSE_INFO:
    case CMD_ANC_SCENARIO_CHOOSE_TRY:
    case CMD_ANC_SCENARIO_CHOOSE_RESULT:
        {
            app_anc_cmd_pre_handle(cmd_id, &cmd_ptr[PARAM_START_POINT], param_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if F_APP_APT_SUPPORT
    case CMD_LLAPT_QUERY:
    case CMD_LLAPT_ENABLE_DISABLE:
    case CMD_APT_VOLUME_INFO:
    case CMD_APT_VOLUME_SET:
    case CMD_APT_VOLUME_STATUS:
#if F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
    case CMD_LLAPT_SCENARIO_CHOOSE_INFO:
    case CMD_LLAPT_SCENARIO_CHOOSE_TRY:
    case CMD_LLAPT_SCENARIO_CHOOSE_RESULT:
#endif
    case CMD_APT_GET_POWER_ON_DELAY_TIME:
    case CMD_APT_SET_POWER_ON_DELAY_TIME:
        {
            app_apt_cmd_pre_handle(cmd_id, &cmd_ptr[PARAM_START_POINT], param_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if F_APP_BRIGHTNESS_SUPPORT
    case CMD_LLAPT_BRIGHTNESS_INFO:
    case CMD_LLAPT_BRIGHTNESS_SET:
    case CMD_LLAPT_BRIGHTNESS_STATUS:
        {
            app_apt_brightness_cmd_pre_handle(cmd_id, &cmd_ptr[PARAM_START_POINT], param_len, cmd_path, app_idx,
                                              ack_pkt);
        }
        break;
#endif

#if F_APP_LISTENING_MODE_SUPPORT
#if NEW_FORMAT_LISTENING_CMD_REPORT
    case CMD_LISTENING_STATE_SET:
    case CMD_LISTENING_STATE_STATUS:
        {
            app_listening_cmd_pre_handle(cmd_id, &cmd_ptr[PARAM_START_POINT], param_len, cmd_path, app_idx,
                                         ack_pkt);
        }
        break;
#endif
#endif

#if F_APP_PBAP_CMD_SUPPORT
    case CMD_PBAP_DOWNLOAD:
    case CMD_PBAP_DOWNLOAD_CONTROL:
    case CMD_PBAP_DOWNLOAD_GET_SIZE:
        {
            app_pbap_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

#if F_APP_AVP_INIT_SUPPORT
    case CMD_AVP_LD_EN:
    case CMD_AVP_ANC_SETTINGS:
    case CMD_AVP_ANC_CYCLE_SET:
    case CMD_AVP_CLICK_SET:
    case CMD_AVP_CONTROL_SET:
        {
            app_avp_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    case CMD_ASSIGN_BUFFER_SIZE:
    case CMD_TONE_GEN:
    case CMD_STRING_MODE:
    case CMD_SET_AND_READ_DLPS:
#if F_APP_BLE_ANCS_CLIENT_SUPPORT
    case CMD_ANCS_REGISTER:
    case CMD_ANCS_GET_NOTIFICATION_ATTR:
#endif
    case CMD_LED_TEST:
    case CMD_SWITCH_TO_HCI_DOWNLOAD_MODE:
#if F_APP_ADC_SUPPORT
    case CMD_GET_PAD_VOLTAGE:
#endif
    case CMD_RF_XTAK_K:
    case CMD_RF_XTAL_K_GET_RESULT:
#if F_APP_ANC_SUPPORT
    case CMD_HCI:
#endif
#if F_APP_OTA_TOOLING_SUPPORT
    case CMD_OTA_TOOLING_PARKING:
#endif
    case CMD_MEMORY_DUMP:
    case CMD_MP_TEST:
        {
            app_cmd_other_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

#if (F_APP_ONE_WIRE_UART_SUPPORT == 1)
    case CMD_FORCE_ENGAGE:
    case CMD_AGING_TEST_CONTROL:
        {
            app_one_wire_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    case CMD_IO_PIN_PULL_HIGH:
    case CMD_ENTER_BAT_OFF_MODE:
#if F_APP_SUPPORT_SASS_SPP_CMD
    case CMD_SASS_FEATURE:
#endif
#if F_APP_DUAL_AUDIO_EFFECT
    case CMD_CUSTOMIZED_SITRON_FEATURE:
#endif
    case CMD_MIC_MP_VERIFY_BY_HFP:
        {
            app_cmd_customized_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;

        /* BBPro2 specialized feature */
#if F_APP_LOCAL_PLAYBACK_SUPPORT
    case CMD_PLAYBACK_QUERY_INFO:
    case CMD_PLAYBACK_GET_LIST_DATA:
    case CMD_PLAYBACK_TRANS_START:
    case CMD_PLAYBACK_TRANS_CONTINUE:
    case CMD_PLAYBACK_REPORT_BUFFER_CHECK:
    case CMD_PLAYBACK_VALID_SONG:
    case CMD_PLAYBACK_TRIGGER_ROLE_SWAP:
    case CMD_PLAYBACK_TRANS_CANCEL:
    case CMD_PLAYBACK_PERMANENT_DELETE_SONG:
    case CMD_PLAYBACK_PERMANENT_DELETE_ALL_SONG:
    case CMD_PLAYBACK_PLAYLIST_ADD_SONG:
    case CMD_PLAYBACK_PLAYLIST_DELETE_SONG:
    case CMD_PLAYBACK_EXIT_TRANS:
        {
            app_playback_trans_cmd_handle(cmd_len, cmd_ptr, app_idx);
        }
        break;
#endif

    case CMD_DSP_DEBUG_SIGNAL_IN:
        {
            if ((cmd_len - 2) != DSP_DEBUG_SIGNAL_IN_PAYLOAD_LEN)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                uint8_t buf[4 + DSP_DEBUG_SIGNAL_IN_PAYLOAD_LEN];
                uint16_t payload_len = 4 + DSP_DEBUG_SIGNAL_IN_PAYLOAD_LEN;

                memcpy(buf + 4, &cmd_ptr[2], DSP_DEBUG_SIGNAL_IN_PAYLOAD_LEN);

                buf[0] = CFG_SET_DSP_DEBUG_SIGNAL_IN & 0xFF;
                buf[1] = (CFG_SET_DSP_DEBUG_SIGNAL_IN & 0xFF00) >> 8;
                buf[2] = (payload_len / 4) & 0xFF;
                buf[3] = ((payload_len / 4) & 0xFF00) >> 8;

                audio_probe_dsp_send(buf, payload_len);
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_CMD, APP_REMOTE_MSG_DSP_DEBUG_SIGNAL_IN_SYNC,
                                                    buf, payload_len);
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

#if F_APP_HEARABLE_SUPPORT
    case CMD_HA_SET_PARAM:
    case CMD_HA_VER_REQ:
    case CMD_HA_ACCESS_EFFECT_INDEX:
    case CMD_HA_ACCESS_ON_OFF:
    case CMD_HA_GET_TOOL_EXTEND_REQ:
    case CMD_HA_SET_DSP_PARAM:
    case CMD_HA_ACCESS_PROGRAM_ID:
    case CMD_HA_SET_NR_PARAM:
    case CMD_HA_GET_PROGRAM_NUM:
    case CMD_HA_ACCESS_PROGRAM_NAME:
    case CMD_HA_SET_OVP_PARAM:
        {
            app_ha_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif /*F_APP_HEARABLE_SUPPORT */
        /* end of BBPro2 specialized feature */

#if F_APP_SS_SUPPORT
    case CMD_SS_REQ:
        {
            app_ss_cmd_handle(cmd_ptr, cmd_len, cmd_path, app_idx, ack_pkt);
        }
        break;
#endif

    default:
        {
            ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;
    }

    APP_PRINT_TRACE1("app_handle_cmd_set: ack 0x%02x", ack_pkt[2]);
}
