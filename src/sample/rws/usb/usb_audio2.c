#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "trace.h"
#include "usb_std_desc.h"
#include "usb_audio_config.h"
#include "usb_audio2_desc.h"
#include "usb_audio_driver.h"
#include "usb_audio2_ctrl.h"
#include "usb_audio.h"

#if(USB_AUDIO_VERSION == USB_AUDIO_VERSION_2)
BOOL_USB_AUDIO_CB *usb_audio2_ctrl_cb = NULL;

#define UAC2_MIC                0
static const T_UAC2_STD_IA_DESC uac2_ia_desc =
{
    .bLength                = sizeof(T_UAC2_STD_IA_DESC),
    .bDescriptorType        = UAC2_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface        = 0,
#if(UAC2_MIC == 1)
    .bInterfaceCount        = 3,
#else
    .bInterfaceCount        = 2,
#endif
    .bFunctionClass         = UAC2_CLASS_CODE_AUDIO,
    .bFunctionSubClass      = UAC2_FUNCTION_SUBCLASS_UNDEFINED,
    .bFunctionProtocol      = UAC2_AF_VERSION_02_00,
    .iFunction              = 0,
};

static T_UAC2_STD_ITF_DESC uac2_std_ac_itf_desc =
{
    .bLength                = sizeof(T_UAC2_STD_ITF_DESC),
    .bDescriptorType        = UAC2_DT_INTERFACE,
    .bInterfaceNumber       = 0,
    .bAlternateSetting      = 0,
    .bNumEndpoints          = 0,
    .bInterfaceClass        = UAC2_CLASS_CODE_AUDIO,
    .bInterfaceSubClass     = UAC2_SUBCLASS_AUDIOCONTROL,
    .bInterfaceProtocol     = IP_VERSION_02_00,
    .iInterface             = 0,
};

const T_UAC2_CLK_SRC_DESC uac2_cs_desc1 =
{
    .bLength                = sizeof(T_UAC2_CLK_SRC_DESC),
    .bDescriptorType        = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC2_CLOCK_SOURCE,
    .bClockID               = ID_CLOCK_SOURCE1,
    .bmAttributes           = 0x03,
    .bmControls             = 0x03,
    .bAssocTerminal         = 0x00,
    .iClockSource           = 0,
};

const T_UAC2_FU_DESC uac2_fu_desc =
{
    .bLength                = sizeof(T_UAC2_FU_DESC),
    .bDescriptorType        = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC2_FEATURE_UNIT,
    .bUnitID                = ID_FEATURE_UNIT,
    .bSourceID              = ID_INPUT_TERMINAL1,
    .bmaControls            =
    {[0] = 0x00000003, [1] = 0x0000000C, [2] = 0x0000000C,},
    .iFeature               = 0,
};

static const T_UAC2_IT_DESC uac2_it_desc =
{
    .bLength                = sizeof(T_UAC2_IT_DESC),
    .bDescriptorType        = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC2_INPUT_TERMINAL,
    .bTerminalID            = ID_INPUT_TERMINAL1,
    .wTerminalType          = UAC2_TERMINAL_STREAMING,
    .bAssocTerminal         = 0x00,
    .bCSourceID             = ID_CLOCK_SOURCE1,
    .bNrChannels            = 0x02,
    .bmChannelConfig        = 0x03,
    .iChannelNames          = 0x00,
    .bmControls             = 0x00,
    .iTerminal              = 0x00,
};

static const T_UAC2_OT_DESC uac2_ot_desc =
{
    .bLength                = sizeof(T_UAC2_OT_DESC),
    .bDescriptorType        = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC2_OUTPUT_TERMINAL,
    .bTerminalID            = ID_OUTPUT_TERMINAL1,
    .wTerminalType          = UAC2_OUTPUT_TERMINAL_SPEAKER,
    .bAssocTerminal         = 0x00,
    .bSourceID              = ID_FEATURE_UNIT,
    .bCSourceID             = ID_CLOCK_SOURCE1,
    .bmControls             = 0x00,
    .iTerminal              = 0x00,
};

