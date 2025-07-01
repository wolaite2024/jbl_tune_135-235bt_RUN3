#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "trace.h"
#include "usb_std_desc.h"
#include "usb_audio_config.h"
#include "usb_audio1_desc.h"
#include "usb_audio_driver.h"
#include "usb_audio1_ctrl.h"
#include "usb_audio.h"

#if(USB_AUDIO_VERSION == USB_AUDIO_VERSION_1)
BOOL_USB_AUDIO_CB *usb_audio1_ctrl_cb = NULL;

static const T_UAC1_IT_DESC     input_terminal_desc0 =
{
    .bLength            = sizeof(T_UAC1_IT_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_INPUT_TERMINAL,
    .bTerminalID        = ID_INPUT_TERMINAL1,
    .wTerminalType      = UAC1_INPUT_TERMINAL_MICROPHONE,
    .bAssocTerminal     = 0,
    .bNrChannels        = 1,
    .wChannelConfig     = 1,
    .iChannelNames      = 0,
    .iTerminal          = 0,
};

static const T_UAC1_IT_DESC     input_terminal_desc1 =
{
    .bLength            = sizeof(T_UAC1_IT_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_INPUT_TERMINAL,
    .bTerminalID        = ID_INPUT_TERMINAL2,
    .wTerminalType      = UAC1_TERMINAL_STREAMING,
    .bAssocTerminal     = 0,
    .bNrChannels        = 2,
    .wChannelConfig     = 3,
    .iChannelNames      = 0,
    .iTerminal          = 0,
};

const T_UAC1_OT_DESC     output_terminal_desc0 =
{
    .bLength            = sizeof(T_UAC1_OT_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_OUTPUT_TERMINAL,
    .bTerminalID        = ID_OUTPUT_TERMINAL1,
    .wTerminalType      = UAC1_OUTPUT_TERMINAL_SPEAKER,
    .bAssocTerminal     = 0,
    .bSourceID          = ID_FEATURE_UNIT1,
    .iTerminal          = 0,
};

const T_UAC1_OT_DESC     output_terminal_desc1 =
{
    .bLength            = sizeof(T_UAC1_OT_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_OUTPUT_TERMINAL,
    .bTerminalID        = ID_OUTPUT_TERMINAL2,
    .wTerminalType      = UAC1_TERMINAL_STREAMING,
    .bAssocTerminal     = 0,
    .bSourceID          = ID_FEATURE_UNIT2,
    .iTerminal          = 0,
};

const T_UAC1_FEATURE_UNIT_DESC_2    feature_uint_desc0 =
{
    .bLength            = sizeof(T_UAC1_FEATURE_UNIT_DESC_2),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_FEATURE_UNIT,
    .bUnitID            = ID_FEATURE_UNIT1,
    .bSourceID          = ID_INPUT_TERMINAL2,
    .bControlSize       = 1,
    .bmaControls[0]     = UAC_CONTROL_BIT(UAC1_FU_MUTE_CONTROL) | UAC_CONTROL_BIT(UAC1_FU_VOLUME_CONTROL),
    .bmaControls[1]     = 0,
    .bmaControls[2]     = 0,
    .iFeature           = 0,
};


const T_UAC1_FEATURE_UNIT_DESC_1    feature_uint_desc1 =
{
    .bLength            = sizeof(T_UAC1_FEATURE_UNIT_DESC_1),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_FEATURE_UNIT,
    .bUnitID            = ID_FEATURE_UNIT2,
    .bSourceID          = ID_INPUT_TERMINAL1,
    .bControlSize       = 1,
    .bmaControls[0]     = UAC_CONTROL_BIT(UAC1_FU_MUTE_CONTROL) | UAC_CONTROL_BIT(UAC1_FU_VOLUME_CONTROL),
    .bmaControls[1]     = 0,
    .iFeature           = 0,
};

static T_UAC1_STD_ITF_DESC     interface_alt0_desc1 =
{
    .bLength            = sizeof(T_UAC1_STD_ITF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static T_UAC1_STD_ITF_DESC     interface_alt1_desc1 =
{
    .bLength            = sizeof(T_UAC1_STD_ITF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 1,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

const T_UAC1_AS_HDR_DESC    as1_header_desc1 =
{
    .bLength            = sizeof(T_UAC1_AS_HDR_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_AS_GENERAL,
    .bTerminalLink      = ID_INPUT_TERMINAL2,
    .bDelay             = 1,
    .wFormatTag         = UAC1_FORMAT_TYPE_I_PCM,
};

const T_UAC1_FMT_TYPE_I_DESC format_type_i_desc1 =
{
    .bLength            = sizeof(T_UAC1_FMT_TYPE_I_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_FORMAT_TYPE,
    .bFormatType        = UAC1_FORMAT_TYPE_I_PCM,
    .bNrChannels        = 2,
    .bSubframeSize      = 3,//TODO
    .bBitResolution     = 24,
    .bSamFreqType       = 1,
    .tSamFreq           =
    {
//      [0] = {[0] = 0x80, [1] = 0xBB, [2] = 0x00},
        [0] = {[0] = 0x44, [1] = 0xAC, [2] = 0x00},
        [1] = {[0] = 0x80, [1] = 0xBB, [2] = 0x00},
        [2] = {[0] = 0x00, [1] = 0x77, [2] = 0x01}
    },

};

T_UAC1_STD_EP_DESC     out_ep_desc =
{
    .bLength            = sizeof(T_UAC1_STD_EP_DESC),
    .bDescriptorType    = UAC1_DT_ENDPOINT,
    .bEndpointAddress   = UAC1_ISO_OUT_ENDPOINT_ADDRESS,
    .bmAttributes       = UAC1_EP_XFER_ISOC | UAC1_EP_SYNC_ADAPTIVE,
    .wMaxPacketSize     = 96 * 3 * 2, //TODO
    .bInterval          = 3,
};

const T_UAC1_ISO_EP_DESC iso_out_ep_desc =
{
    .bLength            = sizeof(T_UAC1_ISO_EP_DESC),
    .bDescriptorType    = UAC1_DT_CS_ENDPOINT,
    .bDescriptorSubtype = UAC1_EP_GENERAL,
    .bmAttributes       = 1,
    .bLockDelayUnits    = 1,
    .wLockDelay         = 1,
};

T_UAC1_STD_ITF_DESC     interface_alt0_desc2 =
{
    .bLength            = sizeof(T_UAC1_STD_ITF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 2,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

T_UAC1_STD_ITF_DESC     interface_alt1_desc2 =
{
    .bLength            = sizeof(T_UAC1_STD_ITF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 2,
    .bAlternateSetting  = 1,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

T_UAC1_AS_HDR_DESC    as_header_desc2 =
{
    .bLength            = sizeof(T_UAC1_AS_HDR_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_AS_GENERAL,
    .bTerminalLink      = ID_OUTPUT_TERMINAL2,
    .bDelay             = 1,
    .wFormatTag         = UAC1_FORMAT_TYPE_I_PCM,
};

const T_UAC1_FMT_TYPE_I_DESC format_type_i_desc2 =
{
    .bLength            = sizeof(T_UAC1_FMT_TYPE_I_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_FORMAT_TYPE,
    .bFormatType        = UAC1_FORMAT_TYPE_I_PCM,
    .bNrChannels        = 1,
    .bSubframeSize      = 2,//TODO
    .bBitResolution     = 16,
    .bSamFreqType       = 1,
    .tSamFreq           =
    {
        [0] = {[0] = 0x80, [1] = 0x3E, [2] = 0x00},
    },

};

const T_UAC1_STD_EP_DESC     in_ep_desc =
{
    .bLength            = sizeof(T_UAC1_STD_EP_DESC),
    .bDescriptorType    = UAC1_DT_ENDPOINT,
    .bEndpointAddress   = UAC1_ISO_IN_ENDPOINT_ADDRESS,
    .bmAttributes       = UAC1_EP_XFER_ISOC | UAC1_EP_SYNC_ADAPTIVE,
    .wMaxPacketSize     = 96 * 3 * 1, //TODO
    .bInterval          = 1,
};

T_UAC1_ISO_EP_DESC iso_in_ep_desc =
{
    .bLength            = sizeof(T_UAC1_ISO_EP_DESC),
    .bDescriptorType    = UAC1_DT_CS_ENDPOINT,
    .bDescriptorSubtype = UAC1_EP_GENERAL,
    .bmAttributes       = 1,
    .bLockDelayUnits    = 0,
    .wLockDelay         = 0,
};

const T_UAC1_STD_ITF_DESC     ac_interface_desc =
{
    .bLength            = sizeof(T_UAC1_STD_ITF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOCONTROL,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

#define UAC1_CS_DESC_LEN             (sizeof(ac_hdr_desc)                 \
                                      + sizeof(input_terminal_desc0)       \
                                      + sizeof(input_terminal_desc1)       \
                                      + sizeof(output_terminal_desc0)      \
                                      + sizeof(output_terminal_desc1)      \
                                      + sizeof(feature_uint_desc0)         \
                                      + sizeof(feature_uint_desc1))

const T_UAC1_AC_HDR_DESC ac_hdr_desc =
{
    .bLength            = sizeof(T_UAC1_AC_HDR_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_HEADER,
    .bcdADC             = 0x0100,
    .wTotalLength       = UAC1_CS_DESC_LEN, //TODO
    .bInCollection      = 2,
    .baInterfaceNr[0]   = 1,
    .baInterfaceNr[1]   = 2,
};


#define AC_DESCS        (void*)&ac_interface_desc,    \
    (void*)&ac_hdr_desc,          \
    (void*)&input_terminal_desc0, \
    (void*)&input_terminal_desc1, \
    (void*)&output_terminal_desc0,\
    (void*)&output_terminal_desc1,\
    (void*)&feature_uint_desc0,   \
    (void*)&feature_uint_desc1,

#define OUT_STREM_DESCS     (void*)&interface_alt0_desc1,\
    (void*)&interface_alt1_desc1,\
    (void*)&as1_header_desc1,    \
    (void*)&format_type_i_desc1, \
    (void*)&out_ep_desc,         \
    (void*)&iso_out_ep_desc,

#define IN_STREM_DESCS      (void*)&interface_alt0_desc2,\
    (void*)&interface_alt1_desc2,\
    (void*)&as_header_desc2,     \
    (void*)&format_type_i_desc2, \
    (void*)&in_ep_desc,          \
    (void*)&iso_in_ep_desc,


const void  *uac1_descs[] =
{
    AC_DESCS
    OUT_STREM_DESCS
    IN_STREM_DESCS
    NULL
};

static int32_t vol_attr_set_spk(void *ctrl, uint8_t cmd, int value)
{
    int32_t vol = ((int32_t *)ctrl)[cmd - 1] - ((int32_t *)ctrl)[UAC1_MIN - 1];

    APP_PRINT_INFO2("vol_attr_set,cmd:0x%x, value:0x%x", cmd, value);
    if (usb_audio1_ctrl_cb && usb_audio1_ctrl_cb[CTRL_CB_SPK_VOL_SET])
    {
        usb_audio1_ctrl_cb[CTRL_CB_SPK_VOL_SET](vol);
    }
    return 0;

}

static int32_t vol_attr_get_spk(void *ctrl, uint8_t cmd)
{
    int32_t data = ((int32_t *)ctrl)[cmd - 1];
    static bool get_min = false;
    static bool get_max = false;
    if (usb_audio1_ctrl_cb && usb_audio1_ctrl_cb[CTRL_CB_SPK_VOL_GET])
    {
        T_USB_AUDIO_VOL vol;
        if (cmd == UAC1_CUR)
        {
            vol.type = USB_AUDIO_VOL_TYPE_CUR;
            vol.value = data;
        }
        else if (cmd == UAC1_MIN)
        {
            get_min = true;
        }
        else if (cmd == UAC1_MAX)
        {
            get_max = true;
        }
        if (get_min & get_max)
        {
            vol.type = USB_AUDIO_VOL_TYPE_RANGE;
            vol.value = ((int32_t *)ctrl)[UAC1_MAX - 1] - ((int32_t *)ctrl)[UAC1_MIN - 1];
            usb_audio1_ctrl_cb[CTRL_CB_SPK_VOL_GET](vol);
        }
    }
    APP_PRINT_INFO2("vol_attr_get,cmd:0x%x-0x%x", cmd, data);
    return data;
}

static int32_t mute_attr_set(void *ctrl, uint8_t cmd, int value)
{
    APP_PRINT_INFO2("mute_attr_set,cmd:0x%x, value:0x%x", cmd, value);
    if (usb_audio1_ctrl_cb && usb_audio1_ctrl_cb[CTRL_CB_SPK_MUTE_SET])
    {
        usb_audio1_ctrl_cb[CTRL_CB_SPK_MUTE_SET](value);
    }
    return 0;

}

static int32_t mute_attr_get(void *ctrl, uint8_t cmd)
{
    int32_t data = ((int *)ctrl)[cmd - 1];

    return data;
}

/*****************************************************************/
/*                             mic                               */
/*****************************************************************/
static int32_t mic_vol_attr_set(void *ctrl, uint8_t cmd, int value)
{

    APP_PRINT_INFO2("mic_vol_attr_set,cmd:0x%x, value:0x%x", cmd, value);
    return 0;

}

static int32_t mic_vol_attr_get(void *ctrl, uint8_t cmd)
{
    int32_t data = ((int32_t *)ctrl)[cmd - 1];

    APP_PRINT_INFO2("mic_vol_attr_get,cmd:0x%x-0x%x", cmd, data);

    return data;
}

static int32_t mic_mute_attr_set(void *ctrl, uint8_t cmd, int value)
{

    APP_PRINT_INFO2("mic_mute_attr_set,cmd:0x%x, value:0x%x", cmd, value);

    return 0;

}

static int32_t mic_mute_attr_get(void *ctrl, uint8_t cmd)
{
    return 0;
}
/******************************************************************/
static T_CTRL_ATTR vol_attr0 =
{
    .attr =
    {
        .cur = 0x0032,
        .min = 0x0002,
        .max = 0x0081,
        .res = 0x0001,
    }
};

static T_CTRL_ATTR vol_attr1 =
{
    .attr =
    {
        .cur = 0x0200,
        .min = 0xF400,
        .max = 0x0700,
        .res = 0x0100,
    }
};

static T_CTRL_ATTR mute_attr0 =
{
    .attr =
    {
        .cur = 0,
        .min = 0,
        .max = 1,
        .res = 1,
    }
};

static T_CTRL_ATTR mute_attr1 =
{
    .attr =
    {
        .cur = 0,
        .min = 0,
        .max = 1,
        .res = 1,
    }
};

const T_USB_AUDIO1_CTRL  vol_ctrl_spk =
{
    .name = "Volume Control",
    .type = UAC1_FU_VOLUME_CONTROL,
    .attr = &vol_attr0,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)vol_attr_get_spk,
        .set = (UAC_KITS_FUNC_INT)vol_attr_set_spk,
    },

};

const T_USB_AUDIO1_CTRL  mute_ctrl_spk =
{
    .name = "Mute Control",
    .type = UAC1_FU_MUTE_CONTROL,
    .attr = &mute_attr0,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)mute_attr_get,
        .set = (UAC_KITS_FUNC_INT)mute_attr_set,
    },

};

const T_USB_AUDIO1_CTRL  mic_vol_ctrl =
{
    .name = "Volume Control",
    .type = UAC1_FU_VOLUME_CONTROL,
    .attr = &vol_attr1,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)mic_vol_attr_get,
        .set = (UAC_KITS_FUNC_INT)mic_vol_attr_set,
    },

};

const T_USB_AUDIO1_CTRL  mic_mute_ctrl =
{
    .name = "Mute Control",
    .type = UAC1_FU_MUTE_CONTROL,
    .attr = &mute_attr1,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)mic_mute_attr_get,
        .set = (UAC_KITS_FUNC_INT)mic_mute_attr_set,
    },

};

const T_USB_AUDIO1_CTRL_KITS  uac1_ctrl_kits[2] =
{
    [0] =
    {
        .name = "Mute & Volume Control",
        .desc = (void *) &feature_uint_desc0,
        .id = ID_FEATURE_UNIT1,
        .type = UAC1_FEATURE_UNIT,

        .ctrl[0] = &vol_ctrl_spk,
        .ctrl[1] = &mute_ctrl_spk,
        .ctrl[2] = NULL,
    },

    [1] =
    {
        .name = "Mute & Volume Control",
        .desc = (void *) &feature_uint_desc1,
        .id = ID_FEATURE_UNIT2,
        .type = UAC1_FEATURE_UNIT,

        .ctrl[0] = &mic_vol_ctrl,
        .ctrl[1] = &mic_mute_ctrl,
        .ctrl[2] = NULL,
    },
};
static void usb_audio1_descs_register(void)
{
    usb_audio_driver_desc_register((void **)uac1_descs, USB_AUDIO_VERSION_1);
}

static void usb_audio1_ctrl_register(void)
{
    T_USB_AUDIO_DRIVER_CTRL audio_ctrl = {.num = sizeof(uac1_ctrl_kits) / sizeof(uac1_ctrl_kits[0]), .buf = (void *)uac1_ctrl_kits};
    usb_audio_driver_ctrl_register((void *)&audio_ctrl, USB_AUDIO_VERSION_1);
}

void usb_audio_ctrl_cb_register(USB_AUDIO_CTRL_CB *cbs)
{
    usb_audio1_ctrl_cb = cbs;
}

void usb_audio_streaming_cb_register(USB_AUDIO_STREAM_CB *cbs)
{
    usb_audio_driver_cb_register(cbs, 0);
}

void usb_audio_init(void)
{
    usb_audio1_descs_register();
    usb_audio1_ctrl_register();
}

#endif
