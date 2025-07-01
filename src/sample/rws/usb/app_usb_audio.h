/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_USB_AUDIO_H_
#define _APP_USB_AUDIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"
#include "ring_buffer.h"
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "audio_track.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_USB_AUDIO App Usb Audio
  * @brief modulization for usb plug or unplug module for simple application usage.
  * @{
  */

#define USB_AUDIO_DS_CHECK_INTERVAL     100

/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup APP_USB_AUDIO_Exported_Functions usb audio Exported Functions
    * @{
    */

/**
    * @brief  App check usb audio plug. And send io message to app for follow-up processing.
    * @param  audio_path indicate usb audio decode or encode path
    * @param  bit_res usb audio DA/AD sample depth
    * @param  sf usb audio DA/AD sampling frequency
    * @param  chann_mode usb audio channel mode, mono or stereo.
    * @return void
    */
void app_usb_audio_plug(uint8_t audio_path, uint8_t bit_res, uint8_t sf, uint8_t chann_mode);

/**
    * @brief  App check usb audio unplug. And send io message to app for follow-up processing.
    * @param  audio_path indicate usb audio decode or encode path
    * @return void
    */
void app_usb_audio_unplug(uint8_t audio_path);

/**
    * @brief  usb audio init, and register callback function.
    * @return void
    */
void app_usb_audio_init(void);

static uint16_t usb_audio_get_frame_size(uint8_t sf_idx, uint8_t bit_res, uint8_t chan_mode);
void app_usb_audio_volume_out_set(int16_t vol_param);
void app_usb_audio_volume_in_set(int16_t vol_param);
void app_usb_audio_volume_in_mute(void);
void app_usb_audio_volume_in_unmute(void);
uint8_t app_usb_audio_is_mic_mute(void);
bool app_usb_connected(void);
bool app_usb_audio_is_active(void);
bool key_action_switch_to_usb_mode(uint8_t *action);
void app_usb_charger_plug_handle_msg(T_IO_MSG *io_driver_msg_recv);
void app_usb_charger_unplug_handle_msg(T_IO_MSG *io_driver_msg_recv);
void app_usb_audio_mode_init(void);
void app_usb_audio_mode_deinit(void);
void app_usb_audio_ds_check_start_timer(uint16_t time_ms);
bool app_usb_audio_plug_in_power_on_check(void);
void app_usb_audio_plug_in_power_on_clear(void);
bool app_usb_audio_is_us_streaming(void);
bool app_usb_audio_is_ds_streaming(void);
bool app_usb_audio_ds_start(void);
bool app_usb_audio_us_start(void);
void app_usb_audio_start(void);
void app_usb_audio_stop(void);
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
T_AUDIO_TRACK_STATE app_teams_multilink_get_usb_ds_track_state(void);
#endif

/** @} */ /* End of group APP_USB_AUDIO_Exported_Functions */

/** @} */ /* End of group APP_USB_AUDIO */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_USB_AUDIO_H_ */
