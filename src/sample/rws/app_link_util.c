/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "remote.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_ble_gap.h"

#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xiaoai_transport.h"
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "app_xiaowei_transport.h"
#endif

#include "audio_track.h"
#include "eq.h"
#include "nrec.h"
#if (F_APP_SIDETONE_SUPPORT == 1)
#include "sidetone.h"
#endif

uint32_t app_connected_profiles(void)
{
    uint32_t i, connected_profiles = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        connected_profiles |= app_db.br_link[i].connected_profile;
    }
    return connected_profiles;
}


uint8_t app_connected_profile_link_num(uint32_t profile_mask)
{
    uint8_t i, link_number = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].connected_profile & profile_mask)
        {
            link_number++;
        }

    }
    return link_number;
}

uint8_t app_find_sco_conn_num(void)
{
    uint8_t i;
    uint8_t num = 0;
    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].sco_handle != 0)
        {
            num++;
        }
    }
    return num;
}

uint8_t app_find_a2dp_start_num(void)
{
    uint8_t i;
    uint8_t num = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true &&
            app_db.br_link[i].streaming_fg == true)
        {
            num++;
        }
    }

    return num;
}

T_APP_BR_LINK *app_find_br_link(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t        i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == true &&
                !memcmp(app_db.br_link[i].bd_addr, bd_addr, 6))
            {
                p_link = &app_db.br_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_APP_BR_LINK *app_find_br_link_by_tts_handle(T_TTS_HANDLE handle)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t        i;

    if (handle != NULL)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if ((app_db.br_link[i].used == true) &&
                (app_db.br_link[i].tts_info.tts_handle == handle))
            {
                p_link = &app_db.br_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_APP_BR_LINK *app_alloc_br_link(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t        i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_db.br_link[i].used == false)
            {
                p_link = &app_db.br_link[i];

                p_link->used = true;
                p_link->id   = i;
                memcpy(p_link->bd_addr, bd_addr, 6);
                break;
            }
        }
    }

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        if ((p_link != NULL) && (bd_addr != NULL))
        {
            app_xiaoai_alloc_br_link(bd_addr);
        }
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        if ((p_link != NULL) && (bd_addr != NULL))
        {
            app_xiaowei_alloc_br_link(bd_addr);
        }
    }
#endif

#if GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        if (p_link != NULL)
        {
            p_link->gfps_inuse_account_key = 0xff;
        }
    }
#endif

    return p_link;
}

bool app_free_br_link(T_APP_BR_LINK *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {
            if (p_link->tts_info.tts_frame_buf != NULL)
            {
                free(p_link->tts_info.tts_frame_buf);
            }

            if (p_link->p_gfps_cmd != NULL)
            {
                free(p_link->p_gfps_cmd);
            }
            if (p_link->sco_track_handle != NULL)
            {
                audio_track_release(p_link->sco_track_handle);
            }
            if (p_link->a2dp_track_handle != NULL)
            {
                audio_track_release(p_link->a2dp_track_handle);
            }

            if (p_link->eq_instance != NULL)
            {
                eq_release(p_link->eq_instance);
            }

#if (F_APP_VOICE_NREC_SUPPORT == 1)
            if (p_link->nrec_instance != NULL)
            {
                nrec_release(p_link->nrec_instance);
            }
#endif

#if (F_APP_SIDETONE_SUPPORT == 1)
            if (p_link->sidetone_instance != NULL)
            {
                sidetone_release(p_link->sidetone_instance);
            }
#endif

            memset(p_link, 0, sizeof(T_APP_BR_LINK));
            return true;
        }
    }

    return false;
}


T_APP_LE_LINK *app_find_le_link_by_conn_id(uint8_t conn_id)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].conn_id == conn_id)
        {
            p_link = &app_db.le_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_find_le_link_by_conn_handle(uint16_t conn_handle)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true &&
            app_db.le_link[i].conn_handle == conn_handle)
        {
            p_link = &app_db.le_link[i];
            break;
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_find_le_link_by_addr(uint8_t *bd_addr)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    if (bd_addr != NULL)
    {
        for (i = 0; i < MAX_BLE_LINK_NUM; i++)
        {
            if (app_db.le_link[i].used == true &&
                !memcmp(app_db.le_link[i].bd_addr, bd_addr, 6))
            {
                p_link = &app_db.le_link[i];
                break;
            }
        }
    }

    return p_link;
}

T_APP_LE_LINK *app_find_le_link_by_tts_handle(T_TTS_HANDLE handle)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    if (handle != NULL)
    {
        for (i = 0; i < MAX_BLE_LINK_NUM; i++)
        {
            if ((app_db.le_link[i].used == true) &&
                (app_db.le_link[i].tts_info.tts_handle == handle))
            {
                p_link = &app_db.le_link[i];
                break;
            }
        }
    }

    return p_link;
}


