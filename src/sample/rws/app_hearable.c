#if (F_APP_HEARABLE_SUPPORT == 1)
#include <stdbool.h>
#include <string.h>
#include "ftl.h"
#include "gap_timer.h"
#include "audio_probe.h"
#include "audio_passthrough.h"
#include "wdrc.h"
#include "nrec.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_cmd.h"
#include "app_sensor.h"
#include "app_hearable.h"

#define HA_WDRC_RECV_BUF_HEADER_LEN         5
#define HA_WDRC_RECV_BUF_IMG_INFO_LEN       3
#define HA_WDRC_RECV_BUF_IMG_LEN            424
#define HA_WDRC_RECV_BUF_MAX_IMG_INFO_LEN   (HA_WDRC_RECV_BUF_IMG_INFO_LEN * 2)
#define HA_WDRC_RECV_BUF_MAX_IMG_LEN        (HA_WDRC_RECV_BUF_IMG_LEN * 2)
#define HA_WDRC_RECV_BUF_TOOL_DATA_LEN      64
#define HA_WDRC_RECV_BUF_LEN                (HA_WDRC_RECV_BUF_HEADER_LEN + HA_WDRC_RECV_BUF_MAX_IMG_INFO_LEN + \
                                             HA_WDRC_RECV_BUF_MAX_IMG_LEN + HA_WDRC_RECV_BUF_TOOL_DATA_LEN + 1)

#define HA_RUNTIME_WDRC_HEADER_LEN          8
#define HA_RUNTIME_WDRC_PAYLOAD_LEN         (HA_RUNTIME_WDRC_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_LEN + \
                                             HA_WDRC_RECV_BUF_TOOL_DATA_LEN)

#define HA_RUNTIME_NR_HEADER_LEN            8
#define HA_RUNTIME_NR_PARAM_LEN             4
#define HA_RUNTIME_NR_PAYLOAD_LEN           (HA_RUNTIME_NR_HEADER_LEN + HA_RUNTIME_NR_PARAM_LEN)

#define HA_RUNTIME_OVP_HEADER_LEN           8
#define HA_RUNTIME_OVP_PARAM_LEN            4
#define HA_RUNTIME_OVP_PAYLOAD_LEN          (HA_RUNTIME_OVP_HEADER_LEN + HA_RUNTIME_OVP_PARAM_LEN)

#define HA_RUNTIME_PROG_NAME_HEADER_LEN     8
#define HA_RUNTIME_PROG_NAME_PARAM_LEN      112
#define HA_RUNTIME_PROG_NAME_PAYLOAD_LEN    (HA_RUNTIME_PROG_NAME_HEADER_LEN + HA_RUNTIME_PROG_NAME_PARAM_LEN)

#define HA_PROG_HEADER_WDRC_NR_SET_LEN      1
#define HA_PROG_HEADER_WDRC_IMG_LEN         2
#define HA_PROG_HEADER_RESERVED_LEN         9
#define HA_PROG_HEADER_WDRC_TYPE_LEN        4
#define HA_PROG_HEADER_LEN                  16
#define HA_PROG_WDRC_LEN                    HA_WDRC_RECV_BUF_IMG_LEN

#define HA_TOOL_DATA_WDRC_LEN               28
#define HA_TOOL_DATA_BEAMFORMING_LEN        4
#define HA_TOOL_DATA_OVP_LEN                4
#define HA_TOOL_DATA_NR_LEN                 4
#define HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN    12
#define HA_TOOL_DATA_TYPE3_PAYLOAD_R_LEN    12
#define HA_TOOL_DATA_LEN                    64

#define HA_OPTION_LEN                       4

#define HA_PROG_NR_SET_OFFSET               (HA_PROG_HEADER_WDRC_NR_SET_LEN + HA_PROG_HEADER_WDRC_IMG_LEN)
#define HA_PROG_BEAMFORMING_OFFSET          (HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN)
#define HA_PROG_OVP_OFFSET                  (HA_PROG_BEAMFORMING_OFFSET + HA_TOOL_DATA_BEAMFORMING_LEN)
#define HA_PROG_NR_OFFSET                   (HA_PROG_OVP_OFFSET + HA_TOOL_DATA_OVP_LEN)
#define HA_PROG_LEN                         (HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_LEN)
#define HA_PROG_NUM                         6

#define APP_IO_TIMER_HA_LISTENING_DELAY     0
#define APP_IO_TIMER_HA_APT_VOL_LEVEL       1

#define HA_LISTENING_DELAY_TIME             2000

//for CMD_HA_SET_DSP_PARAM
#define CFG_SET_DSP_PARAM                   0x70
#define HA_SET_DSP_PARAM_PAYLOAD_LEN        12

typedef enum
{
    HA_FTL_OPTION,
    HA_FTL_PROG,
    HA_FTL_PROG_NAME,
} T_HA_FTL_PARAM_TYPE;

typedef enum
{
    HA_SEGMENT_NOT_FINISH,
    HA_SEGMENT_FINISH,
    HA_SEGMENT_ERROR,
    HA_SEGMENT_PARA_ERROR,
} T_HA_SEGMENT_STATUS;

