#if C_QCY_APP_FEATURE_SUPPORT

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "board.h"
#include "gap_timer.h"
#include "app_qcy_app.h"
#include "app_qcy_control.h"
#include "app_key_process.h"
#include "app_charger.h"
#include "app_hfp.h"
#include "app_led.h"
#include "app_cfg.h"
#include "app_mmi.h"
#include "app_main.h"
#include "app_relay.h"
#include "app_roleswap.h"
#include "voice_prompt.h"
#include "version.h"
#include "bt_bond.h"
#include "app_mmi.h"
#include "app_relay.h"
#include "audio_track.h"
#include "app_listening_mode.h"
#include "app_roleswap.h"
#include "eq_utils.h"
#include "app_eq.h"
#include "app_multilink.h"
#include "app_audio_passthrough.h"
#if (ANC_ENABLE == 1)
#include "app_anc.h"
#endif

extern T_SERVER_ID qcy_ctrl_srv_id;
extern uint8_t apk_eq;
bool app_ctrl_led_blinking_fg = false;

//We should keep conn_id. It would be used when doing notify.
T_NOTIFY_CONN_ID app_qcy_conn_id = {0xFF, 0xFF};

void get_battery_status(T_QCY_BATTERY_STATUS *battery_status)
{
    bool left_charging, right_charging, case_charging;
    uint8_t left_bat, right_bat;
    uint8_t case_bat;
    bool left_in_case = false, right_in_case = false;

    memset(battery_status, 0, sizeof(T_QCY_BATTERY_STATUS));

    app_qcy_get_bat_info(&left_charging, &left_bat, &right_charging, &right_bat,
                         &case_charging, &case_bat);

    // set left battery status
    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        if (app_db.local_loc == BUD_LOC_IN_CASE)
        {
            left_in_case = true;
        }
    }
    else
    {
        if (app_db.remote_loc == BUD_LOC_IN_CASE)
        {
            left_in_case = true;
        }
    }
    battery_status->left_bud_bat_volume |= left_in_case ? BIT(7) : 0;
    battery_status->left_bud_bat_volume |= left_bat;

    // set right battery status
    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT)
    {
        if (app_db.local_loc == BUD_LOC_IN_CASE)
        {
            right_in_case = true;
        }
    }
    else
    {
        if (app_db.remote_loc == BUD_LOC_IN_CASE)
        {
            right_in_case = true;
        }
    }
    battery_status->right_bud_bat_volume |= right_in_case ? BIT(7) : 0;
    battery_status->right_bud_bat_volume |= right_bat;

    // set charger box battery status
    battery_status->chargerbox_bat_volume |= case_charging ? BIT(7) : 0;

    // set box bat volume to 0 when two buds are out of case.
    if ((app_db.local_loc == BUD_LOC_IN_CASE) ||
        (app_db.remote_loc == BUD_LOC_IN_CASE))
    {
        battery_status->chargerbox_bat_volume |= case_bat;
    }

    APP_PRINT_TRACE1("get_battery_status: %b", TRACE_BINARY(sizeof(T_QCY_BATTERY_STATUS),
                                                            battery_status));
}

void get_feature_info(T_QCY_FEATURE_INFO *feature_info)
{
    qcy_feature_info_init(feature_info);

    /* get in ear detect and anc level */
    feature_info->in_ear_detect.status = app_cfg_nv.light_sensor_enable ? 1 : 2;
    feature_info->anc_level.level = 0;

    /* get dac gain level */
    uint8_t active_a2dp_idx = app_get_active_a2dp_idx();
    uint8_t pair_idx;
    uint8_t gain_level = 0;

    if (bt_bond_index_get(app_db.br_link[active_a2dp_idx].bd_addr, &pair_idx) == false)
    {
        APP_PRINT_ERROR0("get_feature_info: get idx failed");
    }
    else
    {
        gain_level = app_cfg_nv.audio_gain_level[pair_idx];
    }

    feature_info->vol_level.left_vol_level = gain_level;
    feature_info->vol_level.right_vol_level = gain_level;
    feature_info->vol_level.max_vol_level = 15;

    /* get gaming mode */
    feature_info->gaming_mode.status = app_db.gaming_mode ? 1 : 2;

    /* get listening mode */
    uint8_t listening_mode;
    if (app_apt_is_apt_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_apt_state))
    {
        listening_mode = LISTENING_MODE_APT;
    }
#if (ANC_ENABLE == 1)
    else if (app_anc_is_anc_on_state((T_ANC_APT_STATE)app_cfg_nv.anc_apt_state))
    {
        listening_mode = LISTENING_MODE_ANC;
    }
#endif
    else
    {
        listening_mode = LISTENING_MODE_ALL_OFF;
    }
    feature_info->listening_mode.status = listening_mode;

    /* get sleep mode */
    feature_info->sleep_mode.status = app_db.qcy_app_db.qcy_sleep_mode;

    APP_PRINT_TRACE1("get_feature_info: %b", TRACE_BINARY(sizeof(T_QCY_FEATURE_INFO), feature_info));
}

T_MMI_ACTION qcy_key_to_mmi(T_QCY_CTRL_KEY_ACTION key_ctrl)
{
    T_MMI_ACTION ret = MMI_NULL;

    switch (key_ctrl)
    {
    case QCY_KEY_ACTION_PLAY_PAUSE:
        {
            ret = MMI_AV_PLAY_PAUSE;
        }
        break;

    case QCY_KEY_ACTION_AV_FWD:
        {
            ret = MMI_AV_FWD;
        }
        break;

    case QCY_KEY_ACTION_AV_BWD:
        {
            ret = MMI_AV_BWD;
        }
        break;

    case QCY_KEY_ACTION_VAD:
        {
            ret = MMI_HF_INITIATE_VOICE_DIAL;
        }
        break;

    case QCY_KEY_ACTION_VOL_UP:
        {
            ret = MMI_DEV_SPK_VOL_UP;
        }
        break;

    case QCY_KEY_ACTION_VOL_DOWN:
        {
            ret = MMI_DEV_SPK_VOL_DOWN;
        }
        break;

    case QCY_KEY_ACTION_GAMING_MODE:
        {
            ret = MMI_DEV_GAMING_MODE_SWITCH;
        }
        break;

    case QCY_KEY_ACTION_APT:
        {
            ret = MMI_AUDIO_APT;
        }
        break;

    case QCY_KEY_ACTION_LISTENING_MODE_CYCLE:
        {
            ret = MMI_LISTENING_MODE_CYCLE;
        }
        break;

    case QCY_KEY_ACTION_NULL:
    default:
        {
            ret = MMI_NULL;
        }
        break;
    }

    return ret;
}

