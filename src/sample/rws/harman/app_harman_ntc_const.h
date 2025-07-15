/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_HARMAN_NTC_CONST_H_
#define _APP_HARMAN_NTC_CONST_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#define BATTERY_NTC_MAX_LENGTH				81

typedef enum
{
	HARMAN_BATTERY_NTC_68K					 = 0x00,
	HARMAN_BATTERY_NTC_10K				 	 = 0x01,
	HARMAN_BATTERY_NTC_47K					 = 0x02,
	HARMAN_BATTERY_NTC_100K					 = 0x03,	
	HARMAN_BATTERY_NTC_MAX					 = 0x04,
} HARMAN_BATTERY_NTC;
	
typedef struct {
    uint8_t ntc_resistance;          // 温度
    uint8_t hysteresis_value;      // adc值
} battery_hysteresis_t;

typedef struct {
    int16_t temperature;          // 温度
    uint16_t bat_ntc_value_array[4];      // adc值
} battery_ntc_value_t;

extern const battery_ntc_value_t battery_ntc_adc_array[];
extern const battery_hysteresis_t ntc_hysteresis_voltage_array[];
extern const int battery_ntc_offset[];
#endif /* _APP_HARMAN_ADC_H_ */


