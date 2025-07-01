/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights resestring */
#include <stdlib.h>
#include "string.h"
#include "os_mem.h"
#include "trace.h"
#include "bt_bond.h"
#include "btm.h"
#include "remote.h"
#include "gap_bond_legacy.h"
#include "app_main.h"
#include "app_cfg.h"
#include "app_bond.h"
#include "app_relay.h"
#if F_APP_TEAMS_BT_POLICY
#include "app_teams_bt_policy.h"
#endif

typedef struct t_link_record_item
{
    struct t_link_record_item *p_next;
    T_APP_LINK_RECORD record;
} T_APP_LINK_RECORD_ITEM;

typedef struct
{
    uint8_t bond_num;
    uint8_t rsvd[3];
    uint8_t link_record[0];
} T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT;

static T_OS_QUEUE sec_diff_link_record_list;

void app_bond_push_sec_diff_link_record(T_APP_LINK_RECORD *link_record)
{
    T_APP_LINK_RECORD_ITEM *record_item;
    T_APP_LINK_RECORD *record;

    record_item = calloc(1, sizeof(T_APP_LINK_RECORD_ITEM));
    if (record_item != NULL)
    {
        record = &record_item->record;

        memcpy(record, link_record, sizeof(T_APP_LINK_RECORD));
        os_queue_in(&sec_diff_link_record_list, record_item);

        APP_PRINT_TRACE8("app_bond_push_sec_diff_link_record: bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                         record->bond_flag, record->priority,
                         record->link_key[0], record->link_key[1],
                         record->link_key[2], record->bd_addr[0],
                         record->bd_addr[1], record->bd_addr[2]);
    }
}

uint8_t app_b2s_bond_num_get()
{
    uint8_t pair_idx;
    uint8_t bond_num;
    uint8_t bud_record_num = 0;
    uint8_t local_addr[6];
    uint8_t peer_addr[6];

    bond_num = bt_bond_num_get();

    remote_local_addr_get(local_addr);
    remote_peer_addr_get(peer_addr);
    if (bt_bond_index_get(local_addr, &pair_idx) == true)
    {
        bud_record_num++;
    }
    if (bt_bond_index_get(peer_addr, &pair_idx) == true)
    {
        bud_record_num++;
    }

    if (bond_num > bud_record_num)
    {
        return bond_num - bud_record_num;
    }

    return 0;
}

bool app_b2s_bond_addr_get(uint8_t priority, uint8_t *bd_addr)
{
    uint8_t i;
    uint8_t bond_num;
    uint8_t addr[6];
    uint8_t local_addr[6];
    uint8_t peer_addr[6];

    remote_local_addr_get(local_addr);
    remote_peer_addr_get(peer_addr);

    bond_num = bt_bond_num_get();
    for (i = 1; i <= bond_num; i++)
    {
        if (bt_bond_addr_get(i, addr) == true)
        {
            if (!memcmp(addr, local_addr, 6) || !memcmp(addr, peer_addr, 6))
            {
                priority++;
            }
            else
            {
                if (priority == i)
                {
                    memcpy(bd_addr, addr, 6);
                    return true;
                }
            }
        }
    }

    return false;
}

void app_adjust_b2b_bond_priority(void)
{
    uint8_t i;
    uint8_t max_bond_num;
    uint8_t addr[6];
    uint8_t local_addr[6];
    uint8_t peer_addr[6];

    remote_local_addr_get(local_addr);
    remote_peer_addr_get(peer_addr);

    max_bond_num = bt_max_bond_num_get();
    for (i = max_bond_num - 1; i <= max_bond_num; i++)
    {
        if (bt_bond_addr_get(max_bond_num, addr) == true)
        {
            if (!memcmp(addr, local_addr, 6) || !memcmp(addr, peer_addr, 6))
            {
                bt_bond_priority_set(addr);
            }
        }
    }
}

bool app_bond_key_set(uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type)
{
    app_adjust_b2b_bond_priority();
    return bt_bond_key_set(bd_addr, linkkey, key_type);
}

bool app_bond_get_pair_idx_mapping(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t pair_idx;

    if (bt_bond_index_get(bd_addr, &pair_idx) == false)
    {
        APP_PRINT_TRACE0("app_bond_get_pair_idx_mapping false");
        return false;
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        if (app_cfg_nv.app_pair_idx_mapping[i] == pair_idx)
        {
            *index = i;
            APP_PRINT_TRACE0("app_bond_get_pair_idx_mapping success");
            return true;
        }
    }

    return false;
}

