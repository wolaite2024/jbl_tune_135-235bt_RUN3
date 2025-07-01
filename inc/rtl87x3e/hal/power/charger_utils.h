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
T_CHARGER_UTILS_ERROR charger_utils_thermistor_enable(uint8_t *enable);
T_CHARGER_UTILS_ERROR charger_utils_get_thermistor_pin(uint8_t *pin);
T_CHARGER_UTILS_ERROR charger_utils_get_auto_enable(uint8_t *auto_start);
T_CHARGER_UTILS_ERROR charger_utils_get_high_temp_warn_voltage(uint16_t *voltage);
T_CHARGER_UTILS_ERROR charger_utils_get_high_temp_error_voltage(uint16_t *voltage);
T_CHARGER_UTILS_ERROR charger_utils_get_low_temp_warn_voltage(uint16_t *voltage);
T_CHARGER_UTILS_ERROR charger_utils_get_low_temp_error_voltage(uint16_t *voltage);
T_CHARGER_UTILS_ERROR charger_utils_get_recharge_voltage(uint16_t *voltage);
T_CHARGER_UTILS_ERROR charger_utils_get_batt_vol(uint16_t *volt);
T_CHARGER_UTILS_ERROR charger_utils_get_thermistor_pin(uint8_t *thermistor_pin);
T_CHARGER_UTILS_ERROR charger_utils_get_bat_temp_hysteresis_voltage(uint16_t *voltage);
#endif