T_APP_LE_LINK *app_alloc_le_link_by_conn_id(uint8_t conn_id)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == false)
        {
            p_link = &app_db.le_link[i];

            p_link->used    = true;
            p_link->id      = i;
            p_link->conn_id = conn_id;
            break;
        }
    }

    return p_link;
}

bool app_free_le_link(T_APP_LE_LINK *p_link)
{
    if (p_link != NULL)
    {
        if (p_link->used == true)
        {
            if (p_link->tts_info.tts_frame_buf != NULL)
            {
                free(p_link->tts_info.tts_frame_buf);
            }
            if (p_link->p_embedded_cmd != NULL)
            {
                free(p_link->p_embedded_cmd);
            }

            if (p_link->audio_eq_info.eq_data_buf != NULL)
            {
                free(p_link->audio_eq_info.eq_data_buf);
            }

            while (p_link->disc_cb_list.count > 0)
            {
                T_LE_DISC_CB_ENTRY *p_entry;
                p_entry = os_queue_out(&p_link->disc_cb_list);
                if (p_entry)
                {
                    free(p_entry);
                }
            }
            memset(p_link, 0, sizeof(T_APP_LE_LINK));
            p_link->conn_id = 0xFF;
            return true;
        }
    }

    return false;
}

bool app_reg_le_link_disc_cb(uint8_t conn_id, P_FUN_LE_LINK_DISC_CB p_fun_cb)
{
    T_APP_LE_LINK *p_link = app_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        T_LE_DISC_CB_ENTRY *p_entry;
        for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
        {
            p_entry = os_queue_peek(&p_link->disc_cb_list, i);
            if (p_entry != NULL && p_entry->disc_callback == p_fun_cb)
            {
                return true;
            }
        }
        p_entry = calloc(1, sizeof(T_LE_DISC_CB_ENTRY));
        if (p_entry != NULL)
        {
            p_entry->disc_callback = p_fun_cb;
            os_queue_in(&p_link->disc_cb_list, p_entry);
            return true;
        }
    }
    return false;
}

uint8_t app_get_ble_link_num(void)
{
    uint8_t num = 0;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true)
        {
            num++;
        }
    }

    return num;
}

uint8_t app_get_ble_encrypted_link_num(void)
{
    uint8_t num = 0;
    uint8_t  i = 0;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used == true && app_db.le_link[i].encryption_status == LE_LINK_ENCRYPTIONED)
        {
            num++;
        }
    }

    return num;
}

bool app_check_b2b_link(uint8_t *bd_addr)
{
    bool ret = false;

    if (!memcmp(bd_addr, app_cfg_nv.bud_local_addr, 6) ||
        !memcmp(bd_addr, app_cfg_nv.bud_peer_addr, 6))
    {
        ret = true;
    }

    return ret;
}

bool app_check_b2b_link_by_id(uint8_t id)
{
    bool ret = false;

    if (app_db.br_link[id].used)
    {
        ret = app_check_b2b_link(app_db.br_link[id].bd_addr);
    }

    return ret;
}

bool app_check_b2s_link(uint8_t *bd_addr)
{
    return !app_check_b2b_link(bd_addr);
}

bool app_check_b2s_link_by_id(uint8_t id)
{
    bool ret = false;

    if (app_db.br_link[id].used)
    {
        ret = app_check_b2s_link(app_db.br_link[id].bd_addr);
    }

    return ret;
}

T_APP_BR_LINK *app_find_b2s_link(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = NULL;

    if (app_check_b2s_link(bd_addr))
    {
        p_link = app_find_br_link(bd_addr);
    }

    return p_link;
}

uint8_t app_find_b2s_link_num(void)
{
    uint8_t num = 0;
    uint8_t i = 0;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
        {
            if (app_db.br_link[i].used)
            {
                num++;
            }
        }
        else
        {
            if (app_check_b2s_link_by_id(i))
            {
                num++;
            }
        }
    }
    return num;
}

#if F_APP_LE_AUDIO_SM
T_APP_BR_LINK *app_find_b2s_link_by_index(uint8_t index)
{
    T_APP_BR_LINK *p_link = NULL;
    p_link = &app_db.br_link[index];
    return p_link;
}

void app_disallow_legacy_stream(bool para)
{
    app_db.disallow_sniff = para;
}

bool app_get_db(T_APP_GETDB para, uint8_t *buff)
{
    bool result = true;
    switch (para)
    {
    case APP_GETDB_GAMING:
        *buff = app_db.gaming_mode;
        break;

    case APP_GETDB_BAU:
        *buff = app_db.remote_is_8753bau;
        break;

    default:
        result = false;
        break;
    }
    return result;
}

T_APP_LE_LINK *app_find_le_link_by_index(uint8_t index)
{
    T_APP_LE_LINK *p_link = NULL;
    if (index >= MAX_BLE_LINK_NUM)
    {
        return p_link;
    }
    p_link = &app_db.le_link[index];
    return p_link;
}
#endif

