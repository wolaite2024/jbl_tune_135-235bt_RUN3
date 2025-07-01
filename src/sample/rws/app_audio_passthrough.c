#include <stdlib.h>
#include <string.h>
#include "os_mem.h"
#include "os_timer.h"
#include "os_sync.h"
#include "console.h"
#include "gap_timer.h"
#include "remote.h"
#include "audio_passthrough.h"
#include "audio.h"
#include "bt_hfp.h"
#include "bt_bond.h"
#include "eq_utils.h"
#include "app_hfp.h"
#include "app_cfg.h"
#include "app_audio_passthrough.h"
#include "app_relay.h"
#include "app_bond.h"
#include "app_eq.h"
#include "app_main.h"
#include "anc_tuning.h"
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#include "app_hfp.h"
#include "trace.h"
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#include "app_cmd.h"
#include "app_multilink.h"
#include "app_sniff_mode.h"
#include "app_audio_policy.h"
#include "app_bt_policy_api.h"
#include "audio_type.h"
#include "app_audio_route.h"
#if (F_APP_LINEIN_SUPPORT == 1)
#include "app_line_in.h"
#endif
#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif
#include "eq.h"
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#endif
#include "app_roleswap_control.h"
#if F_APP_AIRPLANE_SUPPORT
#include "app_airplane.h"
#endif
#include "app_sensor.h"

#if F_APP_HARMAN_FEATURE_SUPPORT
#include "app_harman_vendor_cmd.h"
#endif

#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif

#if F_APP_APT_SUPPORT
#define APT_IMAGE_SUB_TYPE_IDLE_COEF        1
#define APT_IMAGE_SUB_TYPE_A2DP_COEF        2
#define APT_IMAGE_SUB_TYPE_SCO_COEF         3

#define APT_CMD_STATUS_SUCCESS               0x00
#define APT_CMD_STATUS_FAILED                0x01
#define APT_CMD_STATUS_SPK2_NOT_EXIST        0x02
#define APT_CMD_STATUS_SPK2_TIMEOUT          0x03
#define APT_CMD_STATUS_UNKNOW_CMD            0x04
#define APT_CMD_STATUS_CHANNEL_NOT_MATCH     0x05
#define APT_CMD_STATUS_WRONG                 0x06
#define APT_CMD_STATUS_CRC_FAIL              0x07
#define APT_CMD_STATUS_LENGTH_FAIL           0x08
#define APT_CMD_STATUS_SEGMENT_FAIL          0x09
#define APT_CMD_STATUS_MEMORY_LACK           0x0A
#define APT_CMD_STATUS_FLASH_BURN_FAILED     0x0B
#define APT_CMD_STATUS_FLASH_COMFIRM_FAILED  0x0C
#define APT_CMD_STATUS_CMD_LENGTH_INVALID    0x0D

#define GAIN_MUTE_LINEAR_VALUE               0xC000    //-128dB
#define GAIN_0DB_LINEAR_VALUE                0x0000

#define LLAPT_MP_GRP_INFO_BITS               12

#define APT_INVALID_DATA                     0XFF

#define LLAPT_GROUP_INACTIVATE               0xFF
#define LLAPT_GROUP_UNSELECT                 0xFE

static uint16_t apt_report_event;
static uint8_t apt_queue_id = 0;
static void *timer_handle_apt_cmd_wait_sec_respond = NULL;
static T_LLAPT_SCENARIO_INFO llapt_scenario_info;
uint8_t llapt_scenario_choose_start;
uint16_t apt_dac_gain_table[16];

typedef enum
{
    APP_IO_TIMER_APT_CMD_WAIT_SEC_RESPOND       = 0x00,
} T_APP_APT_TIMER;

bool app_apt_set_first_llapt_scenario(T_ANC_APT_STATE *state)
{
    bool ret = true;
    uint8_t first_scenario_index;

    for (first_scenario_index = 0; first_scenario_index < LLAPT_MP_GRP_INFO_BITS;
         first_scenario_index++)
    {
        if (app_cfg_nv.llapt_selected_list & BIT(first_scenario_index))
        {
            break;
        }
    }

    if (first_scenario_index + ANC_OFF_LLAPT_ON_SCENARIO_1 > ANC_OFF_LLAPT_ON_SCENARIO_5)
    {
        ret = false;
    }

    if (ret)
    {
        *state = (T_ANC_APT_STATE)(first_scenario_index + ANC_OFF_LLAPT_ON_SCENARIO_1);
    }

    APP_PRINT_TRACE2("app_apt_set_first_llapt_scenario ret = %x, state = 0x%x", ret,
                     first_scenario_index + ANC_OFF_LLAPT_ON_SCENARIO_1);

    return ret;
}

static void app_apt_llapt_activated_scenario_init(void)
{
    T_LLAPT_EXT_DATA llapt_ext_data;

    anc_tool_read_llapt_ext_data(&llapt_ext_data.d32);

    if (llapt_ext_data.data.rsvd2 == 0)  // has been burned group information
    {
        if (llapt_ext_data.data.grp_info &&
            (32 - __clz(llapt_ext_data.d32)) <=
            llapt_scenario_info.total_scenario_num) // group information valid
            /*  ex: llapt_image_group = 5, invalid case
                          [rsvd2]                 | 0 0 0 0 0 0 1 0 0 0 0 0 |
                _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ | _ _ _ _ _ _ _ _ _ _ _ _ | */

        {
            llapt_scenario_info.llapt_activated_list = (uint16_t)llapt_ext_data.data.grp_info;
        }
        else
        {
            if (llapt_scenario_info.total_scenario_num > 0)
            {
                llapt_scenario_info.llapt_activated_list = UINT16_SET_LSB_BITS(
                                                               llapt_scenario_info.total_scenario_num);
            }

        }
    }
    else  //burn group information never
    {
        if (llapt_scenario_info.total_scenario_num > 0)
        {
            llapt_scenario_info.llapt_activated_list = UINT16_SET_LSB_BITS(
                                                           llapt_scenario_info.total_scenario_num);
        }
    }

    if ((app_apt_get_llapt_selected_scenario_cnt() > LLAPT_MP_GRP_INFO_BITS) ||
        //after factory reset, selected cnt will be 16
        app_apt_get_llapt_selected_scenario_cnt() == 0)    // (i) SOC don't accept scenario cnt be 0
        // (ii) OTA from unsupported version to supported version will be 0
    {
        app_cfg_nv.llapt_selected_list = UINT16_SET_LSB_BITS(app_apt_get_llapt_activated_scenario_cnt());
    }

    APP_PRINT_TRACE3("app_apt_llapt_scenario_init flash_value = %x, llapt_activated_list = %x, llapt_selected_list = %x",
                     llapt_ext_data, llapt_scenario_info.llapt_activated_list, app_cfg_nv.llapt_selected_list);
}

