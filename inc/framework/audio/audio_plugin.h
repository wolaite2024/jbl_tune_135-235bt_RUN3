/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_PLUGIN_H_
#define _AUDIO_PLUGIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_PLUGIN Audio plugin
 *
 * \brief   Customize the audio plugin configurations.
 */

/**
 * audio_plugin.h
 *
 * \brief Define the audio plugin instance handle, created by \ref audio_plugin_create.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef void *T_AUDIO_PLUGIN_HANDLE;

/**
 * audio_plugin.h
 *
 * \brief Define Audio Plugin Device Message.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef struct t_audio_plugin_msg
{
    T_AUDIO_PLUGIN_HANDLE   device;
    void                   *context;
} T_AUDIO_PLUGIN_MSG;

/**
 * audio_plugin.h
 *
 * \brief Define Audio Plugin operations.
 *
 * \details Each Audio Plugin instance shall support a common set of actions as enumed by \ref T_AUDIO_PLUGIN_ACTION.
 *          Each action is executed by a handler with prototype \ref P_AUDIO_PLUGIN_ACTION_HANDLER. The action and
 *          combined handler is registered to Audio Plugin within \ref T_AUDIO_PLUGIN_POLICY when creating an Audio
 *          Plugin instance by \ref audio_plugin_create.
 *
 * \note    The Audio Plugin action is ALWAYS treated as async within audio plugin, no matter whether its
 *          implementation deals with timer or not. This means the Audio Plugin will not know the result of
 *          executing the action until a \ref T_AUDIO_PLUGIN_MSG has been sent by user application through
 *          interface \ref audio_plugin_msg_send.
 *
 * \param[in]     handle       The device be opearted on.
 * \param[in]     context      The parameter required for the operation.
 *
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef void (*P_AUDIO_PLUGIN_ACTION_HANDLER)(T_AUDIO_PLUGIN_HANDLE handle, void *context);

/**
 * audio_plugin.h
 *
 * \brief Define Audio plugin Occasion.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef enum t_audio_plugin_occasion
{
    AUDIO_PLUGIN_OCCASION_CODEC_ON,
    AUDIO_PLUGIN_OCCASION_CODEC_OFF,
    AUDIO_PLUGIN_OCCASION_DSP_ON,
    AUDIO_PLUGIN_OCCASION_DSP_OFF,
    AUDIO_PLUGIN_OCCASION_SPORT_SET,
    AUDIO_PLUGIN_OCCASION_SPORT_CLEAR,
    AUDIO_PLUGIN_OCCASION_SYS_IDLE,
    AUDIO_PLUGIN_OCCASION_NUM,
} T_AUDIO_PLUGIN_OCCASION;

/**
 * audio_plugin.h
 *
 * \brief Define Audio plugin Action.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef enum t_audio_plugin_action
{
    AUDIO_PLUGIN_ACTION_ENABLE,
    AUDIO_PLUGIN_ACTION_DISABLE,
    AUDIO_PLUGIN_ACTION_POWER_ON,
    AUDIO_PLUGIN_ACTION_POWER_OFF,
    AUDIO_PLUGIN_ACTION_RESET,
    AUDIO_PLUGIN_ACTION_NUM,
} T_AUDIO_PLUGIN_ACTION;

/**
 * audio_plugin.h
 *
 * \brief Define Audio plugin Policy.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef struct t_audio_plugin_policy
{
    T_AUDIO_CATEGORY        category;
    T_AUDIO_PLUGIN_OCCASION occasion;
    T_AUDIO_PLUGIN_ACTION   action;
    P_AUDIO_PLUGIN_ACTION_HANDLER handler;
} T_AUDIO_PLUGIN_POLICY;

/**
 * audio_plugin.h
 *
 * \brief   Create an audio plugin device handle by registering operations and usage policies.
 *
 * \param[in] policies    Array of audio plugin device usage policies \ref T_AUDIO_PLUGIN_POLICY.
 * \param[in] count       Number of elements in array policies.
 * \return  The handle audio plugin device.

 *
 * \ingroup AUDIO_PLUGIN
 */
T_AUDIO_PLUGIN_HANDLE audio_plugin_create(const T_AUDIO_PLUGIN_POLICY *policies, uint32_t  count);

/**
 * audio_plugin.h
 *
 * \brief   Release the audio plugin device.
 *
 * \param[in] handle    Audio plugin device handle returned by \ref audio_plugin_create.
 *
 * \ingroup AUDIO_PLUGIN
 */
void audio_plugin_release(T_AUDIO_PLUGIN_HANDLE handle);

/**
 * audio_plugin.h
 *
 * \brief   User application sends message to Audio Plugin to indicate the result of executing action.
 *
 * \param[in] msg       Message \ref T_AUDIO_PLUGIN_MSG.
 *
 * \ingroup AUDIO_PLUGIN
 */
void audio_plugin_msg_send(T_AUDIO_PLUGIN_MSG *msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_PLUGIN_H_ */
