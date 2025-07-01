/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_EQ_H_
#define _APP_EQ_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"
#include "eq.h"
#include "eq_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EQ_START_FRAME_HEADER_LEN       0x09
#define EQ_SINGLE_FRAME_HEADER_LEN      0x07
#define EQ_CONTINUE_FRAME_HEADER_LEN    0x07
#define EQ_END_FRAME_HEADER_LEN         0x07
#define EQ_ABORT_FRAME_HEADER_LEN       0x07

#define EQ_INIT_SEQ                     0x00
#define EQ_SDK_HEADER_SIZE              0x04
#define EQ_HEADER_SIZE                  0x04
#define EQ_INFO_HEADER_SIZE             0x04

#define MCU_TO_SDK_CMD_HDR_SIZE          4
#define PUBLIC_VALUE_SIZE                4
#define STAGE_NUM_SIZE                   20
#define EXT_STAGE_NUM_SIZE               8
#define EXT_PUB_VALUE_SIZE               4
#define EQ_DATA_REV                      2
#define EQ_DATA_TAIL                     2

// for EQ setting
#define BOTH_SIDE_ADJUST        0x02

typedef struct t_audio_eq_info
{
    uint8_t eq_idx;
    uint8_t sw_eq_type;
    uint8_t eq_mode;
    uint8_t eq_seq;

    uint16_t eq_data_offset;
    uint16_t eq_data_len;

    uint8_t *eq_data_buf;
} T_AUDIO_EQ_INFO;

typedef struct t_audio_eq_report_data
{
    T_AUDIO_EQ_INFO *eq_info;

    uint8_t id;
    uint8_t cmd_path;
    uint16_t max_frame_len;
} T_AUDIO_EQ_REPORT_DATA;

typedef struct t_eq_enable_info
{
    T_AUDIO_EFFECT_INSTANCE  instance;
    bool                     is_enable;
    uint8_t                  idx;
} T_EQ_ENABLE_INFO;

typedef enum
{
    EQ_INDEX_REPORT_BY_PLAYING,
    EQ_INDEX_REPORT_BY_CHANGE_MODE,
    EQ_INDEX_REPORT_BY_SWITCH_EQ_INDEX,
    EQ_INDEX_REPORT_BY_GET_EQ_INDEX,
} T_EQ_DATA_UPDATE_EVENT;

/** @defgroup APP_EQ App EQ param report
  * @brief App EQ param report
  * @{
  */

/**
 *
 * \brief   Eq param get.
 *
 */

uint16_t app_eq_dsp_param_get(T_EQ_TYPE eq_type, T_EQ_STREAM_TYPE stream_type,
                              uint8_t eq_mode, uint8_t index, uint8_t *data);

/**
 *
 * \brief   Eq init.
 *
 */
void app_eq_init(void);

/**
 *
 * \brief   Eq deinit.
 *
 */
void app_eq_deinit(void);

/**
 *
 * \brief   Set the active EQ index of the EQ engine.
 *
 * \param[in] index     EQ index.
 *
 * \return  The status of setting the EQ index.
 * \retval  true        EQ index was set successfully.
 * \retval  false       EQ index was failed to set.
 *
 */
bool app_eq_index_set(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index);

/**
 *
 * \brief   Set the specific EQ index parameter.
 *
 * \param[in] index     EQ index.
 * \param[in] data      EQ parameter buffer.
 * \param[in] len       EQ parameter length.
 *
 * \return  The status of setting the EQ parameter.
 * \retval  true    EQ parameter was set successfully.
 * \retval  false   EQ parameter was failed to set.
 *
 */
bool app_eq_param_set(uint8_t eq_mode, uint8_t index, void *data, uint16_t len);

/**
 * \brief   Get the specific EQ enable info.
 *
 * \param[in] eq_mode      EQ mode.
 * \param[in] enable_info  EQ enable info.
 */
void app_eq_enable_info_get(uint8_t eq_mode, T_EQ_ENABLE_INFO *enable_info);

/**
    * @brief  app_eq_report_terminate_param_report
    * @param  p_link the link of br/edr or le
    * @param cmd_path
    */
void app_eq_report_terminate_param_report(void *p_link, uint8_t cmd_path);

/**
    * @brief  app_eq_report_eq_param
    * @param  p_link the link of br/edr or le
    * @param cmd_path
    */
void app_eq_report_eq_param(void *p_link, uint8_t cmd_path);

/**
    * @brief  app_eq_sample_rate_get
    */
uint32_t app_eq_sample_rate_get(void);

/**
    * @brief  play audio eq tone
    */
void app_eq_play_audio_eq_tone(void);

/**
    * @brief  play apt eq tone
    */
void app_eq_play_apt_eq_tone(void);


/**
    * @brief  check idx accord eq mode
    */
void app_eq_idx_check_accord_mode(void);

/**
    * @brief  check eq mode then sync idx
    */
void app_eq_sync_idx_accord_eq_mode(uint8_t eq_mode, uint8_t index);

/**
 *
 * \brief   create eq instance.
 * @param  eq_content_type eq content type
 * @param  stream_type eq stream type
 * @param  eq_type  eq type
 * @param  eq_mode eq mode
 * @param  eq_index eq index
 *
 */
T_AUDIO_EFFECT_INSTANCE app_eq_create(T_EQ_CONTENT_TYPE eq_content_type,
                                      T_EQ_STREAM_TYPE stream_type,
                                      T_EQ_TYPE eq_type, uint8_t eq_mode, uint8_t eq_index);
/**
    * @brief  eq related command handler
    */
void app_eq_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                       uint8_t *ack_pkt);

/**
 *
 * \brief   enable the EQ effect.
 *
 */
void app_eq_audio_eq_enable(T_AUDIO_EFFECT_INSTANCE *eq_instance, bool *audio_eq_enabled);

/**
 *
 * \brief   audio eq mode change
 *
 * \param[in] report_when_eq_mode_change  is reporting eq index when audio eq mode changed
 *
 */
void app_eq_change_audio_eq_mode(bool report_when_eq_mode_change);

/**
 *
 * \brief   judge audio eq mode
 *
 * \return  The new audio eq mode
 * \retval  NORMAL_MODE normal mode
 * \retval  GAMING_MODE gaming mode
 * \retval  ANC_MODE    ANC mode
 *
 */
uint8_t app_eq_judge_audio_eq_mode(void);

/**
 * \brief   Get EQ mode.
 * \param[in] eq_type       EQ type
 * \param[in] stream_type   EQ stream type
 * \return   EQ mode
 */
uint8_t app_eq_mode_get(uint8_t eq_type, uint8_t stream_type);

/**
 * \brief   Enable EQ effect.
 * \param[in] eq_mode   EQ mode
 * \param[in] eq_len    EQ len
 */
void app_eq_enable_effect(uint8_t eq_mode, uint16_t eq_len);

/**
 * \brief   Operate EQ cmd sended by tool.
 * \param[in] eq_mode          EQ mode
 * \param[in] eq_adjust_side   the site of the user EQ
 * \param[in] is_play_eq_tone  is play eq tone
 * \param[in] eq_idx           EQ idx
 * \param[in] eq_len_to_dsp    EQ len
 * \param[in] cmd_ptr          EQ data
 *
 * \return  The status of operate EQ cmd.
 * \retval  true        successfully
 * \retval  false       failed
 */
bool app_eq_cmd_operate(uint8_t eq_mode, uint8_t eq_adjust_side, uint8_t is_play_eq_tone,
                        uint8_t eq_idx, uint8_t eq_len_to_dsp, uint8_t *cmd_ptr);

/** End of APP_EQ
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_EQ_H_ */
