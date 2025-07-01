/**
 * Copyright (c) 2021, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _CHARGER_UTILS_H_
#define _CHARGER_UTILS_H_

#include <stdint.h>

/**  @brief rtk charger function return general error code*/
typedef enum
{
    CHARGER_UTILS_SUCCESS,
    CHARGER_UTILS_NOT_SUPPROTED,
    CHARGER_UTILS_NOT_ENABLED,
    CHARGER_UTILS_INVALID_PARAM,
} T_CHARGER_UTILS_ERROR;

/**
 * charger_utils.h
 *
 * \brief   Get charging current and voltage.
 *
 * \param[out]   charging current, positive in charging mode, negative in discharging mode
 * \param[out]   battery voltage
 *
 * \return          The status of getting infos.
 * \retval  CHARGER_UTILS_SUCCESS           current charging info is getting successfully.
 * \retval  CHARGER_UTILS_NOT_SUPPROTED     charging info getting failed.
 * \retval  CHARGER_UTILS_NOT_ENABLED       charger is not enabled. could not get battery information from charger module
 *
 * \ingroup CHARGER_UTILS
 */
T_CHARGER_UTILS_ERROR charger_utils_get_batt_info(int16_t *current, uint16_t *volt);

#endif