static void app_apt_llapt_init(void)
{
    if (app_cfg_const.llapt_support)
    {
        uint8_t tmp_mode[LLAPT_MP_GRP_INFO_BITS] = {0};
        uint8_t tmp_apt_effect[LLAPT_MP_GRP_INFO_BITS] = {0};

        llapt_scenario_info.total_scenario_num = anc_tool_get_llapt_scenario_info(tmp_mode, tmp_apt_effect);

        llapt_scenario_info.mode = (uint8_t *)malloc(LLAPT_MP_GRP_INFO_BITS);
        llapt_scenario_info.apt_effect = (uint8_t *)malloc(LLAPT_MP_GRP_INFO_BITS);

        if (llapt_scenario_info.mode)
        {
            memcpy(llapt_scenario_info.mode, tmp_mode, llapt_scenario_info.total_scenario_num);
        }

        if (llapt_scenario_info.apt_effect)
        {
            memcpy(llapt_scenario_info.apt_effect, tmp_apt_effect, llapt_scenario_info.total_scenario_num);
        }

        app_apt_llapt_activated_scenario_init();

        APP_PRINT_TRACE3("app_apt_llapt_init num = %x, mode_init = %x, effect_init = %x",
                         llapt_scenario_info.total_scenario_num, (bool)llapt_scenario_info.mode,
                         (bool)llapt_scenario_info.apt_effect);
    }
}

bool app_apt_volume_relay(bool first_time_sync, bool primary_report_phone)
{
    if (app_db.remote_session_state != REMOTE_SESSION_STATE_CONNECTED)
    {
        return false;
    }

    T_APT_VOLUME_RELAY_MSG send_msg;

    send_msg.report_phone = primary_report_phone;
    send_msg.first_sync = first_time_sync;
    send_msg.volume_type = app_db.apt_volume_type;
    send_msg.main_volume_level = app_cfg_nv.apt_volume_out_level;
    send_msg.sub_volume_level = app_cfg_nv.apt_sub_volume_out_level;
#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
    send_msg.rws_disallow_sync_apt_volume = app_cfg_nv.rws_disallow_sync_apt_volume;
#else
    send_msg.rws_disallow_sync_apt_volume = 0;
#endif

    return app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_VOLUME_SYNC,
                                               (uint8_t *)&send_msg, sizeof(T_APT_VOLUME_RELAY_MSG));
}

void app_apt_volume_sync_handle(T_APT_VOLUME_RELAY_MSG *rev_msg)
{
    APP_PRINT_TRACE4("app_apt_volume_sync_handle first_sync = %x, remote_main = %x, remote_sub = %x, disallow_sync = %x",
                     rev_msg->first_sync, rev_msg->main_volume_level, rev_msg->sub_volume_level,
                     rev_msg->rws_disallow_sync_apt_volume);

    //first time sync, volume of primary is applied to secondary
    if (rev_msg->first_sync)
    {
#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
        app_cfg_nv.rws_disallow_sync_apt_volume = rev_msg->rws_disallow_sync_apt_volume;

        if (!app_cfg_nv.rws_disallow_sync_apt_volume)
#endif
        {
#if APT_SUB_VOLUME_LEVEL_SUPPORT
            if (rev_msg->volume_type == MAIN_TYPE_LEVEL)
            {
                app_apt_main_volume_set(rev_msg->main_volume_level);
                app_cfg_nv.apt_sub_volume_out_level = rev_msg->sub_volume_level;
            }
            else // SUB_TYPE_LEVEL
            {
                app_apt_sub_volume_set(rev_msg->sub_volume_level);
                app_cfg_nv.apt_volume_out_level = rev_msg->main_volume_level;
            }
#else
            app_cfg_nv.apt_volume_out_level = rev_msg->main_volume_level;
            audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
#endif
        }
    }

    app_db.remote_apt_volume_out_level = rev_msg->main_volume_level;
    app_db.remote_apt_sub_volume_out_level = rev_msg->sub_volume_level;
}

#if APT_SUB_VOLUME_LEVEL_SUPPORT
void app_apt_volume_update_sub_level(void)
{
    uint16_t mapping_sub_level;
    float percentage;

    percentage = (float)app_cfg_nv.apt_volume_out_level / (float)(app_cfg_const.apt_volume_out_max -
                                                                  app_cfg_const.apt_volume_out_min);

    mapping_sub_level = APT_SUB_VOLUME_LEVEL_MIN + (uint16_t)((APT_SUB_VOLUME_LEVEL_MAX -
                                                               APT_SUB_VOLUME_LEVEL_MIN) * percentage);

    app_cfg_nv.apt_sub_volume_out_level = mapping_sub_level;
}

void app_apt_main_volume_set(uint8_t level)
{
    if (app_db.apt_volume_type == SUB_TYPE_LEVEL)
    {
        uint8_t index;

        //restore original APT DAC gain table
        for (index = app_cfg_const.apt_volume_out_min; index <= app_cfg_const.apt_volume_out_max; index++)
        {
            app_audio_route_dac_gain_set(AUDIO_CATEGORY_APT, index, apt_dac_gain_table[index]);
        }
    }

    app_cfg_nv.apt_volume_out_level = level;

    audio_passthrough_volume_out_set(level);

    app_apt_volume_update_sub_level();

    app_db.apt_volume_type = MAIN_TYPE_LEVEL;

    APP_PRINT_TRACE2("app_apt_main_volume_set main_level = 0x%x, sub_level = 0x%x", level,
                     app_cfg_nv.apt_sub_volume_out_level);
}

void app_apt_sub_volume_set(uint16_t level)
{
    uint16_t min_gain;
    uint16_t max_gain;

    uint16_t mapping_gain;
    uint8_t  mapping_main_level;
    float percentage;

    if (level == 0)
    {
        mapping_main_level = app_cfg_const.apt_volume_out_min;
        mapping_gain = apt_dac_gain_table[app_cfg_const.apt_volume_out_min];
    }
    else
    {
        min_gain = apt_dac_gain_table[app_cfg_const.apt_volume_out_min];
        max_gain = apt_dac_gain_table[app_cfg_const.apt_volume_out_max];

        min_gain = (min_gain <= GAIN_MUTE_LINEAR_VALUE) ?
                   apt_dac_gain_table[app_cfg_const.apt_volume_out_min + 1] :
                   min_gain;
        max_gain = (max_gain == GAIN_0DB_LINEAR_VALUE)  ? 0xFFFF : max_gain;

        percentage = (float)level / (float)APT_SUB_VOLUME_LEVEL_MAX;

        mapping_main_level = app_cfg_const.apt_volume_out_min + (uint8_t)((app_cfg_const.apt_volume_out_max
                                                                           - app_cfg_const.apt_volume_out_min) * percentage);

        mapping_gain = min_gain + (uint16_t)((max_gain - min_gain) * percentage);
    }

    app_audio_route_dac_gain_set(AUDIO_CATEGORY_APT, mapping_main_level,
                                 mapping_gain);  //step 1, update table

    audio_passthrough_volume_out_set(mapping_main_level); //step 2, set level again

    app_cfg_nv.apt_sub_volume_out_level = level;

    app_cfg_nv.apt_volume_out_level = mapping_main_level;

    app_db.apt_volume_type = SUB_TYPE_LEVEL;

    APP_PRINT_TRACE2("app_apt_sub_volume_set sub_level = 0x%x, main_level = 0x%x",
                     app_cfg_nv.apt_sub_volume_out_level, app_cfg_nv.apt_volume_out_level);
}
#endif

