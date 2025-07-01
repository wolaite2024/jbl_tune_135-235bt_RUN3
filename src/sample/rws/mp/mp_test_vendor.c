
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_CLI_BINARY_MP_SUPPORT
#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_msg.h"
#include "trace.h"
#include "bt_types.h"
#include "mp_test.h"
#include "mp_test_vendor.h"
#include "app_led.h"

extern uint8_t mp_app_index;
extern uint8_t mp_cmd_path;

/**
#define KEY0_MASK 0x01
#define KEY1_MASK 0x02
#define KEY2_MASK 0x04
#define KEY3_MASK 0x08
#define KEY4_MASK 0x10
#define KEY5_MASK 0x20
#define KEY6_MASK 0x40
#define KEY7_MASK 0x80
*/
void mp_test_vendor_send_key_event(uint8_t key_index)
{
    uint8_t result      = MP_TEST_RESULT_PASS;
    uint8_t opcode      = MP_TEST_OPCODE_VENDOR_BINARY;
    uint8_t sub_opcode  = MP_TEST_VENDOR_SUB_OPCODE_KEY;
    uint16_t param_len  = sizeof(key_index);

    mp_test_send_test_result(mp_app_index, mp_cmd_path, result, opcode, sub_opcode, &key_index,
                             param_len);
}

bool mp_test_handle_vendor_binary_cmd(uint8_t app_idx, uint8_t cmd_path, uint8_t sub_opcode,
                                      uint8_t *p_param, uint16_t param_len)
{
    bool handle = true;
    uint8_t result = MP_TEST_RESULT_PASS;
    uint16_t param_total_len = 0;

    APP_PRINT_INFO2("mp_test_handle_vendor_binary_cmd: sub_opcode %d, param_len %d", sub_opcode,
                    param_len);
    switch (sub_opcode)
    {
    case MP_TEST_VENDOR_SUB_OPCODE_LED:
        {
            uint8_t led_index = 0;
            uint8_t  on_off_flag = 0;

            if (param_len != sizeof(led_index) + sizeof(on_off_flag))
            {
                result = MP_TEST_RESULT_FAIL_INVALID_LEN;
                break;
            }

            LE_STREAM_TO_UINT8(led_index, p_param);
            LE_STREAM_TO_UINT8(on_off_flag, p_param);

            APP_PRINT_INFO2("mp_test_handle_rtk_cmd: led_index 0x%d, on_off_flag %d", led_index,
                            on_off_flag);

            led_change_mode((T_LED_MODE)on_off_flag, true, false);
        }
        break;

    case MP_TEST_VENDOR_SUB_OPCODE_KEY:
        {
            handle = false;
        }
        break;

    default:
        {
            result = MP_TEST_RESULT_FAIL_NOT_SUPPORTED_SUBOPCODE;
        }
        break;

    }

    if (handle)
    {
        mp_test_send_test_result(app_idx, cmd_path, result, MP_TEST_OPCODE_VENDOR_BINARY,
                                 sub_opcode, NULL, param_total_len);
    }

    return true;
}
#endif

