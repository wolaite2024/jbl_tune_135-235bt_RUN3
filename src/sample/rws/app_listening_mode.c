#include <stdlib.h>
#include <string.h>
#include "app_cfg.h"
#include "app_main.h"
#include "trace.h"
#include "app_cfg.h"
#include "eq.h"
#include "eq_utils.h"
#include "audio.h"
#include "eq_utils.h"
#include "app_multilink.h"
#include "app_eq.h"

#if F_APP_APT_SUPPORT
#include "audio_passthrough.h"
#endif
#include "app_audio_policy.h"
#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif
#include "app_auto_power_off.h"

#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif
#if F_APP_ANC_SUPPORT
#include "anc_tuning.h"
#include "app_anc.h"
#endif
#include "app_bt_policy_api.h"
#include "gap_timer.h"
#include "app_device.h"
#include "app_relay.h"
#include "app_bt_policy_api.h"
#include "app_mmi.h"
#if (F_APP_HEARABLE_SUPPORT == 1)
#include "app_hearable.h"
#endif
#if (F_APP_SLIDE_SWITCH_SUPPORT == 1)
#include "app_slide_switch.h"
#endif
#include "app_hfp.h"
#include "app_roleswap.h"
#if (F_APP_LINEIN_SUPPORT == 1)
#include "app_line_in.h"
#include "audio_route.h"
#endif
#if F_APP_BRIGHTNESS_SUPPORT
#include "app_audio_passthrough_brightness.h"
#endif
#include "app_cmd.h"
#include "app_sensor.h"
#include "sysm.h"

#if F_APP_LISTENING_MODE_SUPPORT
typedef enum
{
    APP_TIMER_DELAY_APPLY_LISTENING_MODE,
    APP_TIMER_DISALLOW_TRIGGER_LISTENING_MODE,
} T_APP_LISTENING_MODE_TIMER;

typedef struct t_apt_anc_pending_evt
{
    bool enable;
    T_ANC_APT_STATE switch_state;
    bool is_need_push_tone;
} T_APT_ANC_PENDING_STATE;

static uint8_t app_listening_mode_timer_queue_id = 0;
static void *timer_handle_delay_apply_listening_mode = NULL;
static void *timer_handle_disallow_trigger_listening_mode = NULL;
static T_APT_ANC_SM_STATE apt_anc_state = APT_ANC_STOPPED;
static T_ANC_APT_STATE temp_listening_state = ANC_OFF_APT_OFF;
static T_ANC_APT_STATE blocked_listening_state = ANC_APT_STATE_TOTAL;
static uint16_t listening_special_event_bitmap;
T_ANC_APT_STATE special_event_state[LISTENING_EVENT_NUM];

T_LISTENING_QUEUE recover_state_queue;
static bool need_update_final_listening_state = false;
static T_AUDIO_TRACK_HANDLE voice_track_handle = NULL;
T_ANC_APT_CMD_POSTPONE_DATA anc_apt_cmd_postpone_data;

#define LISTENING_CMD_STATUS_SUCCESS       0
#define LISTENING_CMD_STATUS_FAILED        1
#define LISTENING_CMD_STATUS_UNKNOW_CMD    4

static void app_listening_special_event_state_reset(void)
{
    uint8_t i;

    for (i = 0; i < LISTENING_EVENT_NUM; i++)
    {
        special_event_state[i] = ANC_APT_STATE_TOTAL;
    }
}

static void app_listening_special_event_state_set(uint8_t event_index, T_ANC_APT_STATE state)
{
    special_event_state[event_index] = state;

    APP_PRINT_TRACE2("app_listening_special_event_state_set event_index = %x, state = %x",
                     event_index, state);
}

static bool app_listening_special_event_state_get(uint8_t event_index, T_ANC_APT_STATE *state)
{
    *state = special_event_state[event_index];

    APP_PRINT_TRACE2("app_listening_special_event_state_get event_index = %x, state = %x",
                     event_index, *state);

    return (*state != ANC_APT_STATE_TOTAL) ? true : false;
}



void app_listening_assign_specific_state(T_ANC_APT_STATE start_state, T_ANC_APT_STATE des_state,
                                         bool tone, bool update_final)
{
    if (des_state != start_state)
    {
        if (des_state == ANC_OFF_APT_OFF)
        {
#if F_APP_APT_SUPPORT
            if (app_apt_is_apt_on_state(start_state))
            {
                app_listening_state_machine(EVENT_APT_OFF, tone, update_final);
            }
            else
#endif
            {
#if F_APP_ANC_SUPPORT
                if (app_anc_is_anc_on_state(start_state))
                {
                    app_listening_state_machine(EVENT_ANC_OFF, tone, update_final);
                }
#endif
            }
        }
#if F_APP_APT_SUPPORT
        else if (app_apt_is_apt_on_state(des_state))
        {
            if (app_cfg_const.normal_apt_support)
            {
                app_listening_state_machine(EVENT_NORMAL_APT_ON, tone, update_final);
            }
            else if (app_cfg_const.llapt_support)
            {
                app_listening_state_machine(LLAPT_STATE_TO_EVENT(des_state), tone, update_final);
            }
        }
#endif
#if F_APP_ANC_SUPPORT
        else if (app_anc_is_anc_on_state(des_state))
        {
            app_listening_state_machine(ANC_STATE_TO_EVENT(des_state), tone, update_final);
        }
#endif
    }
}

static void app_listening_mode_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_listening_mode_timeout_cb: timer_id %d, timer_chann %d",
                     timer_id, timer_chann);
    switch (timer_id)
    {
    case APP_TIMER_DELAY_APPLY_LISTENING_MODE:
        {
            gap_stop_timer(&timer_handle_delay_apply_listening_mode);
            app_listening_state_machine(EVENT_DELAY_APPLY_LISTENING_MODE, timer_chann, true);
            app_db.delay_apply_listening_mode = false;
        }
        break;

    case APP_TIMER_DISALLOW_TRIGGER_LISTENING_MODE:
        {
            gap_stop_timer(&timer_handle_disallow_trigger_listening_mode);
            app_db.key_action_disallow_too_close = 0;//MMI_NULL
        }
        break;

    default:
        break;
    }
}

bool app_listening_is_busy(void)
{
    if (apt_anc_state == APT_ANC_STOPPING ||
        apt_anc_state == APT_ANC_STARTING)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void app_listening_audio_cback(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case AUDIO_EVENT_TRACK_STATE_CHANGED:
        {
            if (voice_track_handle == param->track_state_changed.handle)
            {
                if (param->track_state_changed.state == AUDIO_TRACK_STATE_RELEASED)
                {
                    voice_track_handle = NULL;
                    app_listening_judge_sco_event(APPLY_LISTENING_MODE_VOICE_TRACE_RELEASE);
                }
            }

            T_AUDIO_STREAM_TYPE stream_type;

            if (audio_track_stream_type_get(param->track_state_changed.handle, &stream_type) == false)
            {
                break;
            }

            if (stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (param->track_state_changed.state == AUDIO_TRACK_STATE_CREATED)
                {
                    voice_track_handle = param->track_state_changed.handle;
                }

                if (param->track_state_changed.state == AUDIO_TRACK_STATE_STARTED)
                {
#if F_APP_APT_SUPPORT
                    if (app_cfg_const.normal_apt_support)
                    {
                        if (app_apt_is_apt_on_state(temp_listening_state) && (app_listening_is_busy()))
                        {
                            /*  DSP-APT audio path be blocked by msbc/cvsd audio path,    *
                             *  there is no AUDIO_EVENT_PASSTHROUGH_ENABLED callback to   *
                             *  APP, so APP send commmand to close it.                    */
                            APP_PRINT_TRACE0("app_listening_audio_cback: DSP-APT/SCO can't coexist");

                            app_listening_state_set(APT_ANC_STARTED);
                            app_listening_state_machine(EVENT_ALL_OFF, false, false);
                        }
                    }
#endif
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
        APP_PRINT_INFO1("app_listening_audio_cback: event_type 0x%04x", event_type);
    }
}

static void app_listening_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
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
                        app_listening_anc_apt_relay(false);
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
        APP_PRINT_INFO1("app_listening_bt_cback: event_type 0x%04x", event_type);
    }
}

static bool app_listening_mode_tone_flush_and_play(T_APP_AUDIO_TONE_TYPE tone_type, bool relay)
{
    bool ret = false;

    app_audio_tone_type_cancel(TONE_ANC_APT_OFF, false);
    app_audio_tone_type_cancel(TONE_APT_ON, false);
#if (F_APP_HARMAN_FEATURE_SUPPORT == 0)
    app_audio_tone_type_cancel(TONE_ANC_SCENARIO_1, false);
    app_audio_tone_type_cancel(TONE_ANC_SCENARIO_2, false);
    app_audio_tone_type_cancel(TONE_ANC_SCENARIO_3, false);
    app_audio_tone_type_cancel(TONE_ANC_SCENARIO_4, false);
#endif
    app_audio_tone_type_cancel(TONE_ANC_SCENARIO_5, false);
    app_audio_tone_type_cancel(TONE_LLAPT_SCENARIO_1, false);
    app_audio_tone_type_cancel(TONE_LLAPT_SCENARIO_2, false);
    app_audio_tone_type_cancel(TONE_LLAPT_SCENARIO_3, false);
    app_audio_tone_type_cancel(TONE_LLAPT_SCENARIO_4, false);
    app_audio_tone_type_cancel(TONE_LLAPT_SCENARIO_5, false);

    ret = app_audio_tone_type_play(tone_type, false, relay);


    return ret;
}

void app_listening_report(uint16_t listening_report_event, uint8_t *event_data,
                          uint16_t event_len)
{
    bool handle = true;

    switch (listening_report_event)
    {
#if NEW_FORMAT_LISTENING_CMD_REPORT
    case EVENT_LISTENING_STATE_SET:
        {
            uint8_t cmd_status;

            cmd_status = event_data[0];

            app_report_event_broadcast(EVENT_LISTENING_STATE_SET, &cmd_status, sizeof(cmd_status));
        }
        break;

    case EVENT_LISTENING_STATE_STATUS:
        {
            uint8_t report_data[2] = {0};
#if (F_APP_ANC_SUPPORT | F_APP_APT_SUPPORT)
            uint8_t listening_state;

            listening_state = event_data[0];
#endif

#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)listening_state))
            {
                report_data[0] = LISTENING_STATE_ANC;
                report_data[1] = listening_state - (uint8_t)ANC_ON_SCENARIO_1_APT_OFF;

            }
            else
#endif
#if F_APP_APT_SUPPORT
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)listening_state))
                {
                    if (app_cfg_const.llapt_support)
                    {
                        report_data[0] = LISTENING_STATE_LLAPT;
                        report_data[1] = listening_state - (uint8_t)ANC_OFF_LLAPT_ON_SCENARIO_1;
                    }

                    if (app_cfg_const.normal_apt_support)
                    {
                        report_data[0] = LISTENING_STATE_NORMAL_APT;
                    }
                }
                else
#endif
                {
                    report_data[0] = LISTENING_STATE_ALL_OFF;
                }

            app_report_event_broadcast(EVENT_LISTENING_STATE_STATUS, report_data, sizeof(report_data));
        }
        break;
#endif

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_listening_report = %x", listening_report_event);
    }
}

T_APT_ANC_SM_STATE app_listening_state_get(void)
{
    return apt_anc_state;
}

void app_listening_state_set(T_APT_ANC_SM_STATE state)
{
    apt_anc_state = state;
}

bool app_listening_apply_state_check(T_ANC_APT_STATE apply_state)
{
    uint32_t check_result = 0;

    if (app_db.device_state != APP_DEVICE_STATE_ON)
    {
        if (apply_state != ANC_OFF_APT_OFF)
        {
            check_result |= BIT(0);
        }
    }

    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_BOX))
    {
        if (apply_state != ANC_OFF_APT_OFF)
        {
            check_result |= BIT(1);
        }
    }

    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_B2S_CONNECT))
    {
        if (apply_state != ANC_OFF_APT_OFF)
        {
            check_result |= BIT(2);
        }
    }

    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_B2B_CONNECT))
    {
        if (apply_state != ANC_OFF_APT_OFF)
        {
            check_result |= BIT(3);
        }
    }

