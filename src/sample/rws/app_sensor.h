/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_SENSOR_H_
#define _APP_SENSOR_H_

#include "app_msg.h"
#include "rtl876x_i2c.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_SENSROR App Sensor
  * @brief Sensor module implementation for rws sample project
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SENSOR_PROCESS_Exported_Macros App Senosr Macros
  * @{
  */

#define SENSOR_LD_VENDOR_HX         0
#define SENSOR_LD_VENDOR_JSA1225    1
#define SENSOR_LD_VENDOR_IO         2
#define SENSOR_LD_VENDOR_PX         3
#define SENSOR_LD_VENDOR_IQS873     4
#define SENSOR_LD_VENDOR_JSA1227    5
#define SENSOR_LD_VENDOR_SC7A20     6
#define SENSOR_LD_VENDOR_CAP_TOUCH  7

#define PSENSOR_VENDOR_IO       0
#define PSENSOR_VENDOR_IQS773   1
#define PSENSOR_VENDOR_IQS873   2

#define SENSOR_LD_OUT_EAR      0
#define SENSOR_LD_IN_EAR       1
#define SENSOR_LD_NONE         3

#define SENSOR_LD_POLLING_TIME_ACTIVE   400 //ms in sw timer
#define SENSOR_LD_POLLING_SPEED_UP   200 //ms in sw timer
#define SENSOR_LD_POLLING_TIME_SNIFF    500 //ms in sw timer
#define SENSOR_LD_DETECT_TIME   150 //us in hw timer
#define SENSOR_LD_STABLE_COUNT  4
#define SENSOR_LD_STABLE_OUT_EAR_COUNT  4

/** End of SENSOR_PROCESS_ExGported_Macros
  * @}
  */

/** @defgroup GSENSOR_PROCESS_Exported_Macros App Gsenosr Macros
  * @{
  */
#define GSENSOR_CLICK_DETECT_TIME        50 //ms in sw timer

#define GSENSOR_I2C_SLAVE_ADDR_SILAN     0x19

#define GSENSOR_INT_TRIGGERED    0
#define GSENSOR_INT_RELEASED     1

#define GSENSOR_VENDOR_REG_SL_WHO_AM_I       0x0F
#define GSENSOR_VENDOR_REG_SL_CTRL_REG1      0x20
#define GSENSOR_VENDOR_REG_SL_CTRL_REG2      0x21
#define GSENSOR_VENDOR_REG_SL_CTRL_REG3      0x22
#define GSENSOR_VENDOR_REG_SL_CTRL_REG4      0x23
#define GSENSOR_VENDOR_REG_SL_CTRL_REG5      0x24
#define GSENSOR_VENDOR_REG_SL_CTRL_REG6      0x25
#define GSENSOR_VENDOR_REG_SL_FIFO_CFG       0x2E
#define GSENSOR_VENDOR_REG_SL_FIFO_SRC       0x2F
#define GSENSOR_VENDOR_REG_SL_CLICK_CTRL     0x38
#define GSENSOR_VENDOR_REG_SL_CLICK_THS      0x3A
#define GSENSOR_VENDOR_REG_SL_TIME_LIMIT     0x3B
#define GSENSOR_VENDOR_REG_SL_TIME_LATENCY   0x3C
#define GSENSOR_VENDOR_REG_SL_FIFO_DATA      0xA8

#define LIGHT_SENSOR_ENABLED \
    (app_cfg_const.sensor_support == true && app_cfg_nv.light_sensor_enable == true)

extern uint32_t sendor_ld_hx_sw_cycle;

#if (F_APP_AVP_INIT_SUPPORT == 1)
extern void (*app_sensor_bud_loc_single_hook)(void);
#endif

/** End of GSENSOR_PROCESS_ExGported_Macros
  * @}
  */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup SENSOR_PROCESS_Exported_Types App Sensor Types
    * @{
    */

/**  @brief  Define Sensor state and  data for record Sensor detect status and stable count */
typedef enum t_ld_state
{
    LD_STATE_POLLING,
    LD_STATE_CHECK_IN_EAR,
    LD_STATE_FILTER_HIGH_LIGHT,

    LD_STATE_ALL
} T_LD_STATE;

typedef enum
{
    PSENSOR_I2C_EVENT_NONE = 0x00,
    PSENSOR_I2C_EVENT_PRESS = 0x01,
    PSENSOR_I2C_EVENT_RELEASE = 0x02,
    PSENSOR_I2C_EVENT_IN_EAR = 0x04,
    PSENSOR_I2C_EVENT_OUT_EAR = 0x08,
    PSENSOR_I2C_EVENT_PRESS_INVALID = 0x10,
    PSENSOR_I2C_EVENT_RELEASE_INVALID = 0x20,
} T_PSENSOR_I2C_EVENT;

typedef struct t_sensor_ld_data
{
    T_LD_STATE   ld_state;       /**< Curent Light detect state */
    uint8_t             pre_status;     /**< Previous detected in-ear status */
    uint8_t             cur_status;     /**< Current detected in-ear status */
    uint8_t             status_changed; /**< Detected in-ear status changed */
    uint8_t             stable_count;   /**< Detected in-ear status stable counter */
} T_SENSOR_LD_DATA;
/** End of SENSOR_PROCESS_Exported_Types
    * @}
    */

/**  @brief  Define gsensor state and  data for record Gsensor detect status and stable count */
typedef struct t_gsensor_sl
{
    uint8_t             pp_num;
    uint8_t             click_status;           /**< Single detected status */
    uint8_t             click_timer_cnt;
    uint8_t             click_timer_total_cnt;
    uint8_t             click_final_cnt;
} T_GSENSOR_SL;

extern T_SENSOR_LD_DATA sensor_ld_data;
/** @} */ /* End of group SENSOR_PROCESS_Exported_Functions */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
    * @brief  Sensor Light detect GPIO initial.
    *         Include APB peripheral clock config, GPIO parameter config and
    *         GPIO interrupt mark config.
    * @param  void
    * @return void
    */
void sensor_ld_gpio_init(void);

/**
    * @brief  Sensor Light detect state start detect.
    * @param  void
    * @return void
    */
void sensor_ld_start(void);

/**
    * @brief  Sensor Light detect stop.
    * @param  void
    * @return void
    */
void sensor_ld_stop(void);

/**
    * @brief  Get light sensor enable/disable status
    * @param  void
    * @return sensor enabled or disabled
    */
bool app_sensor_ld_is_light_sensor_enabled(void);
/**
    * @brief  ld sensor init.
    * @param  void
    * @return void
    */
void app_sensor_ld_init(void);

/**
    * @brief  ld sensor handle msg.
    * @param  io_msg
    * @return void
    */
void app_sensor_ld_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  ld io sensor init.
    * @param  void
    * @return void
    */
void app_io_sensor_ld_init(void);

/**
    * @brief  ld io sensor handle msg.
    * @param  io_msg
    * @return void
    */
void app_sensor_ld_io_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  ld io sensor handle msg.
    * @param  void
    * @return void
    */
void sensor_ld_io_int_gpio_intr_handler(void);

/**
    * @brief  sensor I2C init.
    *         Initialize I2C peripheral
    * @param  sensor_addr - sensor i2c slave adress
    * @return void
    */
void sensor_i2c_init(uint8_t sensor_addr);

/**
    * @brief  sensor INT GPIO initial.
    *         Include APB peripheral clock config, GPIO parameter config and
    *         GPIO interrupt mark config. Enable GPIO interrupt.
    * @param  void
    * @return void
    */
void sensor_int_gpio_init(uint8_t pinmux);

/**
    * @brief  GPIO interrupt will be handle in this function.
    *         First disable app enter dlps mode and read current GPIO input data bit.
    *         Disable GPIO interrupt and send IO_GPIO_MSG_TYPE message to app task.
    *         Then enable GPIO interrupt.
    * @param  void
    * @return void
    */
void app_sensor_sl_int_gpio_intr_handler(void);

/**
    * @brief  gsensor customer sc7a20 function: init.
    * @param  void
    * @return void
    */
void app_sensor_sc7a20_as_ls_init(void);

/**
    * @brief  gsensor vendor function: enable detect.
    * @param  void
    * @return void
    */
void gsensor_vendor_sl_enable(void);

/**
    * @brief  gsensor vendor function: disable detect.
    * @param  void
    * @return void
    */
void gsensor_vendor_sl_disable(void);

/**
    * @brief  Gsensor venrod init function.
    * @param  void
    * @return void
    */
void gsensor_vendor_sl_init(void);

/**
    * @brief  Gsensor vendor function: Check if click success detected, read click counts and executed in click INT
    * @param  void
    * @return true: click detect, false: click not detect
    */
bool app_gsensor_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  gsensor module init.
    * @param  void
    * @return void
    */
void app_gsensor_init(void);

/**
    * @brief  app sensor module init.
    * @param  void
    * @return void
    */
void app_sensor_init(void);

/**
    * @brief  psensor io init.
    * @param  void
    * @return void
    */
void app_int_psensor_init(void);

/**
    * @brief  bud location sync
    * @param  void
    * @return void
    */
void app_sensor_bud_loc_sync(void);

/**
    * @brief  bud location detected
    * @param  void
    * @return bud loc
    */
uint8_t app_sensor_bud_loc_detect(void);

/**
    * @brief  sensor related command handler
    * @param  cmd_ptr: sensor command
    * @param  cmd_len: length of command
    * @param  cmd_path: command path
    * @param  app_idx: link index
    * @param  ack_pkt: ack status
    * @return void
    */
void app_sensor_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                           uint8_t *ack_pkt);

/**
    * @brief  i2c read with auto switch slave addr support.
    * @param  slave_addr
    * @param  reg_addr
    * @param  read_buf
    * @param  read_len
    * @return void
    */
I2C_Status app_sensor_i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *read_buf,
                               uint16_t read_len);