static T_QCY_CTRL_KEY_ACTION mmi_to_qcy_key(uint8_t mmi_action)
{
    T_QCY_CTRL_KEY_ACTION ret = QCY_KEY_ACTION_NULL;

    switch (mmi_action)
    {
    case MMI_AV_PLAY_PAUSE:
        {
            ret = QCY_KEY_ACTION_PLAY_PAUSE;
        }
        break;

    case MMI_AV_FWD:
        {
            ret = QCY_KEY_ACTION_AV_FWD;
        }
        break;

    case MMI_AV_BWD:
        {
            ret = QCY_KEY_ACTION_AV_BWD;
        }
        break;

    case MMI_HF_INITIATE_VOICE_DIAL:
        {
            ret = QCY_KEY_ACTION_VAD;
        }
        break;

    case MMI_DEV_SPK_VOL_UP:
        {
            ret = QCY_KEY_ACTION_VOL_UP;
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
            ret = QCY_KEY_ACTION_VOL_DOWN;
        }
        break;

    case MMI_DEV_GAMING_MODE_SWITCH:
        {
            ret = QCY_KEY_ACTION_GAMING_MODE;
        }
        break;

    case MMI_AUDIO_APT:
        {
            ret = QCY_KEY_ACTION_APT;
        }
        break;

    case MMI_LISTENING_MODE_CYCLE:
        {
            ret = QCY_KEY_ACTION_LISTENING_MODE_CYCLE;
        }
        break;

    case MMI_NULL:
    default:
        {
            ret = QCY_KEY_ACTION_NULL;
        }
        break;
    }

    APP_PRINT_TRACE2("mmi_to_qcy_key %02x ret %d", mmi_action, ret);

    return ret;
}

void app_qcy_reset_key_setting(void)
{
    uint8_t i;
    T_QCY_KEY_SETTING *local_key = &app_cfg_nv.qcy_key_info.local_key;
    T_QCY_KEY_SETTING *remote_key = &app_cfg_nv.qcy_key_info.remote_key;
    T_BT_HFP_CALL_STATUS call_status = BT_HFP_CALL_IDLE;
    uint8_t idx = key_search_index(KEY0_MASK);

    memset(local_key, 0, sizeof(T_QCY_KEY_SETTING));
    memset(remote_key, 0, sizeof(T_QCY_KEY_SETTING));

    local_key->single_click_action = mmi_to_qcy_key(app_cfg_const.key_table[0][call_status][idx]);
    local_key->long_press_action = mmi_to_qcy_key(app_cfg_const.key_table[1][call_status][idx]);

    for (i = 0; i < HYBRID_KEY_NUM; i++)
    {
        if (app_cfg_const.hybrid_key_mapping[i][0] == KEY0_MASK)
        {
            switch (app_cfg_const.hybrid_key_mapping[i][1])
            {
            case HYBRID_KEY_2_CLICK:
                {
                    local_key->double_click_action = mmi_to_qcy_key(app_cfg_const.hybrid_key_table[call_status][i]);
                }
                break;

            case HYBRID_KEY_3_CLICK:
                {
                    local_key->triple_click_action = mmi_to_qcy_key(app_cfg_const.hybrid_key_table[call_status][i]);
                }
                break;

            case HYBRID_KEY_4_CLICK:
                {
                    local_key->four_click_action = mmi_to_qcy_key(app_cfg_const.hybrid_key_table[call_status][i]);
                }
                break;
            }
        }
    }

    APP_PRINT_TRACE5("app_qcy_reset_key_setting: %d %d %d %d %d",
                     local_key->single_click_action,
                     local_key->long_press_action,
                     local_key->double_click_action,
                     local_key->triple_click_action,
                     local_key->four_click_action);

}

void get_click_cfg(T_QCY_CLICK_CFG *click_cfg)
{
    T_QCY_KEY_SETTING *local_key = &app_cfg_nv.qcy_key_info.local_key;
    T_QCY_KEY_SETTING *remote_key = &app_cfg_nv.qcy_key_info.remote_key;

    qcy_click_cfg_init(click_cfg);

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_RIGHT) // R_channel is local
    {
        click_cfg->right_bud_sigle_click_action = local_key->single_click_action;
        click_cfg->right_bud_double_click_action = local_key->double_click_action;
        click_cfg->right_bud_triple_click_action = local_key->triple_click_action;
        click_cfg->right_bud_four_click_action = local_key->four_click_action;
        click_cfg->right_bud_long_press_action = local_key->long_press_action;

        click_cfg->left_bud_sigle_click_action = remote_key->single_click_action;
        click_cfg->left_bud_double_click_action = remote_key->double_click_action;
        click_cfg->left_bud_triple_click_action = remote_key->triple_click_action;
        click_cfg->left_bud_four_click_action = remote_key->four_click_action;
        click_cfg->left_bud_long_press_action = remote_key->long_press_action;
    }
    else
    {
        click_cfg->left_bud_sigle_click_action = local_key->single_click_action;
        click_cfg->left_bud_double_click_action = local_key->double_click_action;
        click_cfg->left_bud_triple_click_action = local_key->triple_click_action;
        click_cfg->left_bud_four_click_action = local_key->four_click_action;
        click_cfg->left_bud_long_press_action = local_key->long_press_action;

        click_cfg->right_bud_sigle_click_action = remote_key->single_click_action;
        click_cfg->right_bud_double_click_action = remote_key->double_click_action;
        click_cfg->right_bud_triple_click_action = remote_key->triple_click_action;
        click_cfg->right_bud_four_click_action = remote_key->four_click_action;
        click_cfg->right_bud_long_press_action = remote_key->long_press_action;
    }
}

bool app_ctrl_reset_ble_default(void)
{
    /* reset qcy's related setting */
    app_qcy_cfg_reset();

    /* ask remote bud to reset qcy's setting */
    T_QCY_APP_RELAY_MSG msg;

    remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_QCY_CFG_RESET, &msg,
                           sizeof(T_QCY_APP_RELAY_MSG), false);

    return true;
}

bool app_ctrl_clear_phone_record(void)
{
    /*todo*/
    return true;
}

bool app_ctrl_reset_default(void)
{
    uint8_t action = MMI_DEV_FACTORY_RESET_BY_SPP;

    app_mmi_handle_action(action);
    remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_MMI_SYNC, &action, 1, false);

    return true;
}

bool app_ctrl_music_play(T_APP_CTRL_MUSIC_PLAY music_ctrl)
{
    uint8_t play_status = app_db.avrcp_play_status;
    T_APP_BUD_STREAM_STATE stream_state = app_roleswap_get_bud_stream_state();

    APP_PRINT_TRACE3("app_ctrl_music_play: music_ctrl %d play_status %d stream_state %d",
                     music_ctrl, play_status, stream_state);

    switch (music_ctrl)
    {
    case MUSIC_PLAY_PAUSE:
        {
            app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
        }
        break;

    case MUSIC_PLAY:
        {
            if (play_status == BT_AVRCP_PLAY_STATUS_PAUSED || play_status == BT_AVRCP_PLAY_STATUS_STOPPED)
            {
                app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
            }
        }
        break;

    case MUSIC_PAUSE:
        {
            if ((play_status == BT_AVRCP_PLAY_STATUS_PLAYING) &&
                (stream_state == BUD_STREAM_STATE_AUDIO))
            {
                app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
            }
        }
        break;

    case MUSIC_BWD:
        {
            app_mmi_handle_action(MMI_AV_BWD);
        }
        break;

    case MUSIC_FWD:
        {
            app_mmi_handle_action(MMI_AV_FWD);
        }
        break;
    }

    return true;
}