#if (F_APP_AIRPLANE_SUPPORT == 1)
    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_AIRPLANE))
    {
        if (app_cfg_const.disallow_anc_apt_off_when_airplane_mode
            && (apply_state == ANC_OFF_APT_OFF)
            && (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_BOX)) == 0)
        {
            check_result |= BIT(4);
        }
    }
#endif

    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_XIAOAI))
    {
#if F_APP_APT_SUPPORT
        if (app_apt_is_apt_on_state(apply_state))
        {
            check_result |= BIT(5);
        }
#endif
    }

#if F_APP_APT_SUPPORT
    if (app_apt_is_apt_on_state(apply_state))
    {
        if (!app_apt_open_condition_check())
        {
            check_result |= BIT(6);
        }
    }
    else
#endif
    {
#if F_APP_ANC_SUPPORT
        if (app_anc_is_anc_on_state(apply_state))
        {
            if (!app_anc_open_condition_check())
            {
                check_result |= BIT(7);
            }
        }
#endif
    }

    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_GAMING_MODE))
    {
        if (apply_state != ANC_OFF_APT_OFF)
        {
            check_result |= BIT(8);
        }
    }

#if (F_APP_HEARABLE_SUPPORT == 1)
    if (LIGHT_SENSOR_ENABLED &&
        (!app_db.local_in_ear) &&
        (apply_state != ANC_OFF_APT_OFF))
    {
        //ANC/APT open condition fail: out ear
        check_result |= BIT(9);
    }
#endif

    if (check_result)
    {
        APP_PRINT_INFO1("app_listening_apply_state_check: check_result = 0x%04x", check_result);
    }

    return (check_result == 0) ? true : false;
}

uint8_t app_listening_check_delay_apply_time(T_ANC_APT_STATE new_state, T_ANC_APT_STATE prev_state,
                                             T_ANC_APT_EVENT event, bool tone)
{
    uint8_t delay_apply_time = 0;

#if (F_APP_ANC_SUPPORT == 0)
    if (!app_cfg_const.llapt_support)
    {
        return delay_apply_time;
    }
#endif

    if (!tone && !app_db.power_on_delay_open_apt_time)
    {
        // Gernaral    : no tone with no delay apply
        // Special case: delay to open APT when power on
        return delay_apply_time;
    }

    if ((app_db.delay_apply_listening_mode) &&
        (event != EVENT_DELAY_APPLY_LISTENING_MODE) &&
        (timer_handle_delay_apply_listening_mode))
    {
        gap_stop_timer(&timer_handle_delay_apply_listening_mode);

        delay_apply_time = app_listening_set_delay_apply_time(new_state, prev_state);

        if (delay_apply_time)
        {
            if (event != EVENT_ALL_OFF)
            {
                *app_db.final_listening_state = new_state;
            }

            APP_PRINT_TRACE2("app_listening_state_machine: delay_listening_state AGAIN = %d, time = %d",
                             *app_db.final_listening_state, delay_apply_time);

            gap_start_timer(&timer_handle_delay_apply_listening_mode,
                            "delay_apply_listening_mode", app_listening_mode_timer_queue_id,
                            APP_TIMER_DELAY_APPLY_LISTENING_MODE, 0, delay_apply_time * 1000);
        }
        else
        {
            app_db.delay_apply_listening_mode = false;
        }
    }

    if ((event != EVENT_DELAY_APPLY_LISTENING_MODE) &&
        (event != EVENT_APPLY_PENDING_STATE) &&
        ((event != EVENT_APPLY_FINAL_STATE) ||
         ((event == EVENT_APPLY_FINAL_STATE) &&
          (app_db.power_on_delay_open_apt_time))) &&
        (event != EVENT_ALL_OFF) &&
        (event != EVENT_APPLY_BLOCKED_STATE) &&
        !app_db.delay_apply_listening_mode)
    {
        bool is_delay_apt_power_on = false;
        delay_apply_time = app_listening_set_delay_apply_time(new_state, prev_state);

#if F_APP_APT_SUPPORT
        if ((app_apt_is_apt_on_state(new_state)) && (app_db.power_on_delay_open_apt_time))
        {
            is_delay_apt_power_on = true;
            app_db.power_on_delay_open_apt_time = 0;
        }
#endif

        if (delay_apply_time)
        {
            app_db.delay_apply_listening_mode = true;
            *app_db.final_listening_state = new_state;
            gap_start_timer(&timer_handle_delay_apply_listening_mode,
                            "delay_apply_listening_mode", app_listening_mode_timer_queue_id,
                            APP_TIMER_DELAY_APPLY_LISTENING_MODE, is_delay_apt_power_on, delay_apply_time * 1000);
        }
    }

    return delay_apply_time;
}

void app_listening_stop_delay_apply_state(void)
{
    app_db.delay_apply_listening_mode = false;
    gap_stop_timer(&timer_handle_delay_apply_listening_mode);
}

bool app_listening_is_allow_all_off_condition_check(void)
{
    if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_AIRPLANE))
    {
        if (app_cfg_const.disallow_anc_apt_off_when_airplane_mode)
        {
            return false;
        }
    }

    return true;
}

