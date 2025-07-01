
#ifndef _APP_FLAGS_H_
#define _APP_FLAGS_H_

#define IC_NAME                                     "RTL8763ESH"

//Init value of default features are defined here
#define F_BT_GATT_SERVER_EXT_API_SUPPORT            0
#define APP_CAP_TOUCH_SUPPORT                       0
#define F_APP_MULTI_LINK_ENABLE                     1
#define F_APP_HEARABLE_SUPPORT                      0
#define F_APP_VOICE_SPK_EQ_SUPPORT                  1
#define F_APP_VOICE_MIC_EQ_SUPPORT                  1
#define F_APP_ENABLE_PAUSE_SECOND_LINK              0

#define F_APP_TEST_SUPPORT                          1
#define F_APP_CONSOLE_SUPPORT                       1
#define F_APP_CLI_STRING_MP_SUPPORT                 1
#define F_APP_CLI_BINARY_MP_SUPPORT                 1
#define F_APP_CLI_CFG_SUPPORT                       1
#define F_APP_BQB_CLI_SUPPORT                       1
#define F_APP_BQB_CLI_HFP_AG_SUPPORT                0
#define F_APP_BQB_CLI_MAP_SUPPORT                   0

#define F_APP_A2DP_CODEC_LDAC_SUPPORT               0

#define F_APP_SPECIFIC_UUID_SUPPORT                 1
#define F_APP_IAP_RTK_SUPPORT                       0
#define F_APP_IAP_SUPPORT                           0

#define F_APP_HID_SUPPORT                           0
#define F_APP_HID_MOUSE_SUPPORT                     0
#define F_APP_HID_KEYBOARD_SUPPORT                  0

#define F_APP_HFP_AG_SUPPORT                        0

#define F_APP_DUT_MODE_AUTO_POWER_OFF               1

#define F_APP_RWS_MULTI_SPK_SUPPORT                 0

#define F_APP_LOSS_BATTERY_PROTECT                  0

#define F_APP_BLE_BOND_SYNC_SUPPORT                 1
#define AMA_FEATURE_SUPPORT                         0
#define BISTO_FEATURE_SUPPORT                       0
#define GFPS_FEATURE_SUPPORT                        1
#define GFPS_SASS_SUPPORT                           (1 && GFPS_FEATURE_SUPPORT)
#define GFPS_FINDER_SUPPORT                         (1 && GFPS_FEATURE_SUPPORT)
#define GFPS_FINDER_DULT_SUPPORT                    (1 && GFPS_FINDER_SUPPORT)
#define GFPS_FINDER_DULT_ADV_SUPPORT                0
#define F_APP_BLE_SWIFT_PAIR_SUPPORT                1
#define GFPS_SASS_IN_FOCUS_SUPPORT                  0
#define GFPS_SASS_ON_HEAD_DETECT_SUPPORT            0

/* for GFPS Finder */
#define F_APP_PERIODIC_WAKEUP_RECHARGE              1

/*ld sensor,gsensor,psensor*/
#define F_APP_SENSOR_SUPPORT                        0
#define F_APP_SENSOR_IQS773_873_SUPPORT             0
#define F_APP_SENSOR_JSA1225_SUPPORT                0
#define F_APP_SENSOR_JSA1227_SUPPORT                0
#define F_APP_SENSOR_PX318J_SUPPORT                 0
#define F_APP_SENSOR_HX3001_SUPPORT                 0
#define F_APP_SENSOR_SL7A20_SUPPORT                 0
#define F_APP_SENSOR_CAP_TOUCH_SUPPORT              0
//SC7A20 as ls(light sensor)
#define F_APP_SENSOR_SC7A20_AS_LS_SUPPORT           0