/**
    * @brief  read 1 byte from i2c device reg.
    * @param  slave_addr
    * @param  reg_addr
    * @param  read_buf
    * @return void
    */
I2C_Status app_sensor_i2c_read_8(uint8_t slave_addr, uint8_t reg_addr, uint8_t *read_buf);

/**
    * @brief  read 2 bytes from i2c device reg.
    * @param  slave_addr
    * @param  reg_addr
    * @param  read_buf
    * @return void
    */
I2C_Status app_sensor_i2c_read_16(uint8_t slave_addr, uint8_t reg_addr, uint8_t *read_buf);

/**
    * @brief  read 4 bytes from i2c device reg.
    * @param  slave_addr
    * @param  reg_addr
    * @param  read_buf
    * @return void
    */
I2C_Status app_sensor_i2c_read_32(uint8_t slave_addr, uint8_t reg_addr, uint8_t *read_buf);

/**
    * @brief  i2c write with auto switch slave addr support.
    * @param  slave_addr
    * @param  write_buf
    * @param  write_len
    * @return void
    */
I2C_Status app_sensor_i2c_write(uint8_t slave_addr, uint8_t *write_buf, uint16_t write_len);

/**
    * @brief  write 1 byte to i2c device reg.
    * @param  slave_addr
    * @param  reg_addr
    * @param  reg_value
    * @return void
    */