static T_ANC_APT_STATE app_listening_mode_new_state_decide(T_ANC_APT_STATE current_state,
                                                           T_ANC_APT_EVENT event)
{
    T_ANC_APT_STATE new_state = (T_ANC_APT_STATE)(*app_db.final_listening_state);

    if (event == EVENT_ALL_OFF)
    {
        new_state = ANC_OFF_APT_OFF;
        return new_state;
    }
    else if (event == EVENT_APPLY_PENDING_STATE)
    {
        new_state = (T_ANC_APT_STATE)app_db.current_listening_state;
        return new_state;
    }
    else if ((event == EVENT_DELAY_APPLY_LISTENING_MODE) ||
             (event == EVENT_APPLY_FINAL_STATE))
    {
        return new_state;
    }
    else if (event == EVENT_APPLY_BLOCKED_STATE)
    {
        new_state = blocked_listening_state;
        return new_state;
    }

    switch (current_state)
    {
    case ANC_OFF_APT_OFF:
        {
#if F_APP_APT_SUPPORT
            if (app_apt_related_event(event))
            {
                if (app_cfg_const.normal_apt_support)
                {
                    new_state = ANC_OFF_NORMAL_APT_ON;
                }
                else if (app_cfg_const.llapt_support)
                {
                    new_state = LLAPT_EVENT_TO_STATE(event);
                }
            }
#endif
#if F_APP_ANC_SUPPORT
            if (app_anc_related_event(event))
            {
                new_state = ANC_EVENT_TO_STATE(event);
            }
#endif
            if (event == EVENT_LISTENING_MODE_CYCLE)
            {
                if (app_cfg_nv.listening_mode_cycle == 0)
                {
#if F_APP_ANC_SUPPORT
                    if (app_anc_open_condition_check())
                    {
                        app_anc_set_first_anc_sceanrio(&new_state);
                    }
                    else
#endif
                    {
#if F_APP_APT_SUPPORT
                        if (app_apt_open_condition_check())
                        {
                            if (app_cfg_const.normal_apt_support)
                            {
                                new_state = ANC_OFF_NORMAL_APT_ON;
                            }
                            else if (app_cfg_const.llapt_support)
                            {
                                app_apt_set_first_llapt_scenario(&new_state);
                            }
                        }
#endif
                    }
                }
                else if (app_cfg_nv.listening_mode_cycle == 1)
                {
#if F_APP_APT_SUPPORT
                    if (app_apt_open_condition_check())
                    {
                        if (app_cfg_const.normal_apt_support)
                        {
                            new_state = ANC_OFF_NORMAL_APT_ON;
                        }
                        else if (app_cfg_const.llapt_support)
                        {
                            app_apt_set_first_llapt_scenario(&new_state);
                        }
                    }
                    else
#endif
                    {
#if F_APP_ANC_SUPPORT
                        if (app_anc_open_condition_check())
                        {
                            app_anc_set_first_anc_sceanrio(&new_state);
                        }
#endif
                    }
                }
                else if (app_cfg_nv.listening_mode_cycle == 2)
                {
#if F_APP_APT_SUPPORT
                    if (app_apt_open_condition_check())
                    {
                        if (app_cfg_const.normal_apt_support)
                        {
                            new_state = ANC_OFF_NORMAL_APT_ON;
                        }
                        else if (app_cfg_const.llapt_support)
                        {
                            app_apt_set_first_llapt_scenario(&new_state);
                        }
                    }
                    else
#endif
                    {
#if F_APP_ANC_SUPPORT
                        if (app_anc_open_condition_check())
                        {
                            app_anc_set_first_anc_sceanrio(&new_state);
                        }
#endif
                    }
                }
                else if (app_cfg_nv.listening_mode_cycle == 3)
                {
#if F_APP_ANC_SUPPORT
                    if (app_anc_open_condition_check())
                    {
                        app_anc_set_first_anc_sceanrio(&new_state);
                    }
#endif
                }
            }
        }
        break;

#if F_APP_APT_SUPPORT
    case ANC_OFF_NORMAL_APT_ON:
    case ANC_OFF_LLAPT_ON_SCENARIO_1:
    case ANC_OFF_LLAPT_ON_SCENARIO_2:
    case ANC_OFF_LLAPT_ON_SCENARIO_3:
    case ANC_OFF_LLAPT_ON_SCENARIO_4:
    case ANC_OFF_LLAPT_ON_SCENARIO_5:
        {
            if (event == EVENT_APT_OFF)
            {
                new_state = ANC_OFF_APT_OFF;
            }
#if F_APP_ANC_SUPPORT
            else if (app_anc_related_event(event))
            {
                new_state = ANC_EVENT_TO_STATE(event);
            }
#endif
            else if (event == EVENT_LISTENING_MODE_CYCLE || (event == EVENT_LLAPT_CYCLE))
            {
                if (app_cfg_const.normal_apt_support)
                {
                    if (app_cfg_nv.listening_mode_cycle == 0)
                    {
                        if (app_listening_is_allow_all_off_condition_check())
                        {
                            new_state = ANC_OFF_APT_OFF;
                        }
                        else
                        {
#if F_APP_ANC_SUPPORT
                            if (app_anc_open_condition_check())
                            {
                                app_anc_set_first_anc_sceanrio(&new_state);
                            }
#endif
                        }
                    }
                    else
                    {
#if F_APP_ANC_SUPPORT
                        if (app_anc_open_condition_check())
                        {
                            if (app_cfg_nv.listening_mode_cycle == 3)
                            {
                                new_state = (T_ANC_APT_STATE)app_db.last_listening_state;
                            }
                            else
                            {
                                app_anc_set_first_anc_sceanrio(&new_state);
                            }
                        }
                        else
#endif
                        {
                            if (app_cfg_nv.listening_mode_cycle == 1)
                            {
                                if (app_listening_is_allow_all_off_condition_check())
                                {
                                    new_state = ANC_OFF_APT_OFF;
                                }
                            }
                        }
                    }
                }
                else if (app_cfg_const.llapt_support)
                {
                    if ((event == EVENT_LISTENING_MODE_CYCLE) &&
                        (app_cfg_nv.listening_mode_cycle == 3))
                    {
#if F_APP_ANC_SUPPORT
                        if (app_anc_open_condition_check())
                        {
                            new_state = (T_ANC_APT_STATE)app_db.last_listening_state;
                        }
#endif
                        break;
                    }

                    T_LLAPT_SWITCH_SCENARIO result;
                    T_ANC_APT_STATE llapt_next_scenario;

                    result = app_llapt_switch_next_scenario(current_state, &llapt_next_scenario);

                    if (result == LLAPT_SWITCH_SCENARIO_SUCCESS)
                    {
                        if (app_apt_open_condition_check())
                        {
                            new_state = llapt_next_scenario;
                        }
                    }
                    else
                    {
                        if (event == EVENT_LLAPT_CYCLE)
                        {
                            if (app_apt_open_condition_check())
                            {
                                app_apt_set_first_llapt_scenario(&new_state);
                            }
                        }
                        else if (event == EVENT_LISTENING_MODE_CYCLE)
                        {
                            if (app_cfg_nv.listening_mode_cycle == 0)
                            {
                                if (app_listening_is_allow_all_off_condition_check())
                                {
                                    new_state = ANC_OFF_APT_OFF;
                                }
                                else
                                {
#if F_APP_ANC_SUPPORT
                                    if (app_anc_open_condition_check())
                                    {
                                        app_anc_set_first_anc_sceanrio(&new_state);
                                    }
#endif
                                }
                            }
                            else if (app_cfg_nv.listening_mode_cycle == 1)
                            {
#if F_APP_ANC_SUPPORT
                                if (app_anc_open_condition_check())
                                {
                                    app_anc_set_first_anc_sceanrio(&new_state);
                                }
                                else
#endif
                                {
                                    if (app_listening_is_allow_all_off_condition_check())
                                    {
                                        new_state = ANC_OFF_APT_OFF;
                                    }
                                }
                            }
                            else if (app_cfg_nv.listening_mode_cycle == 2)
                            {
#if F_APP_ANC_SUPPORT
                                if (app_anc_open_condition_check())
                                {
                                    app_anc_set_first_anc_sceanrio(&new_state);
                                }
                                else
#endif
                                {
                                    if (app_apt_open_condition_check())
                                    {
                                        app_apt_set_first_llapt_scenario(&new_state);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if ((app_apt_related_event(event)) &&
                     (app_cfg_const.llapt_support))
            {
                new_state = LLAPT_EVENT_TO_STATE(event);
            }
        }
        break;
#endif

#if F_APP_ANC_SUPPORT
    /* app_cfg.enable_anc_when_sco == false and sco connected should not stay in these state  */
    case ANC_ON_SCENARIO_1_APT_OFF:
    case ANC_ON_SCENARIO_2_APT_OFF:
    case ANC_ON_SCENARIO_3_APT_OFF:
    case ANC_ON_SCENARIO_4_APT_OFF:
    case ANC_ON_SCENARIO_5_APT_OFF:
        {
#if F_APP_APT_SUPPORT
            if (app_apt_related_event(event))
            {
                if (app_cfg_const.normal_apt_support)
                {
                    new_state = ANC_OFF_NORMAL_APT_ON;
                }
                else if (app_cfg_const.llapt_support)
                {
                    new_state = LLAPT_EVENT_TO_STATE(event);
                }
            }
#endif
            if (event == EVENT_ANC_OFF)
            {
                new_state = ANC_OFF_APT_OFF;
            }
            else if ((event == EVENT_LISTENING_MODE_CYCLE) || (event == EVENT_ANC_CYCLE))
            {
                T_ANC_SWITCH_SCENARIO result;
                T_ANC_APT_STATE anc_next_scenario;

                result = app_anc_switch_next_scenario(current_state, &anc_next_scenario);

                if (result == ANC_SWITCH_SCENARIO_SUCCESS)
                {
                    if (app_anc_open_condition_check())
                    {
                        new_state = anc_next_scenario;
                    }
                }
                else
                {
                    if (event == EVENT_ANC_CYCLE)
                    {
                        if (app_anc_open_condition_check())
                        {
                            app_anc_set_first_anc_sceanrio(&new_state);
                        }
                    }
                    else if (event == EVENT_LISTENING_MODE_CYCLE)
                    {
                        if (app_cfg_nv.listening_mode_cycle == 0)
                        {
#if F_APP_APT_SUPPORT
                            if (app_apt_open_condition_check())
                            {
                                if (app_cfg_const.normal_apt_support)
                                {
                                    new_state = ANC_OFF_NORMAL_APT_ON;
                                }
                                else if (app_cfg_const.llapt_support)
                                {
                                    app_apt_set_first_llapt_scenario(&new_state);
                                }
                            }
                            else
#endif
                            {
                                if (app_listening_is_allow_all_off_condition_check())
                                {
                                    new_state = ANC_OFF_APT_OFF;
                                }
                            }
                        }
                        else if (app_cfg_nv.listening_mode_cycle == 1)
                        {
                            if (app_listening_is_allow_all_off_condition_check())
                            {
                                new_state = ANC_OFF_APT_OFF;
                            }
                            else
                            {
#if F_APP_APT_SUPPORT
                                if (app_apt_open_condition_check())
                                {
                                    if (app_cfg_const.normal_apt_support)
                                    {
                                        new_state = ANC_OFF_NORMAL_APT_ON;
                                    }
                                    else if (app_cfg_const.llapt_support)
                                    {
                                        app_apt_set_first_llapt_scenario(&new_state);
                                    }
                                }
#endif
                            }
                        }
                        else if (app_cfg_nv.listening_mode_cycle == 2)
                        {
#if F_APP_APT_SUPPORT
                            if (app_apt_open_condition_check())
                            {
                                if (app_cfg_const.normal_apt_support)
                                {
                                    new_state = ANC_OFF_NORMAL_APT_ON;
                                }
                                else if (app_cfg_const.llapt_support)
                                {
                                    app_apt_set_first_llapt_scenario(&new_state);
                                }
                            }
                            else
#endif
                            {
                                if (app_anc_open_condition_check())
                                {
                                    app_anc_set_first_anc_sceanrio(&new_state);
                                }
                            }
                        }
                        else if (app_cfg_nv.listening_mode_cycle == 3)
                        {
                            if (app_listening_is_allow_all_off_condition_check())
                            {
                                new_state = ANC_OFF_APT_OFF;
                            }
                            else if (app_anc_open_condition_check())
                            {
                                app_anc_set_first_anc_sceanrio(&new_state);
                            }
                        }
                    }
                }
            }
            else if (app_anc_related_event(event))
            {
                new_state = ANC_EVENT_TO_STATE(event);
            }
        }
        break;
#endif

    default:
        break;
    }

#if (F_APP_ANC_SUPPORT && (F_APP_SLIDE_SWITCH_LISTENING_MODE || F_APP_LINEIN_SUPPORT))
    if (app_anc_is_anc_on_state(new_state))
    {
        bool anc_state_change = false;

#if (F_APP_LINEIN_SUPPORT == 1)
        if (app_line_in_plug_state_get())
        {
            anc_state_change = true;
        }
#endif

        if (anc_state_change)
        {
#if (F_APP_LINEIN_SUPPORT == 1)
            if ((T_ANC_APT_STATE)app_line_in_anc_line_in_mode_state_get() != ANC_OFF_APT_OFF)
            {
                new_state = (T_ANC_APT_STATE)app_line_in_anc_line_in_mode_state_get();
            }
#else
            if (app_anc_get_selected_scenario_cnt() >= 2 &&
                new_state != ANC_ON_SCENARIO_2_APT_OFF)
            {
                new_state = ANC_ON_SCENARIO_2_APT_OFF;
            }
#endif
        }
    }
#endif

    if ((app_cfg_nv.listening_mode_cycle == 3)
#if F_APP_APT_SUPPORT
        && !(app_apt_is_apt_on_state(new_state))
#endif
       )
    {
        app_db.last_listening_state = new_state;
    }

    APP_PRINT_TRACE3("app_listening_mode_new_state_decide curr_state = 0x%X, event = 0x%X, new_state = 0x%X",
                     current_state, event, new_state);

    return new_state;
}

static void app_listening_mode_tone_play(T_ANC_APT_STATE new_state, bool is_sync_play)
{
    bool need_play = false;

#if (F_APP_AVP_INIT_SUPPORT == 1)
    if ((app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY) ||
        (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED))
    {
        if (app_hfp_get_call_status() != BT_HFP_VOICE_ACTIVATION_ONGOING)
        {
            need_play = true;
        }
    }
#else
    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) ||
        (is_sync_play == false))
    {
        need_play = true;
    }
#endif

    if (need_play)
    {
        if (new_state == ANC_OFF_APT_OFF)
        {
            //app_audio_tone_play(app_cfg_const.tone_anc_apt_off, false, true);
            app_listening_mode_tone_flush_and_play(TONE_ANC_APT_OFF, is_sync_play);
        }
#if F_APP_APT_SUPPORT
        if (app_apt_is_apt_on_state(new_state))
        {
            switch (new_state)
            {
            case ANC_OFF_NORMAL_APT_ON:
                app_listening_mode_tone_flush_and_play(TONE_APT_ON, is_sync_play);
                break;

            case ANC_OFF_LLAPT_ON_SCENARIO_1:
                app_listening_mode_tone_flush_and_play(TONE_LLAPT_SCENARIO_1, is_sync_play);
                break;

            case ANC_OFF_LLAPT_ON_SCENARIO_2:
                app_listening_mode_tone_flush_and_play(TONE_LLAPT_SCENARIO_2, is_sync_play);
                break;

            case ANC_OFF_LLAPT_ON_SCENARIO_3:
                app_listening_mode_tone_flush_and_play(TONE_LLAPT_SCENARIO_3, is_sync_play);
                break;

            case ANC_OFF_LLAPT_ON_SCENARIO_4:
                app_listening_mode_tone_flush_and_play(TONE_LLAPT_SCENARIO_4, is_sync_play);
                break;

            case ANC_OFF_LLAPT_ON_SCENARIO_5:
                app_listening_mode_tone_flush_and_play(TONE_LLAPT_SCENARIO_5, is_sync_play);
                break;

            default:
                break;
            }
        }
#endif
#if F_APP_ANC_SUPPORT
        if (app_anc_is_anc_on_state(new_state))
        {
            switch (new_state)
            {
            case ANC_ON_SCENARIO_1_APT_OFF:
                //app_audio_tone_play(app_cfg_const.tone_anc_scenario_1, false, true);
                app_listening_mode_tone_flush_and_play(TONE_ANC_SCENARIO_1, is_sync_play);
                break;

            case ANC_ON_SCENARIO_2_APT_OFF:
                //app_audio_tone_play(app_cfg_const.tone_anc_scenario_2, false, true);
                app_listening_mode_tone_flush_and_play(TONE_ANC_SCENARIO_2, is_sync_play);
                break;

            case ANC_ON_SCENARIO_3_APT_OFF:
                //app_audio_tone_play(app_cfg_const.tone_anc_scenario_3, false, true);
                app_listening_mode_tone_flush_and_play(TONE_ANC_SCENARIO_3, is_sync_play);
                break;

            case ANC_ON_SCENARIO_4_APT_OFF:
                //app_audio_tone_play(app_cfg_const.tone_anc_scenario_4, false, true);
                app_listening_mode_tone_flush_and_play(TONE_ANC_SCENARIO_4, is_sync_play);
                break;

            case ANC_ON_SCENARIO_5_APT_OFF:
                //app_audio_tone_play(app_cfg_const.tone_anc_scenario_5, false, true);
                app_listening_mode_tone_flush_and_play(TONE_ANC_SCENARIO_5, is_sync_play);
                break;

            default:
                break;
            }
        }
#endif
    }

    APP_PRINT_TRACE6("app_listening_mode_tone_play: need_play = %d, state = 0x%X, loc = 0x%X, role = 0x%X, remote = 0x%X, is_sync_play = %d",
                     need_play, new_state, app_db.local_loc, app_cfg_nv.bud_role, app_db.remote_session_state,
                     is_sync_play);
}

void app_listening_mode_apply_new_state(T_ANC_APT_STATE new_state)
{
    if (temp_listening_state == (T_ANC_APT_STATE)app_db.current_listening_state)
    {
        //The listening state is the same, no need to assign again
        return;
    }

    /* make the actually anc/apt effect */
    if (new_state == ANC_OFF_APT_OFF)
    {
#if F_APP_ANC_SUPPORT
        if (app_anc_is_anc_on_state(temp_listening_state))
        {
            app_listening_state_set(APT_ANC_STOPPING);
            temp_listening_state = new_state;

            anc_disable();
            if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                (app_bt_policy_get_b2s_connected_num() != 0) && (app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
                (app_audio_get_bud_stream_state() == BUD_STREAM_STATE_IDLE))
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_ANC_APT_MODE_WITH_PHONE_CONNECTED |
                                          AUTO_POWER_OFF_MASK_ANC_APT_MODE,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }
            else if (!app_cfg_const.enable_auto_power_off_when_anc_apt_on)
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_ANC_APT_MODE, app_cfg_const.timer_auto_power_off);
            }
        }
#endif
#if F_APP_APT_SUPPORT
        if (app_apt_is_apt_on_state(temp_listening_state))
        {
            app_listening_state_set(APT_ANC_STOPPING);
            temp_listening_state = new_state;

            audio_passthrough_disable();
            if (app_db.apt_eq_instance != NULL)
            {
                eq_release(app_db.apt_eq_instance);
                app_db.apt_eq_instance = NULL;
            }

            if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                (app_bt_policy_get_b2s_connected_num() != 0) && (app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
                (app_audio_get_bud_stream_state() == BUD_STREAM_STATE_IDLE))
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_ANC_APT_MODE_WITH_PHONE_CONNECTED |
                                          AUTO_POWER_OFF_MASK_ANC_APT_MODE,
                                          app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off);
            }
            else
            {
                app_auto_power_off_enable(AUTO_POWER_OFF_MASK_ANC_APT_MODE, app_cfg_const.timer_auto_power_off);
            }
        }
#endif
    }
#if F_APP_APT_SUPPORT
    else if (app_apt_is_apt_on_state(new_state))
    {
        audio_passthrough_volume_out_set(app_cfg_nv.apt_volume_out_level);

#if F_APP_BRIGHTNESS_SUPPORT
        if (app_cfg_const.llapt_support)
        {
            if (app_apt_brightness_llapt_scenario_support(new_state))
            {

                float alpha;

                if (app_db.brightness_type == MAIN_TYPE_LEVEL)
                {
                    alpha = (float)brightness_gain_table[app_cfg_nv.main_brightness_level] * 0.0025f;
                }
                else //SUB_TYPE_LEVEL
                {
                    alpha = (float)app_cfg_nv.sub_brightness_level / (float)(LLAPT_SUB_BRIGHTNESS_LEVEL_MAX -
                                                                             LLAPT_SUB_BRIGHTNESS_LEVEL_MIN);
                }


                audio_passthrough_brightness_set(alpha);
            }
            else
#endif
            {
                //not support brigntness adjust, it also need the default value
                audio_passthrough_brightness_set(0.5);
            }
#if F_APP_BRIGHTNESS_SUPPORT
        }
#endif

#if F_APP_ANC_SUPPORT
        if (app_anc_is_anc_on_state(temp_listening_state))
        {
            app_listening_state_set(APT_ANC_STOPPING);
            temp_listening_state = ANC_OFF_APT_OFF;
            anc_disable();
        }
        else
#endif
        {
            if (!app_apt_is_apt_on_state(temp_listening_state))
            {
                app_listening_state_set(APT_ANC_STARTING);
                temp_listening_state = new_state;

                if (app_cfg_const.normal_apt_support)
                {
                    audio_passthrough_enable(AUDIO_PASSTHROUGH_MODE_NORMAL, 0);

                    if (app_db.apt_eq_instance != NULL)
                    {
                        eq_release(app_db.apt_eq_instance);
                        app_db.apt_eq_instance = NULL;
                    }

                    app_db.apt_eq_instance = app_eq_create(EQ_CONTENT_TYPE_PASSTHROUGH, EQ_STREAM_TYPE_AUDIO,
                                                           MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);

                    if (app_db.apt_eq_instance != NULL)
                    {
                        //not set default eq when audio connect,enable when set eq para from SRC
                        if (!app_db.eq_ctrl_by_src)
                        {
                            eq_enable(app_db.apt_eq_instance);
                        }
                        audio_passthrough_effect_attach(app_db.apt_eq_instance);
                    }
                }
                else if (app_cfg_const.llapt_support)
                {
                    uint8_t new_llapt_scenario = new_state - ANC_OFF_LLAPT_ON_SCENARIO_1;
                    uint8_t coff_idx = app_apt_llapt_get_coeff_idx(new_llapt_scenario);

                    audio_passthrough_enable(AUDIO_PASSTHROUGH_MODE_LOW_LATENCY, coff_idx);
                }

                if (!app_cfg_const.enable_auto_power_off_when_anc_apt_on)
                {
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ANC_APT_MODE);
                }

                if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
                    (app_bt_policy_get_b2s_connected_num() != 0))
                {
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ANC_APT_MODE_WITH_PHONE_CONNECTED);
                }
            }
            else if ((app_apt_is_apt_on_state(temp_listening_state)) &&
                     (app_cfg_const.llapt_support))
            {
                app_listening_state_set(APT_ANC_STOPPING);
                temp_listening_state = ANC_OFF_APT_OFF;
                audio_passthrough_disable();
                if (app_db.apt_eq_instance != NULL)
                {
                    eq_release(app_db.apt_eq_instance);
                    app_db.apt_eq_instance = NULL;
                }
            }
        }
    }
