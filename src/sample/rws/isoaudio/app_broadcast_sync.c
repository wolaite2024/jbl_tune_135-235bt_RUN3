#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "base_data_parse.h"
#include "bass_mgr.h"
#include "app_broadcast_sync.h"
#include "app_cfg.h"
#include "app_le_audio_mgr.h"
#include "app_le_audio_scan.h"
#include "app_sniff_mode.h"
#include "gap_ext_scan.h"
#include "app_mmi.h"
#include "app_audio_policy.h"
#include "app_bass.h"
#include "app_link_util.h"

#if F_APP_TMAP_BMR_SUPPORT
uint8_t num_bis;
uint8_t bis_idx = 0;
uint16_t bis_conn_handle;

T_BC_SOURCE_INFO source_list[APP_BC_SOURCE_NUM];

uint8_t g_dev_idx = APP_BC_SOURCE_NUM;
static bool big_terminate = 0;

bool app_sink_pa_terminate(uint8_t source_id)
{
    if (ble_audio_pa_terminate(app_bass_get_sync_handle(source_id)) == false)
    {
        goto error;
    }
    return true;
error:
    return false;
}
uint16_t app_bc_get_bis_handle(void)
{
    return bis_conn_handle;
}

T_BC_SOURCE_INFO *app_bc_sync_find_device(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid)
{
    uint8_t i;
    /* Check if device is already in device list*/
    for (i = 0; i < APP_BC_SOURCE_NUM; i++)
    {
        if ((source_list[i].used == true) &&
            (source_list[i].advertiser_address_type == bd_type) &&
            (source_list[i].advertiser_sid == adv_sid) &&
            memcmp(bd_addr, source_list[i].advertiser_address, GAP_BD_ADDR_LEN) == 0)
        {
            return &source_list[i];
        }
    }
    return NULL;
}

T_BC_SOURCE_INFO *app_bc_sync_find_device_by_handle(T_BLE_AUDIO_SYNC_HANDLE sync_handle)
{
    uint8_t i;
    if (sync_handle == NULL)
    {
        return NULL;
    }
    /* Check if device is already in device list*/
    for (i = 0; i < APP_BC_SOURCE_NUM; i++)
    {
        if ((source_list[i].used == true) &&
            (source_list[i].sync_handle == sync_handle))
        {
            return &source_list[i];
        }
    }
    return NULL;
}

bool app_bc_sync_add_device(uint8_t *bd_addr, uint8_t bd_type, uint8_t adv_sid)
{
    uint8_t                 adv_address[GAP_BD_ADDR_LEN];

    if (app_bc_sync_find_device(bd_addr, bd_type, adv_sid) != NULL)
    {
        return true;
    }
    uint8_t i;
    /* Check if device is already in device list*/
    for (i = 0; i < APP_BC_SOURCE_NUM; i++)
    {
        if (source_list[i].used == false)
        {
            if ((app_cfg_const.bis_policy == BIS_POLICY_SPECIFIC) &&
                (memcmp(bd_addr, app_cfg_const.bstsrc, GAP_BD_ADDR_LEN)))
            {
                continue;
            }
            else if (app_cfg_const.bis_policy == BIS_POLICY_RANDOM)
            {
                uint8_t dev_idx =  APP_BC_SOURCE_NUM;
                app_bc_get_active(&dev_idx);
                if (dev_idx != APP_BC_SOURCE_NUM)
                {
                    break;
                }
            }
            /*Add addr to device list list*/
            source_list[i].used = true;
            memcpy(source_list[i].advertiser_address, bd_addr, GAP_BD_ADDR_LEN);
            source_list[i].advertiser_address_type = bd_type;
            source_list[i].advertiser_sid = adv_sid;

            APP_PRINT_INFO8("Add RemoteBd[%d] = [%02x:%02x:%02x:%02x:%02x:%02x] , type %d \r\n",
                            i,
                            source_list[i].advertiser_address[5], source_list[i].advertiser_address[4],
                            source_list[i].advertiser_address[3], source_list[i].advertiser_address[2],
                            source_list[i].advertiser_address[1], source_list[i].advertiser_address[0],
                            source_list[i].advertiser_address_type);
            APP_PRINT_INFO1("sid %d\r\n", source_list[i].advertiser_sid);
            APP_PRINT_INFO6("Add RemoteBd= [%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
                            app_cfg_const.bstsrc[5], app_cfg_const.bstsrc[4],
                            app_cfg_const.bstsrc[3], app_cfg_const.bstsrc[2],
                            app_cfg_const.bstsrc[1], app_cfg_const.bstsrc[0]);

            app_bc_set_active(i);
            APP_PRINT_INFO1("find src, index = %d", i);
            if (app_bc_sync_pa_sync(i) == false)
            {
                APP_PRINT_ERROR1("app_audio_cmd_pasync: failed: dev_idx: 0x%x", i);
            }

            return true;
        }
    }
    return false;
}