bool app_bond_pop_sec_diff_link_record(uint8_t *bd_addr, T_APP_LINK_RECORD *link_record)
{
    bool ret = false;
    T_APP_LINK_RECORD_ITEM *record_item;
    T_APP_LINK_RECORD *record;
    uint8_t i = 0;

    record_item = os_queue_peek(&sec_diff_link_record_list, i++);
    while (record_item != NULL)
    {
        record = &record_item->record;

        if (!memcmp(bd_addr, record->bd_addr, 6))
        {
            ret = true;

            APP_PRINT_TRACE8("app_bond_pop_sec_diff_link_record: bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                             record->bond_flag, record->priority,
                             record->link_key[0], record->link_key[1],
                             record->link_key[2], record->bd_addr[0],
                             record->bd_addr[1], record->bd_addr[2]);

            memcpy(link_record, record, sizeof(T_APP_LINK_RECORD));

            os_queue_delete(&sec_diff_link_record_list, record_item);
            free(record_item);

            break;
        }

        record_item = os_queue_peek(&sec_diff_link_record_list, i++);
    }

    return ret;
}

void app_bond_clear_sec_diff_link_record(void)
{
    T_APP_LINK_RECORD_ITEM *record_item;
    T_APP_LINK_RECORD *record;

    record_item = os_queue_out(&sec_diff_link_record_list);

    while (record_item != NULL)
    {
        record = &record_item->record;

        APP_PRINT_TRACE8("app_bond_clear_sec_diff_link_record: bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                         record->bond_flag, record->priority,
                         record->link_key[0], record->link_key[1],
                         record->link_key[2], record->bd_addr[0],
                         record->bd_addr[1], record->bd_addr[2]);

        free(record_item);

        record_item = os_queue_out(&sec_diff_link_record_list);
    }
}

void app_bond_set_priority(uint8_t *bd_addr)
{
    uint8_t temp_addr[6];

    if (app_b2s_bond_addr_get(1, temp_addr) == true)
    {
        if (memcmp(bd_addr, temp_addr, 6))
        {
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_LINK_RECORD_PRIORITY_SET,
                                                bd_addr, 6);
            app_adjust_b2b_bond_priority();
            bt_bond_priority_set(bd_addr);
        }
    }
}

uint8_t app_get_b2s_link_record(T_APP_LINK_RECORD *link_record, uint8_t link_num)
{
    uint8_t i;
    uint8_t num = 0;
    uint8_t key_type;
    uint8_t key[16];
    uint8_t addr[6];
    uint32_t bond_flag = 0;

    /* collect the link record by priority */
    for (i = 0; i < link_num; i++)
    {
        if (app_b2s_bond_addr_get(i + 1, addr) == false)
        {
            continue;
        }

        bt_bond_flag_get(addr, &bond_flag);

        if (bt_bond_key_get(addr, key, &key_type) == false)
        {
            continue;
        }

        link_record[num].key_type = key_type;
        link_record[num].bond_flag = bond_flag;
        link_record[num].priority = i + 1;
        memcpy(link_record[num].bd_addr, addr, 6);
        memcpy(link_record[num].link_key, key, 16);
        num++;
    }

    return num;
}

bool app_is_b2s_link_record_exist(void)
{
    uint8_t bond_num = app_b2s_bond_num_get();
    T_APP_LINK_RECORD *link_record;

    link_record = malloc(sizeof(T_APP_LINK_RECORD) * bond_num);
    if (link_record != NULL)
    {
        bond_num = app_get_b2s_link_record(link_record, bond_num);
        free(link_record);
    }

    return bond_num ? true : false;
}

bool app_sync_b2s_link_record(void)
{
    T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT *p_record;
    uint8_t bond_num;
    bool ret = false;

    bond_num = app_b2s_bond_num_get();

    p_record = malloc(sizeof(T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT) + bond_num * sizeof(
                          T_APP_LINK_RECORD));

    if (p_record != NULL)
    {
        p_record->bond_num = app_get_b2s_link_record((T_APP_LINK_RECORD *)p_record->link_record, bond_num);
        ret = app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_DEVICE, APP_REMOTE_MSG_LINK_RECORD_XMIT,
                                                  (uint8_t *)p_record, sizeof(T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT) + p_record->bond_num *
                                                  sizeof(T_APP_LINK_RECORD));
        free(p_record);
    }

    return ret;
}