#if(UAC2_MIC == 1)
static const T_UAC2_IT_DESC uac2_it_desc_mic =
{
    .bLength                = sizeof(T_UAC2_IT_DESC),
    .bDescriptorType        = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC_INPUT_TERMINAL,
    .bTerminalID            = ID_INPUT_TERMINAL2,
    .wTerminalType          = UAC_INPUT_TERMINAL_MICROPHONE,
    .bAssocTerminal         = 0x00,
    .bCSourceID             = ID_CLOCK_SOURCE2,
    .bNrChannels            = 0x02,
    .bmChannelConfig        = 0x03,
    .iChannelNames          = 0x00,
    .bmControls             = 0x00,
    .iTerminal              = 0x00,
};

static const T_UAC2_OT_DESC uac2_ot_desc_mic =
{
    .bLength                = sizeof(T_UAC2_OT_DESC),
    .bDescriptorType        = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC_OUTPUT_TERMINAL,
    .bTerminalID            = ID_OUTPUT_TERMINAL2,
    .wTerminalType          = UAC_TERMINAL_STREAMING,
    .bAssocTerminal         = 0x00,
    .bSourceID              = ID_FEATURE_UNIT2,
    .bCSourceID             = ID_CLOCK_SOURCE2,
    .bmControls             = 0x00,
    .iTerminal              = 0x00,
};

static const T_UAC2_FU_DESC_MIC uac2_fu_desc_mic =
{
    .bLength                = sizeof(T_UAC2_FU_DESC),
    .bDescriptorType        = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC_FEATURE_UNIT,
    .bUnitID                = ID_FEATURE_UNIT2,
    .bSourceID              = ID_INPUT_TERMINAL2,
    .bmaControls            =
    {[0] = 0x0000000F, [1] = 0x00000000},
    .iFeature               = 0,
};

#endif

const T_UAC2_STD_EP_DESC uac2_std_ep_desc_intr =
{
    .bLength                = sizeof(T_UAC2_STD_EP_DESC),
    .bDescriptorType        = UAC2_DT_ENDPOINT,
    .bEndpointAddress       = UAC2_DIR_IN | 0x02,
    .bmAttributes           = UAC2_EP_XFER_INT,
    .wMaxPacketSize         = 6,
    .bInterval              = 3,
};

static const T_UAC2_AC_ITF_HDR_DESC uac2_ac_itf_hdr_desc =
{
    .bLength                = sizeof(T_UAC2_AC_ITF_HDR_DESC),
    .bDescriptorType        = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype     = UAC2_HEADER,
    .bcdADC                 = 0x0200,
    .bCategory              = UAC2_CATEGORY_HEADSET,
#if(UAC2_MIC == 1)
    .wTotalLength           = sizeof(uac2_ac_itf_hdr_desc) + sizeof(uac2_cs_desc1) + sizeof(uac2_cs_desc2)
    + sizeof(uac2_fu_desc) + sizeof(uac2_it_desc) + sizeof(uac2_fu_desc_mic)
    + sizeof(uac2_ot_desc) + sizeof(uac2_it_desc_mic) + sizeof(uac2_ot_desc_mic),
#else
    .wTotalLength           = sizeof(uac2_ac_itf_hdr_desc) + sizeof(uac2_cs_desc1)
    + sizeof(uac2_fu_desc) + sizeof(uac2_it_desc)
    + sizeof(uac2_ot_desc),
#endif
    .bmControls             = 0x00,
};

static T_UAC2_STD_ITF_DESC uac2_std_as_itf_desc_alt0 =
{
    .bLength                = sizeof(T_UAC2_STD_ITF_DESC),
    .bDescriptorType        = UAC2_DT_INTERFACE,
    .bInterfaceNumber       = 1,
    .bAlternateSetting      = 0,
    .bNumEndpoints          = 0,
    .bInterfaceClass        = UAC2_CLASS_CODE_AUDIO,
    .bInterfaceSubClass     = UAC2_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol     = IP_VERSION_02_00,
    .iInterface             = 0x00,
};

static T_UAC2_STD_ITF_DESC uac2_std_as_itf_desc_alt1 =
{
    .bLength                = sizeof(T_UAC2_STD_ITF_DESC),
    .bDescriptorType        = UAC2_DT_INTERFACE,
    .bInterfaceNumber       = 1,
    .bAlternateSetting      = 1,
    .bNumEndpoints          = 2,
    .bInterfaceClass        = UAC2_CLASS_CODE_AUDIO,
    .bInterfaceSubClass     = UAC2_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol     = IP_VERSION_02_00,
    .iInterface             = 0x00,
};