#define F_APP_PWM_SUPPORT                           0
#define F_APP_TUYA_SUPPORT                          0
#if F_APP_PWM_SUPPORT
#define F_APP_BUZZER_SUPPORT                        1
#endif
#define F_APP_NFC_SUPPORT                           0
#define F_APP_VOICE_NREC_SUPPORT                    1
#define F_APP_TTS_SUPPORT                           0
#define F_APP_BLE_ANCS_CLIENT_SUPPORT               0
#define F_APP_ADC_SUPPORT                           0
#define F_APP_LOCAL_PLAYBACK_SUPPORT                0
#define F_APP_USB_AUDIO_SUPPORT                     0

/* anc related */
#define F_APP_ANC_SUPPORT                           0
#define F_APP_APT_SUPPORT                           0
#define F_APP_SIDETONE_SUPPORT                      1

#define F_APP_BT_PROFILE_PBAP_SUPPORT               0
#define F_APP_BT_PROFILE_MAP_SUPPORT                0
#define F_APP_TEAMS_FEATURE_SUPPORT                 0
#define F_APP_SINGLE_MUTLILINK_SCENERIO_1           0

/* airplane mode related */
#define F_APP_AIRPLANE_SUPPORT                      0
#define F_APP_LINEIN_SUPPORT                        0
#define F_APP_QOL_MONITOR_SUPPORT                   0
#define F_APP_GPIO_ONOFF_SUPPORT                    1
#define F_APP_ERWS_SUPPORT                          0
#define F_APP_IO_OUTPUT_SUPPORT                     1
#define F_APP_MUTILINK_VA_PREEMPTIVE                1//ERWS_MULTILINK_SUPPORT
#define F_APP_MUTILINK_TRIGGER_HIGH_PRIORITY        0
#define F_APP_SPP_CAPTURE_DSP_DATA                  0
#define F_APP_AMP_SUPPORT                           0
#define F_APP_LISTENING_MODE_SUPPORT                0

#define F_APP_SLIDE_SWITCH_SUPPORT                  0
#define F_APP_SLIDE_SWITCH_POWER_ON_OFF             0
#define F_APP_SLIDE_SWITCH_LISTENING_MODE           0

#define F_APP_BRIGHTNESS_SUPPORT                    0
#define F_APP_POWER_ON_DELAY_APPLY_APT_SUPPORT      0
#define F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT        0
#define F_APP_LLAPT_SCENARIO_CHOOSE_SUPPORT         0
#define F_APP_SEPARATE_ADJUST_APT_VOLUME_SUPPORT    0
#define F_APP_ADJUST_TONE_VOLUME_SUPPORT            0

#define F_APP_GPIO_MICBIAS_SUPPORT                  1

#define F_APP_USB_AUDIO_SUPPORT                     0
#define F_APP_VAD_SUPPORT                           0
#define F_APP_ONE_WIRE_UART_SUPPORT                 0

#define F_APP_KEY_EXTEND_FEATURE                    0

#define F_APP_CFU_SUPPORT                           0

#define F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT     0

#define F_APP_IPHONE_ABS_VOL_HANDLE                 1
#define F_APP_SMOOTH_BAT_REPORT                     1
#define F_APP_RWS_KEY_SUPPORT                       0
#define F_APP_GOLDEN_RANGE                          1
#define F_APP_DUAL_AUDIO_EFFECT                     0
/*
 *   Disable BAU dongle feature in default.
 *   Enable this feature according to target.
*/
#define F_APP_DONGLE_FEATURE_SUPPORT                0
#define F_APP_CONFERENCE_HEADSET_SUPPORT            0

#define F_APP_HFP_CMD_SUPPORT                       0
#define F_APP_DEVICE_CMD_SUPPORT                    0
#define F_APP_AVRCP_CMD_SUPPORT                     0
#define F_APP_PBAP_CMD_SUPPORT                      0

#define F_APP_RSSI_INFO_GET_CMD_SUPPORT             1

#define F_APP_EXT_CHARGER_FEATURE_SUPPORT           1

/**
 * Harman Feature defination
 */