typedef enum
{
    HA_IMG_APT_WDRC_EQ_L      = 0x00,
    HA_IMG_APT_EXTEND_L,
    HA_IMG_SCO_WDRC_EQ_L,
    HA_IMG_SCO_EXTEND_L,
    HA_IMG_APT_WDRC_EQ_R,
    HA_IMG_APT_EXTEND_R,
    HA_IMG_SCO_WDRC_EQ_R,
    HA_IMG_SCO_EXTEND_R,
    HA_IMG_APT_WDRC_EQ_ALL,
    HA_IMG_APT_EXTEND_ALL,
    HA_IMG_SCO_WDRC_EQ_ALL,
    HA_IMG_SCO_EXTEND_ALL
} T_HA_IMG_TYPE;

typedef enum
{
    APP_REMOTE_MSG_HA_SET_PARAM         = 0x00,
    APP_REMOTE_MSG_HA_SET_EFFECT_INDEX  = 0x01,
    APP_REMOTE_MSG_HA_SET_ON_OFF        = 0x02,
    APP_REMOTE_MSG_HA_SET_DSP_PARAM     = 0x03,
    APP_REMOTE_MSG_HA_SET_PROG_ID       = 0x04,
    APP_REMOTE_MSG_HA_SET_NR_PARAM      = 0x05,
    APP_REMOTE_MSG_HA_SET_PROG_NAME     = 0x06,
    APP_REMOTE_MSG_HA_SET_OVP_PARAM     = 0x07,

    APP_REMOTE_MSG_HA_TOTAL             = 0x08,
} T_HA_REMOTE_MSG;

typedef struct t_ovp_payload
{
    uint8_t en;
    uint8_t level;
    uint8_t rsv;
    uint8_t rsv2;
} T_OVP_PAYLOAD;

static uint32_t ha_option = 0;
static uint8_t ha_prog_id = 0;

static uint8_t accumulate_data[HA_WDRC_RECV_BUF_LEN];
static uint8_t accumulate_data_seq;

static uint8_t runtime_wdrc_payload[HA_RUNTIME_WDRC_PAYLOAD_LEN] = {0};
static uint8_t runtime_nr_payload[HA_RUNTIME_NR_PAYLOAD_LEN] = {0};
static uint8_t runtime_ovp_payload[HA_RUNTIME_OVP_PAYLOAD_LEN] = {0};
static uint8_t runtime_prog_name_payload[HA_RUNTIME_PROG_NAME_PAYLOAD_LEN] = {0};
static uint8_t cur_prog_payload[HA_PROG_LEN] = {0};

static uint8_t ha_timer_queue_id = 0;
static void *ha_timer_handle = NULL;

static uint8_t current_apt_vol_level_cnt = 0;
static uint8_t low_to_high_gain_level[5] = {7, 9, 11, 13, 15};

static bool auto_save = true;

static T_AUDIO_EFFECT_INSTANCE ha_apt_eq;
static T_AUDIO_EFFECT_INSTANCE ha_apt_nr;

static uint16_t app_ha_get_hearable_param_base(T_HA_FTL_PARAM_TYPE type, uint8_t value)
{
    uint16_t base = 0;

    switch (type)
    {
    case HA_FTL_OPTION:
        {
            base = APP_RW_HA_PARAM_ADDR;
        }
        break;

    case HA_FTL_PROG:
        {
            if (value >= HA_PROG_NUM)
            {
                value = 0;
            }

            base = APP_RW_HA_PARAM_ADDR + HA_OPTION_LEN + (HA_PROG_LEN * value);
        }
        break;

    case HA_FTL_PROG_NAME:
        {
            base = APP_RW_HA_PARAM_ADDR + HA_OPTION_LEN + (HA_PROG_LEN * HA_PROG_NUM);
        }
        break;

    default:
        break;
    }

    return base;
}

static void app_ha_set_prog_payload(uint16_t offset, uint8_t *data, uint16_t len)
{
    uint16_t base = 0;

    memcpy(cur_prog_payload + offset, data, len);
    base = app_ha_get_hearable_param_base(HA_FTL_PROG, ha_prog_id);

    if (offset % 4 != 0)
    {
        offset = offset - (offset % 4);
    }

    if (len % 4 != 0)
    {
        len = len + 4 - (len % 4);
    }

    ftl_save_to_storage(data, base + offset, len);

    return;
}

static void app_ha_load_prog_payload()
{
    uint16_t base = 0;

    base = app_ha_get_hearable_param_base(HA_FTL_PROG, ha_prog_id);
    ftl_load_from_storage(cur_prog_payload, base, HA_PROG_LEN);

    return;
}

static void app_ha_apply_ovp_effect(T_OVP_PAYLOAD *p)
{
    if (p->en == 1)
    {
        audio_passthrough_ovp_set(p->level);
        audio_passthrough_ovp_enable(p->level);
    }
    else
    {
        audio_passthrough_ovp_disable();
    }

    return;
}