#endif
#if F_APP_ANC_SUPPORT
    else if (app_anc_is_anc_on_state(new_state))
    {
        if (!app_cfg_const.enable_auto_power_off_when_anc_apt_on)
        {
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ANC_APT_MODE);
        }

        if ((app_cfg_const.timer_auto_power_off_while_phone_connected_and_anc_apt_off) &&
            (app_bt_policy_get_b2s_connected_num() != 0))
        {
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_ANC_APT_MODE_WITH_PHONE_CONNECTED);
        }

        uint8_t new_anc_scenario = new_state - ANC_ON_SCENARIO_1_APT_OFF;
        uint8_t coff_idx = app_anc_get_coeff_idx(new_anc_scenario);

#if F_APP_APT_SUPPORT
        if (app_apt_is_apt_on_state(temp_listening_state))
        {
            app_listening_state_set(APT_ANC_STOPPING);
            temp_listening_state = ANC_OFF_APT_OFF;
            audio_passthrough_disable();
            if (app_db.apt_eq_instance != NULL)
            {
                eq_release(app_db.apt_eq_instance);
                app_db.apt_eq_instance = NULL;
            }
        }
        else
#endif
        {
            if (app_anc_is_anc_on_state(temp_listening_state))
            {
                if (app_listening_state_get() == APT_ANC_STARTED)
                {
                    app_listening_state_set(APT_ANC_STOPPING);
                    temp_listening_state = ANC_OFF_APT_OFF;
                    anc_disable();
                }
                else
                {
                    app_listening_state_set(APT_ANC_STARTING);
                    temp_listening_state = new_state;

                    /*
                        Line-in will be conflict with anc in mic selection, when line-in is running, anc FF mic
                        should not be used.
                    */
#if F_APP_LINEIN_SUPPORT
                    if (new_state == (T_ANC_APT_STATE)app_line_in_anc_line_in_mode_state_get())
                    {
                        audio_route_logic_io_disable(AUDIO_CATEGORY_ANC, AUDIO_ROUTE_LOGIC_EXTERNAL_MIC);
                    }
                    else
                    {
                        audio_route_logic_io_enable(AUDIO_CATEGORY_ANC, AUDIO_ROUTE_LOGIC_EXTERNAL_MIC);
                    }
#endif
                    anc_enable(coff_idx);
                }
            }
            else if (temp_listening_state == ANC_OFF_APT_OFF)
            {
                app_listening_state_set(APT_ANC_STARTING);
                temp_listening_state = new_state;

                /*
                    Line-in will be conflict with anc in mic selection, when line-in is running, anc FF mic
                    should not be used.
                */
#if F_APP_LINEIN_SUPPORT
                if (new_state == (T_ANC_APT_STATE)app_line_in_anc_line_in_mode_state_get())
                {
                    audio_route_logic_io_disable(AUDIO_CATEGORY_ANC, AUDIO_ROUTE_LOGIC_EXTERNAL_MIC);
                }
                else
                {
                    audio_route_logic_io_enable(AUDIO_CATEGORY_ANC, AUDIO_ROUTE_LOGIC_EXTERNAL_MIC);
                }
#endif
                anc_enable(coff_idx);
            }
        }
    }
#endif
}

void app_listening_state_machine(T_ANC_APT_EVENT event, bool is_need_push_tone,
                                 bool is_need_update_final_state)
{
    APP_PRINT_INFO4("app_listening_state_machine: event 0x%x is_need_push_tone %d, is_need_update_final_state %d, busy = %d",
                    event, is_need_push_tone, is_need_update_final_state, app_listening_is_busy());
    APP_PRINT_TRACE4("app_listening_state_machine final ptr: %d %d %d %d",
                     (app_db.final_listening_state == NULL) ? 0 :
                     (app_db.final_listening_state == &app_cfg_nv.anc_one_setting) ? 1 :
                     (app_db.final_listening_state == &app_cfg_nv.anc_both_setting) ? 2 :
                     (app_db.final_listening_state == &app_cfg_nv.anc_apt_state) ? 3 : 4,
                     app_cfg_nv.anc_one_setting,
                     app_cfg_nv.anc_both_setting,
                     app_cfg_nv.anc_apt_state);

    T_ANC_APT_STATE prev_state = (T_ANC_APT_STATE)(*app_db.final_listening_state);
    T_ANC_APT_STATE new_state = prev_state;

#if F_APP_ANC_SUPPORT
    T_ANC_FEATURE_MAP anc_feature_map = {.d32 = anc_tool_get_feature_map()};

    if (anc_feature_map.user_mode != ENABLE)
    {
        APP_PRINT_INFO0("app_listening_state_machine: not in user mode, ignore");
        return;
    }
#endif

    new_state = app_listening_mode_new_state_decide(prev_state, event);

    if (is_need_update_final_state)
    {
        *app_db.final_listening_state = new_state;
        app_listening_special_event_state_reset();
        need_update_final_listening_state = true;
    }
    else
    {
        //pending state should not change need_update_final_listening_state
        if (event != EVENT_APPLY_PENDING_STATE)
        {
            need_update_final_listening_state = false;
        }
    }

    APP_PRINT_TRACE2("app_listening_state_machine: blocked = 0x%X, listening_mode_does_not_depend_on_in_ear_status = %d",
                     blocked_listening_state, app_cfg_const.listening_mode_does_not_depend_on_in_ear_status);

    if (app_listening_is_busy())
    {
        //do simulated state switch, and will apply real effect after listening mode state is stable
        if (is_need_push_tone)
        {
            app_listening_mode_tone_play(new_state, true);
        }

        if (!is_need_update_final_state)
        {
            blocked_listening_state = new_state;
        }

        return;
    }
    else
    {
        if ((!is_need_update_final_state) && (event != EVENT_APPLY_PENDING_STATE))
        {
            //Reset blocked_listening_state if the state change is caused by special event
            blocked_listening_state = ANC_APT_STATE_TOTAL;
        }

        if (!app_listening_apply_state_check(new_state))
        {
            //If the new_state is not allowed to be applied, checks if it is a pending event or not
            if (event == EVENT_APPLY_PENDING_STATE)
            {
                //If it is a pending state, applies the blocked_listening_state
                if ((blocked_listening_state != ANC_APT_STATE_TOTAL) &&
                    (app_listening_apply_state_check(blocked_listening_state)))
                {
                    new_state = blocked_listening_state;
                    blocked_listening_state = ANC_APT_STATE_TOTAL;
                }
            }
            else
            {
                if ((is_need_push_tone) &&
                    (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_BOX)))
                {
                    app_listening_mode_tone_play(new_state, true);
                }
                return;
            }
        }

        if (is_need_push_tone)
        {
            if (event == EVENT_DELAY_APPLY_LISTENING_MODE)
            {
                app_listening_mode_tone_play(new_state, false);
            }
            else
            {
                app_listening_mode_tone_play(new_state, true);
            }
        }

        if (app_listening_check_delay_apply_time(new_state, prev_state, event, is_need_push_tone))
        {
            return;
        }

        app_db.current_listening_state = (uint8_t)new_state;

        app_listening_mode_apply_new_state(new_state);

        APP_PRINT_TRACE6("app_listening_state_machine: cfg: (%x -> %x), temp_state: (%x),\
current_state = %x, nv_state = 0x%X,0x%X",
                         prev_state, (T_ANC_APT_STATE)(*app_db.final_listening_state),
                         temp_listening_state, app_db.current_listening_state, app_cfg_nv.anc_both_setting,
                         app_cfg_nv.anc_one_setting);
    }
}

void app_listening_anc_apt_cfg_pointer_change(uint8_t *ptr)
{
    APP_PRINT_TRACE4("anc_apt_cfg_pointer_change: %d %d %d %d",
                     (ptr == NULL) ? 0 :
                     (ptr == &app_cfg_nv.anc_one_setting) ? 1 :
                     (ptr == &app_cfg_nv.anc_both_setting) ? 2 :
                     (ptr == &app_cfg_nv.anc_apt_state) ? 3 : 4,
                     app_cfg_nv.anc_one_setting,
                     app_cfg_nv.anc_both_setting,
                     app_cfg_nv.anc_apt_state);

    app_db.final_listening_state = ptr;
}

