#ifndef _METADATA_DEF_H_
#define _METADATA_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/*The Preferred_Audio_Contexts LTV structure is typically included
in the Metadata field of PAC records exposed by Unicast Servers and Broadcast Sinks. */
#define METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS 0x01

/*The Streaming_Audio_Contexts LTV structure is typically included in a Metadata parameter value
when initiating the Enable or Update Metadata ASE Control operations for unicast Audio Streams,
or in the Metadata parameter value included in a BASE structure for broadcast Audio Streams.*/
#define METADATA_TYPE_STREAMING_AUDIO_CONTEXTS 0x02

#define METADATA_TYPE_VENDOR_SPECIFIC          0x03 //For IOP, different with BAPS_assigned_number 0xFF
#define METADATA_TYPE_CCCD_LIST                0x04 //fixme later, CAP

//BAPS_Assigned_Numbers_v7, different with IOP
#define AUDIO_CONTEXT_UNSPECIFIED          0x0001
#define AUDIO_CONTEXT_CONVERSATIONAL       0x0002
#define AUDIO_CONTEXT_MEDIA                0x0004
#define AUDIO_CONTEXT_GAME                 0x0008
#define AUDIO_CONTEXT_INSTRUCTIONAL        0x0010
#define AUDIO_CONTEXT_VOICE_ASSISTANTS     0x0020
#define AUDIO_CONTEXT_LIVE                 0x0040
#define AUDIO_CONTEXT_SOUND_EFFECTS        0x0080
#define AUDIO_CONTEXT_NOTIFICATIONS        0x0100
#define AUDIO_CONTEXT_RINGTONE             0x0200
#define AUDIO_CONTEXT_ALERTS               0x0400
#define AUDIO_CONTEXT_EMERGENCY_ALERT      0x0800
#define AUDIO_CONTEXT_MASK                 0x0FFF

bool metadata_check_format(uint16_t len, uint8_t *p_data, uint8_t *p_resp_code, uint8_t *p_reason);

#define MCS_CCID_PRE_IDX      0x80
#define TBS_CCID_PRE_IDX      0x40

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