bool app_bc_sync_clear_device(T_BLE_AUDIO_SYNC_HANDLE handle)
{
    uint8_t i;
    if (handle == NULL)
    {
        APP_PRINT_ERROR0("app_bc_sync_clear_device: handle is NULL");
        return false;
    }
    /* Check if device is already in device list*/
    for (i = 0; i < APP_BC_SOURCE_NUM; i++)
    {
        if ((source_list[i].used == true) &&
            (source_list[i].sync_handle == handle))
        {
            source_list[i].used = false;
            memset(source_list[i].advertiser_address, 0, GAP_BD_ADDR_LEN);
            source_list[i].sync_handle = NULL;
            return true;
        }
    }

    return false;

}

static T_BASE_DATA_BIS_PARAM *app_bc_find_src_bis_param(T_BASE_DATA_MAPPING *p_mapping,
                                                        uint8_t bis_idx)
{
    uint8_t i, j;
    for (i = 0; i < p_mapping->num_subgroups; i++)
    {
        if (p_mapping->p_subgroup[i].bis_array & (1 << (bis_idx - 1)))
        {
            for (j = 0; j < p_mapping->p_subgroup[i].num_bis; j ++)
            {
                if (p_mapping->p_subgroup[i].p_bis_param[j].bis_index == bis_idx)
                {
                    return (p_mapping->p_subgroup[i].p_bis_param + j);
                }
            }
        }
    }
    return NULL;
}