static void app_ha_apply_cur_prog_effect()
{
    //apply NR while it is set
    if (cur_prog_payload[0] & BIT1)
    {
        if (cur_prog_payload[HA_PROG_NR_OFFSET] & BIT0)
        {
            nrec_mode_set(ha_apt_nr, (T_NREC_MODE)(0x0F & cur_prog_payload[HA_PROG_NR_OFFSET + 2]));
            nrec_level_set(ha_apt_nr, 0x0F & cur_prog_payload[HA_PROG_NR_OFFSET + 1]);
            nrec_enable(ha_apt_nr);
        }
        else
        {
            nrec_disable(ha_apt_nr);
        }
    }

    //apply WDRC while it is set
    if (cur_prog_payload[0] & BIT0)
    {
        wdrc_set(ha_apt_eq, cur_prog_payload + 12, 4 + HA_WDRC_RECV_BUF_IMG_LEN);
        wdrc_enable(ha_apt_eq);
    }

    //apply OVP while it is set
    if (cur_prog_payload[0] & BIT2)
    {
        app_ha_apply_ovp_effect((T_OVP_PAYLOAD *)(cur_prog_payload + HA_PROG_OVP_OFFSET));
    }
}

static void app_ha_set_runtime_nr_payload(uint8_t *data)
{
    //ignore role(data[0])
    memset(runtime_nr_payload,  HA_RUNTIME_NR_PAYLOAD_LEN, 0);
    memcpy(runtime_nr_payload + HA_RUNTIME_NR_HEADER_LEN, data + 1, 4);
    runtime_nr_payload[0] |= BIT0;

    return;
}

static void app_ha_set_runtime_prog_name_payload(uint8_t *data)
{
    memset(runtime_prog_name_payload, HA_RUNTIME_PROG_NAME_PAYLOAD_LEN, 0);
    memcpy(runtime_prog_name_payload + HA_RUNTIME_PROG_NAME_HEADER_LEN, data,
           HA_RUNTIME_PROG_NAME_PARAM_LEN);
    runtime_prog_name_payload[0] |= BIT0;

    return;
}

static uint8_t app_ha_get_hearable_prog_id()
{
    ha_prog_id = ha_option >> 16;

    if (ha_prog_id >= HA_PROG_NUM)
    {
        return 0;
    }

    return ha_prog_id;
}

static void app_ha_set_hearable_prog_id(uint8_t prog_id)
{
    uint16_t base = 0;
    uint8_t i = 0;

    if (ha_prog_id >= HA_PROG_NUM)
    {
        prog_id = 0;
    }

    ha_prog_id = prog_id;

    for (i = 16; i < 24; i++)
    {
        ha_option &= ~((BIT0) << i);
    }

    ha_option |= ha_prog_id << 16;
    base = app_ha_get_hearable_param_base(HA_FTL_OPTION, 0);
    ftl_save_to_storage(&ha_option, base, HA_OPTION_LEN);

    return;
}

void app_ha_switch_hearable_prog()
{
    app_ha_set_hearable_prog_id((ha_prog_id + 1) % HA_PROG_NUM);
    app_ha_load_prog_payload();
    app_ha_apply_cur_prog_effect();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
        app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_report_event_broadcast(EVENT_HA_PROGRAM_ID_INFO, &ha_prog_id, sizeof(uint8_t));
    }

    return;
}

static void app_ha_get_tool_extend_data(uint8_t prog_id, uint8_t *data)
{
    uint16_t base = 0;

    base = app_ha_get_hearable_param_base(HA_FTL_PROG, prog_id);
    ftl_load_from_storage(data, base + HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN, HA_TOOL_DATA_LEN);

    return;
}

bool app_ha_get_hearable_on()
{
    uint16_t base = 0;

    base = app_ha_get_hearable_param_base(HA_FTL_OPTION, 0);
    ftl_load_from_storage(&ha_option, base, HA_OPTION_LEN);

    return (bool)(ha_option & BIT0);
}

static void app_ha_set_hearable_on(bool value)
{
    uint16_t base = 0;

    ha_option &= ~BIT0;
    ha_option |= value;
    base = app_ha_get_hearable_param_base(HA_FTL_OPTION, 0);
    ftl_save_to_storage(&ha_option, base, HA_OPTION_LEN);

    return;
}

static uint8_t app_ha_get_hearable_effect_index()
{
    return ha_option >> 8;
}

static void app_ha_set_hearable_effect_index(uint8_t effect_index)
{
    uint16_t base = 0;
    uint8_t i = 0;

    for (i = 8; i < 16; i++)
    {
        ha_option &= ~((BIT0) << i);
    }

    ha_option |= effect_index << 8;
    base = app_ha_get_hearable_param_base(HA_FTL_OPTION, 0);

    ftl_save_to_storage(&ha_option, base, HA_OPTION_LEN);

    return;
}