void app_apt_volume_init(void)
{
    app_db.apt_volume_type = MAIN_TYPE_LEVEL;

#if APT_SUB_VOLUME_LEVEL_SUPPORT
    if (app_cfg_nv.apt_sub_volume_out_level == INVALID_LEVEL_VALUE)
    {
        app_apt_volume_update_sub_level();
    }
#endif

    app_db.remote_apt_volume_out_level = (uint8_t)INVALID_LEVEL_VALUE;
    app_db.remote_apt_sub_volume_out_level = INVALID_LEVEL_VALUE;

    APP_PRINT_TRACE2("app_apt_volume_init main_level = 0x%x, sub_level = 0x%x",
                     app_cfg_nv.apt_volume_out_level, app_cfg_nv.apt_sub_volume_out_level);
}

uint8_t app_apt_llapt_scenario_is_busy(void)
{
    return llapt_scenario_choose_start;
}

void app_apt_enter_llapt_scenario_choose_mode(void)
{
    llapt_scenario_choose_start = true;

    app_listening_state_machine(EVENT_ALL_OFF, false, true);
}

void app_apt_exit_llapt_scenario_choose_mode(void)
{
    app_listening_state_machine(EVENT_ALL_OFF, false, true);

    llapt_scenario_choose_start = false;
}

#if F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
static bool app_apt_check_llapt_choose_data_valid(uint32_t l_data, uint32_t r_data)
{
    uint8_t l_scenario_count;
    uint8_t activated_group_number;

    activated_group_number = app_listening_count_1bits(llapt_scenario_info.llapt_activated_list);

    l_scenario_count = app_listening_count_1bits(l_data);

    //check bitmap count valid
    if ((l_data == r_data) &&
        (l_scenario_count > 0) &&
        (l_scenario_count <= activated_group_number))
    {
        //check bitmap position valid
        if (32 - __clz(l_data) <= activated_group_number)
        {
            return true;
        }
    }

    return false;
}
#endif

uint8_t app_apt_get_llapt_selected_scenario_cnt(void)
{
    return app_listening_count_1bits((uint32_t)app_cfg_nv.llapt_selected_list);
}

uint8_t app_apt_get_llapt_activated_scenario_cnt(void)
{
    return app_listening_count_1bits((uint32_t)llapt_scenario_info.llapt_activated_list);
}

uint8_t app_apt_get_llapt_total_scenario_cnt(void)
{
    return llapt_scenario_info.total_scenario_num;
}

void app_apt_report(uint16_t apt_report_event, uint8_t *event_data, uint16_t event_len)
{
    bool handle = true;

    switch (apt_report_event)
    {
#if (NEW_FORMAT_LISTENING_CMD_REPORT == 0)
    case EVENT_REPORT_STATUS:
        {
            uint8_t report_data[2];

            report_data[0] = (app_cfg_const.llapt_support) ? GET_STATUS_LLAPT_STATUS : GET_STATUS_APT_STATUS;
            report_data[1] = event_data[0];

            app_report_event_broadcast(EVENT_REPORT_STATUS, report_data, sizeof(report_data));
        }
        break;
#endif

    case EVENT_LLAPT_QUERY:
        {
            uint8_t report_data[16];
            uint8_t i;
            uint8_t select_list_index = 0;

            report_data[0] = 0; // query_type
            report_data[1] = app_apt_is_llapt_on_state((T_ANC_APT_STATE)app_db.current_listening_state);
            report_data[2] = (report_data[1]) ? (uint8_t)(app_db.current_listening_state -
                                                          ANC_OFF_LLAPT_ON_SCENARIO_1) : APT_INVALID_DATA;
            report_data[3] = llapt_scenario_info.total_scenario_num;

            for (i = 0; i < llapt_scenario_info.total_scenario_num; i++)
            {
                if (llapt_scenario_info.llapt_activated_list & BIT(i))
                {
                    report_data[4 + i] = (app_cfg_nv.llapt_selected_list & BIT(select_list_index)) ?
                                         llapt_scenario_info.mode[i] : LLAPT_GROUP_UNSELECT;
                    select_list_index++;
                }
                else
                {
                    report_data[4 + i] = LLAPT_GROUP_INACTIVATE;
                }
            }

            app_report_event_broadcast(EVENT_LLAPT_QUERY, report_data, report_data[3] + 4);
        }
        break;

#if (NEW_FORMAT_LISTENING_CMD_REPORT == 0)
    case EVENT_LLAPT_ENABLE_DISABLE:
        {
            uint8_t report_data[2];
            uint8_t cmd_status = event_data[0];

            report_data[0] = app_apt_is_llapt_on_state((T_ANC_APT_STATE)app_db.current_listening_state);
            report_data[1] = cmd_status;

            app_report_event_broadcast(EVENT_LLAPT_ENABLE_DISABLE, &report_data[0], sizeof(report_data));
        }
        break;
#endif

    case EVENT_APT_VOLUME_INFO:
        {
            uint8_t report_data[10];

            LE_UINT8_TO_ARRAY(&report_data[0],   APT_MAIN_VOLUME_LEVEL_MAX);
            LE_UINT16_TO_ARRAY(&report_data[1],  APT_SUB_VOLUME_LEVEL_MAX);

            LE_UINT8_TO_ARRAY(&report_data[3],   L_CH_LLAPT_MAIN_VOLUME);
            LE_UINT16_TO_ARRAY(&report_data[4],  L_CH_LLAPT_SUB_VOLUME);
            LE_UINT8_TO_ARRAY(&report_data[6],   R_CH_LLAPT_MAIN_VOLUME);
            LE_UINT16_TO_ARRAY(&report_data[7],  R_CH_LLAPT_SUB_VOLUME);

            LE_UINT8_TO_ARRAY(&report_data[9],   RWS_SYNC_APT_VOLUME);

            app_report_event_broadcast(EVENT_APT_VOLUME_INFO, &report_data[0], sizeof(report_data));
        }
        break;

    case EVENT_APT_VOLUME_SET:
        {
            uint8_t report_data[7];
            uint8_t llapt_status = event_data[0];

            LE_UINT8_TO_ARRAY(&report_data[0], llapt_status);
            LE_UINT8_TO_ARRAY(&report_data[1], L_CH_LLAPT_MAIN_VOLUME);
            LE_UINT16_TO_ARRAY(&report_data[2], L_CH_LLAPT_SUB_VOLUME);
            LE_UINT8_TO_ARRAY(&report_data[4], R_CH_LLAPT_MAIN_VOLUME);
            LE_UINT16_TO_ARRAY(&report_data[5], R_CH_LLAPT_SUB_VOLUME);

            app_report_event_broadcast(EVENT_APT_VOLUME_SET, &report_data[0], sizeof(report_data));
        }
        break;

    case EVENT_APT_VOLUME_STATUS:
        {
            uint8_t report_data[6];

            LE_UINT8_TO_ARRAY(&report_data[0], L_CH_LLAPT_MAIN_VOLUME);
            LE_UINT16_TO_ARRAY(&report_data[1], L_CH_LLAPT_SUB_VOLUME);
            LE_UINT8_TO_ARRAY(&report_data[3], R_CH_LLAPT_MAIN_VOLUME);
            LE_UINT16_TO_ARRAY(&report_data[4], R_CH_LLAPT_SUB_VOLUME);

            app_report_event_broadcast(EVENT_APT_VOLUME_STATUS, &report_data[0], sizeof(report_data));
        }
        break;

#if F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
    case EVENT_APT_POWER_ON_DELAY_TIME:
        {
            uint8_t report_data = app_cfg_nv.time_delay_to_open_apt_when_power_on;

            app_report_event_broadcast(apt_report_event, &report_data, sizeof(report_data));
        }
        break;
#endif

    case EVENT_LLAPT_SCENARIO_CHOOSE_INFO:
        {
            uint8_t report_data[LLAPT_MP_GRP_INFO_BITS + 1];
            uint8_t i;
            uint8_t select_list_index = 0;

            report_data[0] = app_apt_get_llapt_activated_scenario_cnt();

            for (i = 0; i < llapt_scenario_info.total_scenario_num; i++)
            {
                if (llapt_scenario_info.llapt_activated_list & BIT(i))
                {
                    report_data[1 + select_list_index] = llapt_scenario_info.mode[i];
                    select_list_index++;
                }
            }

            app_report_event_broadcast(EVENT_LLAPT_SCENARIO_CHOOSE_INFO, report_data, report_data[0] + 1);
        }
        break;

    case EVENT_LLAPT_SCENARIO_CHOOSE_TRY:
        {
            uint8_t llapt_status = event_data[0];

            app_report_event_broadcast(EVENT_LLAPT_SCENARIO_CHOOSE_TRY, &llapt_status, sizeof(llapt_status));
        }
        break;

    case EVENT_LLAPT_SCENARIO_CHOOSE_RESULT:
        {
            uint8_t llapt_status = event_data[0];

            app_report_event_broadcast(EVENT_LLAPT_SCENARIO_CHOOSE_RESULT, &llapt_status,
                                       sizeof(llapt_status));
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_apt_report = %x", apt_report_event);
    }
}

void app_apt_cmd_pre_handle(uint16_t apt_cmd, uint8_t *param_ptr, uint16_t param_len, uint8_t path,
                            uint8_t app_idx, uint8_t *ack_pkt)
{
    bool apt_need_to_report = false;
    uint8_t cmd_status = APT_CMD_STATUS_FAILED;

#if NEW_FORMAT_LISTENING_CMD_REPORT
    if (apt_cmd == CMD_LLAPT_ENABLE_DISABLE)
    {
        // not supported
        ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
        goto SKIP_OPERATE;
    }
#endif

    switch (apt_cmd)
    {
    case CMD_APT_VOLUME_SET:
        {
            if (apt_cmd == CMD_APT_VOLUME_SET)
            {
                uint16_t l_volume;
                uint16_t r_volume;

                LE_ARRAY_TO_UINT16(l_volume, &param_ptr[1]);
                LE_ARRAY_TO_UINT16(r_volume, &param_ptr[3]);

                if ((l_volume != r_volume) && RWS_SYNC_APT_VOLUME)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    goto SKIP_OPERATE;
                }
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                //rws primary
                if (!app_cmd_relay_command_set(apt_cmd, param_ptr, param_len, APP_MODULE_TYPE_APT,
                                               APP_REMOTE_MSG_RELAY_APT_CMD, true))
                {
                    cmd_status = APT_CMD_STATUS_MEMORY_LACK;
                    apt_need_to_report = true;
                }

                goto SKIP_OPERATE;
            }
        }
        break;

    case CMD_APT_SET_POWER_ON_DELAY_TIME:
        {
            uint8_t delay_time = param_ptr[0];

            /*  valid time value: 0~31s */
            if (delay_time > 31)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                goto SKIP_OPERATE;
            }
        }
        break;

    default:
        break;
    }

    app_report_event(path, EVENT_ACK, app_idx, ack_pkt, 3);
    app_apt_cmd_handle(apt_cmd, param_ptr, param_len, path, app_idx);

