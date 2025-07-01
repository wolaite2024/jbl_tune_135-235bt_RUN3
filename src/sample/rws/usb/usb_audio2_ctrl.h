#ifndef __USB_AUDIO2_CTRL_H_
#define __USB_AUDIO2_CTRL_H_
#include <stdint.h>

#pragma pack(push,1)
typedef struct _layout1_subrange
{
    uint8_t bMIN;
    uint8_t bMAX;
    uint8_t bRES;
} T_LAYOUT1_SUBRANGE;

typedef struct _layout1_rang_param_block
{
    uint16_t wNumSubRanges;
    T_LAYOUT1_SUBRANGE ranges[1];//TODO
} T_LAYOUT1_RANGE_PARAM_BLOCK;

typedef struct _layout1_cur_param_block
{
    uint8_t     bCUR;
} T_LAYOUT1_CUR_PARAM_BLOCK;

typedef struct _layout2_subrange
{
    int16_t wMIN;
    int16_t wMAX;
    int16_t wRES;
} T_LAYOUT2_SUBRANGE;

typedef struct _layout2_rang_param_block
{
    uint16_t wNumSubRanges;
    T_LAYOUT2_SUBRANGE ranges[1];//TODO
} T_LAYOUT2_RANGE_PARAM_BLOCK;

typedef struct _layout2_cur_param_block
{
    int16_t     wCUR;
} T_LAYOUT2_CUR_PARAM_BLOCK;

typedef struct _layout3_subrange
{
    uint32_t dMIN;
    uint32_t dMAX;
    uint32_t dRES;
} T_LAYOUT3_SUBRANGE;

typedef struct _layout3_rang_param_block
{
    uint16_t wNumSubRanges;
    T_LAYOUT3_SUBRANGE ranges[3];//TODO
} T_LAYOUT3_RANGE_PARAM_BLOCK;

typedef struct _layout3_cur_param_block
{
    uint32_t     dCUR;
} T_LAYOUT3_CUR_PARAM_BLOCK;

#pragma pack(pop)

typedef struct _ctrl_attr_freq
{
    T_LAYOUT3_CUR_PARAM_BLOCK   cur;
    T_LAYOUT3_RANGE_PARAM_BLOCK range;

} T_CTRL_ATTR_FREQ;

typedef struct _ctrl_attr_vol
{
    T_LAYOUT2_CUR_PARAM_BLOCK   cur;
    T_LAYOUT2_RANGE_PARAM_BLOCK range;

} T_CTRL_ATTR_VOL;

typedef struct _ctrl_attr_mute
{
    T_LAYOUT1_CUR_PARAM_BLOCK   cur;

} T_CTRL_ATTR_MUTE;

typedef int32_t (*UAC_KITS_FUNC_INT)();

typedef struct _ctrl_func
{
    UAC_KITS_FUNC_INT   set;
    UAC_KITS_FUNC_INT   get;
} T_CTRL_FUNC;

typedef struct _usb_audio_ctrl
{
    const char  *name;
    uint8_t     type;
    void        *attr;
    T_CTRL_FUNC func;
} T_USB_AUDIO_CTRL;

typedef struct _usb_audio_ctrl_kits
{

    uint8_t             id;
    const char          *name;
    uint8_t             type;
    void                *desc;
    const T_USB_AUDIO_CTRL    *ctrl[3];
} T_USB_AUDIO_CTRL_KITS;

typedef struct _usb_audio_ctrl_buf
{
    uint32_t length;
    void     *buf;
} T_USB_AUDIO_CTRL_BUF;

#endif