static void app_ha_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case APP_IO_TIMER_HA_LISTENING_DELAY:
        {
            if (ha_timer_handle)
            {
                app_listening_state_machine(EVENT_APPLY_FINAL_STATE, false, true);

                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                {
                    current_apt_vol_level_cnt = 0;

                    if (low_to_high_gain_level[current_apt_vol_level_cnt] <= app_cfg_nv.apt_volume_out_level)
                    {
                        audio_passthrough_volume_out_set(low_to_high_gain_level[current_apt_vol_level_cnt]);
                        gap_stop_timer(&ha_timer_handle);
                        gap_start_timer(&ha_timer_handle, "ha_apt_vol_level", ha_timer_queue_id,
                                        APP_IO_TIMER_HA_APT_VOL_LEVEL, 0, 500);
                    }
                    else
                    {
                        audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);
                    }
                }
            }
        }
        break;

    case APP_IO_TIMER_HA_APT_VOL_LEVEL:
        {
            if (ha_timer_handle)
            {
                current_apt_vol_level_cnt++;
                gap_stop_timer(&ha_timer_handle);

                if (low_to_high_gain_level[current_apt_vol_level_cnt] <= app_cfg_nv.apt_volume_out_level)
                {
                    audio_passthrough_volume_out_set(low_to_high_gain_level[current_apt_vol_level_cnt]);

                    if (current_apt_vol_level_cnt < 4)
                    {
                        gap_start_timer(&ha_timer_handle, "ha_apt_vol_level", ha_timer_queue_id,
                                        APP_IO_TIMER_HA_APT_VOL_LEVEL, 0, 500);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

void app_ha_listening_turn_on_off(uint8_t status)
{
    if (status == SENSOR_LD_IN_EAR)
    {
        if (app_db.current_listening_state != *app_db.final_listening_state)
        {
            switch (*app_db.final_listening_state)
            {
            case ANC_OFF_NORMAL_APT_ON:
            case ANC_OFF_LLAPT_ON_SCENARIO_1:
            case ANC_OFF_LLAPT_ON_SCENARIO_2:
            case ANC_OFF_LLAPT_ON_SCENARIO_3:
            case ANC_OFF_LLAPT_ON_SCENARIO_4:
            case ANC_OFF_LLAPT_ON_SCENARIO_5:
                {
                    gap_start_timer(&ha_timer_handle, "ha_listening_delay", ha_timer_queue_id,
                                    APP_IO_TIMER_HA_LISTENING_DELAY, 0, HA_LISTENING_DELAY_TIME);
                }
                break;

            default:
                {
                    app_listening_state_machine(EVENT_APPLY_FINAL_STATE, false, true);
                }
                break;
            }

        }
    }
    else if (status == SENSOR_LD_OUT_EAR)
    {
        T_ANC_APT_EVENT pending_event = EVENT_ALL_OFF;

        gap_stop_timer(&ha_timer_handle);

        if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
        {
            T_ANC_APT_STATE listening_state = (T_ANC_APT_STATE)app_db.current_listening_state;

            app_listening_state_machine(EVENT_ALL_OFF, false, true);

            switch (listening_state)
            {
            case ANC_OFF_NORMAL_APT_ON:
                {
                    pending_event = EVENT_NORMAL_APT_ON;
                }
                break;

            case ANC_ON_SCENARIO_1_APT_OFF:
            case ANC_ON_SCENARIO_2_APT_OFF:
            case ANC_ON_SCENARIO_3_APT_OFF:
            case ANC_ON_SCENARIO_4_APT_OFF:
            case ANC_ON_SCENARIO_5_APT_OFF:
                {
                    pending_event = (T_ANC_APT_EVENT)(EVENT_ANC_ON_SCENARIO_1 + (listening_state -
                                                                                 ANC_ON_SCENARIO_1_APT_OFF));
                }
                break;

            case ANC_OFF_LLAPT_ON_SCENARIO_1:
            case ANC_OFF_LLAPT_ON_SCENARIO_2:
            case ANC_OFF_LLAPT_ON_SCENARIO_3:
            case ANC_OFF_LLAPT_ON_SCENARIO_4:
            case ANC_OFF_LLAPT_ON_SCENARIO_5:
                {
                    pending_event = (T_ANC_APT_EVENT)(EVENT_LLAPT_ON_SCENARIO_1 + (listening_state -
                                                                                   ANC_OFF_LLAPT_ON_SCENARIO_1));
                }
                break;

            default:
                break;
            }

            app_listening_state_machine(pending_event, false, true);
        }
    }

    return;
}

static bool app_ha_set_runtime_wdrc_payload(uint8_t *data)
{
    uint8_t img_num = 0;
    uint16_t img_base = 0;
    uint16_t img_len = 0;
    uint8_t img_type = 0;
    uint16_t i = 0;

    // ignore set_dsp/set_ftl (data[0])
    // ignore effect bitmap (data[1], data[2])
    // ignore stream bitmap (data[3])
    img_num = data[4];

    if (img_num > 2)
    {
        return false;
    }

    img_base = HA_WDRC_RECV_BUF_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_INFO_LEN * img_num;
    memset(runtime_wdrc_payload, 0, HA_RUNTIME_WDRC_PAYLOAD_LEN);
    runtime_wdrc_payload[4] = 0xb0;

    for (i = 0; i < img_num; i++)
    {
        img_type = (T_HA_IMG_TYPE)data[HA_WDRC_RECV_BUF_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_INFO_LEN * i];
        img_len = data[HA_WDRC_RECV_BUF_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_INFO_LEN * i + 1] +
                  (data[HA_WDRC_RECV_BUF_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_INFO_LEN * i + 2] << 8);

        if (((T_HA_IMG_TYPE)img_type == HA_IMG_APT_WDRC_EQ_L &&
             app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT) ||
            ((T_HA_IMG_TYPE)img_type == HA_IMG_APT_WDRC_EQ_R &&
             app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT))
        {
            memcpy(runtime_wdrc_payload + HA_RUNTIME_WDRC_HEADER_LEN, data + img_base, img_len);
        }

        img_base += img_len;
    }

    memcpy(runtime_wdrc_payload + HA_RUNTIME_WDRC_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_LEN,
           data + img_base, HA_WDRC_RECV_BUF_TOOL_DATA_LEN);
    runtime_wdrc_payload[0] |= BIT0;

    return true;
}

static void app_ha_apply_runtime_wdrc_effect()
{
    wdrc_set(ha_apt_eq, runtime_wdrc_payload + 4, 4 + HA_WDRC_RECV_BUF_IMG_LEN);
    wdrc_enable(ha_apt_eq);

    return;
}

static void app_ha_save_runtime_wdrc_effect()
{
    if (auto_save)
    {
        uint8_t data = cur_prog_payload[0] & (~BIT0);
        uint8_t nr_buf[4] = {0};
        uint8_t type3_buf[24] = {0};

        memcpy(nr_buf, cur_prog_payload + HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN +
               HA_TOOL_DATA_WDRC_LEN, HA_TOOL_DATA_NR_LEN);
        memcpy(type3_buf, cur_prog_payload + HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN +
               HA_TOOL_DATA_WDRC_LEN + HA_TOOL_DATA_NR_LEN,
               HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN + HA_TOOL_DATA_TYPE3_PAYLOAD_R_LEN);

        if (runtime_wdrc_payload[0] & BIT0)
        {
            data |= BIT0;
        }

        app_ha_set_prog_payload(0, &data, 1);
        app_ha_set_prog_payload(HA_PROG_HEADER_WDRC_NR_SET_LEN + HA_PROG_HEADER_WDRC_IMG_LEN +
                                HA_PROG_HEADER_RESERVED_LEN, runtime_wdrc_payload + 4, 4 + HA_WDRC_RECV_BUF_IMG_LEN);

        app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN,
                                runtime_wdrc_payload + HA_RUNTIME_WDRC_HEADER_LEN + HA_WDRC_RECV_BUF_IMG_LEN,
                                HA_WDRC_RECV_BUF_TOOL_DATA_LEN);

        app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN,
                                nr_buf, HA_TOOL_DATA_NR_LEN);
        app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN +
                                HA_TOOL_DATA_NR_LEN, type3_buf,
                                HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN + HA_TOOL_DATA_TYPE3_PAYLOAD_R_LEN);
    }

    return;
}

static T_HA_SEGMENT_STATUS app_ha_accmulate_cmd(uint8_t *buf, uint16_t cmd_len)
{
    T_HA_SEGMENT_STATUS is_finished = HA_SEGMENT_FINISH;
    uint16_t payload_len = 0;
    uint16_t offset = 0;

    /* skip cmd_id column */
    buf += 2;

    if (cmd_len < 8)
    {
        return HA_SEGMENT_ERROR;
    }

    cmd_len -= 2;

    /*
        buf[0]: seq
        buf[1:3]: total payload len
        buf[3:5]: offset
        buf[5]: is_finished
        buf[6:]: payload

    */

    offset = buf[3] + (buf[4] << 8);

    if (offset == 0) /* start of payload */
    {
        accumulate_data_seq = buf[0];
    }
    else if (buf[0] != accumulate_data_seq)
    {
        return HA_SEGMENT_ERROR;
    }

    is_finished = (T_HA_SEGMENT_STATUS)(buf[5] & BIT0);

    /* 6 is segment header */
    payload_len = cmd_len - 6;

    /* accumulate_data contains payload (func_flag + effect_para) */
    memcpy(accumulate_data + offset, buf + 6, payload_len);

    /* assembled cmd */
    if (is_finished == HA_SEGMENT_FINISH)
    {
        if (app_ha_set_runtime_wdrc_payload(accumulate_data) == false)
        {
            return HA_SEGMENT_PARA_ERROR;
        }

        app_ha_apply_runtime_wdrc_effect();
        app_ha_save_runtime_wdrc_effect();
    }

    return is_finished;
}

static void app_ha_apply_runtime_nr_effect()
{
    if (runtime_nr_payload[HA_RUNTIME_NR_HEADER_LEN] & BIT0)
    {
        nrec_mode_set(ha_apt_nr, (T_NREC_MODE)(0x0F & runtime_nr_payload[HA_RUNTIME_NR_HEADER_LEN + 2]));
        nrec_level_set(ha_apt_nr, 0x0F & runtime_nr_payload[HA_RUNTIME_NR_HEADER_LEN + 1]);
        nrec_enable(ha_apt_nr);
    }
    else
    {
        nrec_disable(ha_apt_nr);
    }

    if (runtime_wdrc_payload[0] & BIT0)
    {
        wdrc_set(ha_apt_eq, runtime_wdrc_payload + 4, 4 + HA_WDRC_RECV_BUF_IMG_LEN);
        wdrc_enable(ha_apt_eq);
    }
    else if (cur_prog_payload[0] & BIT0)
    {
        wdrc_set(ha_apt_eq, cur_prog_payload + 12, 4 + HA_WDRC_RECV_BUF_IMG_LEN);
        wdrc_enable(ha_apt_eq);
    }

    return;
}

static void app_ha_save_runtime_nr_effect()
{
    if (auto_save)
    {
        uint8_t data = cur_prog_payload[0] & 0xfd;

        if (runtime_nr_payload[0] & BIT0)
        {
            data |= BIT1;
        }

        app_ha_set_prog_payload(0, &data, 1);
        app_ha_set_prog_payload(HA_PROG_NR_OFFSET, runtime_nr_payload + HA_RUNTIME_NR_HEADER_LEN,
                                HA_RUNTIME_NR_PARAM_LEN);
    }

    return;
}

static void app_ha_save_runtime_prog_name()
{
    if (auto_save)
    {
        if (runtime_prog_name_payload[0] & BIT0)
        {
            uint16_t base = 0;

            base = app_ha_get_hearable_param_base(HA_FTL_PROG_NAME, 0);
            ftl_save_to_storage(runtime_prog_name_payload, base, HA_RUNTIME_PROG_NAME_PAYLOAD_LEN);
        }
    }

    return;
}

static void app_ha_set_runtime_ovp_payload(uint8_t *data)
{
    memset(runtime_ovp_payload,  HA_RUNTIME_OVP_PAYLOAD_LEN, 0);
    memcpy(runtime_ovp_payload + HA_RUNTIME_OVP_HEADER_LEN, data, 4);
    runtime_ovp_payload[0] |= BIT0;

    return;
}

static void app_ha_apply_runtime_ovp_effect()
{
    if (runtime_ovp_payload[HA_RUNTIME_OVP_HEADER_LEN] & BIT0)
    {
        app_ha_apply_ovp_effect((T_OVP_PAYLOAD *)(runtime_ovp_payload + HA_RUNTIME_OVP_HEADER_LEN));
    }
    else
    {
        audio_passthrough_ovp_disable();
    }

    return;
}

static void app_ha_save_runtime_ovp_effect()
{
    if (auto_save)
    {
        uint8_t data = cur_prog_payload[0] & 0xfb;

        if (runtime_ovp_payload[0] & BIT0)
        {
            data |= BIT2;
        }

        app_ha_set_prog_payload(0, &data, 1);
        app_ha_set_prog_payload(HA_PROG_OVP_OFFSET, runtime_ovp_payload + HA_RUNTIME_OVP_HEADER_LEN,
                                HA_RUNTIME_OVP_PARAM_LEN);
    }

    return;
}

static uint16_t app_ha_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_HA, 0, NULL, true, total);
}

