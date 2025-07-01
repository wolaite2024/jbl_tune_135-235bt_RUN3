#ifndef __USB_AUDIO1_CTRL_H_
#define __USB_AUDIO1_CTRL_H_
#include <stdint.h>

typedef int32_t (*UAC_KITS_FUNC_INT)();

#define UAC1_CUR                        0x1
#define UAC1_MIN                        0x2
#define UAC1_MAX                        0x3
#define UAC1_RES                        0x4
#define UAC1_MEM                        0x5

typedef union ctrl_attr
{
    int32_t    data[5];
    struct
    {
        int32_t    cur;
        int32_t    min;
        int32_t    max;
        int32_t    res;
        int32_t    mem;
    } attr;
} T_CTRL_ATTR;

typedef struct ctrl_func
{
    UAC_KITS_FUNC_INT   set;
    UAC_KITS_FUNC_INT   get;
} T_CTRL_FUNC;

typedef struct _usb_audio1_ctrl
{
    const char  *name;
    uint8_t     type;
    T_CTRL_ATTR *attr;
    T_CTRL_FUNC func;
} T_USB_AUDIO1_CTRL;

typedef struct _usb_audio1_ctrl_kits
{

    uint8_t             id;
    const char          *name;
    uint8_t             type;
    void                *desc;
    const T_USB_AUDIO1_CTRL    *ctrl[3];
} T_USB_AUDIO1_CTRL_KITS;

#endif