void app_qcy_led_blinking(T_APP_CTRL_LED led_ctrl)
{
    if (led_ctrl == LED_START_BLINKING)
    {
        /* start a timer to blink for 2 min */
        gap_start_timer(&timer_handle_app_ctrl_led_blinking, "app_ctrl_led_blinking",
                        app_qcy_timer_queue_id,
                        QCY_APP_TIMER_LED_BLINKING, 0, 120000);

#if 0   /* fix me: need to implemet led all blinking function */
        led_set_mode(LED_MODE_ALL_BLINKING);
#endif
        /* disallow others to set led mode until we finish */
        app_ctrl_led_blinking_fg = true;
    }
    else
    {
        gap_stop_timer(&timer_handle_app_ctrl_led_blinking);
        app_ctrl_led_blinking_fg = false;

#if 0   /* fix me: need to implemet led all blinking function */
        /* recover current state's led */
        led_recover_mode(0);
#endif
    }

}

bool app_ctrl_led_blinking(T_APP_CTRL_LED led_ctrl)
{
    APP_PRINT_TRACE1("app_ctrl_led_blinking: led_ctrl %d", led_ctrl);
    app_qcy_led_blinking(led_ctrl);
    remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_QCY_APP_LED_SYNC,
                           &led_ctrl, sizeof(T_APP_CTRL_LED), false);
    return true;
}

bool app_ctrl_in_ear_detection(T_APP_CTRL_IN_EAR_DETECTION in_ear_ctrl)
{
    uint8_t mmi = MMI_LIGHT_SENSOR_ON_OFF;

    if (in_ear_ctrl == IN_EAR_DETECTION_ON)
    {
        if (app_cfg_nv.light_sensor_enable == false)
        {
            remote_sync_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_MMI_SYNC, &mmi, 1,
                                  REMOTE_TIMER_HIGH_PRECISION,
                                  0, true);
        }
    }
    else
    {
        if (app_cfg_nv.light_sensor_enable == true)
        {
            remote_sync_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_MMI_SYNC, &mmi, 1,
                                  REMOTE_TIMER_HIGH_PRECISION,
                                  0, true);
        }
    }

    return true;
}

bool app_ctrl_anc(uint8_t anc_level)
{
    /*todo*/
    return true;
}

bool app_ctrl_volume(uint8_t left_ear_volume, uint8_t right_ear_volume)
{
    /* NOTE: We assume left and right volume should be equal!! */

    bool ret = true;
    uint8_t active_idx;
    uint8_t pair_idx;
    T_AUDIO_STREAM_TYPE volume_type;

    volume_type = AUDIO_STREAM_TYPE_PLAYBACK;
    active_idx = app_get_active_a2dp_idx();

    if (bt_bond_index_get(app_db.br_link[active_idx].bd_addr, &pair_idx) == false)
    {
        APP_PRINT_ERROR0("app_ctrl_volume: find active a2dp pair index fail");
        ret = false;
        goto ERROR;
    }

    /* Exception: if vol level is larger than 0xF, set level to max level.*/
    if (left_ear_volume > 0x0F)
    {
        left_ear_volume = 0x0F;
    }

    app_cfg_nv.audio_gain_level[pair_idx] = left_ear_volume;

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        uint8_t cmd_ptr[8];

        memcpy(&cmd_ptr[0], app_db.br_link[active_idx].bd_addr, 6);
        cmd_ptr[6] = volume_type;
        cmd_ptr[7] = left_ear_volume;

        ret = remote_sync_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_A2DP_VOLUME_SYNC, cmd_ptr,
                                    sizeof(cmd_ptr),
                                    REMOTE_TIMER_HIGH_PRECISION, 0, false);
    }
    else
    {
        ret = audio_track_volume_out_set(app_db.br_link[active_idx].a2dp_track_handle, left_ear_volume);
    }

ERROR:
    APP_PRINT_TRACE2("app_ctrl_volume: %d %d ", ret, left_ear_volume);
    return ret;
}

bool app_ctrl_gaming_mode(T_APP_CTRL_GAMING_MODE gaming_mode_ctrl)
{
    if (gaming_mode_ctrl == GAMING_MODE_ON)
    {
        if (app_db.gaming_mode == false)
        {
            app_mmi_handle_action(MMI_DEV_GAMING_MODE_SWITCH);
        }
    }
    else
    {
        if (app_db.gaming_mode == true)
        {
            app_mmi_handle_action(MMI_DEV_GAMING_MODE_SWITCH);
        }
    }

    return true;
}

bool app_ctrl_touch_sensor(uint8_t touch_sensor_level)
{
    return true;
}

bool app_ctrl_sleep_mode(uint8_t sleep_mode)
{
    bool new_state = 0;

    if (sleep_mode == 1)
    {
        new_state = 1;
    }
    else
    {
        new_state = 0;
    }

    APP_PRINT_TRACE2("app_ctrl_sleep_mode: %d %d", new_state, app_db.qcy_app_db.qcy_sleep_mode);

    if (new_state != app_db.qcy_app_db.qcy_sleep_mode)
    {
        app_db.qcy_app_db.qcy_sleep_mode = new_state;

        app_qcy_info_sync(false);
    }

    return true;
}

bool app_ctrl_listening_mode(T_APP_CTRL_LISTENING_MODE listening_mode_ctrl)
{
    uint8_t new_state;
    T_ANC_APT_EVENT evt;

    switch (listening_mode_ctrl)
    {
    case LISTENING_MODE_ANC:
        {
            evt = EVENT_ANC_ON_SCENARIO_1;
            new_state = ANC_ON_SCENARIO_1_APT_OFF;
        }
        break;

    case LISTENING_MODE_ALL_OFF:
        {
            evt = EVENT_ALL_OFF;
            new_state = ANC_OFF_APT_OFF;
        }
        break;

    case LISTENING_MODE_APT:
        {
            evt = EVENT_NORMAL_APT_ON;
            new_state = ANC_OFF_NORMAL_APT_ON;
        }
        break;
    }

    app_listening_state_machine(evt, true, true);

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        T_LISTENING_MODE_RELAY_MSG send_msg;

        send_msg.listening_state = new_state;
        send_msg.apt_volume_level = app_cfg_nv.apt_volume_out_level;
        send_msg.apt_eq_idx = app_cfg_nv.apt_eq_idx;
        send_msg.is_need_tone = true;

        remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_LISTENING_MODE_SYNC, &send_msg,
                               sizeof(T_LISTENING_MODE_RELAY_MSG), false);
    }

    return true;
}