static const T_UAC2_AS_ITF_DESC uac2_as_itf_desc =
{
    .bLength               = sizeof(T_UAC2_AS_ITF_DESC),
    .bDescriptorType       = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype    = UAC2_AS_GENERAL,
    .bTerminalLink         = ID_INPUT_TERMINAL1,
    .bmControls            = 0x03,
    .bFormatType           = UAC2_FORMAT_TYPE_I_PCM,
    .bmFormats             = 0x01,//PCM
    .bNrChannels           = 0x02,
    .bmChannelConfig       = 0x03,
    .iChannelNames         = 0x00,
};

static const T_UAC2_AS_TYPE_1_FMT_DESC uac2_as_type_1_fmt_desc =
{
    .bLength               = sizeof(T_UAC2_AS_TYPE_1_FMT_DESC),
    .bDescriptorType       = UAC2_DT_CS_INTERFACE,
    .bDescriptorSubtype    = UAC2_FORMAT_TYPE,
    .bFormatType           = UAC2_FORMAT_TYPE_I_PCM,
    .bSubslotSize          = 3,
    .bBitResolution        = UAC2_SPK_BIT_RES,
};

const T_UAC2_STD_EP_DESC uac2_std_ep_desc =
{
    .bLength                = sizeof(T_UAC2_STD_EP_DESC),
    .bDescriptorType        = UAC2_DT_ENDPOINT,
    .bEndpointAddress       = UAC2_ISO_OUT_ENDPOINT_ADDRESS,
    .bmAttributes           = UAC2_EP_XFER_ISOC | UAC2_EP_SYNC_ADAPTIVE,
    .wMaxPacketSize         = 576,
    .bInterval              = UAC2_DS_INTERVAL,
};

static const T_UAC2_AS_ISO_EP_DESC uac2_iso_ep_desc =
{
    .bLength                = sizeof(T_UAC2_AS_ISO_EP_DESC),
    .bDescriptorType        = UAC2_DT_CS_ENDPOINT,
    .bDescriptorSubtype     = UAC2_EP_GENERAL,
    .bmAttributes           = 0x00,
    .bmControls             = 0x00,
    .bLockDelayUnits        = 0x00,
    .wLockDelay             = 0x0000,
};

static const T_UAC2_STD_EP_DESC uac2_feedback_ep_desc =
{
    .bLength                = sizeof(T_UAC2_STD_EP_DESC),
    .bDescriptorType        = UAC2_DT_ENDPOINT,
    .bEndpointAddress       = UAC2_ISO_IN_FEEDBACK_EP_ADDR,
    .bmAttributes           = UAC2_EP_XFER_ISOC | UAC2_EP_SYNC_NONE | (1 << 4),
    .wMaxPacketSize         = 576,
    .bInterval              = 3,
};

#if(UAC2_MIC == 1)
static struct usb_interface_descriptor uac2_std_as_itf_desc_alt0_mic =
{
    .bLength                = sizeof(struct usb_interface_descriptor),
    .bDescriptorType        = USB_DT_INTERFACE,
    .bInterfaceNumber       = 2,
    .bAlternateSetting      = 0,
    .bNumEndpoints          = 0,
    .bInterfaceClass        = USB_CLASS_AUDIO,
    .bInterfaceSubClass     = USB_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol     = IP_VERSION_0200,
    .iInterface             = 0x00,
};

static struct usb_interface_descriptor uac2_std_as_itf_desc_alt1_mic =
{
    .bLength                = sizeof(struct usb_interface_descriptor),
    .bDescriptorType        = USB_DT_INTERFACE,
    .bInterfaceNumber       = 2,
    .bAlternateSetting      = 1,
    .bNumEndpoints          = 1,
    .bInterfaceClass        = USB_CLASS_AUDIO,
    .bInterfaceSubClass     = USB_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol     = IP_VERSION_0200,
    .iInterface             = 0x00,
};


static const T_UAC2_AS_ITF_DESC uac2_as_itf_desc_mic =
{
    .bLength               = sizeof(T_UAC2_AS_ITF_DESC),
    .bDescriptorType       = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype    = UAC_AS_GENERAL,
    .bTerminalLink         = ID_OUTPUT_TERMINAL2,
    .bmControls            = 0x05,
    .bFormatType           = UAC_FORMAT_TYPE_I_PCM,
    .bmFormats             = 0x01,//PCM
    .bNrChannels           = 0x01,
    .bmChannelConfig       = 0x04,
    .iChannelNames         = 0x00,
};