void app_listening_apply_when_power_on(void)
{
    APP_PRINT_TRACE4("app_listening_apply_when_power_on: listening_mode_power_on_status %d, anc_apt_state 0x%X local_in_case %d, cycle = %d",
                     app_cfg_const.listening_mode_power_on_status, app_cfg_nv.anc_both_setting,
                     app_db.local_in_case, app_cfg_nv.listening_mode_cycle);

    T_ANC_APT_STATE power_on_init_state = ANC_OFF_APT_OFF;
    bool change_init_state = false;

    switch (app_cfg_const.listening_mode_power_on_status)
    {
    case POWER_ON_LISTENING_MODE_FOLLOW_LAST:
        {
            /* listening mode cycle = APT <-> ANC, not allow initial state = ALL OFF */
            if (app_cfg_nv.listening_mode_cycle == 2)
            {
                if (LIGHT_SENSOR_ENABLED)
                {
                    if ((app_cfg_nv.anc_both_setting == ANC_OFF_APT_OFF) ||
                        (app_cfg_nv.anc_one_setting == ANC_OFF_APT_OFF))
                    {
#if F_APP_ANC_SUPPORT
                        app_anc_set_first_anc_sceanrio(&power_on_init_state);
                        change_init_state = true;
#endif
                    }
                }
            }
        }
        break;

#if F_APP_APT_SUPPORT
    case POWER_ON_LISTENING_MODE_APT_ON:
        {
            if (app_cfg_const.normal_apt_support)
            {
                power_on_init_state = ANC_OFF_NORMAL_APT_ON;
            }
            else if (app_cfg_const.llapt_support)
            {
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_both_setting))
                {
                    power_on_init_state = (T_ANC_APT_STATE)app_cfg_nv.anc_both_setting;
                }
                else
                {
                    app_apt_set_first_llapt_scenario(&power_on_init_state);
                }
            }
            change_init_state = true;
        }
        break;
#endif

#if F_APP_ANC_SUPPORT
    case POWER_ON_LISTENING_MODE_ANC_ON:
        {
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_both_setting))
            {
                power_on_init_state = (T_ANC_APT_STATE)app_cfg_nv.anc_both_setting;
            }
            else
            {
                app_anc_set_first_anc_sceanrio(&power_on_init_state);
            }
            change_init_state = true;
        }
        break;
#endif

    default:
        {
            power_on_init_state = ANC_OFF_APT_OFF;
            change_init_state = true;
        }
        break;
    }

#if (F_APP_AIRPLANE_SUPPORT == 1)
    if (app_cfg_const.airplane_mode_when_power_on)
    {
        if (app_cfg_const.keep_listening_mode_status_when_enter_airplane_mode ||
            app_cfg_const.set_listening_mode_status_all_off_when_enter_airplane_mode)
        {
            power_on_init_state = ANC_OFF_APT_OFF;
            change_init_state = true;
        }

        if (!app_cfg_const.apt_on_when_enter_airplane_mode)
        {
#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_both_setting))
            {
                power_on_init_state = (T_ANC_APT_STATE)app_cfg_nv.anc_both_setting;
            }
            else
            {
                app_anc_set_first_anc_sceanrio(&power_on_init_state);
            }

            change_init_state = true;
#endif
        }
        else
        {
#if F_APP_APT_SUPPORT
            if (app_cfg_const.normal_apt_support)
            {
                power_on_init_state = ANC_OFF_NORMAL_APT_ON;
            }
            else if (app_cfg_const.llapt_support)
            {
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_both_setting))
                {
                    power_on_init_state = (T_ANC_APT_STATE)app_cfg_nv.anc_both_setting;
                }
                else
                {
                    app_apt_set_first_llapt_scenario(&power_on_init_state);
                }
            }
            change_init_state = true;
#endif
        }
    }
#endif

    if (LIGHT_SENSOR_ENABLED &&
        !app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
    {
#if (F_APP_HEARABLE_SUPPORT == 1)
        *app_db.final_listening_state = power_on_init_state;
#else
        if ((app_db.remote_loc == BUD_LOC_IN_EAR) ^
            (app_db.local_loc == BUD_LOC_IN_EAR))   /* only one in ear */
        {
            app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_one_setting);
        }
        else if ((app_db.remote_loc == BUD_LOC_IN_EAR) &&
                 (app_db.local_loc == BUD_LOC_IN_EAR))   /* both in ear */
        {
            app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_both_setting);
        }
        else    /* both not in ear */
        {
            app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_apt_state);
        }
#endif
    }
    else
    {
#if (F_APP_AVP_INIT_SUPPORT == 1)
        app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_apt_state);
#else
        app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_both_setting);
#endif
    }

    if (change_init_state)
    {
#if (F_APP_AVP_INIT_SUPPORT == 1)
        APP_PRINT_TRACE1("app_listening_apply_when_power_on: not to config avp anc setting power_on_init_state %d",
                         power_on_init_state);
#else
        app_cfg_nv.anc_both_setting = power_on_init_state;

        if (LIGHT_SENSOR_ENABLED &&
            !app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
        {
#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_both_setting))
            {
                /* single ear mode not allow anc */
                if (app_cfg_const.llapt_support)
                {
#if F_APP_APT_SUPPORT
                    app_apt_set_first_llapt_scenario(&app_cfg_nv.anc_one_setting);
#endif
                }
                else
                {
                    app_cfg_nv.anc_one_setting = ANC_OFF_NORMAL_APT_ON;
                }
            }
            else
#endif
            {
                app_cfg_nv.anc_one_setting = power_on_init_state;
            }
        }
#endif
    }

#if F_APP_APT_SUPPORT
    if (app_apt_is_apt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
    {
        app_db.power_on_delay_open_apt_time = app_cfg_nv.time_delay_to_open_apt_when_power_on;
    }
#endif

    if (app_cfg_const.enable_rtk_charging_box)
    {
        if (app_db.local_in_case)
        {
            APP_PRINT_TRACE0("app_listening_apply_when_power_on: listening apply after case out");
            return;
        }
    }

    if (app_cfg_const.disallow_listening_mode_before_bud_connected ||
        app_cfg_const.disallow_listening_mode_before_phone_connected)
    {
        return;
    }

#if (F_APP_AIRPLANE_SUPPORT == 1)
    if (app_airplane_combine_key_power_on_get())
    {
        return;
    }
#endif

    if (LIGHT_SENSOR_ENABLED &&
        !app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
    {
        return;
    }

#if F_APP_APT_SUPPORT
    if (app_apt_is_apt_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
    {
        if (app_cfg_const.normal_apt_support)
        {
            app_listening_state_machine(EVENT_NORMAL_APT_ON, false, true);
        }
        else if (app_cfg_const.llapt_support)
        {
            app_listening_state_machine(LLAPT_STATE_TO_EVENT(*app_db.final_listening_state), false, true);
        }
    }
    else
#endif
    {
#if F_APP_ANC_SUPPORT
        if (app_anc_is_anc_on_state((T_ANC_APT_STATE)(*app_db.final_listening_state)))
        {
            app_listening_state_machine(ANC_STATE_TO_EVENT(*app_db.final_listening_state), false, true);
        }
        else
#endif
        {
            //all off, do nothing
        }
    }

}

void app_listening_apply_when_power_off(void)
{
    app_listening_stop_delay_apply_state();
    app_listening_special_event_state_reset();
    app_listening_state_machine(EVENT_ALL_OFF, false, false);
}

T_ANC_APT_STATE app_listening_get_temp_state(void)
{
    APP_PRINT_TRACE1("app_listening_get_temp_state = 0x%X", temp_listening_state);
    return temp_listening_state;
}

T_ANC_APT_STATE app_listening_get_blocked_state(void)
{
    APP_PRINT_TRACE1("app_listening_get_blocked_state = 0x%X", blocked_listening_state);
    return blocked_listening_state;
}

bool app_listening_final_state_valid(void)
{
    bool state_valid = false;

    /*if (app_db.delay_apply_listening_mode)
    {
        state_valid = false;
    }*/

    if (need_update_final_listening_state)
    {
        //Allow the mmi event to be triggered under special event
        state_valid = true;
        need_update_final_listening_state = false;
    }

    APP_PRINT_TRACE1("app_listening_final_state_valid valid = %x", state_valid);

    return state_valid;
}

void app_listening_special_event_trigger(T_LISTENING_SPECIAL_EVENT enter_special_event)
{
    APP_PRINT_TRACE2("app_listening_special_event_trigger %x, event_bitmap = 0x%04x",
                     enter_special_event,
                     listening_special_event_bitmap);

#if F_APP_ANC_SUPPORT
    bool recovered_anc_state_in_airplane_mode = false;
#endif

    switch (enter_special_event)
    {
    case LISTENING_MODE_SPECIAL_EVENT_SCO:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_SCO);

#if (F_APP_APT_SUPPORT | F_APP_ANC_SUPPORT)
            bool close_listening = false;

#if F_APP_APT_SUPPORT
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                if (!app_apt_open_condition_check())
                {
                    close_listening = true;
                }
            }
            else
#endif
            {
#if F_APP_ANC_SUPPORT
                if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                {
                    if (!app_anc_open_condition_check())
                    {
                        close_listening = true;
                    }
                }
#endif
            }

            if (close_listening)
            {
                if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
                {
                    app_listening_state_machine(EVENT_ALL_OFF, false, false);
                }
            }
            else
            {
                if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
                {
                    app_listening_special_event_state_set(APP_LISTENING_EVENT_SCO,
                                                          (T_ANC_APT_STATE)app_db.current_listening_state);
                }
            }
#endif
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_B2B_CONNECT:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_B2B_CONNECT);

            if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
            {
                app_listening_state_machine(EVENT_ALL_OFF, false, false);
            }
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_B2S_CONNECT:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_B2S_CONNECT);

            if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
            {
                app_listening_state_machine(EVENT_ALL_OFF, false, false);
            }
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_AIRPLANE:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_AIRPLANE);

            if (app_cfg_const.keep_listening_mode_status_when_enter_airplane_mode)
            {
                //No need to change listening mode status
                break;
            }


            if (app_cfg_const.set_listening_mode_status_all_off_when_enter_airplane_mode)
            {
                app_listening_special_event_state_set(APP_LISTENING_EVENT_AIRPLANE, ANC_OFF_APT_OFF);
#if F_APP_APT_SUPPORT
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                {
                    app_listening_state_machine(EVENT_APT_OFF, false, false);
                }
                else
#endif
                {
#if F_APP_ANC_SUPPORT
                    if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                    {
                        app_listening_state_machine(EVENT_ANC_OFF, false, false);
                    }
#endif
                }
                break;
            }

#if F_APP_ANC_SUPPORT
            if (!app_cfg_const.apt_on_when_enter_airplane_mode)
            {
                T_ANC_APT_STATE first_anc_scenario;

                if (app_anc_set_first_anc_sceanrio(&first_anc_scenario))
                {
                    app_listening_special_event_state_set(APP_LISTENING_EVENT_AIRPLANE, first_anc_scenario);

                    if ((T_ANC_APT_STATE)app_db.current_listening_state != first_anc_scenario)
                    {
                        app_listening_state_machine(ANC_STATE_TO_EVENT(first_anc_scenario), false, false);
                    }
                }
            }
            else
#endif
            {
#if F_APP_APT_SUPPORT
                if (!app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
                {

                    if (app_cfg_const.normal_apt_support)
                    {
                        app_listening_special_event_state_set(APP_LISTENING_EVENT_AIRPLANE, ANC_OFF_NORMAL_APT_ON);
                        app_listening_state_machine(EVENT_NORMAL_APT_ON, false, false);
                    }
                    else if (app_cfg_const.llapt_support)
                    {
                        T_ANC_APT_STATE first_anc_scenario;

                        if (app_apt_set_first_llapt_scenario(&first_anc_scenario))
                        {
                            app_listening_special_event_state_set(APP_LISTENING_EVENT_AIRPLANE,
                                                                  first_anc_scenario);
                            app_listening_state_machine(LLAPT_STATE_TO_EVENT(first_anc_scenario), false, false);
                        }
                    }
                }
#endif
            }
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_XIAOAI:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_XIAOAI);

#if F_APP_APT_SUPPORT
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                app_listening_state_machine(EVENT_ALL_OFF, false, false);
            }
            else
            {
                app_listening_special_event_state_set(APP_LISTENING_EVENT_XIAOAI,
                                                      (T_ANC_APT_STATE)app_db.current_listening_state);
            }
