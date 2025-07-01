/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_mem.h"
#include "os_msg.h"
#include "bt_types.h"
#include "app_io_msg.h"
#include "app_main.h"
#include "cli.h"
#include "cli_lpm.h"


bool lpm_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    T_IO_MSG msg;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    return app_io_send_msg(&msg);
}

bool lpm_cmd(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    uint8_t     action;
    void       *param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p_param[param_len] = '\0';

    if (!strcmp(p_param, "dlps_enable"))
    {
        action = LPM_ACTION_DLPS_ENABLE;
    }
    else if (!strcmp(p_param, "dlps_disable"))
    {
        action = LPM_ACTION_DLPS_DISABLE;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, LPM_CMD);
        LE_UINT8_TO_STREAM(p, action);

        lpm_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf);
    }

    snprintf(buf, buf_len, "Device lpm %s.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (dlps_enable/dlps_disable).\r\n", p_param);
    return false;
}


static T_CLI_CMD lpm_cmd_entry  =
{
    NULL,           /* Next command will be determined when registered into file system. */
    NULL,           /* Next subcommand. */
    "lpm",        /* The command string to type. */
    "\r\nlpm cmd:\r\n dlps_enable dlps_disable\r\n",
    lpm_cmd,     /* The function to run. */
    1,              /* Zero or One parameter is expected. */
    NULL           /* Default command match pattern. */
};

bool lpm_cmd_register(void)
{
    return cli_cmd_register(&lpm_cmd_entry);
}