void app_ctrl_get_remote_key_info(T_QCY_KEY_INFO *remote_key_info)
{
    T_QCY_KEY_INFO *key_info = &app_cfg_nv.qcy_key_info;

    APP_PRINT_TRACE1("app_ctrl_get_remote_key_info: %b",
                     TRACE_BINARY(sizeof(T_QCY_KEY_INFO), (uint8_t *)remote_key_info));

    /* valid key setting from remote (to prevent bud not get remote's key setting before first engaged) */
    if (!(remote_key_info->remote_key.single_click_action == QCY_KEY_ACTION_NULL &&
          remote_key_info->remote_key.double_click_action == QCY_KEY_ACTION_NULL &&
          remote_key_info->remote_key.triple_click_action == QCY_KEY_ACTION_NULL &&
          remote_key_info->remote_key.four_click_action == QCY_KEY_ACTION_NULL))
    {
        memcpy(&key_info->local_key, &remote_key_info->remote_key, sizeof(T_QCY_KEY_SETTING));
    }

    if (!(remote_key_info->local_key.single_click_action == QCY_KEY_ACTION_NULL &&
          remote_key_info->local_key.double_click_action == QCY_KEY_ACTION_NULL &&
          remote_key_info->local_key.triple_click_action == QCY_KEY_ACTION_NULL &&
          remote_key_info->local_key.four_click_action == QCY_KEY_ACTION_NULL))
    {
        memcpy(&key_info->remote_key, &remote_key_info->local_key, sizeof(T_QCY_KEY_SETTING));
    }
}

bool app_ctrl_key_setting(T_APP_BUD_SIDE side, T_KEY_CFG *key_cfg)
{
    bool is_local_setting = false;

    if (app_cfg_const.bud_side == DEVICE_BUD_SIDE_LEFT)
    {
        if (side == LEFT_BUD_SETTING)
        {
            is_local_setting = true;
        }
    }
    else
    {
        if (side == RIGHT_BUD_SETTING)
        {
            is_local_setting = true;
        }
    }

    APP_PRINT_TRACE4("app_ctrl_key_setting: bud_side %d side %d event %d action %d",
                     app_cfg_const.bud_side,
                     side, key_cfg->key_event, key_cfg->key_action);

    T_QCY_KEY_SETTING *key_setting;

    if (is_local_setting)
    {
        key_setting = &app_cfg_nv.qcy_key_info.local_key;
    }
    else
    {
        key_setting = &app_cfg_nv.qcy_key_info.remote_key;
    }

    switch (key_cfg->key_event)
    {
    case QCY_KEY_EVENT_SINGLE_CLICK:
        {
            key_setting->single_click_action = key_cfg->key_action;
        }
        break;

    case QCY_KEY_EVENT_DOUBLE_CLICK:
        {
            key_setting->double_click_action = key_cfg->key_action;
        }
        break;

    case QCY_KEY_EVENT_TRIPLE_CLICK:
        {
            key_setting->triple_click_action = key_cfg->key_action;
        }
        break;

    case QCY_KEY_EVENT_FOUR_CLICK:
        {
            key_setting->four_click_action = key_cfg->key_action;
        }
        break;

    case QCY_KEY_EVENT_LONG_PRESS:
        {
            key_setting->long_press_action = key_cfg->key_action;
        }
        break;
    }

    return true;
}

bool app_qcy_set_language(uint8_t language)
{
    if ((language < VOICE_PROMPT_LANGUAGE_INVALID) &&
        (voice_prompt_supported_languages_get() & BIT(language)))
    {
        if ((voice_prompt_language_set((T_VOICE_PROMPT_LANGUAGE_ID)language) == true))
        {
            APP_PRINT_TRACE1("app_qcy_set_language: set language %d", language);
            app_cfg_nv.voice_prompt_language = language;
            if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
            {
                app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_VP_LANGUAGE);
            }
            return true;
        }
    }
    return false;
}

bool app_ctrl_language(T_APP_CTRL_LANGUAGE language_ctrl)
{
    uint8_t language = VOICE_PROMPT_LANGUAGE_INVALID;
    switch (language_ctrl)
    {
    case LANGUAGE_TONE_ONLY:
        {
            /* Reset to default language */
            language = app_cfg_const.voice_prompt_language;
        }
        break;

    case LANGUAGE_MANDARIN:
        {
            language = VOICE_PROMPT_LANGUAGE_CHINESE;
        }
        break;

    case LANGUAGE_ENGLISH:
        {
            language = VOICE_PROMPT_LANGUAGE_ENGLISH;
        }
        break;

    case LANGUAGE_FRENCH:
        {
            language = VOICE_PROMPT_LANGUAGE_FRENCH;
        }
        break;

    default:
        break;
    }

    bool ret = app_qcy_set_language(language);
    return ret;
}

