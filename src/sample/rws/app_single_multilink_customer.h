/**
 * @file   app_single_multilink_customer.h
 * @brief  struct and interface about teams rws for app
 * @author leon
 * @date   2021.8.26
 * @version 1.0
 * @par Copyright (c):
         Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _APP_TEAMS_MULTILINK_H_
#define _APP_TEAMS_MULTILINK_H_
#include "bt_hfp.h"
#include "app_multilink.h"
#include "btm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
/** @defgroup RWS_APP RWS APP
  * @brief Provides rws app interfaces.
  * @{
  */

/** @defgroup APP_TEAMS_MULTILINK APP TEAMS MULTILINK module init
  * @brief Provides teams multilink profile interfaces.
  * @{
  */

//#define APP_TEAMS_MAX_LINK_NUM  (MAX_BR_LINK_NUM + 1)   // addition link is usb, only support one usb link
#define APP_TEAMS_MAX_LINK_NUM  (MULTILINK_SRC_CONNECTED + 1)   // 2 is bt multilink num, 1 is usb link, only support one usb link 

#define APP_TEAMS_MULTILINK_USB_UPSTREAM_ACTIVE  (0x01)
#define APP_TEAMS_MULTILINK_USB_DOWNSTREAM_ACTIVE  (0x02)

#define APP_TEAMS_MULTILINK_MUSIC_PLAYING_CHECK_TIMEOUT  200

extern uint8_t multilink_usb_addr[6];
extern uint8_t multilink_usb_idx;

typedef enum
{
    T_APP_TEAMS_MULTILINK_APP_TYPE_VOICE,
    T_APP_TEAMS_MULTILINK_APP_TYPE_MUSIC,
    T_APP_TEAMS_MULTILINK_APP_TYPE_RECORD
} T_APP_TEAMS_MULTILINK_APP_TYPE;
typedef enum
{
    T_APP_TEAMS_MULTILINK_APP_VOICE_ACTIVE_IDX_CHANGE,
    T_APP_TEAMS_MULTILINK_APP_MUSIC_ACTIVE_IDX_CHANGE,
    T_APP_TEAMS_MULTILINK_APP_RECORD_ACTIVE_IDX_CHANGE,
    T_APP_TEAMS_MULTILINK_APP_VOICE_STATUS_CHANGE,  //device is call or device is not call
    T_APP_TEAMS_MULTILINK_APP_MUSIC_STATUS_CHANGE,  //device is music or device is not music
    T_APP_TEAMS_MULTILINK_APP_RECORD_STATUS_CHANGE,  //device is record or device is not record
    T_APP_TEAMS_MULTILINK_APP_LINK_STATUS_CHANGE
} T_APP_TEAMS_MULTILINK_APP_CHANGE_EVENT;

typedef enum
{
    T_APP_TEAMS_MULTILINK_APP_PRIORITY_HIGH_LEVEL,
    T_APP_TEAMS_MULTILINK_APP_PRIORITY_LOW_LEVEL
} T_APP_TEAMS_MULTILINK_APP_PRIORITY_LEVEL;

typedef enum
{
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_MUSIC,
    T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_RECORD
} T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_TYPE;

typedef enum
{
    T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_START,
    T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_STOP
} T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_ACTION_TYPE;

typedef enum
{
    APP_TEAMS_MULTILINK_VOICE_PRIORITY_LOW,
    APP_TEAMS_MULTILINK_VOICE_PRIORITY_MIDM,
    APP_TEAMS_MULTILINK_VOICE_PRIORITY_HIGH
} T_APP_TEAMS_MULTILINK_VOICE_PRIORITY_LEVEL;

typedef enum
{
    APP_TEAMS_MULTILINK_PRIORITY_UP,
    APP_TEAMS_MULTILINK_PRIORITY_DOWN
} T_APP_TEAMS_MULTILINK_PRIORITY_DIRECTION;