static void app_ha_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                               T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_HA_SET_PARAM:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_HA_SEGMENT_STATUS status = app_ha_accmulate_cmd((uint8_t *)buf, len);
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_EFFECT_INDEX:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_set_hearable_effect_index(*((uint8_t *)buf));
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_ON_OFF:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_set_hearable_on(*((bool *)buf));
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_DSP_PARAM:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_probe_dsp_send(buf, len);

                if (auto_save)
                {
                    app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN +
                                            HA_TOOL_DATA_NR_LEN, (buf + 8), HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN);
                    app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN +
                                            HA_TOOL_DATA_NR_LEN + HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN, (buf + 8),
                                            HA_TOOL_DATA_TYPE3_PAYLOAD_R_LEN);
                }
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_PROG_ID:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_set_hearable_prog_id(*(uint8_t *)buf);
                app_ha_load_prog_payload();
                app_ha_apply_cur_prog_effect();
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_NR_PARAM:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_set_runtime_nr_payload(buf);
                app_ha_apply_runtime_nr_effect();
                app_ha_save_runtime_nr_effect();
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_PROG_NAME:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_set_runtime_prog_name_payload(buf);
                app_ha_save_runtime_prog_name();
            }
        }
        break;

    case APP_REMOTE_MSG_HA_SET_OVP_PARAM:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_ha_set_runtime_ovp_payload(buf);
                app_ha_apply_runtime_ovp_effect();
                app_ha_save_runtime_ovp_effect();
            }
        }
        break;

    default:
        break;
    }
}

