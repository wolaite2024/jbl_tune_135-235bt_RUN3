#ifndef _APP_BLE_TTS_ADV_H_
#define _APP_BLE_TTS_ADV_H_

#include "stdint.h"
#include "stdbool.h"
#include "ble_ext_adv.h"


/**
    * @brief  get ble tts ibeacon advertising state
    * @param  void
    * @return ble advertising state
    */
T_BLE_EXT_ADV_MGR_STATE tts_ibeacon_adv_get_state(void);

/**
    * @brief  start ble tts ibeacon advertising
    * @param  duration_ms advertising duration time
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool tts_ibeacon_adv_start(uint16_t duration_10ms);

/**
    * @brief  stop ble tts ibeacon advertising
    * @param  app_cause cause
    * @return true  Command has been sent successfully.
    * @return false Command was fail to send.
    */
bool tts_ibeacon_adv_stop(uint8_t app_cause);

/**
    * @brief  set ble tts ibeacon advertising random address
    * @param  random_address random address
    * @return void
    */
void tts_ibeacon_adv_set_random(uint8_t *random_address);

/**
    * @brief  init ble tts ibeacon advertising parameters
    * @param  void
    * @return void
    */
void tts_ibeacon_adv_init(void);


#endif

