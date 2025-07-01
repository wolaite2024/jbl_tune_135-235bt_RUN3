/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_LINK_UTIL_H_
#define _APP_LINK_UTIL_H_

#include <stdint.h>
#include <stdbool.h>
#include "tts.h"
#include "btm.h"
#include "os_queue.h"
#include "audio_type.h"
#include "app_bt_sniffing.h"
#include "app_avrcp.h"
#include "app_eq.h"
#include "app_ble_tts.h"
#if F_APP_LE_AUDIO_SM
#include "audio_track.h"
#include "app_le_audio_mgr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_LINK App Link
  * @brief App Link
  * @{
  */

/** max BR/EDR link num */
#define MAX_BR_LINK_NUM                 4

/** max BLE link num */
#define MAX_BLE_LINK_NUM                2

#if F_APP_MCP_SUPPORT
/** max BLE ASE link num */
#define ASCS_ASE_NUM 3
#endif
#if F_APP_CCP_SUPPORT
/** max BLE CCP call list num */
#define CCP_CALL_LIST_NUM               2
#endif

/** bitmask of profiles */
#define A2DP_PROFILE_MASK               0x00000001    /**< A2DP profile bitmask */
#define AVRCP_PROFILE_MASK              0x00000002    /**< AVRCP profile bitmask */
#define HFP_PROFILE_MASK                0x00000004    /**< HFP profile bitmask */
#define RDTP_PROFILE_MASK               0x00000008    /**< Remote Control vendor profile bitmask */
#define SPP_PROFILE_MASK                0x00000010    /**< SPP profile bitmask */
#define IAP_PROFILE_MASK                0x00000020    /**< iAP profile bitmask */
#define PBAP_PROFILE_MASK               0x00000040    /**< PBAP profile bitmask */
#define HSP_PROFILE_MASK                0x00000080    /**< HSP profile bitmask */
#define HID_PROFILE_MASK                0x00000100    /**< HID profile bitmask */
#define MAP_PROFILE_MASK                0x00000200    /**< MAP profile bitmask */
#define GATT_PROFILE_MASK               0x00008000    /**< GATT profile bitmask */
#define GFPS_PROFILE_MASK               0x00010000    /**< GFPS profile bitmask */
#define XIAOAI_PROFILE_MASK             0x00020000    /**< XIAOAI profile bitmask */
#define AMA_PROFILE_MASK                0x00040000    /**< AMA profile bitmask */
#define AVP_PROFILE_MASK                0x00080000    /**< AVP profile bitmask */
#define DID_PROFILE_MASK                0x80000000    /**< DID profile bitmask */

#define ALL_PROFILE_MASK                0xffffffff

#define MAX_ADV_SET_NUMBER              6

/**  @brief iap data session status */
typedef enum
{
    DATA_SESSION_CLOSE = 0x00,
    DATA_SESSION_LAUNCH = 0x01,
    DATA_SESSION_OPEN = 0x02,
} T_DATA_SESSION_STATUS;

typedef enum
{
    APP_HF_STATE_STANDBY = 0x00,
    APP_HF_STATE_CONNECTED = 0x01,
} T_APP_HF_STATE;

typedef enum
{
    APP_REMOTE_DEVICE_UNKNOWN = 0x00,
    APP_REMOTE_DEVICE_IOS     = 0x01,
    APP_REMOTE_DEVICE_OTHERS  = 0x02,
} T_APP_REMOTE_DEVICE_VEDDOR_ID;

#if F_APP_LE_AUDIO_SM
typedef enum
{
    APP_GETDB_GAMING = 0x01,
    APP_GETDB_BAU    = 0x02,
    APP_GETDB_MAX,
} T_APP_GETDB;
#endif
/**  @brief  APP's Bluetooth BR/EDR link connection database */
typedef struct
{
    void                *timer_handle_role_switch;
    void                *timer_handle_later_avrcp;
    void                *timer_handle_check_role_switch;
    void                *timer_handle_later_hid;

    uint8_t             bd_addr[6];
    bool                used;
    uint8_t             id;

    uint8_t            *p_embedded_cmd;
    uint8_t            *uart_rx_dt_pkt_ptr;

    T_TTS_INFO          tts_info;

    T_AUDIO_EQ_INFO     audio_eq_info;

    T_AUDIO_EFFECT_INSTANCE eq_instance;
    T_AUDIO_EFFECT_INSTANCE nrec_instance;

    uint16_t            embedded_cmd_len;
    uint16_t            uart_rx_dt_pkt_len;

    struct
    {
        uint8_t             credit;
        uint8_t             authen_flag;
        uint16_t            frame_size;
        struct
        {
            uint8_t         session_status;
            uint16_t        session_id;
        } rtk;
    } iap;



    uint16_t            rfc_frame_size;
    uint16_t            rfc_spp_frame_size;

    uint8_t             rfc_spp_credit;
    uint8_t             rfc_credit;


    uint32_t            connected_profile;

    bool                call_id_type_check;
    bool                call_id_type_num;
    uint8_t             tx_event_seqn;
    uint8_t             rx_cmd_seqn;
    uint8_t             resume_fg;
    uint8_t             call_status;
    bool                service_status;
    uint8_t             a2dp_codec_type;

    union
    {
        uint32_t sampling_frequency;
        struct
        {
            uint32_t sampling_frequency;
            uint8_t channel_mode;
            uint8_t block_length;
            uint8_t subbands;
            uint8_t allocation_method;
            uint8_t min_bitpool;
            uint8_t max_bitpool;
        } sbc;
        struct
        {
            uint32_t sampling_frequency;
            uint32_t bit_rate;
            uint8_t  object_type;
            uint8_t  channel_number;
            bool     vbr_supported;
        } aac;
        struct
        {
            uint8_t sampling_frequency;
            uint8_t channel_mode;
        } ldac;
        struct
        {
            uint8_t  info[12];
        } vendor;
    } a2dp_codec_info;

    void                *a2dp_track_handle;
    void                *sco_track_handle;

    uint8_t             sco_seq_num;
    uint8_t             duplicate_fst_sco_data;
    T_BT_LINK_ROLE      acl_link_role;
    uint8_t             pbap_repos;

    uint16_t            acl_handle;
    uint16_t            sco_handle;

    bool                sco_initial;
    uint8_t             streaming_fg;
    uint8_t             avrcp_play_status;
    bool                is_inband_ring;

    uint8_t             *p_gfps_cmd;

    uint16_t            gfps_cmd_len;
    uint8_t             gfps_rfc_chann;
    uint8_t             gfps_session_nonce[8];
    uint8_t             gfps_inuse_account_key;
    bool                auth_flag;

    // 0 not encrpyt; 1 encrypted; 2 de-encrpyted (only for b2b link)
    uint8_t             link_encrypt;
    uint8_t             link_role_switch_count;
    T_BT_SNIFFING_TYPE  sniffing_type;
    bool                sniffing_active;
    T_APP_BT_SNIFFING_STATE     bt_sniffing_state;
    T_APP_BT_SNIFFING_STATE     sniffing_type_before_roleswap;
    bool                pending_ind_confirm;
    T_APP_HF_STATE      app_hf_state;

    uint16_t            src_conn_idx;
    uint16_t            remote_hfp_brsf_capability;
    T_APP_REMOTE_DEVICE_VEDDOR_ID  remote_device_vendor_id;
    uint16_t            remote_device_vendor_version;

    uint8_t             disconn_acl_flg: 1;
    uint8_t             acl_link_in_sniffmode_flg: 1;
    uint8_t             roleswitch_check_after_unsniff_flg: 1;
    uint8_t             set_a2dp_fake_lv0_gain: 1;
    uint8_t             rtk_vendor_spp_active: 1;
    uint8_t             b2s_connected_vp_is_played : 1;
    uint8_t             reserved: 2;
    uint16_t            sniff_mode_disable_flag;

    bool                stop_after_shadow;
    uint8_t             tpoll_status;
    T_APP_ABS_VOL_STATE abs_vol_state;
    bool                cmd_set_enable;
    bool                acl_decrypted;

    bool                voice_muted;
    bool                playback_muted;
    bool                audio_eq_enabled;

#if F_APP_TEAMS_GLOBAL_MUTE_SUPPORT
    bool                is_mute;
#endif

#if (F_APP_SIDETONE_SUPPORT == 1)
    T_AUDIO_EFFECT_INSTANCE sidetone_instance;
#endif
    bool                connected_by_linkback;
    bool                harman_silent_check;

#if F_APP_VOICE_SPK_EQ_SUPPORT
    T_AUDIO_EFFECT_INSTANCE voice_mic_eq_instance;
    T_AUDIO_EFFECT_INSTANCE voice_spk_eq_instance;
#endif

    uint8_t             device_name_crc[2];
} T_APP_BR_LINK;

typedef void(*P_FUN_LE_LINK_DISC_CB)(uint8_t conn_id, uint8_t local_disc_cause,
                                     uint16_t disc_cause);

typedef struct t_le_disc_cb_entry
{
    struct t_le_disc_cb_entry  *p_next;
    P_FUN_LE_LINK_DISC_CB disc_callback;
} T_LE_DISC_CB_ENTRY;

#if F_APP_CCP_SUPPORT
typedef struct T_BLE_ASE_LINK
{
    bool    used;
    uint8_t ase_id;
    uint16_t conn_handle;
    uint16_t cis_conn_handle;
    uint16_t audio_context;
    uint8_t frame_num;
    uint8_t handshake_fg;
    T_AUDIO_TRACK_STATE state;
    T_AUDIO_TRACK_HANDLE handle;
    T_CODEC_CFG codec_cfg;
    uint32_t presentation_delay;
} T_BLE_ASE_LINK;


typedef struct t_ble_call_list
{
    bool    used;
    uint8_t call_index;
    uint8_t call_state;
} T_BLE_CALL_LINK;
#endif

/**  @brief  App define le link connection database */
typedef struct
{
    uint8_t             bd_addr[6];
    bool                used;
    uint8_t             id;

    uint8_t            *p_embedded_cmd;

    T_TTS_INFO          tts_info;

    T_AUDIO_EQ_INFO     audio_eq_info;

    uint16_t            embedded_cmd_len;
    uint16_t            mtu_size;
    uint16_t            conn_handle;

    uint8_t             state;
    uint8_t             conn_id;
    uint8_t             rx_cmd_seqn;
    uint8_t             tx_event_seqn;

    T_OS_QUEUE          disc_cb_list;

    uint8_t             transmit_srv_tx_enable_fg;
    uint8_t             local_disc_cause;
    uint8_t             encryption_status;
    bool                cmd_set_enable;
#if F_APP_LE_AUDIO_SM
    uint8_t             link_state;
    uint8_t             media_state;
    T_BLE_ASE_LINK      ble_ase_link[ASCS_ASE_NUM];
    bool                general_mcs;
    T_BLE_CALL_LINK     ble_call_link[CCP_CALL_LIST_NUM];
    T_BT_CCP_CALL_STATUS call_status;
    bool                general_tbs;
    uint8_t             active_call_index;
    uint8_t             *call_uri;
#endif
#if F_APP_VCS_SUPPORT
    uint8_t             volume_setting;
    uint8_t             mute;
#endif
#if F_APP_AICS_SUPPORT
    int8_t              gain_setting;
#endif
#if BT_GATT_CLIENT_SUPPORT
    bool                mtu_received;
    bool                auth_cmpl;
#endif
} T_APP_LE_LINK;

uint32_t app_connected_profiles(void);

/**
    * @brief  get BR/EDR link num wich connected with phone by the mask profile
    * @param  profile_mask the mask profile
    * @return BR/EDR link num
    */
uint8_t app_connected_profile_link_num(uint32_t profile_mask);

/**
    * @brief  get current SCO connected link number
    * @param  void
    * @return SCO connected number
    */
uint8_t app_find_sco_conn_num(void);

/**
    * @brief  get current a2dp start number
    * @param  void
    * @return a2dp start number
    */
uint8_t app_find_a2dp_start_num(void);

/**
    * @brief  get the BR/EDR link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_find_br_link(uint8_t *bd_addr);

/**
    * @brief  get the BR/EDR link by tts handle
    * @param  handle tts handle
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_find_br_link_by_tts_handle(T_TTS_HANDLE handle);

/**
    * @brief  alloc the BR/EDR link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_alloc_br_link(uint8_t *bd_addr);

/**
    * @brief  free the BR/EDR link
    * @param  p_link the BR/EDR link
    * @return true: success; false: fail
    */
bool app_free_br_link(T_APP_BR_LINK *p_link);

/**
    * @brief  find the BLE link by connected id
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_find_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  find the BLE link by connected handle
    * @param  conn_handle BLE connection handle
    * @return the BLE link
    */
T_APP_LE_LINK *app_find_le_link_by_conn_handle(uint16_t conn_handle);

/**
    * @brief  find the BLE link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BLE link
    */
T_APP_LE_LINK *app_find_le_link_by_addr(uint8_t *bd_addr);

/**
    * @brief  find the BLE link by tts handle
    * @param  handle tts handle
    * @return the BLE link
    */
T_APP_LE_LINK *app_find_le_link_by_tts_handle(T_TTS_HANDLE handle);

/**
    * @brief  alloc the BLE link by link id(slot)
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_alloc_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  free the BLE link
    * @param  p_link the BLE link
    * @return true: success; false: fail
    */
bool app_free_le_link(T_APP_LE_LINK *p_link);

bool app_reg_le_link_disc_cb(uint8_t conn_id, P_FUN_LE_LINK_DISC_CB p_fun_cb);

uint8_t app_get_ble_link_num(void);
uint8_t app_get_ble_encrypted_link_num(void);
/**
    * @brief  judge if the link is bud2bud link
    * @param  bd_addr bluetooth address
    * @return true/false
    */
bool app_check_b2b_link(uint8_t *bd_addr);

bool app_check_b2b_link_by_id(uint8_t id);

/**
    * @brief  judge if the link is bud2phone link
    * @param  bd_addr bluetooth address
    * @return true/false
    */
bool app_check_b2s_link(uint8_t *bd_addr);

bool app_check_b2s_link_by_id(uint8_t id);

T_APP_BR_LINK *app_find_b2s_link(uint8_t *bd_addr);

/**
    * @brief  get the bud2phone link num
    * @param  void
    * @return link num
    */
uint8_t app_find_b2s_link_num(void);

#if F_APP_LE_AUDIO_SM
T_APP_BR_LINK *app_find_b2s_link_by_index(uint8_t index);
T_APP_LE_LINK *app_find_le_link_by_index(uint8_t index);
void app_disallow_legacy_stream(bool para);
bool app_get_db(T_APP_GETDB para, uint8_t *buff);
#endif
/** End of APP_LINK
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_LINK_UTIL_H_ */
