#include <stdint.h>
#include <stddef.h>
#include "trace.h"
#include "usb_hid_desc.h"
#include "usb_hid.h"
#include "app_mmi.h"

#define BIT_POS_VOL_UP        0
#define BIT_POS_VOL_DOWN      1
#define BIT_POS_PLAY_PAUSE    2
#define BIT_POS_NEXT_TRACK    3
#define BIT_POS_PREV_TRACK    4

void app_usb_mmi_handle_action(uint8_t action)
{
    uint8_t bit_pos = 0;
    uint8_t report[2] = {HID_REPORT_ID_AUDIO_CONTROL, 0};

    APP_PRINT_INFO1("app_hid_ac_handle_mmi, action:0x%x", action);
    switch (action)
    {
    case MMI_DEV_SPK_VOL_UP:
        {
            bit_pos = BIT_POS_VOL_UP;
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
            bit_pos = BIT_POS_VOL_DOWN;
        }
        break;

    case MMI_AV_PLAY_PAUSE:
        {
            bit_pos = BIT_POS_PLAY_PAUSE;
        }
        break;

    case MMI_AV_FWD:
        {
            bit_pos = BIT_POS_NEXT_TRACK;
        }
        break;

    case MMI_AV_BWD:
        {
            bit_pos = BIT_POS_PREV_TRACK;
        }
        break;

    default:
        {
            return;
        }
    }

    report[1] = 1 << bit_pos;
    usb_hid_data_send(NULL, report, 2, NULL);

    report[1] = 0;
    usb_hid_data_send(NULL, report, 2, NULL);
}

