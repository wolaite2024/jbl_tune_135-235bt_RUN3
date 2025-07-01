#include <string.h>
#include "trace.h"
#include "aics_mgr.h"
#include "mics_mgr.h"
#include "vocs_mgr.h"
#include "vcs_mgr.h"
#include "app_cfg.h"
#include "app_link_util.h"
#include "app_mcp.h"
#include "app_vcs.h"

#if F_APP_VCS_SUPPORT
#if F_APP_AICS_SUPPORT
const char audio_input_des_bt[] = "Bluetooth";
const char audio_input_des_line_in[] = "Line In";
#endif

const char audio_output_des_left[] = "Left Speaker";
const char audio_output_des_right[] = "Right Speaker";

T_APP_RESULT app_vcs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_vcs_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_VCS_VOLUME_STATE_CHANGE:
        {
            T_APP_LE_LINK *p_link;
            T_VCS_VOL_STATE *p_vcs_vol_state = (T_VCS_VOL_STATE *)buf;

            p_link = app_find_le_link_by_conn_handle(p_vcs_vol_state->conn_handle);
            if (p_link != NULL)
            {
                T_VCS_PARAM vcs_param;
                p_link->volume_setting = p_vcs_vol_state->volume_setting;
                p_link->mute = p_vcs_vol_state->mute;

                vcs_get_param(&vcs_param);
                if (vcs_param.volume_setting != p_link->volume_setting ||
                    vcs_param.mute != p_link->mute)
                {
                    vcs_param.volume_setting = p_link->volume_setting;
                    vcs_param.mute = p_link->mute;
                    vcs_param.change_counter++;
                    vcs_set_param(&vcs_param);
                }
                app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_VCS_VOL_CHANGE, NULL);
                APP_PRINT_INFO3("app_vcs_handle_msg: conn_handle 0x%x, volume_setting %x, mute %x",
                                p_vcs_vol_state->conn_handle, p_vcs_vol_state->volume_setting, p_vcs_vol_state->mute);
            }
        }
        break;

    default:
        break;
    }
    return app_result;
}

#if F_APP_VOCS_SUPPORT
T_APP_RESULT app_vocs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_vocs_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_VOCS_OFFSET_STATE_CHANGE:
        {
            T_APP_LE_LINK *p_link;
            T_VOCS_VOL_OFFSET *p_vol_offset = (T_VOCS_VOL_OFFSET *)buf;

            p_link = app_find_le_link_by_conn_id(p_vol_offset->conn_id);
            if (p_link != NULL)
            {
                if (((int16_t)(p_link->volume_setting + p_vol_offset->volume_offset) <= MAX_VCS_VOLUME_SETTING) &&
                    (int16_t)(p_link->volume_setting + p_vol_offset->volume_offset) >= 0)
                {
                    T_VCS_PARAM vcs_param;
                    p_link->volume_setting += p_vol_offset->volume_offset;
                    p_link->mute = p_vcs_vol_state->mute;

                    vcs_get_param(&vcs_param);
                    if (vcs_param.volume_setting != p_link->volume_setting ||
                        vcs_param.mute != p_link->mute)
                    {
                        vcs_param.volume_setting = p_link->volume_setting;
                        vcs_param.mute = p_link->mute;
                        vcs_param.change_counter++;
                        vcs_set_param(&vcs_param);
                    }
                    app_le_audio_link_sm(p_link->conn_handle, LE_AUDIO_VCS_VOL_CHANGE, NULL);
                    APP_PRINT_INFO3("app_vocs_handle_msg: conn_handle 0x%x, volume_offset %x, volume_setting %x",
                                    p_vol_offset->conn_handle, p_vol_offset->volume_offset, p_link->volume_setting);
                }
            }
        }
        break;

    case LE_AUDIO_MSG_VOCS_AUDIO_LOCATION_CHANGE:
        {
            T_VOCS_CHNL_LOC *p_chnl_loc = (T_VOCS_CHNL_LOC *)buf;
            APP_PRINT_INFO2("app_vocs_handle_msg: conn_handle 0x%x, chnl_loc %x",
                            p_chnl_loc->conn_handle, p_chnl_loc->chnl_loc);
        }
        break;
    default:
        break;
    }
    return app_result;
}
#endif

#if F_APP_AICS_SUPPORT
T_APP_RESULT app_aics_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    APP_PRINT_INFO1("app_aics_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_AICS_INPUT_CP_OP:
        {
            T_AICS_OP *p_aics_op = (T_AICS_OP *)buf;

            APP_PRINT_INFO3("app_aics_handle_msg: idx %x, opcode %x, gain_setting %x",
                            p_aics_op->idx, p_aics_op->opcode, p_aics_op->gain_setting);
        }
        break;
    default:
        break;
    }
    return app_result;
}
#endif