I2C_Status app_sensor_i2c_write_8(uint8_t slave_addr, uint8_t reg_addr, uint8_t reg_value);

/**
    * @brief  write 2 bytes to i2c device reg.
    * @param  slave_addr
    * @param  reg_addr
    * @param  reg_value
    * @return void
    */
I2C_Status app_sensor_i2c_write_16(uint8_t slave_addr, uint8_t reg_addr, uint16_t reg_value);

/**
    * @brief  write 4 bytes to i2c device reg.
    * @param  slave_addr
    * @param  reg_addr
    * @param  reg_value
    * @return void
    */
I2C_Status app_sensor_i2c_write_32(uint8_t slave_addr, uint8_t reg_addr, uint32_t reg_value);

/**
    * @brief   control sensor on or off
    * @param  new_state
    * @param  is_push_tone
    * @param  is_during_power_on
    * @return void
    */
void app_sensor_control(uint8_t new_state, bool is_push_tone, bool is_during_power_on);

/**
    * @brief  handle gsensor mfb int
    * @param  void
    * @return void
    */
void app_sensor_sl_mfb_intr_handler(void);

/**
    * @brief  get i2c device fail status
    * @param  void
    * @return true is i2c device fail
    */
bool  app_sensor_is_i2c_device_fail(void);

/**
    * @brief  set i2c device fail status
    * @param  i2c device status
    * @return void
    */
void app_sensor_set_i2c_device_fail(bool status);

/** @} */ /* End of group SENSOR_PROCESS_Exported_Functions */
/** @} */ /* End of group SENSOR_PROCESS */
#ifdef __cplusplus
}
#endif
#endif