void init_eq(T_EQ_INFO *eq, uint8_t *gain)
{
    uint32_t default_freq[QCY_APP_EQ_STAGE_NUM] = {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    int i = 0;

    for (i = 0; i < QCY_APP_EQ_STAGE_NUM; i++)
    {
        eq[i].gain = (int8_t)gain[i];
        eq[i].q = EQ_DEFAULT_Q_VALUE;
        eq[i].freq = default_freq[i];
    }
}

double db_to_linear(double value)
{
    float linear = 0;

    linear = pow(10.0, value / (20.0));

    return linear;
}

int signed_quant(double in, int total_bits, int frac_bits)
{
    double tmp = 0;

    if ((total_bits == 32) && (frac_bits == 30)) /* handle this case separately for speed up */
    {
        if (in >= 2)
        {
            tmp = pow(2.0, 31) - 1.0;
        }
        else if (in < -2)
        {
            tmp = -1 * pow(2.0, 31);
        }
        else
        {
            tmp = round(in * pow(2, 30));
        }
    }
    else if ((total_bits == 24) && (frac_bits == 12)) /* handle this case separately for speed up */
    {
        double pow_2_23th = pow(2, 23);
        tmp = round(in * pow(2, 12));

        if (tmp >= pow_2_23th)
        {
            tmp = pow_2_23th - 1.0;
        }
        else if (tmp < -1 * pow_2_23th)
        {
            tmp = -1 * pow_2_23th;
        }
    }
    else
    {
        if (frac_bits >= total_bits)
        {
            frac_bits = total_bits - 1;
        }

        if (total_bits > 32)
        {
            total_bits = 32;
        }

        tmp = in * pow(2.0, frac_bits);
        tmp = round(tmp);

        if (tmp >= pow(2.0, total_bits - 1))
        {
            tmp = pow(2.0, total_bits - 1) - 1.0;
        }

        else
        {
            if (tmp <= (-1) * pow(2.0, total_bits - 1))
            {
                tmp = pow(2.0, total_bits - 1) * (-1);
            }
        }
    }

    return ((int)tmp);
}


void peak_filter(T_EQ_FILTER_COEF *iir_filter, double gain, double fc, double q, double fs)
{
    double t0 = 0;
    double beta = 0;
    int i = 0;
    double v = 1, max = 0, abs_val = 0;
    int cnt = 0;

    t0 = 2 * PI * fc / fs;
    iir_filter->order = 2;
    if (gain >= 1)
    {
        beta = t0 / (2 * q);
    }

    else
    {
        beta = t0 / (2 * gain * q);
    }

    iir_filter->den[2] = -0.5 * (1 - beta) / (1 + beta);
    iir_filter->den[1] = (0.5 - iir_filter->den[2]) * cos(t0);
    iir_filter->num[0] = (gain - 1) * (0.25 + 0.5 * iir_filter->den[2]) + 0.5;
    iir_filter->num[1] = -iir_filter->den[1];
    iir_filter->num[2] = -(gain - 1) * (0.25 + 0.5 * iir_filter->den[2]) - iir_filter->den[2];

    for (i = 0; i <= 2; i++)
    {
        iir_filter->num[i] *= 2;
        iir_filter->den[i] *= -2;
    }

    iir_filter->den[0] = 1;

    for (i = 0; i <= 2; i++)
    {
        abs_val = abs(iir_filter->num[i]);

        if (abs_val >= 2)
        {
            cnt++;
        }

        if (abs_val >= max)
        {
            max = abs_val;
        }
    }

    if (cnt != 0)
    {
        v = max;
        for (i = 0; i < 3; i++)
        {
            iir_filter->num[i] = iir_filter->num[i] / v;
        }
    }

    iir_filter->gain = v;
}

void filter_coef_fix(T_EQ_FILTER_COEF *in, double *pow_2_bits)
{
    int i = 0;
    double tmp = 0;

    for (i = 1; i <= in->order; i++)
    {
        tmp = in->den[i] * (*pow_2_bits);
        tmp = round(tmp);
        in->den[i] = tmp / (*pow_2_bits);
        tmp = in->num[i] * (*pow_2_bits);
        tmp = round(tmp);
        in->num[i] = tmp / (*pow_2_bits);
    }
}

bool switch_eq_index(T_APP_CTRL_EQ index)
{
    bool ret = true;

    uint8_t eq_num = eq_utils_num_get(SPK_SW_EQ, app_db.gaming_mode);
    uint32_t *eq_map = eq_utils_map_get(SPK_SW_EQ, app_db.gaming_mode);
    uint8_t target_eq_idx = eq_num;
    uint32_t i;

    for (i = 0; i < eq_num; i++)
    {
        /* from qcy eq 1 ~ 6 mapping to our eq 0 ~ 5 */
        if (eq_map[i] == index - 1)
        {
            target_eq_idx = i;
            break;
        }
    }

    /* if support the eq */
    if (target_eq_idx != eq_num)
    {
        if (app_eq_index_set(SPK_SW_EQ, app_db.gaming_mode, target_eq_idx) == true)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
            {
                app_eq_play_audio_eq_tone();
            }

            uint16_t eq_len = eq_utils_param_get(SPK_SW_EQ, app_db.gaming_mode, app_cfg_nv.eq_idx,
                                                 app_db.dynamic_eq_buf, app_db.dynamic_eq_len);
            app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_EQ_INDEX);
            remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_EQ_DATA, app_db.dynamic_eq_buf,
                                   eq_len, false);
        }
    }

    APP_PRINT_TRACE2("switch_eq_index: %d, %d", index, target_eq_idx);

    return ret;
}

void calculate_eq_data(uint8_t *gain, void *result, uint32_t sample_rate)
{
    T_EQ_FILTER_COEF filter[QCY_APP_EQ_STAGE_NUM] = {0};
    T_EQ_INFO eq[QCY_APP_EQ_STAGE_NUM] = {0};
    T_EQ_COEF *eq_data = NULL;
    double global_gain = 1;
    uint8_t i;
    double pow_2_bits = pow(2.0, BIT_VALID - BIT_MARGIN);

    init_eq(eq, gain);
    eq_data = (T_EQ_COEF *)result;

    for (i = 0; i < QCY_APP_EQ_STAGE_NUM; i++)
    {
        peak_filter(&filter[i], db_to_linear(eq[i].gain), eq[i].freq, eq[i].q, sample_rate);
        filter_coef_fix(&filter[i], &pow_2_bits);
        global_gain *= filter[i].gain;
    }

    eq_data->gain = signed_quant(global_gain, 24, 12);
    eq_data->guadBit = 2;
    eq_data->stage = QCY_APP_EQ_STAGE_NUM;

    for (i = 0; i < QCY_APP_EQ_STAGE_NUM; i++)
    {
        eq_data->coef[i][0] = signed_quant(filter[i].num[0], 32, 30);
        eq_data->coef[i][1] = signed_quant(filter[i].num[1], 32, 30);
        eq_data->coef[i][2] = signed_quant(filter[i].num[2], 32, 30);
        eq_data->coef[i][3] = signed_quant(filter[i].den[1], 32, 30);
        eq_data->coef[i][4] = signed_quant(filter[i].den[2], 32, 30);
    }
}

void set_qcy_user_defined_eq(uint8_t *gain)
{
    T_QCY_EQ_GAIN_DATA *tmp = &app_cfg_nv.qcy_eq_gain;

    tmp->gain_freq_32 = gain[0] & 0xf;
    tmp->gain_first_bit_freq_32 = (gain[0] & 0xf0) ? 1 : 0;

    tmp->gain_freq_64 = gain[1] & 0xf;
    tmp->gain_first_bit_freq_64 = (gain[1] & 0xf0) ? 1 : 0;

    tmp->gain_freq_125 = gain[2] & 0xf;
    tmp->gain_first_bit_freq_125 = (gain[2] & 0xf0) ? 1 : 0;

    tmp->gain_freq_250 = gain[3] & 0xf;
    tmp->gain_first_bit_freq_250 = (gain[3] & 0xf0) ? 1 : 0;

    tmp->gain_freq_500 = gain[4] & 0xf;
    tmp->gain_first_bit_freq_500 = (gain[4] & 0xf0) ? 1 : 0;

    tmp->gain_freq_1000 = gain[5] & 0xf;
    tmp->gain_first_bit_freq_1000 = (gain[5] & 0xf0) ? 1 : 0;

    tmp->gain_freq_2000 = gain[6] & 0xf;
    tmp->gain_first_bit_freq_2000 = (gain[6] & 0xf0) ? 1 : 0;

    tmp->gain_freq_4000 = gain[7] & 0xf;
    tmp->gain_first_bit_freq_4000 = (gain[7] & 0xf0) ? 1 : 0;

    tmp->gain_freq_8000 = gain[8] & 0xf;
    tmp->gain_first_bit_freq_8000 = (gain[8] & 0xf0) ? 1 : 0;

    tmp->gain_freq_16000 = gain[9] & 0xf;
    tmp->gain_first_bit_freq_16000 = (gain[9] & 0xf0) ? 1 : 0;
}