#endif
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_IN_BOX:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_BOX);

            if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
            {
                app_listening_state_machine(EVENT_ALL_OFF, false, false);
            }
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_GAMING_MODE:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_GAMING_MODE);

            if ((T_ANC_APT_STATE)app_db.current_listening_state != ANC_OFF_APT_OFF)
            {
                app_listening_state_machine(EVENT_ALL_OFF, false, false);
            }
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_A2DP:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_A2DP);

#if F_APP_APT_SUPPORT
            if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                app_listening_state_machine(EVENT_ALL_OFF, false, false);
            }
            else
            {
                app_listening_special_event_state_set(APP_LISTENING_EVENT_A2DP,
                                                      (T_ANC_APT_STATE)app_db.current_listening_state);
            }
#endif
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_DIRECT_APT_ON:
        {
            listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_DIRECT_APT_ON);

#if F_APP_APT_SUPPORT
            if (!app_apt_is_apt_on_state((T_ANC_APT_STATE)app_db.current_listening_state))
            {
                if (app_cfg_const.normal_apt_support)
                {
                    app_listening_special_event_state_set(APP_LISTENING_EVENT_DIRECT_APT_ON,
                                                          ANC_OFF_NORMAL_APT_ON);
                    app_listening_state_machine(EVENT_NORMAL_APT_ON, true, false);
                }
                else if (app_cfg_const.llapt_support)
                {
                    app_listening_special_event_state_set(APP_LISTENING_EVENT_DIRECT_APT_ON,
                                                          (T_ANC_APT_STATE)app_db.last_llapt_on_state);
                    app_listening_state_machine(LLAPT_STATE_TO_EVENT((T_ANC_APT_STATE)app_db.last_llapt_on_state), true,
                                                false);
                }
            }
#endif
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_SCO_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_SCO));

            app_listening_special_event_state_set(APP_LISTENING_EVENT_SCO, ANC_APT_STATE_TOTAL);
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_B2B_CONNECT_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_B2B_CONNECT));
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_B2S_CONNECT_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_B2S_CONNECT));
        }
        break;


    case LISTENING_MODE_SPECIAL_EVENT_AIRPLANE_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_AIRPLANE));

            app_listening_special_event_state_set(APP_LISTENING_EVENT_AIRPLANE, ANC_APT_STATE_TOTAL);
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_OUT_BOX:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_BOX));
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_IN_EAR:
    case LISTENING_MODE_SPECIAL_EVENT_OUT_EAR:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_EITHER_OUT_EAR));

#if F_APP_AIRPLANE_SUPPORT
            if (app_airplane_mode_get())
            {
                if (app_db.local_loc == BUD_LOC_IN_EAR)
                {
                    app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_both_setting);
                }
                else /* not in ear */
                {
                    listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_EITHER_OUT_EAR);
                    app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_apt_state);
                }

#if F_APP_ANC_SUPPORT
                T_ANC_APT_STATE recover_state = ANC_APT_STATE_TOTAL;

                app_listening_special_event_state_get(APP_LISTENING_EVENT_AIRPLANE, &recover_state);

                if ((recover_state != ANC_APT_STATE_TOTAL) &&
                    app_anc_is_anc_on_state(recover_state))
                {
                    if ((T_ANC_APT_STATE)app_db.current_listening_state != recover_state)
                    {
                        app_listening_state_machine(ANC_STATE_TO_EVENT(recover_state), false, false);
                    }

                    recovered_anc_state_in_airplane_mode = true;
                }
#endif
            }
            else
#endif
            {
                if ((app_db.remote_loc == BUD_LOC_IN_EAR) ^
                    (app_db.local_loc == BUD_LOC_IN_EAR))   /* only one in ear */
                {
                    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
                    {
                        listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_EITHER_OUT_EAR);
                    }
                    app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_one_setting);
                }
                else if ((app_db.remote_loc == BUD_LOC_IN_EAR) &&
                         (app_db.local_loc == BUD_LOC_IN_EAR))   /* both in ear */
                {
                    app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_both_setting);
                }
                else    /* both not in ear */
                {
                    listening_special_event_bitmap |= BIT(APP_LISTENING_EVENT_EITHER_OUT_EAR);
                    app_listening_anc_apt_cfg_pointer_change(&app_cfg_nv.anc_apt_state);
                }
            }
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_XIAOAI_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_XIAOAI));

            app_listening_special_event_state_set(APP_LISTENING_EVENT_XIAOAI, ANC_APT_STATE_TOTAL);
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_GAMING_MODE_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_GAMING_MODE));
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_A2DP_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_A2DP));

            app_listening_special_event_state_set(APP_LISTENING_EVENT_A2DP, ANC_APT_STATE_TOTAL);
        }
        break;

    case LISTENING_MODE_SPECIAL_EVENT_DIRECT_APT_ON_END:
        {
            listening_special_event_bitmap &= ~(BIT(APP_LISTENING_EVENT_DIRECT_APT_ON));

            app_listening_special_event_state_set(APP_LISTENING_EVENT_DIRECT_APT_ON, ANC_APT_STATE_TOTAL);
        }
        break;

    default:
        break;
    }

    if (enter_special_event == LISTENING_MODE_SPECIAL_EVENT_IN_EAR ||
        enter_special_event == LISTENING_MODE_SPECIAL_EVENT_OUT_EAR)
    {
#if (F_APP_SLIDE_SWITCH_LISTENING_MODE == 1)
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE &&
            enter_special_event == LISTENING_MODE_SPECIAL_EVENT_IN_EAR)
        {
            if (app_slide_switch_anc_apt_state_get() == APP_SLIDE_SWITCH_APT_ON)
            {
                T_ANC_APT_STATE first_apt_scenario;
#if F_APP_APT_SUPPORT
                app_apt_set_first_llapt_scenario(&first_apt_scenario);
#endif
                app_cfg_nv.anc_one_setting = (app_cfg_const.llapt_support == 1) ? first_apt_scenario :
                                             ANC_OFF_NORMAL_APT_ON;
            }
            else if (app_slide_switch_anc_apt_state_get() == APP_SLIDE_SWITCH_ANC_ON)
            {
#if F_APP_ANC_SUPPORT
                app_anc_set_first_anc_sceanrio(&app_cfg_nv.anc_one_setting);
#endif
            }
            else if (app_slide_switch_anc_apt_state_get() == APP_SLIDE_SWITCH_ALL_OFF)
            {
                if (app_slide_switch_between_anc_apt_support() ||
                    app_slide_switch_anc_on_off_support() ||
                    app_slide_switch_apt_on_off_support())
                {
                    /*
                        may fail to restore anc/apt state if there is also other way to open/close anc/apt.
                    */
                    app_cfg_nv.anc_one_setting = ANC_OFF_APT_OFF;
                }
            }
        }
#endif

        if (((T_ANC_APT_STATE)app_db.current_listening_state != (T_ANC_APT_STATE)
             (*app_db.final_listening_state))
#if F_APP_ANC_SUPPORT
            && !recovered_anc_state_in_airplane_mode
#endif
           )
        {
            app_listening_state_machine(EVENT_APPLY_FINAL_STATE, false, true);
        }
    }
    else if (enter_special_event >= LISTENING_MODE_SPECIAL_EVENT_SCO_END &&
             enter_special_event <= (LISTENING_MODE_SPECIAL_EVENT_SCO_END + LISTENING_EVENT_NUM - 1))
    {
        T_ANC_APT_STATE recover_state = ANC_APT_STATE_TOTAL;

        if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_BOX) ||
            listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_B2B_CONNECT) ||
            listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_B2S_CONNECT) ||
            listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_GAMING_MODE))
        {
            recover_state = ANC_OFF_APT_OFF;
        }
        else if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_AIRPLANE))
        {
            app_listening_special_event_state_get(APP_LISTENING_EVENT_AIRPLANE, &recover_state);
        }
        else if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_DIRECT_APT_ON))
        {
            app_listening_special_event_state_get(APP_LISTENING_EVENT_DIRECT_APT_ON, &recover_state);
        }
        else if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_SCO))
        {
            app_listening_special_event_state_get(APP_LISTENING_EVENT_SCO, &recover_state);
        }
        else if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_A2DP))
        {
            app_listening_special_event_state_get(APP_LISTENING_EVENT_A2DP, &recover_state);
        }
        else if (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_XIAOAI))
        {
            app_listening_special_event_state_get(APP_LISTENING_EVENT_XIAOAI, &recover_state);
        }

        if (recover_state != ANC_APT_STATE_TOTAL)
        {
            app_listening_assign_specific_state((T_ANC_APT_STATE)app_db.current_listening_state, recover_state,
                                                false, false);
        }
        else
        {
            //if ((T_ANC_APT_STATE)app_db.current_listening_state != (T_ANC_APT_STATE)(
            //        *app_db.final_listening_state))
            {
                if (enter_special_event == LISTENING_MODE_SPECIAL_EVENT_DIRECT_APT_ON_END)
                {
                    app_listening_state_machine(EVENT_APPLY_FINAL_STATE, true, false);
                }
                else
                {
                    app_listening_state_machine(EVENT_APPLY_FINAL_STATE, false, false);
                }
            }
        }
    }
}

bool app_listening_get_is_special_event_ongoing(void)
{
    APP_PRINT_TRACE1("app_listening_get_is_special_event_ongoing event_bitmap = %x",
                     listening_special_event_bitmap);

    if ((listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_SCO)) |
        (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_AIRPLANE)) |
        (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_XIAOAI)) |
        (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_A2DP)) |
        (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_DIRECT_APT_ON)))
    {
        return true;
    }

    return false;
}

void app_listening_set_disallow_trigger_again(void)
{
    APP_PRINT_TRACE1("app_listening_set_disallow_trigger_again time = %d",
                     app_cfg_const.disallow_trigger_listening_mode_again_time);

    gap_start_timer(&timer_handle_disallow_trigger_listening_mode,
                    "disallow_trigger_listening_mode", app_listening_mode_timer_queue_id,
                    APP_TIMER_DISALLOW_TRIGGER_LISTENING_MODE, 0,
                    app_cfg_const.disallow_trigger_listening_mode_again_time * 1000);
}

uint8_t app_listening_set_delay_apply_time(T_ANC_APT_STATE new_state, T_ANC_APT_STATE prev_state)
{
    uint8_t delay_apply_time = 0;

    if (new_state == ANC_OFF_APT_OFF)
    {
#if F_APP_APT_SUPPORT
        if (app_apt_is_llapt_on_state(prev_state))
        {
            delay_apply_time = app_cfg_const.time_delay_to_close_llapt;
        }
        else
#endif
        {
#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state(prev_state))
            {
                delay_apply_time = app_cfg_const.time_delay_to_close_anc;
            }
#endif
        }
    }
#if F_APP_APT_SUPPORT
    else if (new_state == ANC_OFF_NORMAL_APT_ON)
    {
        if ((app_cfg_nv.time_delay_to_open_apt_when_power_on) &&
            (app_db.power_on_delay_open_apt_time))
        {
            delay_apply_time = app_db.power_on_delay_open_apt_time;
        }
        else
        {
#if F_APP_ANC_SUPPORT
            if (app_anc_is_anc_on_state(prev_state))
            {
                delay_apply_time = app_cfg_const.time_delay_to_close_anc;
            }
#endif
        }
    }
    else if (app_apt_is_llapt_on_state(new_state))
    {
        if ((app_cfg_nv.time_delay_to_open_apt_when_power_on) &&
            (app_db.power_on_delay_open_apt_time))
        {
            delay_apply_time = app_db.power_on_delay_open_apt_time;
        }
        else
        {
            if (prev_state == ANC_OFF_APT_OFF)
            {
                delay_apply_time = app_cfg_const.time_delay_to_open_llapt;
            }
            else if (app_apt_is_llapt_on_state(prev_state))
            {
                if (app_cfg_const.time_delay_to_close_llapt > app_cfg_const.time_delay_to_open_llapt)
                {
                    delay_apply_time = app_cfg_const.time_delay_to_close_llapt;
                }
                else
                {
                    delay_apply_time = app_cfg_const.time_delay_to_open_llapt;
                }
            }
#if F_APP_ANC_SUPPORT
            else if (app_anc_is_anc_on_state(prev_state))
            {
                if (app_cfg_const.time_delay_to_close_anc > app_cfg_const.time_delay_to_open_llapt)
                {
                    delay_apply_time = app_cfg_const.time_delay_to_close_anc;
                }
                else
                {
                    delay_apply_time = app_cfg_const.time_delay_to_open_llapt;
                }
            }
#endif
        }
    }
#endif
#if F_APP_ANC_SUPPORT
    else if (app_anc_is_anc_on_state(new_state))
    {
        if (prev_state == ANC_OFF_APT_OFF)
        {
            delay_apply_time = app_cfg_const.time_delay_to_open_anc;
        }
#if F_APP_APT_SUPPORT
        else if (prev_state == ANC_OFF_NORMAL_APT_ON)
        {
            delay_apply_time = app_cfg_const.time_delay_to_open_anc;
        }
        else if (app_apt_is_llapt_on_state(prev_state))
        {
            if (app_cfg_const.time_delay_to_close_llapt > app_cfg_const.time_delay_to_open_anc)
            {
                delay_apply_time = app_cfg_const.time_delay_to_close_llapt;
            }
            else
            {
                delay_apply_time = app_cfg_const.time_delay_to_open_anc;
            }
        }
#endif
        else if (app_anc_is_anc_on_state(prev_state))
        {
            if (app_cfg_const.time_delay_to_close_anc > app_cfg_const.time_delay_to_open_anc)
            {
                delay_apply_time = app_cfg_const.time_delay_to_close_anc;
            }
            else
            {
                delay_apply_time = app_cfg_const.time_delay_to_open_anc;
            }
        }
    }
#endif

    APP_PRINT_TRACE1("app_listening_set_delay_apply_time = %d", delay_apply_time);

    return delay_apply_time;
}