static const T_UAC2_AS_TYPE_1_FMT_DESC uac2_as_type_1_fmt_desc_mic =
{
    .bLength               = sizeof(T_UAC2_AS_TYPE_1_FMT_DESC),
    .bDescriptorType       = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype    = UAC_FORMAT_TYPE,
    .bFormatType           = UAC_FORMAT_TYPE_I_PCM,
    .bSubslotSize          = 2,
    .bBitResolution        = BIT_RES_16,
};

const struct usb_endpoint_descriptor uac2_std_ep_desc_mic =
{
    .bLength                = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType        = USB_DT_ENDPOINT,
    .bEndpointAddress       = UAC2_ISO_IN_ENDPOINT_ADDRESS,
    .bmAttributes           = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE,
    .wMaxPacketSize         = 1024,
    .bInterval              = 3,
};

static const T_UAC2_AS_ISO_EP_DESC uac2_iso_ep_desc_mic =
{
    .bLength                = sizeof(T_UAC2_AS_ISO_EP_DESC),
    .bDescriptorType        = USB_DT_CS_ENDPOINT,
    .bDescriptorSubtype     = UAC_EP_GENERAL,
    .bmAttributes           = 0x00,
    .bmControls             = 0x01,
    .bLockDelayUnits        = 0x00,
    .wLockDelay             = 0x0000,
};
#endif

static const void *uac2_descs[] =
{
    (void *) &uac2_ia_desc,
    (void *) &uac2_std_ac_itf_desc,
    (void *) &uac2_ac_itf_hdr_desc,
    (void *) &uac2_cs_desc1,
    (void *) &uac2_fu_desc,
    (void *) &uac2_it_desc,
    (void *) &uac2_ot_desc,
    (void *) &uac2_std_as_itf_desc_alt0,
    (void *) &uac2_std_as_itf_desc_alt1,
    (void *) &uac2_as_itf_desc,
    (void *) &uac2_as_type_1_fmt_desc,
    (void *) &uac2_std_ep_desc,
    (void *) &uac2_iso_ep_desc,
    (void *) &uac2_feedback_ep_desc,
    NULL
};

static int32_t vol_attr_set_spk(void *ctrl, uint8_t cmd, int value)
{
    T_CTRL_ATTR_VOL *vol_attr = (T_CTRL_ATTR_VOL *)(*(uint32_t *)ctrl);
    T_USB_AUDIO_CTRL_BUF *vol_ctrl_buf = (T_USB_AUDIO_CTRL_BUF *)value;
    uint16_t vol = *((uint16_t *)vol_ctrl_buf->buf) - vol_attr->range.ranges[0].wMIN;;
    if (usb_audio2_ctrl_cb && usb_audio2_ctrl_cb[CTRL_CB_SPK_VOL_SET])
    {
        usb_audio2_ctrl_cb[CTRL_CB_SPK_VOL_SET](vol);
    }
    APP_PRINT_INFO2("vol_attr_set_spk:0x%x-0x%x", cmd, vol);
    return 0;

}

static int32_t vol_attr_get_spk(void *ctrl, uint8_t cmd)
{
    T_CTRL_ATTR_VOL *vol_attr = (T_CTRL_ATTR_VOL *)(*(uint32_t *)ctrl);

    T_USB_AUDIO_CTRL_BUF *vol_ctrl_buf =  malloc((sizeof(T_USB_AUDIO_CTRL_BUF)));

    if (cmd == UAC2_CUR)
    {
        vol_ctrl_buf->length = sizeof(vol_attr->cur.wCUR);
        vol_ctrl_buf->buf = &(vol_attr->cur.wCUR);
    }
    else if (cmd == UAC2_RANGE)
    {
        vol_ctrl_buf->length = sizeof(vol_attr->range);
        vol_ctrl_buf->buf = &(vol_attr->range);
    }

    if (usb_audio2_ctrl_cb && usb_audio2_ctrl_cb[CTRL_CB_SPK_VOL_GET])
    {
        T_USB_AUDIO_VOL vol;
        if (cmd == UAC2_CUR)
        {
            vol.type = USB_AUDIO_VOL_TYPE_CUR;
            vol.value = vol_attr->cur.wCUR - vol_attr->range.ranges[0].wMIN;
        }
        else if (cmd == UAC2_RANGE)
        {
            vol.type = USB_AUDIO_VOL_TYPE_RANGE;
            vol.value = vol_attr->range.ranges[0].wMAX - vol_attr->range.ranges[0].wMIN;
        }
        usb_audio2_ctrl_cb[CTRL_CB_SPK_VOL_GET](vol);
    }
    APP_PRINT_INFO4("vol_attr_get_spk:0x%x-0x%x-0x%x-0x%x", cmd, vol_ctrl_buf->length,
                    vol_ctrl_buf->buf, vol_attr);
    return (int32_t)vol_ctrl_buf;
}