uint8_t combine_gain(uint8_t first_bit, uint8_t gain)
{
    uint8_t ret = gain;

    if (first_bit)
    {
        ret |= 0xf0;
    }

    return ret;
}

void get_qcy_user_defined_eq(int8_t *gain)
{
    T_QCY_EQ_GAIN_DATA *tmp = &app_cfg_nv.qcy_eq_gain;

    gain[0] = combine_gain(tmp->gain_first_bit_freq_32, tmp->gain_freq_32);
    gain[1] = combine_gain(tmp->gain_first_bit_freq_64, tmp->gain_freq_64);
    gain[2] = combine_gain(tmp->gain_first_bit_freq_125, tmp->gain_freq_125);
    gain[3] = combine_gain(tmp->gain_first_bit_freq_250, tmp->gain_freq_250);
    gain[4] = combine_gain(tmp->gain_first_bit_freq_500, tmp->gain_freq_500);
    gain[5] = combine_gain(tmp->gain_first_bit_freq_1000, tmp->gain_freq_1000);
    gain[6] = combine_gain(tmp->gain_first_bit_freq_2000, tmp->gain_freq_2000);
    gain[7] = combine_gain(tmp->gain_first_bit_freq_4000, tmp->gain_freq_4000);
    gain[8] = combine_gain(tmp->gain_first_bit_freq_8000, tmp->gain_freq_8000);
    gain[9] = combine_gain(tmp->gain_first_bit_freq_16000, tmp->gain_freq_16000);
}

void app_qcy_reset_user_defined_eq(void)
{
    uint8_t gain[QCY_APP_EQ_STAGE_NUM] = {0};

    /* set all qcy user defined eq gain to 0 */
    set_qcy_user_defined_eq(gain);
}

bool app_ctrl_set_eq(T_APP_CTRL_EQ eq_ctrl, uint8_t *data)
{
    bool ret = true;
    uint8_t paras[EQ_PARAM_SIZE] = {0};
    uint32_t sample_rate = 0;

    if (eq_ctrl == EQ_SELF_DEFINED)
    {
        uint8_t app_idx = app_get_active_a2dp_idx();
        bool is_play_eq_tone = false;

        if (app_cfg_nv.eq_idx != EQ_MAX_INDEX)  /* use eq 9 as qcy's realtime eq */
        {
            /* change eq, needs to play tone */
            is_play_eq_tone = true;
        }

        sample_rate = (app_eq_sample_rate_get(app_idx) == SAMPLE_RATE_44K) ? 44100 : 48000;

        /* SDK header */
        paras[0] = 0;
        paras[1] = 0;
        paras[2] = 0;
        paras[3] = 1;

        calculate_eq_data(data, &paras[4], sample_rate);

        /* use eq 9 as qcy's realtime eq */
        if (app_eq_param_set(EQ_MAX_INDEX, paras, EQ_PARAM_SIZE) == true)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                if (is_play_eq_tone)
                {
                    app_eq_play_audio_eq_tone();
                }

                app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_EQ_INDEX);
                remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_EQ_DATA, paras,
                                       EQ_PARAM_SIZE, false);
            }
        }
    }
    else
    {
        switch_eq_index(eq_ctrl);
    }

    set_qcy_user_defined_eq(data);
    remote_async_msg_relay(app_db.relay_handle, APP_REMOTE_MSG_QCY_EQ_GAIN_SYNC,
                           &app_cfg_nv.qcy_eq_gain,
                           sizeof(T_QCY_EQ_GAIN_DATA), false);
    APP_PRINT_TRACE3("app_ctrl_set_eq: %x %x %b", eq_ctrl, app_cfg_nv.eq_idx,
                     TRACE_BINARY(QCY_APP_EQ_STAGE_NUM, data));

    return ret;
}

bool app_ctrl_get_eq(T_APP_CTRL_EQ eq_ctrl, uint8_t *data, uint8_t len)
{
    int8_t gain[QCY_APP_EQ_STAGE_NUM + 1] = {0};

    if (app_cfg_nv.eq_idx == EQ_MAX_INDEX)
    {
        /* use eq 9 as qcy's realtime eq */
        gain[0] = EQ_SELF_DEFINED;
    }
    else
    {
        gain[0] = app_cfg_nv.eq_idx + 1;
    }

    get_qcy_user_defined_eq(&gain[1]);

    qcy_ctrl_set_parameter(QCY_PARAM_DEFAULT_EQ, sizeof(gain), (uint8_t *)gain);

    APP_PRINT_TRACE1("app_ctrl_get_eq: %b", TRACE_BINARY(sizeof(gain), gain));

    return true;
}

void app_handle_read_request(uint8_t read_index)
{
    APP_PRINT_TRACE1("app_handle_read_request: %d", read_index);

    switch (read_index)
    {
    case QCY_PARAM_GET_FEATURE:
        {
            T_QCY_FEATURE_INFO tmp_feature_info;

            get_feature_info(&tmp_feature_info);

            qcy_ctrl_set_parameter((T_QCY_PARAM_TYPE)read_index, sizeof(tmp_feature_info),
                                   (uint8_t *) &tmp_feature_info);
        }
        break;

    case QCY_PARAM_CLICK_CFG:
        {
            T_QCY_CLICK_CFG tmp_click_cfg;

            get_click_cfg(&tmp_click_cfg);

            qcy_ctrl_set_parameter((T_QCY_PARAM_TYPE)read_index, sizeof(tmp_click_cfg),
                                   (uint8_t *) &tmp_click_cfg);
        }
        break;

    case QCY_PARAM_VERSION_INFO:
        {
            /*
            Divide VERSION_BUILD_NUM to 3bytes for version.
            for example: if VERSION_BUILD_NUM is 1058,
                          version will be 0.4.34.
            */
            uint8_t tmp_version_info[6] = {0};

            uint32_t version_build_num = VERSION_MAJOR * 10000 +
                                         VERSION_MINOR * 1000  +
                                         VERSION_REVISION * 100 +
                                         VERSION_BUILDNUM;

            tmp_version_info[2] = tmp_version_info[5] = version_build_num & 0x0000FF;
            tmp_version_info[1] = tmp_version_info[4] = ((version_build_num & 0x00FF00) >> 8);
            tmp_version_info[0] = tmp_version_info[3] = ((version_build_num & 0xFF0000) >> 16);

            qcy_ctrl_set_parameter((T_QCY_PARAM_TYPE)read_index, 6, tmp_version_info);
        }
        break;

    case QCY_PARAM_BATTERY_STATUS:
        {
            T_QCY_BATTERY_STATUS battery_status;

            get_battery_status(&battery_status);

            qcy_ctrl_set_parameter((T_QCY_PARAM_TYPE)read_index, sizeof(battery_status),
                                   (uint8_t *) &battery_status);
        }
        break;

    case QCY_PARAM_LANG_CFG:
        {
            uint8_t tmp_lang_cfg = 0;
            switch (app_cfg_nv.voice_prompt_language)
            {
            case VOICE_PROMPT_LANGUAGE_CHINESE:
                {
                    tmp_lang_cfg = LANGUAGE_MANDARIN;
                }
                break;

            case VOICE_PROMPT_LANGUAGE_ENGLISH:
                {
                    tmp_lang_cfg = LANGUAGE_ENGLISH;
                }
                break;

            case VOICE_PROMPT_LANGUAGE_FRENCH:
                {
                    tmp_lang_cfg = LANGUAGE_FRENCH;
                }
                break;
            }

            qcy_ctrl_set_parameter((T_QCY_PARAM_TYPE)read_index, sizeof(tmp_lang_cfg), &tmp_lang_cfg);
        }
        break;

    case QCY_PARAM_DEFAULT_EQ:
    case QCY_PARAM_CUSTOMER_EQ:
        {
            app_ctrl_get_eq(EQ_SELF_DEFINED, NULL, 0);
        }
        break;

    default:
        {
            //No corresponding handler.
        }
        break;
    }
}

