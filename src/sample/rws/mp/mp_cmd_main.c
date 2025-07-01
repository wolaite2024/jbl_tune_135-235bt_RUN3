
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_CLI_BINARY_MP_SUPPORT
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "os_mem.h"
#include "os_msg.h"
#include "app_io_msg.h"
#include "mp_cmd.h"
#include "console.h"
#include "app_cmd.h"
#include "app_util.h"
#include "app_main.h"
#include "trace.h"


bool mp_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

T_MP_CMD_MODULE mp_cmd_module;
bool mp_cmd_set_parser(uint8_t *buf, uint32_t len)
{
    uint16_t cmd_len;
    uint16_t temp_index;
    //uint8_t rx_seqn;

    if (mp_cmd_module.rx_w_idx + len <= mp_cmd_module.rx_buf_size)
    {
        memcpy(&mp_cmd_module.rx_buffer[mp_cmd_module.rx_w_idx], buf, len);
        mp_cmd_module.rx_count += len;
        mp_cmd_module.rx_w_idx += len;
        if (mp_cmd_module.rx_w_idx == mp_cmd_module.rx_buf_size)
        {
            mp_cmd_module.rx_w_idx = 0;
        }
    }
    else
    {
        uint32_t temp;

        temp = mp_cmd_module.rx_buf_size - mp_cmd_module.rx_w_idx;
        memcpy(&mp_cmd_module.rx_buffer[mp_cmd_module.rx_w_idx], buf, temp);
        memcpy(&mp_cmd_module.rx_buffer[0], &buf[temp], len - temp);
        mp_cmd_module.rx_count += len;
        mp_cmd_module.rx_w_idx  = len - temp;
    }


    while (mp_cmd_module.rx_count)
    {
        if (mp_cmd_module.rx_buffer[mp_cmd_module.rx_process_offset] == CMD_SYNC_BYTE)
        {
            temp_index = mp_cmd_module.rx_process_offset;
            //sync_byte(1) and Seqn(1) and length(2) and cmd id(2) and checksum(1)
            if (mp_cmd_module.rx_count >= 7)
            {
                temp_index++;
                if (temp_index >= MP_CMD_RX_BUFFER_SIZE)
                {
                    temp_index = 0;
                }
                //rx_seqn = mp_cmd_module.rx_buffer[temp_index];
                temp_index++;
                if (temp_index >= MP_CMD_RX_BUFFER_SIZE)
                {
                    temp_index = 0;
                }
                cmd_len = mp_cmd_module.rx_buffer[temp_index];
                temp_index++;
                if (temp_index >= MP_CMD_RX_BUFFER_SIZE)
                {
                    temp_index = 0;
                }
                cmd_len |= (mp_cmd_module.rx_buffer[temp_index] << 8);
                //only process when received whole command
                if (mp_cmd_module.rx_count >= (cmd_len + 5))
                {
                    uint8_t     *temp_ptr;
                    uint8_t     check_sum;

                    temp_ptr = os_mem_alloc(RAM_TYPE_DATA_ON, (cmd_len + 5));
                    if (temp_ptr != NULL)
                    {
                        if ((mp_cmd_module.rx_process_offset + cmd_len + 5) > MP_CMD_RX_BUFFER_SIZE)
                        {
                            uint16_t    temp_len;

                            temp_len = MP_CMD_RX_BUFFER_SIZE - mp_cmd_module.rx_process_offset;
                            memcpy(temp_ptr, &mp_cmd_module.rx_buffer[mp_cmd_module.rx_process_offset], temp_len);
                            temp_index = temp_len;
                            temp_len = (cmd_len + 5) - (MP_CMD_RX_BUFFER_SIZE - mp_cmd_module.rx_process_offset);
                            memcpy(&temp_ptr[temp_index], &mp_cmd_module.rx_buffer[0], temp_len);
                        }
                        else
                        {
                            memcpy(temp_ptr, &mp_cmd_module.rx_buffer[mp_cmd_module.rx_process_offset],
                                   (cmd_len + 5));
                        }
                        check_sum = app_util_calc_checksum(&temp_ptr[1], cmd_len + 3);//from seqn
                        if (check_sum == temp_ptr[cmd_len + 4])
                        {
                            mp_send_msg(IO_MSG_CONSOLE_BINARY_RX, temp_ptr);
                        }
                        else
                        {
                            APP_PRINT_ERROR2("mp_cmd_set_parser: checksum error calc check_sum 0x%x, rx check_sum 0x%x",
                                             check_sum, temp_ptr[cmd_len + 4]);
                            os_mem_free(temp_ptr);
                        }
                    }
                    mp_cmd_module.rx_count -= (cmd_len + 5);
                    mp_cmd_module.rx_process_offset += (cmd_len + 5);
                    if (mp_cmd_module.rx_process_offset >= MP_CMD_RX_BUFFER_SIZE)
                    {
                        mp_cmd_module.rx_process_offset -= MP_CMD_RX_BUFFER_SIZE;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            mp_cmd_module.rx_count--;
            mp_cmd_module.rx_process_offset++;
            if (mp_cmd_module.rx_process_offset >= MP_CMD_RX_BUFFER_SIZE)
            {
                mp_cmd_module.rx_process_offset = 0;
            }
        }
    }
    return true;
}


void mp_cmd_module_init(void)
{
    mp_cmd_module.rx_buffer = os_mem_alloc(RAM_TYPE_DATA_ON, MP_CMD_RX_BUFFER_SIZE);
    mp_cmd_module.rx_buf_size = MP_CMD_RX_BUFFER_SIZE;
    mp_cmd_module.rx_count = 0;
    mp_cmd_module.rx_process_offset = 0;
    mp_cmd_module.rx_w_idx = 0;
}
bool mp_cmd_register(void)
{
    return console_register_parser(&mp_cmd_set_parser);
}
#endif