static int32_t mute_attr_set_spk(void *ctrl, uint8_t cmd, int value)
{
    T_USB_AUDIO_CTRL_BUF *mut_ctrl_buf = (T_USB_AUDIO_CTRL_BUF *)value;
    uint8_t mute = *((uint8_t *)mut_ctrl_buf->buf);
    if (usb_audio2_ctrl_cb && usb_audio2_ctrl_cb[CTRL_CB_SPK_MUTE_SET])
    {
        usb_audio2_ctrl_cb[CTRL_CB_SPK_MUTE_SET](mute);
    }
    APP_PRINT_INFO2("mute_attr_set_spk:0x%x-0x%x", cmd, mute);
    return 0;

}

static int32_t mute_attr_get_spk(void *ctrl, uint8_t cmd)
{

    T_CTRL_ATTR_MUTE *mute_attr = (T_CTRL_ATTR_MUTE *)(*(uint32_t *)ctrl);

    T_USB_AUDIO_CTRL_BUF *mute_ctrl_buf =  malloc(sizeof(T_USB_AUDIO_CTRL_BUF));

    if (cmd == UAC2_CUR)
    {
        mute_ctrl_buf->length = sizeof(mute_attr->cur.bCUR);
        mute_ctrl_buf->buf = &(mute_attr->cur.bCUR);
    }

//    if(usb_audio2_ctrl_cb && usb_audio2_ctrl_cb[CTRL_CB_SPK_MUTE_GET])
//    {
//        usb_audio2_ctrl_cb[CTRL_CB_SPK_MUTE_GET](mute_ctrl_buf->buf);
//    }
    APP_PRINT_INFO4("mute_attr_get_spk:0x%x-0x%x-0x%x-0x%x", cmd, mute_ctrl_buf->length,
                    mute_ctrl_buf->buf, mute_attr);
    return (int32_t)mute_ctrl_buf;
}

static int32_t sam_freq_get_spk(void *ctrl, uint8_t cmd)
{
    T_CTRL_ATTR_FREQ *freq_attr = (T_CTRL_ATTR_FREQ *)(*(uint32_t *)ctrl);

    T_USB_AUDIO_CTRL_BUF *freq_ctrl_buf =  malloc(sizeof(T_USB_AUDIO_CTRL_BUF));

    if (cmd == UAC2_CUR)
    {
        freq_ctrl_buf->length = sizeof(freq_attr->cur.dCUR);
        freq_ctrl_buf->buf = &(freq_attr->cur.dCUR);
        usb_audio_driver_freq_set(freq_attr->cur.dCUR, USB_AUDIO_VERSION_2);
    }
    else if (cmd == UAC2_RANGE)
    {
        freq_ctrl_buf->length = sizeof(freq_attr->range);
        freq_ctrl_buf->buf = &(freq_attr->range);
    }
    APP_PRINT_INFO4("sam_freq_get_spk:0x%x-0x%x-0x%x-0x%x", cmd, freq_ctrl_buf->length,
                    freq_ctrl_buf->buf, freq_attr);
    return (int32_t)freq_ctrl_buf;
}

static int32_t sam_freq_set_spk(void *ctrl, uint8_t cmd, int value)
{
    T_CTRL_ATTR_FREQ *freq_attr = (T_CTRL_ATTR_FREQ *)(*(uint32_t *)ctrl);
    T_USB_AUDIO_CTRL_BUF *freq_ctrl_buf = (T_USB_AUDIO_CTRL_BUF *)value;
    freq_attr->cur.dCUR = *((uint32_t *)freq_ctrl_buf->buf);
    usb_audio_driver_freq_set(*((uint32_t *)freq_ctrl_buf->buf), USB_AUDIO_VERSION_2);
    APP_PRINT_INFO4("sam_freq_set_spk:0x%x-0x%x-0x%x-0x%x", cmd, freq_ctrl_buf->length,
                    *((uint32_t *)freq_ctrl_buf->buf), freq_attr);
    return 0;

}