void app_handle_remote_link_record_xmit(void *buf)
{
    T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT *peer_link_record;
    T_APP_LINK_RECORD *local_link_record;
    T_APP_LINK_RECORD *pri_link_record;
    T_APP_LINK_RECORD *sec_link_record;
    T_APP_LINK_RECORD *merge_link_record;
    uint8_t pri_link_record_num = 0;
    uint8_t sec_link_record_num = 0;
    uint8_t merge_link_record_num = 0;
    uint8_t local_bond_num, max_bond_num;
    uint32_t bond_flag = 0;
    int i, j;

    app_bond_clear_sec_diff_link_record();

    peer_link_record = (T_APP_REMOTE_MSG_PAYLOAD_LINK_RECORD_XMIT *)buf;
    if (peer_link_record->bond_num == 0)
    {
        return;
    }

    local_bond_num = app_b2s_bond_num_get();
    if (local_bond_num == 0)
    {
        merge_link_record = (T_APP_LINK_RECORD *)peer_link_record->link_record;
        for (i = peer_link_record->bond_num; i > 0; i--)
        {
            app_bond_key_set(merge_link_record[i - 1].bd_addr, merge_link_record[i - 1].link_key,
                             merge_link_record[i - 1].key_type);
            bt_bond_flag_set(merge_link_record[i - 1].bd_addr, merge_link_record[i - 1].bond_flag);
        }

        return;
    }

    local_link_record = malloc(sizeof(T_APP_LINK_RECORD) * local_bond_num);
    if (local_link_record != NULL)
    {
        local_bond_num = app_get_b2s_link_record(local_link_record, local_bond_num);
        if (local_bond_num == 0)
        {
            free(local_link_record);
            return;
        }
    }
    else
    {
        return;
    }

    max_bond_num = bt_max_bond_num_get();

    if (max_bond_num > 2)
    {
        max_bond_num = max_bond_num - 2;
    }
    else
    {
        max_bond_num = 0;
    }

    if (max_bond_num == 0)
    {
        free(local_link_record);
        return;
    }

    merge_link_record = malloc(sizeof(T_APP_LINK_RECORD) * max_bond_num);
    if (merge_link_record == NULL)
    {
        free(local_link_record);
        return;
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        pri_link_record = local_link_record;
        sec_link_record = (T_APP_LINK_RECORD *)peer_link_record->link_record;
        pri_link_record_num = local_bond_num;
        sec_link_record_num = peer_link_record->bond_num;
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        pri_link_record = (T_APP_LINK_RECORD *)peer_link_record->link_record;
        sec_link_record = local_link_record;
        pri_link_record_num = peer_link_record->bond_num;
        sec_link_record_num = local_bond_num;
    }
    else
    {
        free(local_link_record);
        free(merge_link_record);
        return;
    }

    /* copy all pri's link record to merge_link_record */
    for (i = 0; i < pri_link_record_num; i++)
    {
        memcpy(&merge_link_record[merge_link_record_num++], &pri_link_record[i], sizeof(T_APP_LINK_RECORD));
    }

    /* copy sec's link record which is not repeated & high prority to merge_link_record*/
    for (i = 0; i < sec_link_record_num && merge_link_record_num < max_bond_num; i++)
    {
        for (j = 0; j < merge_link_record_num; j++)
        {
            if (memcmp(merge_link_record[j].bd_addr, sec_link_record[i].bd_addr, 6) == 0)
            {
                if (memcmp(&merge_link_record[j], &sec_link_record[i], sizeof(T_APP_LINK_RECORD)))
                {
                    app_bond_push_sec_diff_link_record(&sec_link_record[i]);
                }

                break;
            }
        }

        if (j == merge_link_record_num)
        {
            memcpy(&merge_link_record[merge_link_record_num++], &sec_link_record[i], sizeof(T_APP_LINK_RECORD));
        }
    }

    /* save link record */
    for (i = merge_link_record_num; i > 0 ; i--)
    {
        j = local_bond_num;

        if (bt_bond_flag_get(merge_link_record[i - 1].bd_addr, &bond_flag) == true)
        {
            for (j = 0; j < local_bond_num; j++)
            {
                if ((memcmp(merge_link_record[i - 1].bd_addr, local_link_record[j].bd_addr, 6) == 0) &&
                    (memcmp(merge_link_record[i - 1].link_key, local_link_record[j].link_key, 6) == 0) &&
                    (merge_link_record[i - 1].key_type == local_link_record[j].key_type) &&
                    (merge_link_record[i - 1].bond_flag == local_link_record[j].bond_flag))
                {
                    app_adjust_b2b_bond_priority();
                    bt_bond_priority_set(local_link_record[j].bd_addr);
                    break;
                }
            }
        }

        if (j == local_bond_num)
        {
            app_bond_key_set(merge_link_record[i - 1].bd_addr, merge_link_record[i - 1].link_key,
                             merge_link_record[i - 1].key_type);
            bt_bond_flag_set(merge_link_record[i - 1].bd_addr, merge_link_record[i - 1].bond_flag);
        }
    }

    for (i = 0; i < merge_link_record_num; i++)
    {
        APP_PRINT_TRACE8("app_handle_remote_link_record_xmit: dump link info, bond %x, priority %x, link_key %x %x %x, bd %x %x %x",
                         merge_link_record[i].bond_flag, merge_link_record[i].priority,
                         merge_link_record[i].link_key[0], merge_link_record[i].link_key[1],
                         merge_link_record[i].link_key[2], merge_link_record[i].bd_addr[0],
                         merge_link_record[i].bd_addr[1], merge_link_record[i].bd_addr[2]);
    }

    free(local_link_record);
    free(merge_link_record);
}