bool app_listening_b2b_connected(void)
{
    bool ret = false;

    if (app_bt_policy_get_b2b_connected())
    {
        ret = true;
    }

    return ret;
}

bool app_listening_b2s_connected(void)
{
    bool ret = false;

    if (app_db.b2s_connected_num > 0)
    {
        ret = true;
    }

    return ret;
}

void app_listening_judge_conn_disc_evnet(T_APPLY_LISTENING_MODE_EVENT event)
{
    APP_PRINT_TRACE3("app_listening_judge_conn_disc_evnet: event = %x, b2b_conn = %x, b2s_conn = %x",
                     event,
                     app_listening_b2b_connected(), app_listening_b2s_connected());

    if (event == APPLY_LISTENING_MODE_SRC_CONNECTED)
    {
        if (app_listening_b2s_connected())
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_B2S_CONNECT_END);
        }
    }
    else if (event == APPLY_LISTENING_MODE_BUD_CONNECTED)
    {
        if (app_listening_b2b_connected())
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_B2B_CONNECT_END);
        }
    }
    else if (event == APPLY_LISTENING_MODE_SRC_DISCONNECTED)
    {
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_B2S_CONNECT);
    }
    else if (event == APPLY_LISTENING_MODE_BUD_DISCONNECTED)
    {
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_B2B_CONNECT);
    }
}

void app_listening_judge_sco_event(T_APPLY_LISTENING_MODE_EVENT event)
{
    APP_PRINT_TRACE3("app_listening_judge_sco_event event = 0x%02x, call_status = 0x%02x, voice_track_handle = %p",
                     event, app_hfp_get_call_status(), voice_track_handle);

    if (event == APPLY_LISTENING_MODE_CALL_NOT_IDLE)
    {
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_SCO);
    }
    else if (event == APPLY_LISTENING_MODE_CALL_IDLE)
    {
        if (voice_track_handle == NULL)
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_SCO_END);
        }
    }
    else if (event == APPLY_LISTENING_MODE_VOICE_TRACE_RELEASE)
    {
        if (app_hfp_get_call_status() == BT_HFP_CALL_IDLE)
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_SCO_END);
        }
    }
}

void app_listening_judge_a2dp_event(T_APPLY_LISTENING_MODE_EVENT event)
{
    APP_PRINT_TRACE3("app_listening_judge_a2dp_event event = 0x%02x, AVRCP_play_status = 0x%02x, %d",
                     event, app_db.avrcp_play_status, app_cfg_const.apt_auto_on_off_while_music_playing);

    if (event == APPLY_LISTENING_MODE_AVRCP_PLAY_STATUS_CHANGE)
    {
#if F_APP_APT_SUPPORT
        if ((app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING) &&
            ((listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_A2DP)) == 0))
        {
            if (app_cfg_const.apt_auto_on_off_while_music_playing)
            {
                app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_A2DP);
            }
        }
        else if (((app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_PAUSED) ||
                  (app_db.avrcp_play_status == BT_AVRCP_PLAY_STATUS_STOPPED)) &&
                 (listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_A2DP)))
        {
            app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_A2DP_END);
        }
#endif
    }
}

void app_listening_handle_remote_state_sync(void *msg)
{
    T_LISTENING_MODE_RELAY_MSG *rev_msg = (T_LISTENING_MODE_RELAY_MSG *)msg;
    T_ANC_APT_STATE start_state = (T_ANC_APT_STATE)(*app_db.final_listening_state);
    T_ANC_APT_STATE dest_state;

    APP_PRINT_TRACE4("app_listening_handle_remote_state_sync remote = %x, %x, %x, final = %x",
                     rev_msg->anc_one_setting, rev_msg->anc_both_setting,
                     rev_msg->anc_apt_state, *app_db.final_listening_state);


    app_cfg_nv.anc_one_setting = rev_msg->anc_one_setting;
    app_cfg_nv.anc_both_setting = rev_msg->anc_both_setting;
    app_cfg_nv.anc_apt_state = rev_msg->anc_apt_state;

    if (rev_msg->apt_eq_idx != app_cfg_nv.apt_eq_idx)
    {
        app_cfg_nv.apt_eq_idx = rev_msg->apt_eq_idx;
    }

    dest_state = (T_ANC_APT_STATE)(*app_db.final_listening_state);
    *app_db.final_listening_state = start_state;

#if F_APP_APT_SUPPORT
    if (app_db.current_listening_state == dest_state)
    {
        /* send sec status for apt eq */
        if (app_apt_is_apt_on_state(dest_state) && app_cfg_const.normal_apt_support)
        {
            app_listening_report_status_change(app_db.current_listening_state);
        }
    }
#endif

    app_listening_assign_specific_state(start_state, dest_state, rev_msg->is_need_tone, true);
}

void app_listening_direct_apt_on_off_handling(void)
{
#if F_APP_APT_SUPPORT
    if ((listening_special_event_bitmap & BIT(APP_LISTENING_EVENT_DIRECT_APT_ON)) &&
        (app_apt_is_apt_on_state((T_ANC_APT_STATE)(app_db.current_listening_state))))
    {
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_DIRECT_APT_ON_END);
    }
    else
    {
        if (app_listening_get_is_special_event_ongoing())
        {
            //MMI triggered, change final to current
            *app_db.final_listening_state = app_db.current_listening_state;
        }
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_DIRECT_APT_ON);
    }
#endif
}

bool app_listening_report_secondary_state_condition(void)
{
    bool ret = false;

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (app_db.local_loc == BUD_LOC_IN_CASE &&
                app_db.remote_loc != BUD_LOC_IN_CASE)
            {
                //Primary in case, Secondary out case
                //In this condition, DUT report status of secondary to phone
                ret = true;
            }
        }

        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (app_db.local_loc != BUD_LOC_IN_CASE &&
                app_db.remote_loc == BUD_LOC_IN_CASE)
            {
                //Primary in case, Secondary out case
                //In this condition, DUT report status of secondary to phone
                ret = true;
            }
        }
    }

    if (ret)
    {
        APP_PRINT_TRACE1("app_listening_report_secondary_state_condition %x", ret);
    }

    return ret;
}

uint8_t app_listening_count_1bits(uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x0000003f;
}

static void app_listening_cmd_postpone_reset(void)
{
    anc_apt_cmd_postpone_data.reason = ANC_APT_CMD_POSTPONE_NONE;
    anc_apt_cmd_postpone_data.pos_anc_apt_cmd = 0;
    anc_apt_cmd_postpone_data.pos_param_len = 0;
    anc_apt_cmd_postpone_data.pos_path = 0;
    anc_apt_cmd_postpone_data.pos_app_idx = 0;

    if (anc_apt_cmd_postpone_data.pos_param_ptr)
    {
        free(anc_apt_cmd_postpone_data.pos_param_ptr);
        anc_apt_cmd_postpone_data.pos_param_ptr = NULL;
    }
}

T_ANC_APT_CMD_POSTPONE_REASON app_listening_cmd_postpone_reason_get(void)
{
    return anc_apt_cmd_postpone_data.reason;
}

void app_listening_cmd_postpone_stash(uint16_t anc_apt_cmd, uint8_t *param_ptr,
                                      uint16_t param_len, uint8_t path, uint8_t app_idx, T_ANC_APT_CMD_POSTPONE_REASON postpone_reason)
{
    APP_PRINT_TRACE2("app_listening_cmd_postpone_stash ori = %x, stash = %x",
                     anc_apt_cmd_postpone_data.reason, postpone_reason);

    if (anc_apt_cmd_postpone_data.reason != ANC_APT_CMD_POSTPONE_NONE)
    {
        return;
    }

    if (postpone_reason == ANC_APT_CMD_POSTPONE_ANC_WAIT_MEDIA_BUFFER ||
        postpone_reason == ANC_APT_CMD_POSTPONE_LLAPT_WAIT_MEDIA_BUFFER)
    {
        T_APP_BR_LINK *p_link;

        p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);

        if (p_link && p_link->a2dp_track_handle)
        {
            //guarantee audio track at started state, it will call back AUDIO_EVENT_TRACK_STATE_CHANGED event
            audio_track_stop(p_link->a2dp_track_handle);
        }
    }

    anc_apt_cmd_postpone_data.reason = postpone_reason;
    anc_apt_cmd_postpone_data.pos_anc_apt_cmd = anc_apt_cmd;
    anc_apt_cmd_postpone_data.pos_param_ptr = (uint8_t *)malloc(param_len);
    anc_apt_cmd_postpone_data.pos_param_len = param_len;
    anc_apt_cmd_postpone_data.pos_path = path;
    anc_apt_cmd_postpone_data.pos_app_idx = app_idx;
    memcpy(anc_apt_cmd_postpone_data.pos_param_ptr, param_ptr, param_len);
}

void app_listening_cmd_postpone_pop(void)
{
    APP_PRINT_TRACE1("app_listening_cmd_postpone_pop = %x", anc_apt_cmd_postpone_data.reason);

#if (F_APP_ANC_SUPPORT | F_APP_APT_SUPPORT)
    T_APP_BR_LINK *p_link;
    p_link = app_find_br_link(app_db.br_link[app_get_active_a2dp_idx()].bd_addr);
#endif

    switch (anc_apt_cmd_postpone_data.reason)
    {
#if F_APP_ANC_SUPPORT
    case ANC_APT_CMD_POSTPONE_ANC_WAIT_MEDIA_BUFFER:
        {
            app_anc_cmd_handle(anc_apt_cmd_postpone_data.pos_anc_apt_cmd,
                               anc_apt_cmd_postpone_data.pos_param_ptr,
                               anc_apt_cmd_postpone_data.pos_param_len,
                               anc_apt_cmd_postpone_data.pos_path, anc_apt_cmd_postpone_data.pos_app_idx);

            if (p_link && p_link->a2dp_track_handle)
            {
                audio_track_start(p_link->a2dp_track_handle);
            }
        }
        break;
#endif

#if F_APP_APT_SUPPORT
    case ANC_APT_CMD_POSTPONE_LLAPT_WAIT_MEDIA_BUFFER:
        {
            app_apt_cmd_handle(anc_apt_cmd_postpone_data.pos_anc_apt_cmd,
                               anc_apt_cmd_postpone_data.pos_param_ptr,
                               anc_apt_cmd_postpone_data.pos_param_len,
                               anc_apt_cmd_postpone_data.pos_path, anc_apt_cmd_postpone_data.pos_app_idx);

            if (p_link && p_link->a2dp_track_handle)
            {
                audio_track_start(p_link->a2dp_track_handle);
            }
        }
        break;
#endif

#if F_APP_ANC_SUPPORT
    case ANC_APT_CMD_POSTPONE_WAIT_USER_MODE_CLOSE:
        {
            app_anc_cmd_handle(anc_apt_cmd_postpone_data.pos_anc_apt_cmd,
                               anc_apt_cmd_postpone_data.pos_param_ptr,
                               anc_apt_cmd_postpone_data.pos_param_len,
                               anc_apt_cmd_postpone_data.pos_path, anc_apt_cmd_postpone_data.pos_app_idx);
        }
        break;
#endif

    default:
        break;
    }

    app_listening_cmd_postpone_reset();
}