SKIP_OPERATE:
    app_report_event(path, EVENT_ACK, app_idx, ack_pkt, 3);

    if (apt_need_to_report)
    {
        app_apt_report(apt_cmd, &cmd_status, 1);
    }
}

void app_apt_cmd_handle(uint16_t apt_cmd, uint8_t *param_ptr, uint16_t param_len, uint8_t path,
                        uint8_t app_idx)
{
    bool only_report_status = false;
    uint8_t cmd_status = APT_CMD_STATUS_FAILED;

    apt_report_event = apt_cmd;

    APP_PRINT_TRACE5("app_apt_cmd_handle: apt_cmd 0x%04X, param_ptr 0x%02X 0x%02X 0x%02X 0x%02X",
                     apt_cmd, param_ptr[0], param_ptr[1], param_ptr[2], param_ptr[3]);

    switch (apt_cmd)
    {
    case CMD_LLAPT_QUERY:
        {
            app_apt_report(EVENT_LLAPT_QUERY, NULL, 0);
        }
        break;

#if (NEW_FORMAT_LISTENING_CMD_REPORT == 0)
    case CMD_LLAPT_ENABLE_DISABLE:
        {
            uint8_t enable = param_ptr[0];
            uint8_t cmd_ptr[3] = {0};

            if (enable == ENABLE)
            {
                if (app_apt_open_condition_check() == false)
                {
                    cmd_status = APT_CMD_STATUS_FAILED;
                }
                else if (param_ptr[1] < app_apt_get_llapt_selected_scenario_cnt())
                {
                    // example:
                    // activate count is 5, parameter 0~4 will be valid
                    cmd_ptr[0] = LLAPT_STATE_TO_EVENT(ANC_OFF_LLAPT_ON_SCENARIO_1 + param_ptr[1]);
                    cmd_ptr[1] = true;
                    cmd_ptr[2] = true;

                    cmd_status = APT_CMD_STATUS_SUCCESS;
                }
                else
                {
                    cmd_status = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
            else if (enable == DISABLE)
            {
                if (app_apt_is_llapt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
                {
                    cmd_ptr[0] = EVENT_APT_OFF;
                    cmd_ptr[1] = true;
                    cmd_ptr[2] = true;

                    cmd_status = APT_CMD_STATUS_SUCCESS;
                }
            }

            if (cmd_status == APT_CMD_STATUS_SUCCESS)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_LISTENING_MODE,
                                                       APP_REMOTE_MSG_LISTENING_SOURCE_CONTROL,
                                                       cmd_ptr, 3, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                }
                else
                {
                    app_listening_state_machine((T_ANC_APT_EVENT)cmd_ptr[0], true, true);
                }
            }

            app_apt_report(EVENT_LLAPT_ENABLE_DISABLE, &cmd_status, 1);
        }
        break;
#endif

    case CMD_APT_VOLUME_INFO:
        {
            app_apt_report(EVENT_APT_VOLUME_INFO, NULL, 0);
        }
        break;

    case CMD_APT_VOLUME_SET:
        {
            cmd_status = APT_CMD_STATUS_SUCCESS;

            uint8_t volume_type;
            uint16_t l_volume;
            uint16_t r_volume;
            uint16_t used_volume;

            volume_type = param_ptr[0];
            LE_ARRAY_TO_UINT16(l_volume, &param_ptr[1]);
            LE_ARRAY_TO_UINT16(r_volume, &param_ptr[3]);

            APP_PRINT_TRACE4("app_apt_cmd_handle CMD_APT_VOLUME_SET side = %d, type = %d, l_volume = %d, r_volume = %d",
                             app_cfg_const.bud_side, volume_type, l_volume, r_volume);

            used_volume = (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT) ? l_volume : r_volume;

            if (volume_type == MAIN_TYPE_LEVEL && used_volume <= APT_MAIN_VOLUME_LEVEL_MAX)
            {
                if (app_db.apt_volume_type == SUB_TYPE_LEVEL ||
                    (app_db.apt_volume_type == MAIN_TYPE_LEVEL && used_volume != app_cfg_nv.apt_volume_out_level))
                {
#if APT_SUB_VOLUME_LEVEL_SUPPORT
                    app_apt_main_volume_set(used_volume);
#else
                    app_cfg_nv.apt_volume_out_level = used_volume;
                    audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
#endif

                    app_apt_volume_relay(false, false);
                }
            }
#if APT_SUB_VOLUME_LEVEL_SUPPORT
            else if (volume_type == SUB_TYPE_LEVEL && (used_volume <= APT_SUB_VOLUME_LEVEL_MAX))
            {
                if (app_db.apt_volume_type == MAIN_TYPE_LEVEL ||
                    (app_db.apt_volume_type == SUB_TYPE_LEVEL && used_volume != app_cfg_nv.apt_sub_volume_out_level))
                {
                    app_apt_sub_volume_set(used_volume);
                    app_apt_volume_relay(false, false);
                }
            }
#endif
            if ((path == CMD_PATH_RWS_SYNC) && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
                (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
            {
                //sync execute
                //rws primary, waiting for response of secondary
                gap_start_timer(&timer_handle_apt_cmd_wait_sec_respond, "apt_cmd_wait_sec_respond", apt_queue_id,
                                APP_IO_TIMER_APT_CMD_WAIT_SEC_RESPOND, 0, 3000);
            }
            else if ((path == CMD_PATH_RWS_SYNC) && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) &&
                     (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
            {
                //sync execute
                //rws secondary, response to primary
                app_cmd_relay_event(apt_cmd, &cmd_status, sizeof(cmd_status), APP_MODULE_TYPE_APT,
                                    APP_REMOTE_MSG_RELAY_APT_EVENT);
            }
            else
            {
                //unsync execute
                //single primary
                app_apt_report(EVENT_APT_VOLUME_SET, &cmd_status, 1);
            }
        }
        break;

    case CMD_APT_VOLUME_STATUS:
        {
            app_apt_report(EVENT_APT_VOLUME_STATUS, NULL, 0);

        }
        break;

#if F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT
    case CMD_APT_GET_POWER_ON_DELAY_TIME:
        {
            app_apt_report(EVENT_APT_POWER_ON_DELAY_TIME, NULL, 0);
        }
        break;

    case CMD_APT_SET_POWER_ON_DELAY_TIME:
        {
            uint8_t delay_time = param_ptr[0];

            if (delay_time != app_cfg_nv.time_delay_to_open_apt_when_power_on)
            {
                app_cfg_nv.time_delay_to_open_apt_when_power_on = delay_time;
                app_cfg_store(&app_cfg_nv.offset_xtal_k_result, 1);

                app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY,
                                       APP_REMOTE_MSG_SYNC_APT_POWER_ON_DELAY_APPLY_TIME);
            }

            app_apt_report(EVENT_APT_POWER_ON_DELAY_TIME, NULL, 0);
        }
        break;
#endif

#if F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT
    case CMD_LLAPT_SCENARIO_CHOOSE_INFO:
        {
            app_apt_report(EVENT_LLAPT_SCENARIO_CHOOSE_INFO, NULL, 0);
        }
        break;

    case CMD_LLAPT_SCENARIO_CHOOSE_TRY:
        {
            uint8_t llapt_enable;
            uint8_t llapt_scenario;

            llapt_enable = (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT) ? param_ptr[0] : param_ptr[2];
            llapt_scenario = (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT) ? param_ptr[1] : param_ptr[3];

            if (llapt_scenario_choose_start == false)
            {
                app_apt_enter_llapt_scenario_choose_mode();
            }

            APP_PRINT_TRACE3("app_apt_cmd_handle CMD_LLAPT_SCENARIO_CHOOSE_TRY side = %x, enable = %x, scenario = %x",
                             app_cfg_const.bud_side, llapt_enable, llapt_scenario);

            if (llapt_enable == ENABLE)
            {
                if (llapt_scenario < app_apt_get_llapt_activated_scenario_cnt())
                {
                    app_listening_state_machine(LLAPT_STATE_TO_EVENT(ANC_OFF_LLAPT_ON_SCENARIO_1 + llapt_scenario),
                                                true, false);
                }
            }
            else
            {
                app_listening_state_machine(EVENT_APT_OFF, true, false);
            }

            cmd_status = APT_CMD_STATUS_SUCCESS;

            if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
                (path != CMD_PATH_RWS_ASYNC))
            {
                //rws primary
                if (app_cmd_relay_command_set(apt_cmd, &param_ptr[0], param_len, APP_MODULE_TYPE_APT,
                                              APP_REMOTE_MSG_RELAY_APT_CMD, false))
                {
                    gap_start_timer(&timer_handle_apt_cmd_wait_sec_respond, "apt_cmd_wait_sec_respond", apt_queue_id,
                                    APP_IO_TIMER_APT_CMD_WAIT_SEC_RESPOND, 0, 3000);
                }
                else
                {
                    cmd_status = APT_CMD_STATUS_MEMORY_LACK;
                    only_report_status = true;
                }
            }
            else if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                     (path == CMD_PATH_RWS_ASYNC))
            {
                //rws secondary
                app_cmd_relay_event(apt_cmd, &cmd_status, sizeof(cmd_status), APP_MODULE_TYPE_APT,
                                    APP_REMOTE_MSG_RELAY_APT_EVENT);
            }
            else
            {
                //single primary
                app_apt_report(EVENT_LLAPT_SCENARIO_CHOOSE_TRY, &cmd_status, 1);
            }
        }
        break;

    case CMD_LLAPT_SCENARIO_CHOOSE_RESULT:
        {
            if (param_len != 0)
            {
                uint32_t l_ext_data;
                uint32_t r_ext_data;

                LE_ARRAY_TO_UINT16(l_ext_data, &param_ptr[0]);
                LE_ARRAY_TO_UINT16(r_ext_data, &param_ptr[4]);

                APP_PRINT_TRACE3("app_apt_cmd_handle CMD_LLAPT_SCENARIO_CHOOSE_RESULT side = %x, l_ext_data = %x, r_ext_data = %x",
                                 app_cfg_const.bud_side, l_ext_data, r_ext_data);

                cmd_status = app_apt_check_llapt_choose_data_valid(l_ext_data,
                                                                   r_ext_data) ? APT_CMD_STATUS_SUCCESS : APT_CMD_STATUS_FAILED;

                if (cmd_status == APT_CMD_STATUS_SUCCESS)
                {
                    uint32_t used_ext_data;

                    used_ext_data = (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT) ? l_ext_data : r_ext_data;

                    app_cfg_nv.llapt_selected_list = (uint16_t)used_ext_data;
                    app_cfg_store(&app_cfg_nv.llapt_selected_list, sizeof(uint16_t));

                    app_apt_set_first_llapt_scenario(&app_db.last_llapt_on_state);
                }
            }
            else
            {
                //exit the choose action and do nothing
                cmd_status = APT_CMD_STATUS_SUCCESS;
            }

            app_apt_exit_llapt_scenario_choose_mode();

            if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) &&
                (path != CMD_PATH_RWS_ASYNC))
            {
                //rws primary
                if (app_cmd_relay_command_set(apt_cmd, &param_ptr[0], param_len, APP_MODULE_TYPE_APT,
                                              APP_REMOTE_MSG_RELAY_APT_CMD, false))
                {
                    gap_start_timer(&timer_handle_apt_cmd_wait_sec_respond, "apt_cmd_wait_sec_respond", apt_queue_id,
                                    APP_IO_TIMER_APT_CMD_WAIT_SEC_RESPOND, 0, 3000);
                }
                else
                {
                    cmd_status = APT_CMD_STATUS_MEMORY_LACK;
                    only_report_status = true;
                }
            }
            else if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                     (path == CMD_PATH_RWS_ASYNC))
            {
                //rws secondary
                app_cmd_relay_event(apt_cmd, &cmd_status, sizeof(cmd_status), APP_MODULE_TYPE_APT,
                                    APP_REMOTE_MSG_RELAY_APT_EVENT);
            }
            else
            {
                //single primary
                app_apt_report(EVENT_LLAPT_SCENARIO_CHOOSE_RESULT, &cmd_status, 1);
            }
        }
        break;