void app_handle_remote_link_record_msg(uint16_t msg, void *buf)
{
    switch (msg)
    {
    case APP_REMOTE_MSG_LINK_RECORD_ADD:
        {
            T_APP_REMOTE_MSG_PAYLOAD_LINK_KEY_ADDED *p_info;

            p_info = (T_APP_REMOTE_MSG_PAYLOAD_LINK_KEY_ADDED *)buf;
            app_bond_key_set(p_info->bd_addr, p_info->link_key, p_info->key_type);

            uint8_t mapping_idx;
            app_bt_update_pair_idx_mapping();
            if (app_bond_get_pair_idx_mapping(p_info->bd_addr, &mapping_idx))
            {
                app_cfg_nv.audio_gain_level[mapping_idx] = app_cfg_const.playback_volume_default;
                app_cfg_nv.voice_gain_level[mapping_idx] = app_cfg_const.voice_out_volume_default;
            }
        }
        break;

    case APP_REMOTE_MSG_LINK_RECORD_DEL:
        {
            uint8_t *bd_addr = (uint8_t *)buf;

            bt_bond_delete(bd_addr);
        }
        break;

    case APP_REMOTE_MSG_LINK_RECORD_XMIT:
        {
            app_handle_remote_link_record_xmit(buf);
            app_bt_update_pair_idx_mapping();
        }
        break;

    case APP_REMOTE_MSG_LINK_RECORD_PRIORITY_SET:
        {
            uint8_t *bd_addr = (uint8_t *)buf;

            app_adjust_b2b_bond_priority();
            bt_bond_priority_set(bd_addr);
        }
        break;

    default:
        break;
    }
}

void app_handle_remote_profile_connected_msg(void *buf)
{
    T_APP_REMOTE_MSG_PAYLOAD_PROFILE_CONNECTED  *p_msg;

    p_msg = (T_APP_REMOTE_MSG_PAYLOAD_PROFILE_CONNECTED *)buf;

    switch (p_msg->prof_mask)
    {
    case A2DP_PROFILE_MASK:
        bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_A2DP);
        break;

    case HFP_PROFILE_MASK:
        {
            bt_bond_flag_remove(p_msg->bd_addr, BOND_FLAG_HSP);
            bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_HFP);
        }
        break;

    case HSP_PROFILE_MASK:
        {
            bt_bond_flag_remove(p_msg->bd_addr, BOND_FLAG_HFP);
            bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_HSP);
        }
        break;

    case SPP_PROFILE_MASK:
        bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_SPP);
        break;

    case PBAP_PROFILE_MASK:
        bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_PBAP);
        break;

    case IAP_PROFILE_MASK:
        bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_IAP);
        break;
    /*
    case DID_PROFILE_MASK:
        bt_bond_flag_add(p_msg->bd_addr, BOND_FLAG_DONGLE); //reserve for keeping dongle link record
        break;
    */

    default:
        break;
    }
}

void app_bond_clear_non_rws_keys(void)
{
    uint8_t i;
    uint8_t bond_num;
    T_APP_LINK_RECORD *link_record;
#if F_APP_TEAMS_BT_POLICY
    T_TEAMS_DEVICE_TYPE device_type;
#endif

    bond_num = app_b2s_bond_num_get();

    link_record = malloc(sizeof(T_APP_LINK_RECORD) * bond_num);
    if (link_record != NULL)
    {
        bond_num = app_get_b2s_link_record(link_record, bond_num);

        for (i = 0; i < bond_num; i++)
        {
#if F_APP_TEAMS_BT_POLICY
            device_type = app_bt_policy_get_cod_type_by_addr(link_record[i].bd_addr);
            if (device_type == T_TEAMS_DEVICE_DONGLE_TYPE)
            {
                continue;
            }
#endif
#if (F_APP_DONGLE_FEATURE_SUPPORT == 1)
            /*
            if(link_record[i].bond_flag & BOND_FLAG_DONGLE)
            {
                continue; //reserve for keeping dongle link record
            }
            */
#endif
            bt_bond_delete(link_record[i].bd_addr);
        }
    }
    free(link_record);
}

void app_bond_init(void)
{
    os_queue_init(&sec_diff_link_record_list);
}