bool app_excecute_set_feature_cmd(uint8_t *cmd, uint8_t len)
{
    APP_PRINT_TRACE1("app_excecute_set_feature_cmd: %b", TRACE_BINARY(len, cmd));

    switch (cmd[0])
    {
    case ID_RESET_BLE_DEFAULT:
        {
            app_ctrl_reset_ble_default();
        }
        break;

    case ID_CLEAR_PHONE_RECORD:
        {
            app_ctrl_clear_phone_record();
        }
        break;

    case ID_RESET_DEFAULT:
        {
            app_ctrl_reset_default();
        }
        break;

    case ID_MUSIC_PLAY:
        {
            app_ctrl_music_play((T_APP_CTRL_MUSIC_PLAY)cmd[2]);
        }
        break;

    case ID_LED_BLINKING:
        {
            T_APP_CTRL_LED led_ctrl;

            if (cmd[2] == 1)
            {
                led_ctrl = LED_START_BLINKING;
            }
            else
            {
                led_ctrl = LED_STOP_BLINKING;
            }

            app_ctrl_led_blinking(led_ctrl);
        }
        break;

    case ID_IN_EAR_DETECTION:
        {
            app_ctrl_in_ear_detection((T_APP_CTRL_IN_EAR_DETECTION)cmd[2]);
        }
        break;

    case ID_CTRL_ANC:
        {
            app_ctrl_anc(cmd[2]);
        }
        break;

    case ID_CTRL_VOLUME:
        {
            app_ctrl_volume(cmd[2], cmd[3]);
        }
        break;

    case ID_GAMING_MODE:
        {
            app_ctrl_gaming_mode((T_APP_CTRL_GAMING_MODE)cmd[2]);
        }
        break;

    case ID_LISTENING_MODE:
        {
            app_ctrl_listening_mode((T_APP_CTRL_LISTENING_MODE)cmd[2]);
        }
        break;

    case ID_OTA_MODE:
        {
            /*todo*/
        }
        break;

    case ID_SLEEP_MODE:
        {
            app_ctrl_sleep_mode(cmd[2]);
        }
        break;
    }
    return true;
}

bool app_parse_cmd(T_QCY_PARAM_TYPE param, uint8_t *write_data)
{
    switch (param)
    {
    case QCY_PARAM_SET_FEATURE:
        {
            if (write_data[0] != 0xff)
            {
                return false;
            }

            uint8_t total_len = write_data[1] + 2;
            uint8_t *sub_cmd = &write_data[2];
            uint8_t sub_cmd_len;

            while (sub_cmd < (write_data + total_len))
            {
                sub_cmd_len = sub_cmd[1];

                app_excecute_set_feature_cmd(sub_cmd, sub_cmd_len + 2);

                sub_cmd += (2 + sub_cmd_len); /* cmd byte + len byte + payload */
            }
        }
        break;

    case QCY_PARAM_CLICK_CFG:
        {
            T_KEY_CFG key_cfg;
            key_cfg.key_event = (T_QCY_CTRL_KEY_EVENT)((write_data[0] - 1) / 2);
            key_cfg.key_action = (T_QCY_CTRL_KEY_ACTION)write_data[1];

            if (write_data[0] % 2) // odd => left bud key setting
            {
                app_ctrl_key_setting(LEFT_BUD_SETTING, &key_cfg);
            }
            else // even => right bud key setting
            {
                app_ctrl_key_setting(RIGHT_BUD_SETTING, &key_cfg);
            }
        }
        break;

    case QCY_PARAM_LANG_CFG:
        {
            app_ctrl_language((T_APP_CTRL_LANGUAGE)write_data[0]);
        }
        break;

    case QCY_PARAM_DEFAULT_EQ:
        {
            app_ctrl_set_eq((T_APP_CTRL_EQ)write_data[0], &write_data[1]);
        }
        break;

    case QCY_PARAM_CUSTOMER_EQ:
        {
            app_ctrl_set_eq((T_APP_CTRL_EQ)write_data[0], &write_data[1]);
        }
        break;
    }

    app_qcy_info_sync(false);

    return true;
}

void app_handle_write_request(T_QCY_WRITE_MSG write_msg)
{
    APP_PRINT_TRACE1("app_handle_write_request: %d", write_msg.write_index);

    switch (write_msg.write_index)
    {
    case QCY_PARAM_SET_FEATURE:
        {
            APP_PRINT_TRACE2("QCY_PARAM_SET_FEATURE: %x %x", write_msg.write_data[0], write_msg.write_data[1]);
            app_parse_cmd(QCY_PARAM_SET_FEATURE, write_msg.write_data);
        }
        break;

    case QCY_PARAM_CLICK_CFG:
        {
            APP_PRINT_TRACE2("QCY_PARAM_CLICK_CFG: %x %x", write_msg.write_data[0], write_msg.write_data[1]);
            app_parse_cmd(QCY_PARAM_CLICK_CFG, write_msg.write_data);
        }
        break;

    case QCY_PARAM_LANG_CFG:
        {
            APP_PRINT_TRACE1("QCY_PARAM_LANG_CFG: %x", write_msg.write_data[0]);
            app_parse_cmd(QCY_PARAM_LANG_CFG, write_msg.write_data);
        }
        break;

    case QCY_PARAM_DEFAULT_EQ:
        {
            APP_PRINT_TRACE1("QCY_PARAM_DEFAULT_EQ: %x", write_msg.write_data[0]);
            app_parse_cmd(QCY_PARAM_DEFAULT_EQ, write_msg.write_data);
        }
        break;

    case QCY_PARAM_CUSTOMER_EQ:
        {
            APP_PRINT_TRACE1("QCY_PARAM_CUSTOMER_EQ: %x", write_msg.write_data[0]);
            app_parse_cmd(QCY_PARAM_CUSTOMER_EQ, write_msg.write_data);
        }
        break;

    default:
        {
            //No corresponding handler.
        }
        break;
    }
}

