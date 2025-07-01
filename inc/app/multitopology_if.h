/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MULTIPRO_IF_H_
#define _MULTIPRO_IF_H_


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief  MTC Return Result List */
typedef enum
{
    MTC_RESULT_SUCCESS                    = 0x00,
    MTC_RESULT_NOT_REGISTER               = 0x01,
    MTC_RESULT_PENDING                    = 0x02,
    MTC_RESULT_ACCEPT                     = 0x03,
    MTC_RESULT_REJECT                     = 0x04,
    MTC_RESULT_NOT_RELEASE                = 0x05,

} T_MTC_RESULT;

typedef enum
{
    MTC_IF_TO_AUDIO_POLICY              = 0x00,
    MTC_IF_FM_AUDIO_POLICY              = 0x01,
    MTC_IF_TO_MULTILINK                 = 0x02,
    MTC_IF_FM_MULTILINK                 = 0x03,
    MTC_IF_MAX                          = 0x04,
} T_MTC_IF_IDX;

/** @brief  Module List */
typedef enum
{
    MTC_IF_GROUP_AUDIO_POLICY           = 0x0000,
    MTC_IF_GROUP_MULTILINK              = 0x0100,
} T_MTC_IF_GROUP;

/** @brief  Interface List */
typedef enum
{
    MTC_IF_TO_AP_RESUME_SCO                         = MTC_IF_GROUP_AUDIO_POLICY | 0x00,
    MTC_IF_FROM_AP_SCO_CMPL                         = MTC_IF_GROUP_AUDIO_POLICY | 0x80,

    MTC_IF_TO_ML                                    = MTC_IF_GROUP_MULTILINK | 0x00,
    MTC_IF_FROM_ML                                  = MTC_IF_GROUP_MULTILINK | 0x80,
} T_MTC_IF_MSG;

typedef struct
{
    uint8_t if_index;
    void *inbuf;
    void *outbuf;
    T_MTC_IF_MSG msg;
} T_MTC_IF_INFO;


/*============================================================================*
 *                              Functions
 *============================================================================*/
T_MTC_RESULT mtc_if_to_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