#endif

    default:
        {
            only_report_status = true;
            cmd_status = APT_CMD_STATUS_UNKNOW_CMD;
        }
        break;
    }

    if (only_report_status == true)
    {
        app_report_event(path, apt_cmd, app_idx, &cmd_status, 1);
    }
}

/*
* When call is not idle, it is not allowed to open normal apt
*/
bool app_apt_open_condition_check(void)
{
    /* check SCO condition to open APT */
    if (app_cfg_const.normal_apt_support)
    {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
        if (app_teams_multilink_get_voice_status() != BT_HFP_CALL_IDLE)
#else
        if (app_hfp_get_call_status() != BT_HFP_CALL_IDLE)
#endif
        {
            return false;
        }
        else if (LIGHT_SENSOR_ENABLED &&
                 (!app_cfg_const.listening_mode_does_not_depend_on_in_ear_status))
        {
            if (((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
                 ((app_db.remote_loc != BUD_LOC_IN_EAR)
#if F_APP_AIRPLANE_SUPPORT
                  && (!app_airplane_mode_get())
#endif
                 ) &&
                 (app_db.local_loc != BUD_LOC_IN_EAR)) ||
                ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) &&
                 (app_db.local_loc != BUD_LOC_IN_EAR)))
            {
                return false;
            }
        }
#if (F_APP_LINEIN_SUPPORT == 1)
        else if (app_line_in_plug_state_get())
        {
            return false;
        }
#endif
#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
        else if (app_slide_switch_anc_apt_state_get() == APP_SLIDE_SWITCH_ANC_ON)
        {
            return false;
        }
#endif
    }
    else if (app_cfg_const.llapt_support)
    {
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
        if ((app_teams_multilink_get_voice_status() != BT_HFP_CALL_IDLE) &&
            (!app_cfg_const.enable_llapt_when_sco))
#else
        if ((app_hfp_get_call_status() != BT_HFP_CALL_IDLE) && (!app_cfg_const.enable_llapt_when_sco))
#endif
        {
            return false;
        }
        else if (LIGHT_SENSOR_ENABLED &&
                 (!app_cfg_const.listening_mode_does_not_depend_on_in_ear_status))
        {
            if (((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE) &&
                 ((app_db.remote_loc != BUD_LOC_IN_EAR)
#if F_APP_AIRPLANE_SUPPORT
                  && (!app_airplane_mode_get())
#endif
                 ) &&
                 (app_db.local_loc != BUD_LOC_IN_EAR)) ||
                ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE) &&
                 (app_db.local_loc != BUD_LOC_IN_EAR)))
            {
                return false;
            }
        }