static T_CTRL_ATTR_FREQ freq_attr =
{
    .cur =  {.dCUR = UAC2_SPK_SAM_RATE_48000},
    .range =
    {
        .wNumSubRanges = UAC2_SPK_SAM_RATE_NUM,
        .ranges =
        {
            [0] = {.dMIN = UAC2_SPK_SAM_RATE_48000, .dMAX = UAC2_SPK_SAM_RATE_48000, .dRES = 0},
            [1] = {.dMIN = UAC2_SPK_SAM_RATE_96000, .dMAX = UAC2_SPK_SAM_RATE_96000, .dRES = 0},
            [2] = {.dMIN = UAC2_SPK_SAM_RATE_192000, .dMAX = UAC2_SPK_SAM_RATE_192000, .dRES = 0},

        },
    },
};

static const T_CTRL_ATTR_VOL vol_attr =
{
    .cur =  {.wCUR = UAC2_SPK_VOL_CUR},
    .range =
    {
        .wNumSubRanges = 1,
        .ranges =
        {
            [0] = {.wMIN = UAC2_SPK_VOL_MIN, .wMAX = UAC2_SPK_VOL_MAX, .wRES = UAC2_SPK_VOL_RES}
        },
    },
};

static const T_CTRL_ATTR_MUTE mute_attr =
{
    .cur =  {.bCUR = 0},
};


static const T_USB_AUDIO_CTRL  freq_ctrl =
{
    .name = "Freq Control",
    .type = UAC2_CS_SAM_FREQ_CONTROL,
    .attr = (void *) &freq_attr,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)sam_freq_get_spk,
        .set = (UAC_KITS_FUNC_INT)sam_freq_set_spk,
    },

};

static const T_USB_AUDIO_CTRL vol_ctrl =
{
    .name = "Volume Control",
    .type = UAC2_FU_VOLUME_CONTROL,
    .attr = (void *) &vol_attr,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)vol_attr_get_spk,
        .set = (UAC_KITS_FUNC_INT)vol_attr_set_spk,
    },

};

static const T_USB_AUDIO_CTRL  mute_ctrl =
{
    .name = "Mute Control",
    .type = UAC2_FU_MUTE_CONTROL,
    .attr = (void *) &mute_attr,
    .func =
    {
        .get = (UAC_KITS_FUNC_INT)mute_attr_get_spk,
        .set = (UAC_KITS_FUNC_INT)mute_attr_set_spk,
    },
};

static const T_USB_AUDIO_CTRL_KITS uac2_ctrl_kits[] =
{
    [0] =
    {
        .name = "Mute & Volume Control",
        .desc = (void *) &uac2_fu_desc,
        .id   = ID_FEATURE_UNIT,
        .type = UAC2_FEATURE_UNIT,
        .ctrl[0] = &vol_ctrl,
        .ctrl[1] = &mute_ctrl,
        .ctrl[2] = NULL,
    },

    [1] =
    {
        .name = "Freq Control",
        .desc = (void *) &uac2_cs_desc1,
        .id   = ID_CLOCK_SOURCE1,
        .type = UAC2_CLOCK_SOURCE,
        .ctrl[0] = &freq_ctrl,
        .ctrl[2] = NULL,
    },
};

static void usb_audio2_descs_register(void)
{
    usb_audio_driver_desc_register((void **)uac2_descs, USB_AUDIO_VERSION_2);
}

static void usb_audio2_ctrl_register(void)
{
    T_USB_AUDIO_DRIVER_CTRL audio_ctrl = {.num = sizeof(uac2_ctrl_kits) / sizeof(uac2_ctrl_kits[0]), .buf = (void *)uac2_ctrl_kits};
    usb_audio_driver_ctrl_register((void *)&audio_ctrl, USB_AUDIO_VERSION_2);
}

void usb_audio_ctrl_cb_register(USB_AUDIO_CTRL_CB *cbs)
{
    usb_audio2_ctrl_cb = cbs;
}

void usb_audio_streaming_cb_register(USB_AUDIO_STREAM_CB *cbs)
{
    usb_audio_driver_cb_register(cbs, 0);
}

void usb_audio_init(void)
{
    usb_audio2_descs_register();
    usb_audio2_ctrl_register();
}

#endif