#define F_APP_HARMAN_FEATURE_SUPPORT                        1

#define HARMAN_LOW_BAT_WARNING_TIME_SET_SUPPORT             0
#define HARMAN_LOW_BAT_WARNING_TIME                         900

#define HARMAN_ENTER_STANDBY_MODE_AFTER_PAIRING_TIMEOUT     1
#define HARMAN_DETECT_BATTERY_STATUS_LED_SUPPORT            0 // 5S continues
#define HARMAN_ONLY_CONN_ONE_DEVICE_WHEN_PAIRING            0
#define HARMAN_LED_BREATH_SUPPORT                           1
#define HARMAN_VP_DATA_HEADER_GET_SUPPORT                   1
#define HARMAN_ADJUST_LINKBACK_RETRY_TIME_SUPPORT           1
#define HARMAN_FIND_MY_BUDS_TONE_SUPPORT                    1
#define HARMAN_SPP_CMD_SUPPORT                              1
#define HARMAN_ADJUST_VOLUME_ONLY_A2DP_OR_HFP_SUPPORT       0
#define HARMAN_ADJUST_MAX_MIN_VOLUME_FROM_PHONE_SUPPORT     0

#define HARMAN_DISABLE_STANDBY_LED_WHEN_LOW_BATTERY         1
#define HARMAN_ADD_GFPS_FINDER_STOP_ADV_TONE                1

#define HARMAN_DELAY_CHARGING_FINISH_SUPPORT                1

#define HARMAN_VP_LR_BALANCE_SUPPORT                        0
#define HARMAN_DSP_LICENSE_SUPPORT                          0

#define HARMAN_AUTO_ACCEPT_SECOND_CALL_SUPPORT              0

#define HARMAN_ALLOW_POWER_OFF_ON_CALL_ACTIVE_SUPPORT       0
#define HARMAN_NOT_ALLOW_ENTER_PAIR_MODE_ON_CALLING_SUPPORT 0

#define HARMAN_POWER_DISPLAY_ACCURACY_1_PERCENTAGE_SUPPORT  1

#define HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT                0
#define HARMAN_BONDING_LEGACY_AND_BLE_LINK                  0

#define HARMAN_DELAY_HANDLE_ADP_OUT_SUPPORT                 0
#define HARMAN_HW_TIMER_REPLACE_OS_TIMER                    1

#define HARMAN_SPECAIL_ULTRA_LONG_KEY_TIME                  0

#define HARMAN_REQ_REMOTE_DEVICE_NAME_TIME                  1

#if F_APP_HARMAN_FEATURE_SUPPORT
#define HARMAN_AUTO_POWER_OFF_DEFAULT_TIME                  (30 * 60)  // s

#define HARMAN_SECURITY_CHECK                               0

#define HARMAN_OTA_COMPLETE_NONEED_POWERON                  1
#define HARMAN_OTA_VERSION_CHECK                            0
#define HARMAN_OPEN_LR_FEATURE                              1

#define HARMAN_EXTERNAL_CHARGER_SUPPORT                     1
#define HARMAN_NTC_DETECT_PROTECT                           1
#define HARMAN_DISCHARGER_NTC_DETECT_PROTECT                1
#define HARMAN_BRIGHT_LED_WHEN_ADP_IN                       0

// set according to differrnt Project(T135/Run3/Pace)
#define HARMAN_EXTERNAL_CHARGER_DZ581_SUPPORT               0
#define HARMAN_EXTERNAL_CHARGER_DZ582_SUPPORT               0
#define HARMAN_SECOND_NTC_DETECT_PROTECT                    0
#define HARMAN_USB_CONNECTOR_PROTECT                        0

#define HARMAN_SUPPORT_WATER_EJECTION                       0
#define HARMAN_SUPPORT_MIC_STATUS_RECORD                    1
#define HARMAN_SUPPORT_CONNECT_VP_IN_HFP                    0
#define HARMAN_DISCONN_ACTIVE_A2DP_WHEN_OTA                 0