#if F_APP_MICS_SUPPORT
T_APP_RESULT app_mics_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_MICS_WRITE_MUTE_VALUE:
        {
            uint8_t mute_value = *(uint8_t *)buf;
            if (mute_value == AICS_NOT_MUTED)
            {

            }
            else if (mute_value == AICS_MUTED)
            {

            }
            else
            {

            }
            APP_PRINT_INFO1("app_mics_handle_msg: mute_value %x", mute_value);
        }
        break;
    default:
        break;
    }
    return app_result;
}
#endif

void app_vcs_init(void)
{
    uint8_t vocs_num = 0;
    uint8_t aics_num = 0;
#if F_APP_VOCS_SUPPORT
    vocs_num = F_APP_VOCS_NUM;
#endif
#if F_APP_AICS_SUPPORT
    aics_num = F_APP_AICS_NUM;
#endif
    T_VCS_PARAM vcs_param;
    vcs_init(vocs_num, aics_num);

    vcs_param.volume_setting = 245;
    vcs_param.mute = VCS_NOT_MUTED;
    vcs_param.change_counter = 33;
    vcs_param.volume_flags = VCS_RESET_VOLUME_SETTING;
    vcs_param.step_size = 5;
    vcs_set_param(&vcs_param);

#if F_APP_VOCS_SUPPORT
    {
        uint8_t feature = 0;
        T_VOCS_PARAM_SET param1;

        vocs_init(vocs_num, &feature);
        if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            param1.mask = VOCS_VOLUME_OFFSET_MASK | VOCS_AUDIO_LOCATION_MASK | VOCS_OUTPUT_DES_MASK;
            param1.volume_offset = 30;
            param1.change_counter = 24;
            param1.audio_location = AUDIO_LOCATION_FL;
            param1.output_des.p_output_des = (uint8_t *)audio_output_des_left;
            param1.output_des.output_des_len = sizeof(audio_output_des_left);
            vocs_set_param(F_APP_VOCS_LEFT, &param1);
        }
        else if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            param1.mask = VOCS_VOLUME_OFFSET_MASK | VOCS_AUDIO_LOCATION_MASK | VOCS_OUTPUT_DES_MASK;
            param1.volume_offset = -30;
            param1.change_counter = 24;
            param1.audio_location = AUDIO_LOCATION_FR;
            param1.output_des.p_output_des = (uint8_t *)audio_output_des_right;
            param1.output_des.output_des_len = sizeof(audio_output_des_right);

            vocs_set_param(F_APP_VOCS_RIGHT, &param1);
        }
    }
#endif
#if F_APP_AICS_SUPPORT
    {
        T_AICS_INPUT_STATE input_state;
        T_AICS_GAIN_SETTING_PROP gain_prop;
        uint8_t input_type = MICROPHONE_AUDIO_INTPUT;
        uint8_t input_status = AICS_INPUT_STATUS_ACTIVE;

        input_state.change_counter = 30;
        input_state.gain_mode = AICS_GAIN_MODE_MANUAL;
        input_state.gain_setting = 20;
        input_state.mute = AICS_NOT_MUTED;
        gain_prop.gain_setting_max = 30;
        gain_prop.gain_setting_min = -30;
        gain_prop.gain_setting_units = 5;

        aics_init(aics_num);

        aics_set_param(F_APP_AICS_MIC_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                       (uint8_t *)&input_state, true);
        aics_set_param(F_APP_AICS_MIC_IDX, AICS_PARAM_GAIN_SETTING_PROP,
                       sizeof(T_AICS_GAIN_SETTING_PROP), (uint8_t *)&gain_prop, false);
        aics_set_param(F_APP_AICS_MIC_IDX, AICS_PARAM_INPUT_TYPE, sizeof(uint8_t), &input_type, false);
        aics_set_param(F_APP_AICS_MIC_IDX, AICS_PARAM_INPUT_STATUS, sizeof(uint8_t), &input_status,
                       false);
        aics_set_param(F_APP_AICS_MIC_IDX, AICS_PARAM_INPUT_DES, strlen(audio_input_des_bt),
                       (uint8_t *)audio_input_des_bt, false);
    }
#endif

#if F_APP_MICS_SUPPORT
    {
        uint8_t id_array = 0;

        T_MICS_PARAM mics_param;
        mics_init(F_APP_AICS_NUM, &id_array);

        mics_param.mute_value = MICS_NOT_MUTE;
        mics_set_param(&mics_param);
    }
#endif
}
#endif