void app_qcy_set_notify_value(uint8_t conn_id, T_QCY_PARAM_TYPE param)
{
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_PRIMARY)
    {
        return;
    }

    APP_PRINT_TRACE1("app_qcy_set_notify_value: %d", param);

    switch (param)
    {
    case QCY_PARAM_GET_FEATURE:
        {
            T_QCY_FEATURE_INFO tmp_feature_info;

            get_feature_info(&tmp_feature_info);

            qcy_feature_info_notify(conn_id, qcy_ctrl_srv_id, &tmp_feature_info);
        }
        break;

    case QCY_PARAM_BATTERY_STATUS:
        {
            T_QCY_BATTERY_STATUS battery_status;

            get_battery_status(&battery_status);

            qcy_battery_status_value_notify(conn_id, qcy_ctrl_srv_id, &battery_status);
        }
        break;

    default:
        {
            //No corresponding handler.
        }
        break;
    }
}
void app_handle_notify_indicate_request(uint8_t conn_id,
                                        T_QCY_NOTIFY_INDICATE_MSG notify_indicate_msg)
{
    APP_PRINT_TRACE3("app_handle_notify_indicate_request: %d, %d, %d",
                     conn_id, notify_indicate_msg.index, notify_indicate_msg.status);

    switch (notify_indicate_msg.index)
    {
    case QCY_PARAM_GET_FEATURE:
        {
            if (notify_indicate_msg.status == QCY_NOTIFY_FEATURE_STATUS_ENABLE)
            {
                app_cfg_nv.qcy_feature_status_notify = 1;
                app_qcy_conn_id.feature_status_conn_id = conn_id;
            }
            else
            {
                app_cfg_nv.qcy_feature_status_notify = 0;
                app_qcy_conn_id.feature_status_conn_id = 0xFF;
            }
        }
        break;

    case QCY_PARAM_BATTERY_STATUS:
        {
            if (notify_indicate_msg.status == QCY_NOTIFY_BATTERY_STATUS_ENABLE)
            {
                app_cfg_nv.qcy_battery_status_notify = 1;
                app_qcy_conn_id.battery_status_conn_id = conn_id;
            }
            else
            {
                app_cfg_nv.qcy_battery_status_notify = 0;
                app_qcy_conn_id.battery_status_conn_id = 0xFF;
            }
        }
        break;

    default:
        {
            //No corresponding handler.
        }
        break;
    }
}

void app_qcy_key_handle_single_click(uint8_t key, uint8_t type)
{
    uint8_t key_action;

    key_action = app_cfg_const.key_table[type][app_hfp_get_call_status()][key_search_index(key)];

    if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
        (key_search_index(key) == 0))
    {
        T_QCY_CTRL_KEY_ACTION qcy_key = QCY_KEY_ACTION_NULL;

        if (type == 0) // short press
        {
            qcy_key = (T_QCY_CTRL_KEY_ACTION)app_cfg_nv.qcy_key_info.local_key.single_click_action;
        }
        else // long press
        {
            qcy_key = (T_QCY_CTRL_KEY_ACTION)app_cfg_nv.qcy_key_info.local_key.long_press_action;
        }

        if (qcy_key != QCY_KEY_ACTION_NULL)
        {
            key_action = qcy_key_to_mmi(qcy_key);
        }
    }

    key_execute_action(key_action);
}

void app_qcy_key_handle_hybrid_click(uint8_t key, uint8_t index, uint8_t hybrid_type)
{
    uint8_t key_action;

    key_action = app_cfg_const.hybrid_key_table[app_hfp_get_call_status()][index];

    if ((app_hfp_get_call_status() == BT_HFP_CALL_IDLE) &&
        (key_search_index(key) == 0))
    {
        T_QCY_CTRL_KEY_ACTION qcy_key = QCY_KEY_ACTION_NULL;

        if (hybrid_type == HYBRID_KEY_2_CLICK)
        {
            qcy_key = (T_QCY_CTRL_KEY_ACTION)app_cfg_nv.qcy_key_info.local_key.double_click_action;
        }
        else if (hybrid_type == HYBRID_KEY_3_CLICK)
        {
            qcy_key = (T_QCY_CTRL_KEY_ACTION)app_cfg_nv.qcy_key_info.local_key.triple_click_action;
        }
        else if (hybrid_type == HYBRID_KEY_4_CLICK)
        {
            qcy_key = (T_QCY_CTRL_KEY_ACTION)app_cfg_nv.qcy_key_info.local_key.four_click_action;
        }

        if (qcy_key != QCY_KEY_ACTION_NULL)
        {
            key_action = qcy_key_to_mmi(qcy_key);
        }
    }

    key_execute_action(key_action);
}

/* if qcy key cfg is set even though mcu cfg not set; we still should excecute this action */
bool app_qcy_key_hybrid_click_is_set(uint8_t hybrid_type)
{
    bool ret = false;
    T_QCY_KEY_SETTING *key_setting = &app_cfg_nv.qcy_key_info.local_key;

    if (hybrid_type == HYBRID_KEY_2_CLICK)
    {
        if (key_setting->double_click_action != QCY_KEY_ACTION_NULL)
        {
            ret = true;
        }
    }
    else if (hybrid_type == HYBRID_KEY_3_CLICK)
    {
        if (key_setting->triple_click_action != QCY_KEY_ACTION_NULL)
        {
            ret = true;
        }
    }
    else if (hybrid_type == HYBRID_KEY_4_CLICK)
    {
        if (key_setting->four_click_action != QCY_KEY_ACTION_NULL)
        {
            ret = true;
        }
    }

    APP_PRINT_TRACE2("app_qcy_key_check_hybrid_click_is_set: type %d ret %d",
                     hybrid_type, ret);

    return ret;
}

/* reset QCY's specific cfg */
void app_qcy_cfg_reset(void)
{
    APP_PRINT_TRACE0("app_qcy_cfg_reset");

    /* reset key setting */
    app_qcy_reset_key_setting();

    /* reset eq setting */
    if (app_db.gaming_mode)
    {
        app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_gaming_mode_record;
    }
    else
    {
        app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;
    }
    app_qcy_reset_user_defined_eq();
    app_eq_index_set(SPK_SW_EQ, app_db.gaming_mode, app_cfg_nv.eq_idx);
    app_cfg_nv.qcy_reset_flg = 1;

    /* reset language setting */
    app_qcy_set_language(app_cfg_const.voice_prompt_language);

    app_qcy_info_sync(false);
}

#endif