#if (F_APP_LINEIN_SUPPORT == 1)
        else if (app_line_in_plug_state_get())
        {
            return false;
        }
#endif
#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
        else if (app_slide_switch_anc_apt_state_get() == APP_SLIDE_SWITCH_ANC_ON)
        {
            return false;
        }
#endif
        else if (app_apt_get_llapt_selected_scenario_cnt() == 0)
        {
            return false;
        }
    }
    else
    {
        //If don't support any kinds of APT, return false
        return false;
    }

    if (app_db.gaming_mode)
    {
        if (app_cfg_const.disallow_anc_apt_when_gaming_mode)
        {
            return false;
        }
    }

    if ((app_cfg_const.apt_auto_on_off_while_music_playing)
        && (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING))
    {
        return false;
    }

    return true;
}

static void app_local_a2dp_volume_set(void)
{
    uint8_t active_idx;
    uint8_t pair_idx_mapping;

    active_idx = app_get_active_a2dp_idx();
    if (app_bond_get_pair_idx_mapping(app_db.br_link[active_idx].bd_addr, &pair_idx_mapping) == false)
    {
        return;
    }
#if HARMAN_OPEN_LR_FEATURE
    app_harman_lr_balance_set(AUDIO_STREAM_TYPE_PLAYBACK,
                              app_cfg_nv.audio_gain_level[pair_idx_mapping], __func__, __LINE__);
#endif
    audio_track_volume_out_set(app_db.br_link[active_idx].a2dp_track_handle,
                               app_cfg_nv.audio_gain_level[pair_idx_mapping]);
}

uint8_t app_apt_llapt_get_coeff_idx(uint8_t scenario_id)
{
    int8_t order = -1;

    for (uint8_t i = 0; i < LLAPT_MP_GRP_INFO_BITS; i++)
    {
        if (llapt_scenario_info.llapt_activated_list & BIT(i))
        {
            order++;
            if (order == scenario_id)
            {
                APP_PRINT_TRACE1("app_apt_llapt_get_coeff_idx, coeff idx:%d", i);
                return i;
            }
        }
    }

    APP_PRINT_ERROR0("app_apt_llapt_get_coeff_idx, scenario id out of range");

    return 0xFF;
}

bool app_apt_related_event(T_ANC_APT_EVENT event)
{
    bool ret = false;

    switch (event)
    {
    case EVENT_NORMAL_APT_ON:
    case EVENT_LLAPT_ON_SCENARIO_1:
    case EVENT_LLAPT_ON_SCENARIO_2:
    case EVENT_LLAPT_ON_SCENARIO_3:
    case EVENT_LLAPT_ON_SCENARIO_4:
    case EVENT_LLAPT_ON_SCENARIO_5:
        {
            ret = true;
        }
        break;

    default:
        break;
    }

    return ret;
}

void app_apt_play_apt_volume_tone(void)
{
    T_APP_AUDIO_TONE_TYPE apt_vol_tone = TONE_TYPE_INVALID;

    if (app_cfg_nv.apt_volume_out_level < 3)
    {
        apt_vol_tone = (T_APP_AUDIO_TONE_TYPE)((TONE_APT_VOL_0) + (app_cfg_nv.apt_volume_out_level));
    }
    else if ((app_cfg_nv.apt_volume_out_level >= 3) && (app_cfg_nv.apt_volume_out_level < 10))
    {
        apt_vol_tone = (T_APP_AUDIO_TONE_TYPE)((TONE_APT_VOL_3) + (app_cfg_nv.apt_volume_out_level - 3));
    }

    if (apt_vol_tone != TONE_TYPE_INVALID)
    {
#if F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT
        if (app_cfg_nv.rws_disallow_sync_apt_volume)
        {
            app_audio_tone_type_play(apt_vol_tone, false, false);
        }
        else
#endif
        {
            app_audio_tone_type_play(apt_vol_tone, false, true);
        }
    }
}



