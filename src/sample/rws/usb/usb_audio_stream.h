#ifndef __USB_AUDIO_STREAM_H__
#define __USB_AUDIO_STREAM_H__
#include "audio_type.h"

typedef enum
{
    STREAM_STATE_IDLE,
    STREAM_STATE_OPENING,
    STREAM_STATE_OPENED,
    STREAM_STATE_STARTING,
    STREAM_STATE_STARTED,
    STREAM_STATE_SUSPEND,
    STREAM_STATE_DATA_XMIT,
    STREAM_STATE_STOPING,
    STREAM_STATE_STOPED,
    STREAM_STATE_CLOSING,
} T_AUDIO_STREAM_STATE;

#define T_STREAM_ATTR   T_AUDIO_PCM_ATTR
typedef struct _usb_audio_stream
{
    void                 *track;
    T_STREAM_ATTR        attr;
    T_AUDIO_STREAM_STATE state;
} T_USB_AUDIO_STREAM;


typedef bool (*T_USB_AUDIO_ASYNC_IO)();
#endif