void app_bc_sync_sync_cb(T_BLE_AUDIO_SYNC_HANDLE handle, uint8_t cb_type, void *p_cb_data)
{

    T_BLE_AUDIO_SYNC_CB_DATA *p_sync_cb = (T_BLE_AUDIO_SYNC_CB_DATA *)p_cb_data;
    T_BC_SOURCE_INFO *p_bc_source = app_bc_sync_find_device_by_handle(handle);
    if (p_bc_source == NULL)
    {
        return;
    }
    APP_PRINT_INFO1("app_bc_sync_sync_cb: cb_type %d,\r\n",
                    cb_type);

    switch (cb_type)
    {
    case MSG_BLE_AUDIO_PA_SYNC_STATE:
        {
            APP_PRINT_INFO3("MSG_BLE_AUDIO_PA_SYNC_STATE: sync_state %d, action %d, cause 0x%x\r\n",
                            p_sync_cb->p_pa_sync_state->sync_state,
                            p_sync_cb->p_pa_sync_state->action,
                            p_sync_cb->p_pa_sync_state->cause);

            //PA sync lost
            if (((p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_LOST) ||
                 (p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_SYNC)) &&
                (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED) &&
                (p_sync_cb->p_pa_sync_state->cause == 0x13e))
            {
                uint8_t dev_idx;
                app_bc_get_active(&dev_idx);
                if (dev_idx < APP_BC_SOURCE_NUM)
                {
                    app_bc_sync_pa_terminate(dev_idx);
                    if (ble_audio_sync_realese(handle))
                    {
                        app_bc_sync_clear_device(handle);
                        app_bc_reset();
                    }
                }
            }
            else if (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING_WAIT_SCANNING ||
                     p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZING)
            {

            }
            else if ((p_sync_cb->p_pa_sync_state->cause == 0) &&
                     (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED))
            {

            }
            else if (((p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_IDLE) &&
                      (p_sync_cb->p_pa_sync_state->cause == 0) &&
                      (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATING)) ||
                     ((p_sync_cb->p_pa_sync_state->action == BLE_AUDIO_PA_TERMINATE) &&
                      (p_sync_cb->p_pa_sync_state->cause == 0x116) &&
                      (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_TERMINATED)))
            {
                uint8_t dev_idx;
                app_le_audio_scan_stop();
                app_bc_get_active(&dev_idx);
                app_bc_sync_pa_terminate(dev_idx);
            }
            else if ((p_sync_cb->p_pa_sync_state->cause == 0) &&
                     (p_sync_cb->p_pa_sync_state->sync_state == GAP_PA_SYNC_STATE_SYNCHRONIZED))
            {
                /*after pa synchronized, set big_terminate to 0,
                big sync can be continued according to the state of biginfo.*/
                big_terminate = 0;
            }
        }
        break;

    case MSG_BLE_AUDIO_PA_REPORT_INFO:
        {
            APP_PRINT_INFO1("MSG_BLE_AUDIO_PA_REPORT_INFO: p_sync_cb->p_le_periodic_adv_report_info->sync_handle: 0x%x",
                            p_sync_cb->p_le_periodic_adv_report_info->sync_handle);

            T_BASE_DATA_MAPPING *p_mapping = base_data_parse_data(
                                                 p_sync_cb->p_le_periodic_adv_report_info->data_len,
                                                 p_sync_cb->p_le_periodic_adv_report_info->p_data);

            if (p_mapping != NULL)
            {
                APP_PRINT_INFO6("p_mapping: %d, %d, 0x%x,app_cfg_const.subgroup=%d, %d, %d",
                                p_mapping->num_bis, p_mapping->num_subgroups, p_mapping->p_subgroup->p_bis_param->bis_index,
                                app_cfg_const.subgroup, app_cfg_const.iso_mode, app_cfg_const.bis_mode);
                num_bis = p_mapping->num_bis;

                T_CODEC_CFG bis_codec_cfg;
                uint8_t i, j;
                for (i = 0; i < p_mapping->num_subgroups; i++)
                {
                    for (j = 0; j < p_mapping->p_subgroup[i].num_bis; j ++)
                    {
                        if (app_cfg_const.subgroup == p_mapping->p_subgroup[i].p_bis_param[j].bis_index)
                        {
                            bis_idx = p_mapping->p_subgroup[i].p_bis_param[j].bis_index ;
                            bis_codec_cfg.frame_duration = p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.frame_duration;
                            bis_codec_cfg.sample_frequency =
                                p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.sample_frequency;
                            bis_codec_cfg.codec_frame_blocks_per_sdu =
                                p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.codec_frame_blocks_per_sdu;
                            bis_codec_cfg.octets_per_codec_frame =
                                p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.octets_per_codec_frame;
                            bis_codec_cfg.audio_channel_allocation =
                                p_mapping->p_subgroup[i].p_bis_param[j].bis_codec_cfg.audio_channel_allocation;
                            bis_codec_cfg.presentation_delay  = p_mapping->presentation_delay;
                            APP_PRINT_INFO6("bis_codec_cfg: 0x%x, 0x%x, 0x%x, 0x%x,0x%x,0%x,",
                                            bis_codec_cfg.frame_duration,
                                            bis_codec_cfg.sample_frequency,
                                            bis_codec_cfg.codec_frame_blocks_per_sdu,
                                            bis_codec_cfg.octets_per_codec_frame,
                                            bis_codec_cfg.audio_channel_allocation,
                                            bis_codec_cfg.presentation_delay);
                            APP_PRINT_INFO3("bud role: %d, %d  bis_idx: 0x%x", app_cfg_const.bud_side, app_cfg_nv.bud_role,
                                            bis_idx);
                            app_le_audio_bis_cb_allocate(bis_idx, &bis_codec_cfg);
                            goto bis_choosed;

                        }
                    }
                }

bis_choosed:
                base_data_print(p_mapping);
                base_data_free(p_mapping);
            }

        }
        break;

    case MSG_BLE_AUDIO_PA_BIGINFO:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_PA_BIGINFO: num_bis %d, big_terminate: 0x%x",
                            p_sync_cb->p_le_biginfo_adv_report_info->num_bis, big_terminate);

            //According to the value of big_terminate, determine whether big sync can be done.
            if (!big_terminate)
            {
                APP_PRINT_INFO1("MSG_BLE_AUDIO_PA_BIGINFO: g_dev_idx: 0x%x", g_dev_idx);
                if (g_dev_idx < APP_BC_SOURCE_NUM)
                {
                    app_bc_sync_big_establish(g_dev_idx);
                }
                else
                {
                    APP_PRINT_ERROR1("MSG_BLE_AUDIO_PA_BIGINFO: invaild dev_idx: 0x%x", g_dev_idx);
                }
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SYNC_STATE:
        {
            APP_PRINT_INFO3("MSG_BLE_AUDIO_BIG_SYNC_STATE: sync_state %d, action %d, cause 0x%x\r\n",
                            p_sync_cb->p_big_sync_state->sync_state,
                            p_sync_cb->p_big_sync_state->action,
                            p_sync_cb->p_big_sync_state->cause);

            if ((p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
                (p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_TERMINATE) &&
                (p_sync_cb->p_big_sync_state->cause == 0))
            {
                /*when big terminate is initiated by app, set big_terminate to 1,
                to avoid continuing to do big sync through biginfo*/
                big_terminate = 1;
                APP_PRINT_INFO1("app_bc_sync_sync_cb: BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED, bis_conn_handle: 0x%x",
                                bis_conn_handle);
                T_BIS_CB *p_bis_cb = app_le_audio_find_bis_by_conn_handle(bis_conn_handle);
                if (p_bis_cb)
                {
                    if (!app_le_audio_free_bis(p_bis_cb))
                    {
                        APP_PRINT_ERROR0("app_bc_sync_sync_cb: free_bis_cb failed.");
                        return;
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("MSG_BLE_AUDIO_BIG_SYNC_STATE: bis_cb not found");
                    return;
                }
                app_bc_sync_pa_terminate(g_dev_idx);
                if (ble_audio_sync_realese(handle))
                {
                    app_bc_sync_clear_device(handle);
                    app_bc_reset();
                    app_lea_bis_state_change(LE_AUDIO_BIS_STATE_IDLE);
                    mtc_topology_dm(MTC_TOPO_EVENT_BIS_STOP);
                }
            }
            else if ((p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_TERMINATED) &&
                     ((p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_SYNC) ||
                      (p_sync_cb->p_big_sync_state->action == BLE_AUDIO_BIG_IDLE)) &&
                     ((p_sync_cb->p_big_sync_state->cause == 0x113) || (p_sync_cb->p_big_sync_state->cause == 0x108)))
            {
                /*set big_terminate to 0, big sync can be continued according to the state of biginfo.*/
                big_terminate = 0;
                if (g_dev_idx < APP_BC_SOURCE_NUM)
                {
                    if (!app_bc_sync_big_establish(g_dev_idx))
                    {
                        if (ble_audio_sync_realese(handle))
                        {
                            app_bc_sync_clear_device(handle);
                            app_bc_reset();
                            app_lea_bis_state_change(LE_AUDIO_BIS_STATE_IDLE);
                        }
                        app_audio_tone_type_play(TONE_BIS_LOSST, false, false);
                        mtc_start_timer(MTC_TMR_BIS, MTC_BIS_TMR_RESYNC, (app_cfg_const.bis_resync_to * 1000));
                        mtc_set_beep(MTC_PRO_BEEP_NONE);
                        app_lea_trigger_mmi_handle_action(MMI_BIG_START, true);
                    }
                }
                else
                {
                    APP_PRINT_ERROR1("MSG_BLE_AUDIO_BIG_SYNC_STATE: invaild dev_idx: 0x%x", g_dev_idx);
                }
            }
            else if (p_sync_cb->p_big_sync_state->sync_state == BIG_SYNC_RECEIVER_SYNC_STATE_SYNCHRONIZED)
            {
                //terminate PA sync to save bandwitch
                app_bc_sync_pa_terminate(g_dev_idx);

                uint8_t codec_id[5] = {1, 0, 0, 0, 0};
                uint32_t controller_delay = 0x1122;
                codec_id[0] = LC3_CODEC_ID;
                APP_PRINT_INFO1("app_bc_sync_sync_cb: MSG_BLE_AUDIO_BIG_SYNC_STATE: bis_idx: %d", bis_idx);
                ble_audio_bis_setup_data_path(handle, bis_idx, codec_id, controller_delay, 0, NULL);
            }
        }
        break;

    case MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH:
        {
            APP_PRINT_INFO2("MSG_BLE_AUDIO_BIG_SETUP_DATA_PATH: bis_idx %d, cause 0x%x\r\n",
                            p_sync_cb->p_setup_data_path->bis_idx,
                            p_sync_cb->p_setup_data_path->cause);
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_ISOAUDIO);
            bis_conn_handle = p_sync_cb->p_setup_data_path->bis_conn_handle;
            app_le_audio_track_create_for_bis(p_sync_cb->p_setup_data_path->bis_conn_handle, 0,
                                              p_sync_cb->p_setup_data_path->bis_idx);
            app_le_audio_bis_state_machine(LE_AUDIO_STREAMING, NULL);
            app_disallow_legacy_stream(false);
        }
        break;

    case MSG_BLE_AUDIO_BIG_REMOVE_DATA_PATH:
        {
            T_BIS_CB *p_bis_cb = NULL;
            p_bis_cb = app_le_audio_find_bis_by_conn_handle(app_bc_get_bis_handle());
            app_lea_bis_data_path_reset(p_bis_cb);
        }
        break;

    default:
        break;
    }
    return;
}

bool app_bc_set_active(uint8_t dev_idx)
{
    APP_PRINT_INFO1("app_bc_set_active, dev_idx: 0x%x", dev_idx);
    if (dev_idx < APP_BC_SOURCE_NUM)
    {
        g_dev_idx = dev_idx;
        return true;
    }
    return false;
}

bool app_bc_get_active(uint8_t *dev_idx)
{
    *dev_idx = g_dev_idx;

    return true;
}

void app_bc_reset(void)
{
    g_dev_idx = APP_BC_SOURCE_NUM;
}

bool app_bc_sync_pa_sync(uint8_t dev_idx)
{
    APP_PRINT_INFO1("app_bc_sync_pa_sync, dev_idx: 0x%x", dev_idx);
    uint8_t options = 0;
    uint8_t sync_cte_type = 0;
    uint16_t skip = 0;
    uint16_t sync_timeout = 1000;
    T_BC_SOURCE_INFO *p_bc_source = NULL;

    if (dev_idx < APP_BC_SOURCE_NUM && source_list[dev_idx].used == true)
    {
        p_bc_source = &source_list[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            p_bc_source->sync_handle = ble_audio_sync_create(app_bc_sync_sync_cb,
                                                             p_bc_source->advertiser_address_type,
                                                             p_bc_source->advertiser_address, p_bc_source->advertiser_sid, NULL);
        }
        if (p_bc_source->sync_handle == NULL)
        {
            goto error;
        }
    }
    else
    {
        goto error;
    }

    if (p_bc_source == NULL)
    {
        goto error;
    }
    if (ble_audio_pa_sync_establish(p_bc_source->sync_handle,
                                    options, sync_cte_type, skip, sync_timeout) == false)
    {
        goto error;
    }
    return true;
error:
    return false;
}

bool app_bc_sync_pa_terminate(uint8_t dev_idx)
{
    T_BC_SOURCE_INFO *p_bc_source = NULL;
    if (dev_idx < APP_BC_SOURCE_NUM && source_list[dev_idx].used == true)
    {
        p_bc_source = &source_list[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return ble_audio_pa_terminate(p_bc_source->sync_handle);
}

bool app_bc_sync_big_establish(uint8_t dev_idx)
{
    T_BC_SOURCE_INFO *p_bc_source = NULL;
    T_BIG_MGR_SYNC_RECEIVER_BIG_CREATE_SYNC_PARAM sync_param;
    uint8_t i = 0;
    if (dev_idx < APP_BC_SOURCE_NUM && source_list[dev_idx].used == true)
    {
        p_bc_source = &source_list[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            APP_PRINT_ERROR0("app_bc_sync_big_establish___0");
            return false;
        }
    }
    else
    {
        APP_PRINT_ERROR0("app_bc_sync_big_establish___1");
        return false;
    }
    sync_param.encryption = 0;
    sync_param.mse = 0;
    sync_param.big_sync_timeout = 100;
    //sync_param.num_bis = num_bis; //sync all

    //for (i = 0; i < sync_param.num_bis; i++)
    //{
    //    sync_param.bis[i] = i + 1;
    //}
    sync_param.num_bis = 1;
    sync_param.bis[0] = bis_idx;
    APP_PRINT_ERROR0("app_bc_sync_big_establish___OK");

    if (!ble_audio_big_sync_establish(p_bc_source->sync_handle, &sync_param))
    {
        APP_PRINT_ERROR1("app_bc_sync_big_establish: sync failed, num_bis: 0x%x", sync_param.num_bis);
        return false;
    }
    return true;
}

bool app_bc_sync_big_terminate(uint8_t dev_idx)
{
    T_BC_SOURCE_INFO *p_bc_source = NULL;
    //APP_PRINT_ERROR2("app_bc_sync_big_terminate: dev_idx=%d, used: %d", dev_idx,
    //                   source_list[dev_idx].used);
    if (dev_idx < APP_BC_SOURCE_NUM && source_list[dev_idx].used == true)
    {
        p_bc_source = &source_list[dev_idx];
        if (p_bc_source->sync_handle == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    APP_PRINT_ERROR0("app_bc_sync_big_terminate: OK");
    return ble_audio_big_terminate(p_bc_source->sync_handle);
}
bool app_bc_find_handle(T_BLE_AUDIO_SYNC_HANDLE *handle, uint8_t *dev_idx)
{
    T_BC_SOURCE_INFO *p_bc_source = NULL;
    if (*dev_idx < APP_BC_SOURCE_NUM && source_list[*dev_idx].used == true)
    {
        p_bc_source = &source_list[*dev_idx];
        if (p_bc_source->sync_handle != NULL)
        {
            *handle = p_bc_source->sync_handle;
            return true;
        }
        else
        {
            p_bc_source->used = false;
            memset(p_bc_source->advertiser_address, 0, GAP_BD_ADDR_LEN);
            return false;
        }
    }
    return false;
}

#endif