bool app_apt_is_apt_on_state(T_ANC_APT_STATE state)
{
    bool ret = false;

    switch (state)
    {
    case ANC_OFF_NORMAL_APT_ON:
    case ANC_OFF_LLAPT_ON_SCENARIO_1:
    case ANC_OFF_LLAPT_ON_SCENARIO_2:
    case ANC_OFF_LLAPT_ON_SCENARIO_3:
    case ANC_OFF_LLAPT_ON_SCENARIO_4:
    case ANC_OFF_LLAPT_ON_SCENARIO_5:
        {
            ret = true;
        }
        break;

    default:
        break;
    }

    return ret;
}


bool app_apt_is_llapt_on_state(T_ANC_APT_STATE state)
{
    bool ret = false;

    if (app_cfg_const.llapt_support)
    {
        switch (state)
        {
        case ANC_OFF_LLAPT_ON_SCENARIO_1:
        case ANC_OFF_LLAPT_ON_SCENARIO_2:
        case ANC_OFF_LLAPT_ON_SCENARIO_3:
        case ANC_OFF_LLAPT_ON_SCENARIO_4:
        case ANC_OFF_LLAPT_ON_SCENARIO_5:
            {
                ret = true;
            }
            break;

        default:
            break;
        }
    }

    return ret;
}

T_LLAPT_SWITCH_SCENARIO app_llapt_switch_next_scenario(T_ANC_APT_STATE llapt_current_scenario,
                                                       T_ANC_APT_STATE *llapt_next_scenario)
{
    if (!app_apt_is_llapt_on_state(llapt_current_scenario))
    {
        return LLAPT_SWITCH_SCENARIO_NO_DEFINE;
    }

    T_ANC_APT_STATE scenario_index = ++llapt_current_scenario;

    for (; scenario_index <= ANC_OFF_LLAPT_ON_SCENARIO_5; scenario_index++)
    {
        if (app_cfg_nv.llapt_selected_list & BIT(scenario_index - ANC_OFF_LLAPT_ON_SCENARIO_1))
        {
            *llapt_next_scenario = scenario_index;
            return LLAPT_SWITCH_SCENARIO_SUCCESS;
        }
    }

    return LLAPT_SWITCH_SCENARIO_MAX;
}

static void app_apt_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_apt_timer_callback: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_IO_TIMER_APT_CMD_WAIT_SEC_RESPOND:
        {
            if (timer_handle_apt_cmd_wait_sec_respond != NULL)
            {
                gap_stop_timer(&timer_handle_apt_cmd_wait_sec_respond);

                uint8_t report_status;

                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    report_status = APT_CMD_STATUS_SPK2_TIMEOUT;
                }
                else
                {
                    report_status = APT_CMD_STATUS_SPK2_NOT_EXIST;
                }

                app_apt_report(apt_report_event, &report_status, 1);
            }
        }
        break;

    default:
        break;
    }
}

#if (NEW_FORMAT_LISTENING_CMD_REPORT == 0)
static void app_apt_report_status_change(uint8_t status_on)
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (!app_listening_report_secondary_state_condition())
            {
                app_apt_report(EVENT_REPORT_STATUS, &status_on, 1);
            }
        }

        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_SECONDARY_APT_STATUS,
                                                &status_on, 1);
        }
    }
    else
    {
        app_apt_report(EVENT_REPORT_STATUS, &status_on, 1);
    }
}
#endif

static void app_apt_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    bool apt_status_stable = true;

#if F_APP_ANC_SUPPORT
    bool anc_tool = false;

    T_ANC_FEATURE_MAP feature_map;
    feature_map.d32 = anc_tool_get_feature_map();

    if (event_type == AUDIO_EVENT_PASSTHROUGH_ENABLED)
    {
        /* In user mdoe, APP execute app_anc_enter_test_mode(LLAPT), driver reports AUDIO_EVENT_PASSTHROUGH_ENABLED */
        if (feature_map.user_mode == DISABLE)
        {
            handle = false;
            anc_tool = true;
            goto SKIP;
        }
    }

    if (event_type == AUDIO_EVENT_PASSTHROUGH_DISABLED)
    {
        /* In LLAPT test mdoe, APP execute app_anc_exit_test_mode, driver reports AUDIO_EVENT_PASSTHROUGH_DISABLED */
        if (feature_map.user_mode == DISABLE)
        {
            handle = false;
            anc_tool = true;

            goto SKIP;
        }

        if (feature_map.mic_path != ANC_TEST_MODE_EXIT)
        {
            handle = false;
            anc_tool = true;

            feature_map.mic_path = ANC_TEST_MODE_EXIT;
            anc_tool_set_feature_map(feature_map.d32);

            goto SKIP;
        }
    }

#endif

    switch (event_type)
    {
    case AUDIO_EVENT_PASSTHROUGH_ENABLED:
        {
            app_listening_state_set(APT_ANC_STARTED);

#if (NEW_FORMAT_LISTENING_CMD_REPORT == 0)
            app_apt_report_status_change(ENABLE);
#endif

            if (app_cfg_const.llapt_support)
            {
                app_db.last_llapt_on_state = app_db.current_listening_state;
            }

            if (app_listening_get_blocked_state() != ANC_APT_STATE_TOTAL)
            {
                app_listening_state_machine(EVENT_APPLY_BLOCKED_STATE, false, false);
                apt_status_stable = false;
            }
            else if (app_listening_final_state_valid())
            {
                if ((T_ANC_APT_STATE)app_db.current_listening_state != (T_ANC_APT_STATE)
                    (*app_db.final_listening_state))
                {
                    app_listening_state_machine(EVENT_APPLY_FINAL_STATE, false, true);
                    apt_status_stable = false;
                }
            }

            if (apt_status_stable)
            {
#if (NEW_FORMAT_LISTENING_CMD_REPORT)
                app_listening_report_status_change(app_db.current_listening_state);
#endif

                app_eq_change_audio_eq_mode(true);
            }

            if (app_cfg_const.rws_sniff_negotiation)
            {
                app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ANC_APT);
            }

            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED))
            {
                if ((app_cfg_const.normal_apt_support) && (eq_utils_num_get(MIC_SW_EQ, APT_MODE) != 0))
                {
                    if (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE)
                    {
                        app_report_apt_index_info(EQ_INDEX_REPORT_BY_APT_ENABLE);
                    }
                }
            }
        }
        break;

    case AUDIO_EVENT_PASSTHROUGH_DISABLED:
        {
            app_listening_state_set(APT_ANC_STOPPED);

#if (NEW_FORMAT_LISTENING_CMD_REPORT == 0)
            app_apt_report_status_change(DISABLE);
#endif

            //fix sbc/aac volume issue
            if (app_cfg_const.normal_apt_support)
            {
                app_local_a2dp_volume_set();
            }

            if (app_listening_get_temp_state() != (T_ANC_APT_STATE)app_db.current_listening_state)
            {
                app_listening_state_machine(EVENT_APPLY_PENDING_STATE, false, false);
                apt_status_stable = false;
            }
            else if (app_listening_get_blocked_state() != ANC_APT_STATE_TOTAL)
            {
                app_listening_state_machine(EVENT_APPLY_BLOCKED_STATE, false, false);
                apt_status_stable = false;
            }
            else
            {
                if (app_listening_final_state_valid())
                {
                    if ((T_ANC_APT_STATE)app_db.current_listening_state != (T_ANC_APT_STATE)
                        (*app_db.final_listening_state))
                    {
                        app_listening_state_machine(EVENT_APPLY_FINAL_STATE, false, true);
                        apt_status_stable = false;
                    }
                }
            }

            if (apt_status_stable)
            {
#if (NEW_FORMAT_LISTENING_CMD_REPORT)
                app_listening_report_status_change(app_db.current_listening_state);
#endif
            }

            if (app_cfg_const.rws_sniff_negotiation)
            {
                app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_ANC_APT);
            }

