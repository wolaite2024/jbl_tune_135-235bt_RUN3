/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_SASS_POLICY_H_
#define _APP_SASS_POLICY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SASS_PAGESCAN_WINDOW                0x12
#define SASS_PAGESCAN_INTERVAL              0x400

#define SWITCH_RET_OK               0x00
#define SWITCH_RET_FAILED           0x01
#define SWITCH_RET_REDUNDANT        0x02

typedef enum
{
    CONNECTION_STATE_NO_CONNECTION = 0x00,
    CONNECTION_STATE_PAGING = 0x01,
    CONNECTION_STATE_CONNECTED_WITH_NO_DATA_TRANSFERRING = 0x02,
    CONNECTION_STATE_NON_AUDIO_DATA_TRANSFERING = 0x03,
    CONNECTION_STATE_A2DP_STREAMING_ONLY = 0x04,
    CONNECTION_STATE_A2DP_STREAMING_WITH_AVRCP = 0x05,
    CONNECTION_STATE_HFP_STREAMING = 0x06,
    CONNECTION_STATE_LE_AUDIO_MEDIA_STREAMING_WITHOUT_CONTROL = 0x07,
    CONNECTION_STATE_LE_AUDIO_MEDIA_STREAMING_WITH_CONTROL = 0x08,
    CONNECTION_STATE_LE_AUDIO_CALL_STREAMING = 0x09,
    CONNECTION_STATE_LE_AUDIO_BIS    = 0x0A,
    CONNECTION_STATE_DISABLE_CONNECTION_SWITCH = 0x0F,
} T_MULTILINK_CONNECTION_STATE;

void app_sass_policy_sync_bitmap(uint8_t bitmap);
uint8_t app_sass_policy_get_raw_bitmap(void);
uint8_t app_sass_policy_get_multipoint_state(void);
void app_sass_policy_set_multipoint_state(uint8_t enable);
bool app_sass_src_disc_handle(void);
void app_sass_link_switch(uint8_t *bd_addr, bool resume);
bool app_sass_judge_enter_pair(void);
void app_sass_conn_finish_post_handle(void);
bool app_sass_conn_finish_pre_handle(bool is_link_back);
void app_sass_set_acl_switch_para(uint8_t *addr);
uint8_t app_sass_policy_switch_active_link(uint8_t *bd_addr, uint8_t switch_flag, bool self);
void app_sass_record_prev_con_addr(uint8_t *con_bd_addr);
void app_sass_record_prev_dis_addr(uint8_t *con_bd_addr);
void app_sass_record_prev_addr(uint8_t *con_bd_addr);
void app_sass_policy_switch_back(uint8_t event);
uint8_t app_sass_policy_get_disc_link(void);
void app_sass_policy_link_back_end(void);
void app_sass_policy_profile_conn_handle(uint8_t id);
void app_sass_policy_init(void);
void app_sass_policy_initial_conn_handle(uint8_t *addr, bool sass_init);
uint8_t app_sass_policy_get_conn_bit_map(void);
void app_sass_policy_reset(void);
bool app_sass_policy_is_sass_device(uint8_t *addr);
bool app_sass_policy_support_easy_connection_switch(void);
bool app_sass_policy_support_common_used_device_policy(void);
bool app_sass_policy_is_target_drop_device(uint8_t idx);
void app_sass_policy_set_switch_preference(uint8_t flag);
uint8_t app_sass_policy_get_switch_preference(void);
void app_sass_policy_set_switch_setting(uint8_t flag);
uint8_t app_sass_policy_get_switch_setting(void);
void app_sass_policy_set_support(uint8_t *addr);
T_MULTILINK_CONNECTION_STATE app_sass_policy_get_connection_state(void);
bool app_sass_policy_get_org_enable_multi_link(void);

#ifdef __cplusplus
}
#endif

#endif