void app_ha_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                       uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));

    switch (cmd_id)
    {
    case CMD_HA_SET_PARAM:
        {
            if (app_ha_get_hearable_on())
            {
                T_HA_SEGMENT_STATUS status = app_ha_accmulate_cmd(cmd_ptr, cmd_len);

                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_PARAM,
                                                    (uint8_t *)cmd_ptr, cmd_len);

                if (status == HA_SEGMENT_FINISH) /* assembled cmd*/
                {
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
                else if (status == HA_SEGMENT_NOT_FINISH)
                {
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
                else /* HA_SEGMENT_ERROR */
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_HA_VER_REQ:
        {
            uint8_t event_data[4] = {};
            uint32_t version = 0;

            //TODO: remove fixed version
            version = 0x12345678;
            memcpy(event_data, &version, sizeof(uint32_t));

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_HA_VER_INFO, app_idx, event_data, sizeof(uint32_t));
        }
        break;

    case CMD_HA_ACCESS_EFFECT_INDEX:
        {
            if (app_ha_get_hearable_on())
            {
                uint8_t is_set = cmd_ptr[2];
                uint8_t effect_idx = 0;

                if (is_set == 0)
                {
                    effect_idx = app_ha_get_hearable_effect_index();
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_HA_EFFECT_INDEX_INFO, app_idx, &effect_idx, sizeof(uint8_t));
                }
                else if (cmd_len - 2 == 2)
                {
                    effect_idx = cmd_ptr[3];
                    app_ha_set_hearable_effect_index(effect_idx);
                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_EFFECT_INDEX,
                                                        (uint8_t *)&effect_idx, sizeof(uint8_t));
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_HA_EFFECT_INDEX_INFO, app_idx, &effect_idx, sizeof(uint8_t));
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_HA_ACCESS_ON_OFF:
        {
            uint8_t is_set = cmd_ptr[2];
            uint8_t ha_enable = 0;

            if (is_set == 0)
            {
                ha_enable = (uint8_t)app_ha_get_hearable_on();
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_HA_ON_OFF_INFO, app_idx, &ha_enable, sizeof(uint8_t));
            }
            else if (cmd_len - 2 == 2)
            {
                ha_enable = cmd_ptr[3];
                app_ha_set_hearable_on((bool)ha_enable);
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_ON_OFF,
                                                    (uint8_t *)&ha_enable, sizeof(uint8_t));
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_HA_ON_OFF_INFO, app_idx, &ha_enable, sizeof(uint8_t));
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_HA_GET_TOOL_EXTEND_REQ:
        {
            if (app_ha_get_hearable_on() && cmd_ptr[3] < HA_PROG_NUM)
            {
                uint8_t event_data[HA_TOOL_DATA_LEN];

                memset(event_data, 0, HA_TOOL_DATA_LEN);
                app_ha_get_tool_extend_data(cmd_ptr[3], event_data);

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_HA_TOOL_EXTEND_INFO, app_idx, (uint8_t *)event_data,
                                 sizeof(uint8_t) * HA_TOOL_DATA_LEN);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_HA_SET_DSP_PARAM:
        {
            // role_type, type3_payload
            if ((cmd_len - 2) != 1 + HA_SET_DSP_PARAM_PAYLOAD_LEN)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                uint8_t buf[8 + HA_SET_DSP_PARAM_PAYLOAD_LEN];
                uint16_t payload_len = 8 + HA_SET_DSP_PARAM_PAYLOAD_LEN;

                memset(buf, 0, 8 + HA_SET_DSP_PARAM_PAYLOAD_LEN);
                memcpy(buf + 8, &cmd_ptr[3], (cmd_len - 3));

                buf[0] = CFG_SET_DSP_PARAM & 0xFF;
                buf[1] = CFG_SET_DSP_PARAM >> 8;
                buf[2] = (payload_len / 4) & 0xFF;
                buf[3] = (payload_len / 4) >> 8;
                buf[4] |= 0xb3; /* HA type3 payload */

                audio_probe_dsp_send(buf, payload_len);

                if (auto_save)
                {
                    app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN +
                                            HA_TOOL_DATA_NR_LEN, (&cmd_ptr[3]), HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN);
                    app_ha_set_prog_payload(HA_PROG_HEADER_LEN + HA_PROG_WDRC_LEN + HA_TOOL_DATA_WDRC_LEN +
                                            HA_TOOL_DATA_NR_LEN + HA_TOOL_DATA_TYPE3_PAYLOAD_L_LEN, (&cmd_ptr[3]),
                                            HA_TOOL_DATA_TYPE3_PAYLOAD_R_LEN);
                }

                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_DSP_PARAM,
                                                    buf, payload_len);
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_HA_ACCESS_PROGRAM_ID:
        {
            uint8_t is_set = cmd_ptr[2];
            uint8_t prog_id = 0;

            if (is_set == 0)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_HA_PROGRAM_ID_INFO, app_idx, &ha_prog_id,
                                 sizeof(uint8_t));
            }
            else if (cmd_len - 2 == 2)
            {
                prog_id = cmd_ptr[3];

                if (prog_id < HA_PROG_NUM)
                {
                    if (prog_id != ha_prog_id)
                    {
                        app_ha_set_hearable_prog_id(prog_id);
                        app_ha_load_prog_payload();
                        app_ha_apply_cur_prog_effect();

                        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_PROG_ID,
                                                            (uint8_t *)&prog_id, sizeof(uint8_t));
                    }

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    app_report_event(cmd_path, EVENT_HA_PROGRAM_ID_INFO, app_idx, &ha_prog_id,
                                     sizeof(uint8_t));
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_HA_SET_NR_PARAM:
        {
            // role + type4 payload
            if ((cmd_len - 2) != (1 + HA_TOOL_DATA_NR_LEN))
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            else
            {
                uint8_t event_data[1 + HA_TOOL_DATA_NR_LEN] = {0};

                memcpy(event_data, cmd_ptr + 2, 1 + HA_TOOL_DATA_NR_LEN);

                app_ha_set_runtime_nr_payload(cmd_ptr + 2);
                app_ha_apply_runtime_nr_effect();
                app_ha_save_runtime_nr_effect();

                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_NR_PARAM,
                                                    (uint8_t *)&event_data, 1 + HA_TOOL_DATA_NR_LEN);
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_HA_GET_PROGRAM_NUM:
        {
            uint8_t hearable_program_num = HA_PROG_NUM;

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_HA_PROGRAM_NUM_INFO, app_idx, &hearable_program_num,
                             sizeof(uint8_t));
        }
        break;

    case CMD_HA_ACCESS_PROGRAM_NAME:
        {
            uint8_t is_set = cmd_ptr[2];

            if (is_set == 0)
            {
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_HA_PROGRAM_NAME_INFO, app_idx,
                                 (uint8_t *)runtime_prog_name_payload + HA_RUNTIME_PROG_NAME_HEADER_LEN,
                                 HA_RUNTIME_PROG_NAME_PARAM_LEN);
            }
            else if (cmd_len - 3 == HA_RUNTIME_PROG_NAME_PARAM_LEN)
            {
                app_ha_set_runtime_prog_name_payload(cmd_ptr + 3);
                app_ha_save_runtime_prog_name();
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_PROG_NAME,
                                                    (uint8_t *)cmd_ptr + 3, cmd_len - 1);
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                app_report_event(cmd_path, EVENT_HA_PROGRAM_NAME_INFO, app_idx, (uint8_t *)cmd_ptr + 3,
                                 cmd_len - 1);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_HA_SET_OVP_PARAM:
        {
            if (cmd_len - 2 ==  HA_TOOL_DATA_OVP_LEN)
            {
                uint8_t event_data[HA_TOOL_DATA_OVP_LEN] = {0};

                memcpy(event_data, cmd_ptr + 2, HA_TOOL_DATA_OVP_LEN);

                app_ha_set_runtime_ovp_payload(cmd_ptr + 2);
                app_ha_apply_runtime_ovp_effect();
                app_ha_save_runtime_ovp_effect();

                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_SET_OVP_PARAM,
                                                    (uint8_t *)&event_data, HA_TOOL_DATA_OVP_LEN);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    default:
        break;
    }
}

void app_ha_init()
{
    uint8_t i = 0;
    uint16_t base = 0;
    uint8_t nr_param[4] = {1, 2, 3, 0};
    uint8_t buf[HA_PROG_LEN];

    base = app_ha_get_hearable_param_base(HA_FTL_OPTION, 0);
    ftl_load_from_storage(&ha_option, base, HA_OPTION_LEN);

    for (i = 0; i < HA_PROG_NUM; i++)
    {
        uint16_t base = 0;

        base = app_ha_get_hearable_param_base(HA_FTL_PROG, i);

        if (ftl_load_from_storage(buf, base, HA_PROG_LEN))
        {
            memset(buf, 0, HA_PROG_LEN);

            buf[0] = 0x0;
            ftl_save_to_storage(buf, base, HA_PROG_LEN);
            ftl_save_to_storage(nr_param, base + HA_PROG_NR_OFFSET, HA_TOOL_DATA_NR_LEN);
        }
    }

    base = app_ha_get_hearable_param_base(HA_FTL_PROG_NAME, 0);

    if (ftl_load_from_storage(runtime_prog_name_payload, base, HA_RUNTIME_PROG_NAME_PAYLOAD_LEN))
    {
        memset(runtime_prog_name_payload, 0, HA_RUNTIME_PROG_NAME_PAYLOAD_LEN);
    }

    gap_reg_timer_cb(app_ha_timer_callback, &ha_timer_queue_id);

    ha_apt_nr = nrec_create(NREC_CONTENT_TYPE_PASSTHROUGH, NREC_MODE_HIGH_SOUND_QUALITY, 0);
    audio_passthrough_effect_attach(ha_apt_nr);

    ha_apt_eq = wdrc_create(WDRC_CONTENT_TYPE_PASSTHROUGH, NULL, 0);
    audio_passthrough_effect_attach(ha_apt_eq);

    app_ha_get_hearable_prog_id();
    app_ha_load_prog_payload();

    // apply nr param if it is set
    if (cur_prog_payload[0] & BIT1)
    {
        if (cur_prog_payload[HA_PROG_NR_OFFSET] & BIT0)
        {
            nrec_mode_set(ha_apt_nr, (T_NREC_MODE)(0x0F & cur_prog_payload[HA_PROG_NR_OFFSET + 2]));
            nrec_level_set(ha_apt_nr, 0x0F & cur_prog_payload[HA_PROG_NR_OFFSET + 1]);
            nrec_enable(ha_apt_nr);
        }
        else
        {
            nrec_disable(ha_apt_nr);
        }
    }

    // apply wdrc param if it is set
    if (cur_prog_payload[0] & BIT0)
    {
        wdrc_set(ha_apt_eq, cur_prog_payload + 12, 4 + HA_WDRC_RECV_BUF_IMG_LEN);
        wdrc_enable(ha_apt_eq);
    }

    if (cur_prog_payload[0] & BIT2)
    {
        app_ha_apply_ovp_effect((T_OVP_PAYLOAD *)(cur_prog_payload + HA_PROG_OVP_OFFSET));
    }

    app_relay_cback_register(app_ha_relay_cback, app_ha_parse_cback,
                             APP_MODULE_TYPE_HA, APP_REMOTE_MSG_HA_TOTAL);

    return;
}

#endif