#if NEW_FORMAT_LISTENING_CMD_REPORT
void app_listening_report_status_change(uint8_t state)
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (!app_listening_report_secondary_state_condition())
            {
                app_listening_report(EVENT_LISTENING_STATE_STATUS, &state, 1);
            }
        }

        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_LISTENING_MODE,
                                                APP_REMOTE_MSG_SECONDARY_LISTENING_STATUS,
                                                &state, 1);
        }
    }
    else
    {
        app_listening_report(EVENT_LISTENING_STATE_STATUS, &state, 1);
    }
}
#endif

bool app_listening_mode_mmi(uint8_t action)
{
    bool ret = false;

    if ((action == MMI_AUDIO_APT) ||
        (action == MMI_ANC_ON_OFF) ||
        (action == MMI_LISTENING_MODE_CYCLE) ||
        (action == MMI_ANC_CYCLE) ||
        (action == MMI_LLAPT_CYCLE))
    {
        ret = true;
    }

    return ret;
}

void app_listening_rtk_in_case(void)
{
    APP_PRINT_TRACE0("app_listening_rtk_in_case");

    app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_IN_BOX);
}

void app_listening_rtk_out_case(void)
{
    APP_PRINT_TRACE0("app_listening_rtk_out_case");

    app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_OUT_BOX);
}

void app_listening_rtk_in_ear(void)
{
    APP_PRINT_TRACE0("app_listening_rtk_in_ear");

    if (LIGHT_SENSOR_ENABLED &&
        !app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
    {
#if (F_APP_HEARABLE_SUPPORT == 1)
        app_ha_listening_turn_on_off(SENSOR_LD_IN_EAR);
#else
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_IN_EAR);
#endif
    }
}

void app_listening_rtk_out_ear(void)
{
    APP_PRINT_TRACE0("app_listening_rtk_out_ear");

    if (LIGHT_SENSOR_ENABLED &&
        !app_cfg_const.listening_mode_does_not_depend_on_in_ear_status)
    {
#if (F_APP_HEARABLE_SUPPORT == 1)
        app_ha_listening_turn_on_off(SENSOR_LD_OUT_EAR);
#else
        app_listening_special_event_trigger(LISTENING_MODE_SPECIAL_EVENT_OUT_EAR);
#endif
    }
}

void app_listening_anc_apt_relay(bool need_tone)
{
    T_LISTENING_MODE_RELAY_MSG send_msg;

    send_msg.anc_one_setting = app_cfg_nv.anc_one_setting;
    send_msg.anc_both_setting = app_cfg_nv.anc_both_setting;
    send_msg.anc_apt_state = app_cfg_nv.anc_apt_state;
    send_msg.apt_eq_idx = app_cfg_nv.apt_eq_idx;
    send_msg.is_need_tone = need_tone;

    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_LISTENING_MODE,
                                        APP_REMOTE_MSG_LISTENING_ENGAGE_SYNC,
                                        (uint8_t *)&send_msg, sizeof(T_LISTENING_MODE_RELAY_MSG));
}

void app_listening_cmd_pre_handle(uint16_t listening_cmd, uint8_t *param_ptr, uint16_t param_len,
                                  uint8_t path, uint8_t app_idx, uint8_t *ack_pkt)
{
    /*
        switch (listening_cmd)
        {
        // reserved for some SKIP_OPERATE condition check
        default:
            break;
        }
    */
    app_report_event(path, EVENT_ACK, app_idx, ack_pkt, 3);
    app_listening_cmd_handle(listening_cmd, param_ptr, param_len, path, app_idx);

    /*
    SKIP_OPERATE:
        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
    */
}

void app_listening_cmd_handle(uint16_t listening_cmd, uint8_t *param_ptr, uint16_t param_len,
                              uint8_t path, uint8_t app_idx)
{
    bool only_report_status = false;
    uint8_t cmd_status = LISTENING_CMD_STATUS_FAILED;

    APP_PRINT_TRACE5("app_listening_cmd_handle: listening_cmd 0x%04X, param_ptr 0x%02X 0x%02X 0x%02X 0x%02X",
                     listening_cmd, param_ptr[0], param_ptr[1], param_ptr[2], param_ptr[3]);

    switch (listening_cmd)
    {
#if NEW_FORMAT_LISTENING_CMD_REPORT
    case CMD_LISTENING_STATE_SET:
        {
            uint8_t listening_state_type;
#if (F_APP_ANC_SUPPORT | F_APP_APT_SUPPORT)
            uint8_t listening_state_index;
#endif
            uint8_t relay_ptr[3];

            listening_state_type = param_ptr[0];
#if (F_APP_ANC_SUPPORT | F_APP_APT_SUPPORT)
            listening_state_index = param_ptr[1];
#endif

#if F_APP_ANC_SUPPORT
            if (listening_state_type == LISTENING_STATE_ANC)
            {
                if (app_anc_open_condition_check() == false)
                {
                    cmd_status = LISTENING_CMD_STATUS_FAILED;
                }
                else if (app_cfg_nv.anc_selected_list & BIT(listening_state_index))
                {
                    // example:
                    // activate count is 5, parameter 0~4 will be valid
                    relay_ptr[0] = ANC_ON_SCENARIO_1_APT_OFF + listening_state_index;

                    cmd_status = LISTENING_CMD_STATUS_SUCCESS;
                }
                else
                {
                    cmd_status = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
#endif

#if F_APP_APT_SUPPORT
            if (listening_state_type == LISTENING_STATE_NORMAL_APT)
            {
                if (!app_cfg_const.normal_apt_support)
                {
                    cmd_status = CMD_SET_STATUS_PARAMETER_ERROR;
                }
                else if (app_apt_open_condition_check() == false)
                {
                    cmd_status = LISTENING_CMD_STATUS_FAILED;
                }
                else
                {
                    relay_ptr[0] = ANC_OFF_NORMAL_APT_ON;
                    cmd_status = LISTENING_CMD_STATUS_SUCCESS;
                }
            }

            if (listening_state_type == LISTENING_STATE_LLAPT)
            {
                if (!app_cfg_const.llapt_support)
                {
                    cmd_status = CMD_SET_STATUS_PARAMETER_ERROR;
                }
                else if (app_cfg_const.llapt_support && app_apt_open_condition_check() == false)
                {
                    cmd_status = LISTENING_CMD_STATUS_FAILED;
                }
                else if (app_cfg_nv.llapt_selected_list & BIT(listening_state_index))
                {
                    // example:
                    // activate count is 5, parameter 0~4 will be valid
                    relay_ptr[0] = ANC_OFF_LLAPT_ON_SCENARIO_1 + listening_state_index;
                    cmd_status = LISTENING_CMD_STATUS_SUCCESS;
                }
                else
                {
                    cmd_status = CMD_SET_STATUS_PARAMETER_ERROR;
                }
            }
#endif

            if (listening_state_type == LISTENING_STATE_ALL_OFF)
            {
                if (app_listening_is_allow_all_off_condition_check())
                {
                    relay_ptr[0] = ANC_OFF_APT_OFF;
                    cmd_status = LISTENING_CMD_STATUS_SUCCESS;
                }
            }

            if (cmd_status == LISTENING_CMD_STATUS_SUCCESS)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    relay_ptr[1] = true;
                    relay_ptr[2] = true;

                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_LISTENING_MODE,
                                                       APP_REMOTE_MSG_LISTENING_SOURCE_CONTROL,
                                                       relay_ptr, 3, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                }
                else
                {
                    app_listening_assign_specific_state((T_ANC_APT_STATE)*app_db.final_listening_state,
                                                        (T_ANC_APT_STATE)relay_ptr[0], true, true);
                }
            }

            app_listening_report(EVENT_LISTENING_STATE_SET, &cmd_status, 1);
        }
        break;

    case CMD_LISTENING_STATE_STATUS:
        {
            app_listening_report(EVENT_LISTENING_STATE_STATUS, &app_db.current_listening_state, 1);
        }
        break;
#endif

    default:
        {
            only_report_status = true;
            cmd_status = LISTENING_CMD_STATUS_UNKNOW_CMD;
        }
        break;
    }

    if (only_report_status == true)
    {
        app_report_event(path, listening_cmd, app_idx, &cmd_status, 1);
    }
}

uint16_t app_listening_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_LISTENING_MODE_CYCLE_SYNC:
        {
            payload_len = 1;
            msg_ptr = (uint8_t *)&app_cfg_nv.offset_listening_mode_cycle;
            skip = false;
        }
        break;

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_LISTENING_MODE, payload_len, msg_ptr, skip,
                              total);
}

static void app_listening_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                      T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_listening_parse_cback: msg = 0x%x, status = %x", msg_type, status);

    switch (msg_type)
    {
    case APP_REMOTE_MSG_LISTENING_ENGAGE_SYNC:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_listening_handle_remote_state_sync(buf);
            }
        }
        break;

    case APP_REMOTE_MSG_LISTENING_SOURCE_CONTROL:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED)
            {
                T_ANC_APT_STATE listening_state = (T_ANC_APT_STATE)buf[0];
                bool need_tone = (bool)buf[1];
                bool update_final = (bool)buf[2];

                app_listening_assign_specific_state((T_ANC_APT_STATE)*app_db.final_listening_state,
                                                    (T_ANC_APT_STATE)listening_state, need_tone, update_final);
            }
        }
        break;

#if NEW_FORMAT_LISTENING_CMD_REPORT
    case APP_REMOTE_MSG_SECONDARY_LISTENING_STATUS:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_listening_report_secondary_state_condition())
                {
                    app_listening_report(EVENT_LISTENING_STATE_STATUS, buf, len);
                }

#if F_APP_APT_SUPPORT
                if (app_apt_is_apt_on_state((T_ANC_APT_STATE)buf[0]))
                {
                    if ((app_cfg_const.normal_apt_support) && (eq_utils_num_get(MIC_SW_EQ, APT_MODE) != 0))
                    {
                        app_report_apt_index_info(EQ_INDEX_REPORT_BY_RWS_CONNECTED);
                    }
                }
#endif
            }
        }
        break;
#endif

    case APP_REMOTE_MSG_LISTENING_MODE_CYCLE_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t *p_info = (uint8_t *)buf;
                    app_cfg_nv.listening_mode_cycle = (p_info[0] & 0x03);
                    app_cfg_store(&app_cfg_nv.offset_listening_mode_cycle, 4);
                }
            }
        }
        break;

    default:
        break;
    }
}

static void app_listening_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;

    switch (event_type)
    {
    case SYS_EVENT_POWER_ON:
        {

#if (F_APP_AVP_INIT_SUPPORT == 1)
            //rsv for avp
#else
            app_listening_rtk_out_ear();
#endif

            if (app_cfg_const.disallow_listening_mode_before_bud_connected)
            {
                app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_BUD_DISCONNECTED);
            }

            if (app_cfg_const.disallow_listening_mode_before_phone_connected)
            {
                app_listening_judge_conn_disc_evnet(APPLY_LISTENING_MODE_SRC_DISCONNECTED);
            }
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_listening_apply_when_power_off();
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
        APP_PRINT_INFO1("app_listening_dm_cback: event_type 0x%04x", event_type);
    }
}

void app_listening_mode_init(void)
{
    app_db.final_listening_state = &app_cfg_nv.anc_both_setting;  //pointer initial
    app_db.last_listening_state = ANC_OFF_APT_OFF;
    app_listening_special_event_state_reset();
    audio_mgr_cback_register(app_listening_audio_cback);
    sys_mgr_cback_register(app_listening_dm_cback);
    bt_mgr_cback_register(app_listening_bt_cback);
    gap_reg_timer_cb(app_listening_mode_timeout_cb, &app_listening_mode_timer_queue_id);
    app_relay_cback_register(app_listening_relay_cback, app_listening_parse_cback,
                             APP_MODULE_TYPE_LISTENING_MODE, APP_REMOTE_MSG_LISTENING_MODE_TOTAL);
}

#endif