typedef enum
{
    APP_TEAMS_MULTILINK_USB_UPSTREAM_START,
    APP_TEAMS_MULTILINK_USB_UPSTREAM_STOP,
    APP_TEAMS_MULTILINK_USB_DOWNSTREAM_START,
    APP_TEAMS_MULTILINK_USB_DOWNSTREAM_STOP
} T_APP_TEAMS_MULTILINK_USB_STREAM_STATUS_EVENT;

typedef enum
{
    T_APP_TEAMS_MULTILINK_USB_INCOMING_RING,
    T_APP_TEAMS_MULTILINK_USB_OUTGOING_RING,
    T_APP_TEAMS_MULTILINK_USB_INVALID_RING
} T_APP_TEAMS_MULTILINK_USB_RING_TYPE;

typedef enum
{
    APP_TEAMS_MULTILINK_CHECK_MUSIC_PLAYING,
    APP_TEAMS_MULTILINK_HANDLE_USB_ALERT
} T_APP_TEAMS_MULTILINK_TIMER;

typedef struct
{
    uint8_t idx;
    T_BT_HFP_CALL_STATUS voice_status;
} T_APP_TEAMS_MULTILINK_APP_HIGH_PRIORITY_ARRAY_DATA;

typedef struct
{
    uint8_t idx;
    //music
    T_APP_JUDGE_A2DP_EVENT music_event;
    //record
    bool record_status;
} T_APP_TEAMS_MULTILINK_APP_LOW_PRIORITY_ARRAY_DATA;

typedef struct
{
    uint8_t idx;
    bool app_actived;
    uint8_t res[2];
    uint32_t connect_time;
    uint32_t app_end_time;
} T_APP_TEAMS_MULTILINK_PRIORITY_DATA;

void app_teams_multilink_handle_first_voice_profile_connect(uint8_t index);

void app_teams_multilink_handle_voice_profile_disconnect(uint8_t index);

void app_teams_multilink_voice_priority_rearrangment(uint8_t index,
                                                     T_BT_HFP_CALL_STATUS hfp_status);

void app_teams_multilink_record_priority_rearrangment(uint8_t index, bool record_status);

void app_teams_multilink_music_priority_rearrangment(uint8_t index, T_APP_JUDGE_A2DP_EVENT event);

void app_teams_multilink_high_app_action_trigger_low_app_action(uint8_t index,
                                                                T_APP_TEAMS_MULTILINK_HIGH_PRIORITY_APP_ACTION_TYPE action_type);

uint8_t app_teams_multilink_find_sco_conn_num(void);

void app_teams_multilink_set_active_voice_index(uint8_t index);

void app_teams_multilink_voice_track_start(uint8_t index);

void app_teams_multilink_record_track_start(uint8_t index);

uint8_t app_teams_multilink_get_active_voice_index(void);

uint8_t app_teams_multilink_get_active_music_index(void);

uint8_t app_teams_multilink_get_active_record_index(void);

bool app_teams_multiink_get_record_status_by_link_id(uint8_t index);

void app_teams_multilink_update_usb_stream_status(T_APP_TEAMS_MULTILINK_USB_STREAM_STATUS_EVENT
                                                  event);

uint8_t app_teams_multilink_get_active_index(void);

uint8_t app_teams_multilink_get_active_low_priority_app_index(void);

void app_teams_multilink_handle_link_connected(uint8_t index);

void app_teams_multilink_handle_link_disconnected(uint8_t index);

uint8_t app_teams_multilink_get_second_hfp_priority_index(void);

T_BT_HFP_CALL_STATUS app_teams_multilink_get_voice_status(void);

bool app_teams_multilink_check_device_music_is_playing(void);

bool app_teams_multilink_check_usb_record_data_stream_running(void);

bool app_teams_multilink_check_low_priority_app_start_condition(void);

void app_teams_multilink_init(void);

/** End of APP_TEAMS_MULTILINK
* @}
*/

/** End of RWS_APP
* @}
*/
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_TEAMS_MULTILINK_H_ */