#if HARMAN_T135_SUPPORT || HARMAN_T235_SUPPORT
#undef  HARMAN_EXTERNAL_CHARGER_DZ581_SUPPORT
#define HARMAN_EXTERNAL_CHARGER_DZ581_SUPPORT               1
#define HARMAN_VBAT_ADC_DETECTION                           0
#undef  HARMAN_SECURITY_CHECK
#define HARMAN_SECURITY_CHECK                               1

#elif HARMAN_RUN3_SUPPORT
#undef  HARMAN_EXTERNAL_CHARGER_DZ581_SUPPORT
#define HARMAN_EXTERNAL_CHARGER_DZ581_SUPPORT               1
#undef HARMAN_SECOND_NTC_DETECT_PROTECT
#define HARMAN_SECOND_NTC_DETECT_PROTECT                    0
#undef HARMAN_USB_CONNECTOR_PROTECT
#define HARMAN_USB_CONNECTOR_PROTECT                        1
#undef  HARMAN_SECURITY_CHECK
#define HARMAN_SECURITY_CHECK                               1
#undef  HARMAN_FIND_MY_BUDS_TONE_SUPPORT
#define HARMAN_FIND_MY_BUDS_TONE_SUPPORT                    0
#undef  HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT
#define HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT                1
#undef  HARMAN_SPECAIL_ULTRA_LONG_KEY_TIME
#define HARMAN_SPECAIL_ULTRA_LONG_KEY_TIME                  1

#elif HARMAN_PACE_SUPPORT
#undef  HARMAN_EXTERNAL_CHARGER_DZ582_SUPPORT
#define HARMAN_EXTERNAL_CHARGER_DZ582_SUPPORT               1
#undef  HARMAN_SECOND_NTC_DETECT_PROTECT
#define HARMAN_SECOND_NTC_DETECT_PROTECT                    1
#undef  HARMAN_USB_CONNECTOR_PROTECT
#define HARMAN_USB_CONNECTOR_PROTECT                        1
#undef  HARMAN_SECURITY_CHECK
#define HARMAN_SECURITY_CHECK                               0
#undef  HARMAN_DSP_LICENSE_SUPPORT
#define HARMAN_DSP_LICENSE_SUPPORT                          1
#undef  HARMAN_SUPPORT_WATER_EJECTION
#define HARMAN_SUPPORT_WATER_EJECTION                       1
#undef  HARMAN_BRIGHT_LED_WHEN_ADP_IN
#define HARMAN_BRIGHT_LED_WHEN_ADP_IN                       1
#undef  HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT
#define HARMAN_BLE_ENCRYPTED_CONNECT_SUPPORT                1
#undef  HARMAN_DELAY_HANDLE_ADP_OUT_SUPPORT
#define HARMAN_DELAY_HANDLE_ADP_OUT_SUPPORT                 1
#undef  HARMAN_SPECAIL_ULTRA_LONG_KEY_TIME
#define HARMAN_SPECAIL_ULTRA_LONG_KEY_TIME                  1
#undef  HARMAN_SUPPORT_CONNECT_VP_IN_HFP
#define HARMAN_SUPPORT_CONNECT_VP_IN_HFP                    1
#undef  HARMAN_DISCONN_ACTIVE_A2DP_WHEN_OTA
#define HARMAN_DISCONN_ACTIVE_A2DP_WHEN_OTA                 1

#undef F_APP_SPP_CAPTURE_DSP_DATA
#define F_APP_SPP_CAPTURE_DSP_DATA                          1
#undef F_APP_ENABLE_PAUSE_SECOND_LINK
#define F_APP_ENABLE_PAUSE_SECOND_LINK                      1
#else
#endif

#endif /* F_APP_HARMAN_FEATURE_SUPPORT */

#endif /* _APP_FLAGS_H_ */
