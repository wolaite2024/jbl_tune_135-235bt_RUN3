/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DUAL_AUDIO_EFFECT_H_
#define _APP_DUAL_AUDIO_EFFECT_H_


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*dual audio effect:
**NO_EFFECT , MUSIC_MODE, GAMING_MODE, MOVIE_MODE, PARTY_MODE
*/

#define SITRONIX_SECP_INDEX            0x06
#define SPP_UPADTE_DUAL_AUDIO_EFFECT   0x0D
#define SPP_UPDATE_SECP_DATA           0x0F
#define SITRONIX_SECP_NUM              0x10
#define SITRONIX_SECP_SET_APP_VALUE    0x11
#define SITRONIX_SECP_GET_APP_VALUE    0x12
#define SITRONIX_SECP_SPP_RESTART      0x13
#define SITRONIX_SECP_SPP_RESTART_ACK  0x14
typedef enum
{
    VENDOR_MUSIC_MODE  = 0,
    VENDOR_GAMING_MODE = 1,
    VENDOR_MOVIE_MODE  = 2,
    VENDOR_PARTY_MODE  = 3,
    VENDOR_NONE_EFFECT = 5
} DUAL_EFFECT_TYPE;

typedef enum
{
    DUAL_EFFECT_CUR_NON   = 0,
    DUAL_EFFECT_CUR_AUDIO = 1,
} T_DUAL_EFFECT_CUR;

typedef enum
{
    DUAL_AUDIO_REMOTE_NO                     = 0x0000,
    DUAL_AUDIO_REMOTE_SYNC_PLAY              = 0x0001,
    DUAL_AUDIO_REMOTE_SEC_REQ                = 0x0002,
    DUAL_AUDIO_REMOTE_PRI_SYNC               = 0x0003,
    DUAL_AUDIO_REMOTE_SYNC_SECP_INIT         = 0x0004,
    DUAL_AUDIO_REMOTE_SYNC_SPP_DATA          = 0x0005,
    DUAL_AUDIO_REMOTE_SYNC_44_OFFSET         = 0x0006,
    DUAL_AUDIO_REMOTE_SYNC_DOWNSTREAM        = 0x0007,
    DUAL_AUDIO_REMOTE_SIGNAL_CTS             = 0x0008,
    DUAL_AUDIO_REMOTE_SYNC_APP_KEY_VAL       = 0x0009,
    DUAL_AUDIO_REMOTE_SYNC_SPP_RESTART       = 0x000C,
} T_DUAL_AUDIO_REMOTE_MSG;


typedef enum
{
    MODE_INFO_L_R_DIV2        = 0,
    MODE_INFO_L               = 1,
    MODE_INFO_R               = 2,
    MODE_INFO_LINE_IN         = 3,
} T_DUAL_EFFECT_MODE_INFO;
/*============================================================================*
 *                              Variables
 *============================================================================*/




/**
    * @brief  Avp init
    * @return void
    */
void dual_audio_effect_init(void);
void dual_audio_effect_reset(void);
void dual_audio_set_effect(DUAL_EFFECT_TYPE mode,  bool active);
void dual_audio_effect_switch(void);
void dual_audio_sync_info(void);
void dual_audio_restore(void);
bool dual_audio_get_a2dp_ignore(void);
void dual_audio_sync_data(uint8_t *buf, uint16_t len, T_DUAL_AUDIO_REMOTE_MSG msg);
void dual_audio_effect_hook_reg(void);
void dual_audio_spp_update_data(uint8_t *buf);
void dual_audio_effect_spp_init(uint16_t offset_44K, uint8_t app_idx);
void dual_audio_effect_request_data(uint8_t len);
void dual_audio_lr_info(T_DUAL_EFFECT_MODE_INFO parameter, bool action, uint8_t para_chanel_out);
void dual_audio_single_effect(void);
void dual_audio_effect_restart_track(void);
uint16_t dual_audio_get_sitronix_effect_num(void);
void dual_audio_sync_app_key_val(uint8_t *data, bool to_dsp);
void dual_audio_app_key_to_dsp(void);
void dual_audio_sync_all_app_key_val(bool to_dsp);
uint8_t dual_audio_get_mod_num(void);
void dual_audio_set_app_key_val(uint8_t index, uint16_t val);
uint16_t dual_audio_get_app_key_val(uint8_t index);
uint32_t dual_audio_save_app_key_val(void);
uint32_t dual_audio_load_app_key_val(void);
void dual_audio_set_report_para(uint8_t app_idx, uint8_t path);
void dual_audio_report_effect(void);
void dual_audio_report_app_key(uint16_t idx);
void dual_audio_effect_restart_data(void);
void dual_audio_effect_restart_ack(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