#if F_APP_ANC_SUPPORT
            if (app_listening_cmd_postpone_reason_get() == ANC_APT_CMD_POSTPONE_WAIT_USER_MODE_CLOSE)
            {
                /* User mode is at all off state, APP ready to execute ANC tool command */
                app_listening_cmd_postpone_pop();
            }
#endif
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

#if F_APP_ANC_SUPPORT
SKIP:
#endif
    if (handle == true)
    {
        APP_PRINT_TRACE1("app_apt_audio_cback: event 0x%04x", event_type);
    }

#if F_APP_ANC_SUPPORT
    if (anc_tool)
    {
        APP_PRINT_TRACE3("app_apt_audio_cback: ANC_TOOL event 0x%04x, user_mode = %x, mic_path = %x",
                         event_type, feature_map.user_mode, feature_map.mic_path);
    }
#endif
}

uint16_t app_apt_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_APT_EQ_INDEX_SYNC:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&app_cfg_nv.apt_eq_idx;
            skip = false;
        }
        break;

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_APT, payload_len, msg_ptr, skip, total);
}

static void app_apt_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_apt_parse_cback: msg = 0x%x, status = %x", msg_type, status);

    switch (msg_type)
    {
#if F_APP_APT_SUPPORT
    case APP_REMOTE_MSG_APT_VOLUME_OUT_LEVEL:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_cfg_nv.apt_volume_out_level = *((uint8_t *)buf);
                audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
            }
        }
        break;
#endif

    case APP_REMOTE_MSG_APT_EQ_INDEX_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_cfg_nv.apt_eq_idx = *((uint8_t *)buf);
            }
        }
        break;

    case APP_REMOTE_MSG_APT_EQ_DEFAULT_INDEX_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD || status == REMOTE_RELAY_STATUS_SYNC_EXPIRED
                || status == REMOTE_RELAY_STATUS_SYNC_TOUT)
            {
                app_cfg_nv.apt_eq_idx = *((uint8_t *)buf);
                app_eq_index_set(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);
                eq_enable(app_db.apt_eq_instance);
            }
        }
        break;

    case APP_REMOTE_MSG_APT_VOLUME_SYNC:
        {
            T_APT_VOLUME_RELAY_MSG *relay_msg = (T_APT_VOLUME_RELAY_MSG *)buf;

            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_apt_volume_sync_handle(relay_msg);

                if (relay_msg->first_sync)
                {
                    //secondary relay first sync result
                    app_apt_volume_relay(false, true);
                }
            }

            if (status == REMOTE_RELAY_STATUS_ASYNC_SENT_OUT ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (relay_msg->report_phone)
                    {
                        app_apt_report(EVENT_APT_VOLUME_STATUS, NULL, 0);
                    }
                }
            }
        }
        break;

    case APP_REMOTE_MSG_RELAY_APT_CMD:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD ||
                status == REMOTE_RELAY_STATUS_SYNC_TOUT  ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                /* bypass_cmd
                 * byte [0,1]  : cmd_id   *
                 * byte [2,3]  : cmd_len  *
                 * byte [4]    : cmd_path *
                 * byte [5-N]  : cmd      */

                uint16_t cmd_id;
                uint16_t anc_cmd_len;

                cmd_id = (buf[0] | buf[1] << 8);
                anc_cmd_len = (buf[2] | buf[3] << 8);

                app_apt_cmd_handle(cmd_id, &buf[5], anc_cmd_len, buf[4], 0);
            }
        }
        break;

    case APP_REMOTE_MSG_RELAY_APT_EVENT:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                /* report
                 * byte [0,1] : event_id    *
                 * byte [2,3] : report_len  *
                 * byte [4-N] : report      */

                uint16_t event_id;
                uint16_t event_len;

                event_id = (buf[0] | buf[1] << 8);
                event_len = (buf[2] | buf[3] << 8);

                gap_stop_timer(&timer_handle_apt_cmd_wait_sec_respond);

                if (event_id == CMD_APT_VOLUME_SET)
                {
                    app_apt_report(EVENT_APT_VOLUME_SET, &buf[4], event_len);
                }
                else if (event_id == CMD_LLAPT_SCENARIO_CHOOSE_TRY)
                {
                    app_apt_report(EVENT_LLAPT_SCENARIO_CHOOSE_TRY, &buf[4], event_len);
                }
                else if (event_id == CMD_LLAPT_SCENARIO_CHOOSE_RESULT)
                {
                    app_apt_report(EVENT_LLAPT_SCENARIO_CHOOSE_RESULT, &buf[4], event_len);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_EXIT_LLAPT_CHOOSE_MODE:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_apt_llapt_scenario_is_busy())
                {
                    app_apt_exit_llapt_scenario_choose_mode();
                }
            }
        }
        break;

    case APP_REMOTE_MSG_SECONDARY_APT_STATUS:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_listening_report_secondary_state_condition())
                {
                    uint8_t remote_apt_status_on = *(uint8_t *)buf;

                    app_apt_report(EVENT_REPORT_STATUS, &remote_apt_status_on, 1);
                }

                if ((*(uint8_t *)buf) == ENABLE)
                {
                    if ((app_cfg_const.normal_apt_support) && (eq_utils_num_get(MIC_SW_EQ, APT_MODE) != 0))
                    {
                        app_report_apt_index_info(EQ_INDEX_REPORT_BY_RWS_CONNECTED);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_apt_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->remote_conn_cmpl.bd_addr);

            if (p_link != NULL)
            {
                if (app_db.device_state != APP_DEVICE_STATE_OFF)
                {
                    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_apt_volume_relay(true, false);
                    }
                }
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_apt_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_apt_init(void)
{
    app_apt_volume_init();
    app_apt_llapt_init();
    app_apt_set_first_llapt_scenario(&app_db.last_llapt_on_state);
    gap_reg_timer_cb(app_apt_timer_callback, &apt_queue_id);
    audio_mgr_cback_register(app_apt_audio_cback);
    app_relay_cback_register(app_apt_relay_cback, app_apt_parse_cback,
                             APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_TOTAL);
    bt_mgr_cback_register(app_apt_bt_cback);
}

#endif
