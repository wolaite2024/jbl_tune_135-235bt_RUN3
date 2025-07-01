/**
***********************************************************************************
* Copyright (c) 2015 By Realtek Semiconductor Corp., Ltd. All Rights Reserved.
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of Realtek Semiconductor Corporation.
* The copyright notice above does not evidence any actual or intended
* Publication of such source code.
*
* @file      shm_command.h
* @brief
* @details
* @author    Y.T. Lin, chingting_peng
* @date      2015/12/11
* @version
***********************************************************************************
*/

#ifndef __SHM_COMMAND_H_
#define __SHM_COMMAND_H_

#define H2D_DSP_CONFIG_SIZE             1 // double word (32 bits)
#define H2D_TEST_MODE_SIZE              1
#define TM_MIC2SPK_LP                   0x01 // Loopback from MIC directly to SPK without passing through any signal processing functions
#define TM_TT1                          0x02 // insert test tone at the output of Audio codec
#define TM_TT2                          0x04 // insert test tone  at the output of SigProc, downstream path
#define TM_TT3                          0x08 // insert test tone at the output of MIC path
#define TM_TT4                          0x10 // insert test tone at the output of SigProc, upstream path
#define H2D_CODER_CONFIG_SIZE           1
#define FRAME_SIZE_MASK                 0x0FFF
#define GET_FRMAE_SIZE(x)               ((x) & FRAME_SIZE_MASK)
#define CVSD_FRAME_SIZE_30              30
#define CVSD_FRAME_SIZE_60              60
#define MSBC_FRAME_SIZE                 120
#define SBC_FRAME_SIZE                  256     // L+R
#define LDAC_FRAME_SIZE                 256
#define AACLC_FRAME_SIZE                2048    // L+R
#define SRC_MASK                        0xF000
#define CODER_ALGORITHM_MASK            0x1F0000
#define GET_CODER_ALGORITHM(x)          (((x) & CODER_ALGORITHM_MASK) >> 16)
#define NUM_VOICE_CODER                 4
/*
#define ALGORITHM_PROPRIETARY_VOICE     0
#define ALGORITHM_G711_ALAW             1
#define ALGORITHM_CVSD                  2
#define ALGORITHM_MSBC                  3
#define ALGORITHM_USB_SPEECH            4
#define ALGORITHM_VOICE                 ALGORITHM_USB_SPEECH
#define ALGORITHM_SBC                   5
#define ALGORITHM_AAC                   6
#define ALGORITHM_PROPRIETARY_AUDIO1    7 // SONY LDAC
#define ALGORITHM_PROPRIETARY_AUDIO2    8 // can be MP3, WMA, FLAC
#define ALGORITHM_MP3                   9
#define ALGORITHM_LINE_IN               10
#define ALGORITHM_USB_AUDIO             11
#define ALGORITHM_SUB_WOOFER            12
#define ALGORITHM_END                   13
*/

//Remember to update DecFuncTable,EncFuncTable,DecStructTable,EncStructTable in coder.c
//and DecInitTable,EncInitTable in ipc_command.c

typedef enum
{
    ALGORITHM_PROPRIETARY_VOICE,    //00
    ALGORITHM_G711_ALAW,            //01
    ALGORITHM_CVSD,                 //02
    ALGORITHM_MSBC,                 //03
    ALGORITHM_OPUS_VOICE,           //04
    ALGORITHM_LC3_VOICE,            //05
    ALGORITHM_USB_SPEECH,           //06
    ALGORITHM_SBC,                  //07
    ALGORITHM_AAC,                  //08
    ALGORITHM_PROPRIETARY_AUDIO1,   //09
    ALGORITHM_PROPRIETARY_AUDIO2,   //10
    ALGORITHM_MP3,                  //11
    ALGORITHM_USB_AUDIO,            //12
    ALGORITHM_LC3,                  //13
    ALGORITHM_LDAC,                 //14 SONY
    ALGORITHM_UHQ,                  //15 Samsung
    ALGORITHM_LHDC,                 //16 Savitech
    ALGORITHM_OPUS_AUDIO,           //17
    ALGORITHM_PROPRIETARY_AUDIO3,   //18
    ALGORITHM_PROPRIETARY_AUDIO4,   //19
    ALGORITHM_LINE_IN,              //20, MUST before ALGORITHM_END
    ALGORITHM_END                   //21
} T_ALGORITHM_TYPE;

#define ALGORITHM_VOICE                 ALGORITHM_USB_SPEECH

#define H2D_ANALOG_CODEC_CONTROL_SIZE       2
/*
#define ICODEC_SR_8K                    (0)
#define ICODEC_SR_16K                   (1)
#define ICODEC_SR_32K                   (2)
#define ICODEC_SR_44K                   (3)
#define ICODEC_SR_48K                   (4)
#define ICODEC_SR_88K                   (5)
#define ICODEC_SR_96K                   (6)
#define ICODEC_SR_192K                  (7)
#define ICODEC_SR_12K                   (8)
#define ICODEC_SR_24K                   (9)
#define ICODEC_SR_11K                   (10)
#define ICODEC_SR_22K                   (11)
#define ICODEC_SR_ALL                   (15)
*/
#define H2D_ANALOG_CODEC_SIZE               2
#define ICODEC_DAC_PATH_IN_DAC              (1<<7)
#define ICODEC_DAC_PATH_IN_LIN_BP           (1<<6)
#define ICODEC_DAC_PATH_IN_MIC_ST           (1<<5)
#define ICODEC_DAC_PATH_OUT_CAPLESS         (1<<4)
#define ICODEC_DAC_PATH_OUT_MASK            (0x0F)
#define ICODEC_DAC_PATH_NONE                0 //Not enable DAC
#define ICODEC_DAC_PATH_LOUT                1
#define ICODEC_DAC_PATH_BT_LOUT             2
#define ICODEC_DAC_PATH_1HP_OUT             3 //Headphone Output for Mono    (SCO Condition)L ->L/R
#define ICODEC_DAC_PATH_2HP_OUT             4 //Headphone Output for Stereo  (SBC Stereo Condition)L/R->L/R
#define ICODEC_DAC_CTL_DEEMPHON             (1<<7)
//#define ICODEC_DACdb(db,pdb)               (((-db*10+pdb)/15)&0xF)
//#define ICODEC_ADCPath_MicBoostGain        (1<<7)
#define ICODEC_ADC_PATH_MIC_BIAS_ON         (1<<6)
#define ICODEC_ADC_PATH_ADC_OFF             (1<<4)
#define ICODEC_ADC_PATH_IN_MASK             (0x0F)
#define ICODEC_ADC_PATH_NONE                0 //Not Enable ADC => SBC decode only
#define ICODEC_ADC_PATH_LIN                 1
#define ICODEC_ADC_PATH_DAC_MIX             2
#define ICODEC_ADC_PATH_1MIC_IN_R           3 /* One Micphone Input */
#define ICODEC_ADC_PATH_2MIC_IN_R           4 /* Two Micphone Input */
#define ICODEC_ADC_PATH_1MIC_IN_L           5 /* One Differential Micphone Input */
#define ICODEC_ADC_PATH_2MIC_IN_L           6 /* Two Differential Micphone Input */
#define ICODEC_ADC_PATH_LOOPBACK1           7 /* MIC1 to DAC1 and MIC2 to DAC2 */
#define ICODEC_ADC_PATH_LOOPBACK2           8 /* MIC2 to DAC1 and MIC1 to DAC2 */
#define ICODEC_ADC_CTL_AGC_ON               (1<<7)
#define ICODEC_ADC_CTL_HPF_ON               (1<<2)

#define H2D_DIGITAL_VOLUME_SIZE                 2
#define H2D_I2S_CONFIG_SIZE                     5
#define H2D_SIG_PROCESSING_CONFIG_SIZE          (128 >> 2)
//#define H2D_TONEGEN_CONFIG_SIZE                 15
#define H2D_ACTION_CTRL_SIZE                    1
#define H2D_DAC_GAIN_SIZE                       1
#define H2D_ADC_GAIN_SIZE                       1
#define H2D_SBC_ENCODE_HDR_CONFIG_SIZE          1
#define H2D_CODEC_PARAMETER_CONFIG_SIZE         5
#define H2D_RAW_ADC_DAC_CAPTURE_ADDR_SIZE       2
#define H2D_ASRC_CONFIG_SIZE                    2
#define H2D_MIC_SPK_LR_CONFIG_SIZE              1
#define H2D_SIG_PROC_FUN_RUNTIME_CONFIG_SIZE    1
#define H2D_DSP_AUDIO_ACTION_SIZE               1
#define H2D_VOICEPROMPT_FADEINOUT_CONTROL_SIZE  1
#define H2D_PROMPT_CONFIG_SIZE                  1

#define H2D_SET_CODEC_STATE_SIZE            1
#define CODEC_STATE_RESET                   0
#define CODEC_STATE_SLEEP                   1
#define CODEC_STATE_STANDBY                 2
#define CODEC_STATE_ACTIVE                  3
#define CODEC_STATE_DSP_READY               0x10

#define CODEC_RX_SPORT0_TX_SPORT0           0
#define CODEC_RX_SPORT0_TX_SPORT1           1
#define CODEC_RX_SPORT1_TX_SPORT0           2
#define CODEC_RX_SPORT1_TX_SPORT1           3

#define D2H_FADE_OUT_COMPLETE_SIZE          0
#define D2H_CODEC_STATE_SIZE                1
#define D2H_BOOT_DONE_SIZE                  0
#define D2H_BOOT_QUERY_SIZE                 1

#define D2H_MIC_SPK_LR_SIZE                 1
#define GET_SPK_LR_CONFIG(x)                ((x) & 0x0F)
#define GET_MIC_LR_CONFIG(x)                (((x) & 0xF0) >> 4)
#define MIC_SPK_LR_CONFIG_DEFAULT           0
#define MIC_SPK_LR_CONFIG_L_ONLY            1
#define MIC_SPK_LR_CONFIG_R_ONLY            2
#define MIC_SPK_LR_CONFIG_MID               3
#define MIC_SPK_LR_CONFIG_SWAP              4
/*
#define DSP_AUDIO_STOP                      0
#define DSP_AUDIO_REC_START                 1
#define DSP_AUDIO_PLAY_START                2
#define DSP_AUDIO_REC_PLAY_START            3
*/
typedef enum
{
    DSP_STREAM_REC_PLAY_STOP,
    DSP_STREAM_REC_START,
    DSP_STREAM_PLAY_START,
    DSP_STREAM_REC_PLAY_START,
    DSP_STREAM_REC_STOP,
    DSP_STREAM_PLAY_STOP,
} T_STREAM_COMMAND;

typedef enum
{
    DSP_STREAM_AD_DA_READY,
    DSP_STREAM_AD_READY,
    DSP_STREAM_DA_READY,
} T_STREAM_STATUS;

/*
#define PKT_TYPE_SCO                        0
#define PKT_TYPE_A2DP                       1
#define PKT_TYPE_VOICE_PROMPT               2
#define PKT_TYPE_LOST_PACKET                3
#define PKT_TYPE_IOT_DATA                   4
#define PKT_TYPE_RAW_AUDIO_DATA             5
#define PKT_TYPE_ZERO_PACKET                6
#define PKT_TYPE_DUMMY                      0xFF
*/
//for D2H_DSP_STATUS_IND type
#define D2H_IND_TYPE_RINGTONE               0
#define D2H_IND_TYPE_FADE_IN                1
#define D2H_IND_TYPE_FADE_OUT               2
#define D2H_IND_TYPE_DECODE_EMPTY           3
#define D2H_IND_TYPE_PROMPT                 4
#define D2H_IND_TYPE_TX_PATH_RAMP_GAIN      5

//for D2H_IND_TYPE_RINGTONE status
#define D2H_IND_STATUS_RINGTONE_STOP        0
//for D2H_IND_TYPE_FADE_IN status
#define D2H_IND_STATUS_FADE_IN_COMPLETE     0
//for D2H_IND_TYPE_FADEOUT status
#define D2H_IND_STATUS_FADE_OUT_COMPLETE    0
//for D2H_IND_TYPE_PROMPT status
#define D2H_IND_STATUS_PROMPT_REQUEST       0
#define D2H_IND_STATUS_PROMPT_FINISH        1
#define D2H_IND_STATUS_PROMPT_DECODE_ERROR  2
//for D2H_IND_TYPE_TX_PATH_RAMP_GAIN status
#define D2H_IND_STATUS_RAMP_GAIN_STABLE     0

//D2H_SILENCE_DETECT_REPORT
#define D2H_SILENCE_STATUS_NON_SILENCE      0
#define D2H_SILENCE_STATUS_SILENCED         1

#define EQ_COEFF_5BAND_LENGTH               21
#define EQ_COEFF_10BAND_LENGTH              41

#define D2H_DATA_LEN(x)                     (x<<16)
#define D2H_CMD_LEN(x)                      ((x+1)<<2)
/*
#define DSP_OUTPUT_LOG_NONE                 0
#define DSP_OUTPUT_LOG_BY_UART              1
#define DSP_OUTPUT_LOG_BY_MCU               2
*/
/**
  * @brief D2H command IDs
*/
typedef enum
{
    D2H_RESERVED, //Command id: 0x00

    D2H_CODEC_STATE, //Command id: 0x01
    /* 1 word: Reporting current DSP/codec state
     * 4 0: DSP_RESET_STATE_READY // analog codec is power off
     * 4 1: DSP_SLEEP_STATE_READY
     * 4 2: DSP_STANDBY_STATE_READY // I2S interface is disabled MCU can load codes to DSP to swtich application scenarios, such as SCO ??A2DP, SCO ??LineIn.
     * 4 3: DSP_ACTIVE_STATE_READY // DSP is active, playing/recording SPK/MIC signal
     *
     */

    D2H_DSP_STATUS_IND, //Command id: 0x02
    /*
    bit[7:0] : Indication Type
    0: Ringtone
    1: Fade in
    2: Fade out
    3: Decode empty
    4: Voice prompt

    bit[15:8] : Indication Status
    */

    D2H_SILENCE_DETECT_REPORT, //Command id: 0x03
    /*
     * Assume initial state not_silence
     * DSP report silence status when condition is changed
     * bit 0: 1 for Spk path is silence, 0 for Spk path is not silence
     *
     */

    D2H_DSP_POWER_DOWN_ACK, //Command id: 0x04
    /*
     * No data report
     */

    D2H_SAMPLE_NUMBER_ACK, //Command id: 0x05
    /*
     * Bit[31:0]: Total sample in speaker path FIFO (Decoder FIFO + DMA FIFO)
     */

    D2H_RWS_MASTER_STAMP,  //Command id: 0x06
    /*
       struct RwsStamp_T {
            unsigned int SerialNum          :5;
            unsigned int TimeStampClk       :27;

            unsigned int Queue              :13;
            int          AsrcOffset         :19;

            unsigned int I2sLen             :16; //u(16,2f)
            int          SubAsrcOffset      :16;

            unsigned int SubSerialNum       :3;
            unsigned int FrameSerialNum     :8;
            unsigned int TimeStampCnt       :10;
            unsigned int Reserved           :3;
            unsigned int                    :8;   //15byte

            //Structure 16byte  //content total 15 byte;
        };
     */

    D2H_RWS_SLAVE_SYNC_STATUS, //Command id: 0x07
    /*
     *  Word 0: [31:0] Loop_n
        Word 1: [31:0] error of samples (Q4)
        Word 2: [31:0] :
                (-3)RWS  reSync
                (-2)Spk1 timestamp lose
                (-1)I2S FIFO empty
                (0) initial condition
                (1) rws sync. unlock
                (2) rws sync. Lock
                (3) rws RWSV2 Seamless OK
     */
    D2H_DSP_FW_ROM_VERSION_ACK, //Command id: 0x08
    /*
     * 2 words for version report
     * Word1: Bit[31:8]: IC part number
     * Word1: Bit[7:0]: FW ROM version
     * Word2: Bit{31:0]: 0
     */

    D2H_DSP_FW_RAM_VERSION_ACK, //Command id: 0x09
    /*
     * 2 words for version report
     * Word1: Bit[31:8]: IC part number
     * Word1: Bit[7:0]: FW ROM version
     * Word2: Bit[31:16]: FW RAM version
     */

    D2H_DSP_FW_PATCH_VERSION_ACK, //Command id: 0x0A
    D2H_DSP_FW_SDK_VERSION_ACK, //Command id: 0x0B
    /*
     * 1 word for version report
     * Word0: Bit[31:0]: SDK Version number
     */

    D2H_DSP_MIPS_REQIURED, //Command id: 0x0C
    /* Word 0: [31:0] mips required
    */

    D2H_DSP_ADC_GAIN_REQUEST,////Command id:0x0D
    /* Word 0: 0: MIC Gain Down
               1: MIC Gain Up
    */

    D2H_DSP_DAC_GAIN_REQUEST,////Command id:0x0E
    /* Word 0: 0: SPK Gain Down
               1: SPK Gain Up
    */

    D2H_DSP_SEND_FLASH_DATA, //Command id:0x0F
    /*
     * Word 0: FLASH address offset
     * Word 1: Data length (in bytes)
     * Word 2~N: Data to be stored
     */

    D2H_DSP_GET_FLASH_DATA, //Command id:0x10
    /*
     * Word 0: FLASH address offset
     * Word 1: Data length (in bytes)
     */

    D2H_DSP_SDK_GENERAL_CMD = 0xC0,////Command id:0xC0
    /* Word 0: Command payload pointer

    */

    D2H_DSP_BOOT_DONE = 0xF0,
    D2H_BOOT_READY,
    D2H_DSP_DBG,
    BID_SHM_LOOPBACK_TEST_ACK,
    BID_SHM_LOOPBACK_MPC_TEST_ACK,
    D2H_FT_TEST_REPORT,
    D2H_TEST_RESP
    /* Byte 0: Test Type, 1:MIC TEST; others to be defined
     * Byte 1: Para Len(0~255)
     * Byte[2~257]: Paras
     *     for MIC TEST, Byte[2] = 0 for fail, Byte[2] = 1 for success
     *                   Byte[3] = 1 for MIC1 Test,Byte[3] = 2 for MIC2 Test,
     *                   Byte[3] = 0 for MIC1&MIC2 Test,
     */
} T_SHM_CMD_D2H;

/**
  * @brief H2D command IDs
*/
typedef enum
{
    H2D_EMPTY, //Command id: 0x00

    H2D_DSP_CONFIG, //Command id: 0x01
    /*
     * 1 byte : DSP_CLK_Rate (MHz)
     * Parameter length unit : 32 bit
    */

    H2D_TEST_MODE, //Command id: 0x02
    /*
     * 1 byte : Test_Mode // if this byte is not zero, then  test mode is entered.
     * Parameter length unit : 32 bit
    */

    H2D_CODER_CONFIG, //Command id: 0x03
    /*
        Word 0: Decoder and Tx ASRC configuration [default: 0x0178, default at mSBC mode]
          Bit[11:0] : Decoder frame size (unit : sample)
             Speech mode: CVSD: 30 or 60 samples, mSBC: 120 samples.
             Audio mode: SBC: 128 samples, LDAC 256 samples, AAC-LC : 1024 samples
          Bit 12:rsvd
          Bit 13:channel mode
          Bit [19:16]: Decoder algorithm
                ALGORITHM_PROPRIETARY_VOICE,    //00
                ALGORITHM_G711_ALAW,            //01
                ALGORITHM_CVSD,                 //02
                ALGORITHM_MSBC,                 //03
                ALGORITHM_USB_SPEECH,           //04
                ALGORITHM_SBC,                  //05
                ALGORITHM_AAC,                  //06
                ALGORITHM_PROPRIETARY_AUDIO1,   //07
                ALGORITHM_PROPRIETARY_AUDIO2,   //08
                ALGORITHM_MP3,                  //09
                ALGORITHM_LINE_IN,              //10
                ALGORITHM_USB_AUDIO,            //11
                ALGORITHM_SUB_WOOFER,           //12

          Bit [21:20]: Bit width
             0x0: 16Bit
             0x1: 24Bit
             0x2: Reserved
             0x3: Reserved
        Word 1: Encoder and Rx ASRC configuration [default: 0x0078, default at mSBC mode]
          Bit [11:0]: Encoder frame size (unit : sample)
          Bit 12: 1 0: disable SPK(Tx or downstream) path ASRC
          Bit [19:16]: Encoder algorithm
                ALGORITHM_PROPRIETARY_VOICE,    //00
                ALGORITHM_G711_ALAW,            //01
                ALGORITHM_CVSD,                 //02
                ALGORITHM_MSBC,                 //03
                ALGORITHM_USB_SPEECH,           //04
                ALGORITHM_SBC,                  //05
                ALGORITHM_AAC,                  //06
                ALGORITHM_PROPRIETARY_AUDIO1,   //07
                ALGORITHM_PROPRIETARY_AUDIO2,   //08
                ALGORITHM_MP3,                  //09
                ALGORITHM_LINE_IN,              //10
                ALGORITHM_USB_AUDIO,            //11
                ALGORITHM_SUB_WOOFER,           //12
    */

    H2D_ANALOG_CODEC_CONTROL, //Command id: 0x04
    /*
        Word 0:
             Bit[3:0] : DAC SampleRate
               4 0: 8kHz
               4 1: 16kHz
               4 2: 32kHz
               4 3: 44.1kHz
               4 4: 48kHz
               4 5: 88.2kHz
               4 6: 96kHz
             Bit[7:4] : DAC Path //
               Not defined yet, [TBD]
             Bit[7:4] : DAC Control
               Not defined yet, [TBD]
             Bit[7:4] : DAC Gain
               Not defined yet, [TBD]
        Word 1:
             Bit[3:0] : ADC SampleRate
               4 0: 8kHz
               4 1: 16kHz
               4 2: 32kHz
               4 3: 44.1kHz
               4 4: 48kHz
               4 5: 88.2kHz
               4 6: 96kHz
             Bit[7:4] : ADC Path
               Not defined yet, [TBD]
             Bit[7:4] : ADC Control
               Not defined yet, [TBD]
             Bit[7:4] :ADC Gain
               Not defined yet, [TBD]
    */

    H2D_DIGITAL_VOLUME, //Command id: 0x05

    H2D_I2S_CONFIG, //Command id: 0x06
    /* DSP passes I2S parameters from MCU to I2S hardware
     * 01 byte : I2S_para0
     * 02 byte : I2S_para1
     * 03 byte : I2S_para2
     * 04 byte : I2S_para3
     * 05 byte : I2S_para4
     * 06 byte : I2S_para5
     * 07 byte : I2S_para6
     * 08 byte : I2S_para7
     * 09 byte : I2S_para8
     * 10 byte : I2S_para9
     * 11 byte : I2S_para10
     * 12 byte : I2S_para11
     * 13 byte : I2S_para12
     * 14 byte : I2S_para13
     * 15 byte : I2S_para14
     * 16 byte : I2S_para15
     * 17 byte : I2S_para16
     * 18 byte : I2S_para17
     * 19 byte : I2S_para18
     * 20 byte : I2S_para19
     */

    H2D_SIG_PROCESSING_CONFIG, //Command id: 0x07
    /*   2:1 byte : audio/voice functions on/off
     * 128:3 byte : audio/voice processing parameters
     *
     * What type of parameters (audio or voice) this command carries
     * depends on the encoder/decoder algorithm.
    */

    H2D_TONE_GEN_CONFIG, //Command id: 0x08
    /* Command Packet
        This message command is to deliver the default ringtone prompt
        The ringtone can be a mixture of two sinusoid tone with individual amplitude weighting.
        MCU provide desired tone frequency and DSP shall generate its sine tone based on various DAC sampling rate setting.
        3 Words:
          Word 0:
        Bit[15:0]: tone1 frequency(Hz). i.e. 0x200 denotes 512Hz.
        Bit[31:16]: tone1 amplitude weighting ranging from 1 to 0. 0x7FFF (0.9999)~
        0x0000 (0.0)
          Word 1:
        Bit[15:0]: tone2 frequency(Hz). i.e. 0x200 denotes 512Hz.
        Bit[31:16]: tone2 amplitude weighting ranging from 1 to 0. 0x7FFF(0.9999)~
        0x0000 (0.0)
          Word 2:
            Bit [15:0]: ringtone period in msec. DSP shall convert this setting according to DAC
        sampling rate.
            Bit [23:16]: Fade-In time in msec.
            Bit [23:16]: Fade-Out time in msec.
    */

    H2D_SPK_EQ_COEFF, //Command id: 0x09
    /*
        In speech mode, 5-band EQ is supported for speaker (TX, downsteam) path
        In audio playback mode, 10-band EQ is supported for speaker path

        In speech mode, 21 words.
        5 band * 4 coefficient * 1 word + 1 gain* 1word = 21 words
        In audio mode, 41 words.
        10 band * 4 coefficient * 1 word + 1 gain* 1word = 41 words
    */

    H2D_MIC_EQ_COEFF, //Command id: 0x0A
    /*
        In speech mode, 5-band EQ is supported for mic (RX, upsteam) path
        In audio recorder mode, such as BT microphone, 10-band EQ is supported for mic path

        In speech mode, 21 words.
        5 band * 4 coefficient * 1 word + 1 gain* 1word = 21 words
        In audio mode, 41 words.
        10 band * 4 coefficient * 1 word + 1 gain* 1word = 41 words
    */
    //IIR Base EQ

    H2D_SET_CODEC_STATE, //Command id: 0x0B
    /* Software Note :
         MCU informs DSP the desired analog codec state. Each state represents different analog codec state.
          Codec Reset: codec clock and VDD power is completely removed. Power-down mode
          Codec Sleep: codec VDD may be still present but codec clock is stopped.
          Codec_Standby: codec VDD/clock are both present but not accepting I2S interface input waiting for configuration change.
          Codec_Active: operation mode.

        1 Word:
          4 0: Codec_Reset : State 0
          4 1: Codec_Sleep: State1
          4 2: Codec_Standby: State 2
          4 3: Codec_Active: State 3
    */
    //#define ICODEC_ADCdb(db,pdb)               (((db*10+pdb)/15)&0xF)

    H2D_DAC_GAIN, //Command id: 0x0C
    /* Command Packet
        MCU informs DSP to change the DAC gain level. This action only to configure
        1 Word:
          Byte[0] : Codec gain
    */

    H2D_ADC_GAIN, //Command id: 0x0D
    /* Command Packet
        MCU informs DSP to change the ADC gain level. This action only to configure
        1 Word:
          Byte[0] : ADC gain
    */
    //  #define ICODEC_ATTdb(db,pdb)    ((db*10+pdb)/15)

    H2D_FADE_IN_OUT_CONTROL, //Command id: 0x0E
    /* Command Packet
      00 byte : Command
      01 byte : Tx Path Fade In Out Step (AY0 > 0 Fade In, AY0<0 Fad Out)
         Default Value :
            txFadeStep  = 0  (Disable)
         Noted :
            txFadeStep  ==> 0 = Diable
                        1 = 683 mSec FadeIn @ 48K
                        2 = 341 mSec FadeIn @ 48K
                        3 = 227 mSec FadeIn @ 48K
                          :
                        6 = 114 mSec FadeIn @ 48K   (recommanded)
                          :
                       -1 = 683 msec FadeOut @ 48K
                       -2 = 341 msec FadeOut @ 48K
                       -3 = 227 msec FadeOut @ 48K
                          :
                       -6 = 114 mSec FadeOut @ 48K  (recommanded)
                          :
                          ==> 32768/SampleRate/txFadeStep
    */

    H2D_SBC_ENCODE_HDR_CONFIG, //Command id: 0x0F
    /*
     * 1 byte : HDR
     * 2 byte : bitpool
     * 3 byte : frame number
     *
     * sample frequency : HDR bit 7:6
     * 00 : 16k
     * 01 : 32k
     * 10 : 44.1k
     * 11 : 48k
     * ===========================
     * blocks : HDR bit 5:4
     * 00 : 4
     * 01 : 8
     * 10 : 12
     * 11 : 16
     * ============================
     * channel mode : HDR bit 3:2
     * 00 : mono
     * 01 : dual channel
     * 10 : stereo
     * 11 : joint stereo
     * ============================
     * allocation method : HDR bit 1
     * 0 : loundness
     * 1 : SNR
     * ============================
     * subbands : HDR bit 0
     * 0 : 4
     * 1 : 8
    */

    H2D_CODEC_PARAMETER_CONFIG, //Command id: 0x10
    /* Command Packet
      00 byte : Codec seletion
      01 byte : codec_para1
      02 byte : codec_para2
      03 byte : codec_para3
      04 byte : codec_para4
      05 byte : codec_para5
      06 byte : codec_para6
      07 byte : codec_para7
      08 byte : codec_Reg_64
    */

    H2D_RAW_ADC_DAC_CAPTURE_ADDR, //Command id: 0x11
    /* Command Packet
    // used for capturing Raw ADC/DAC data and sent back via share memory
      00 byte : Command
      01 byte : MCU Address high byte
      02 byte : MCU Address low byte
    */

    H2D_ASRC_CONFIG, //Command id: 0x12
    /*  Word 0:
            Bit [31:0]: Stepsize_SPK
        Word 1:
            Bit [31:0]: Stepsize_MIC
    */

    H2D_MIC_SPK_LR_CONFIG, //Command id: 0x13
    // After signal processing
    // signal format: SPORT format
    //   stereo: [L R L R...]
    //   mono:   [L L L L...]
    /* byte[0] : 4'b0 : No LR Mixing at the SPK path
     *                   [dec=stereo,SPORT=stereo] L->L, R->R
     *                   [dec=stereo,SPORT=mono]   L->L
     *                   [dec=mono,SPORT=stereo]   L->L, L->R
     *                   [dec=mono,SPORT=mono]     L->L
     *            4'b1 : only L channel at the SPK path
     *                   [dec=stereo,SPORT=stereo] L->L, L->R
     *                   [dec=stereo,SPORT=mono]   L->L
     *                   [dec=mono,SPORT=stereo]   L->L, L->R
     *                   [dec=mono,SPORT=mono]     L->L
     *            4'b2 : only R channel at the SPK path
     *                   [dec=stereo,SPORT=stereo] R->L, R->R
     *                   [dec=stereo,SPORT=mono]   R->L
     *                   [dec=mono,SPORT=stereo]   L->L, L->R
     *                   [dec=mono,SPORT=mono]     L->L
     *            4'b3 : output middle to all channel at SPK path
     *                   [dec=stereo,SPORT=stereo] (L+R)/2->L, (L+R)/2->R
     *                   [dec=stereo,SPORT=mono]   (L+R)/2->L
     *                   [dec=mono,SPORT=stereo]   L->L, L->R
     *                   [dec=mono,SPORT=mono]     L->L
     *            4'b4 : LR swap at SPK path
     *                   [dec=stereo,SPORT=stereo] R->L, L->R
     *                   [dec=stereo,SPORT=mono]   R->L
     *                   [dec=mono,SPORT=stereo]   L->L, L->R
     *                   [dec=mono,SPORT=mono]     L->L
     * byte[1] : 4'b0 : No LR Mixing at the MIC path, L->L, R->R
     *            4'b1 : L->L, L->R
     *            4'b2 : R->L, R->R
     *            4'b3 : (L+R)/2->L, (L+R)/2->R
     *            4'b4 : R->L, L->R
    */

    H2D_SIG_PROC_FUN_RUNTIME_CONFIG, //Command id: 0x14
    /*
     To runtime disable the control word of the signal processing function.
     1 Word:
        An example in SCO is:
            bit [31:0]: runtime disable one of 16 different speech functions,
                Bit 0: 1 0 :disable speech func 1 //Mic NR
                Bit 1: 1 0 :disable speech func 2 //Spk NR
                Bit 2: 1 0 :disable speech func 3 //AEC
                Bit 3: 1 0 :disable speech func 4 //AES
                Bit 4: 1 0 :disable speech func 5 //Mic DRC
                Bit 5: 1 0 :disable speech func 6 //Spk DRC
                Bit 6: 1 0 :disable speech func 7 //Mic High-pass filer
                Bit 7: 1 0 :disable speech func 8 //Spk High-pass filer
                Bit 8: 1 0 :disable speech func 9 //Mic DC Remover
                Bit 9: 1 0 :disable speech func 10 //Spk DC Remover
                Bit 10: 1 0 :disable speech func 11 //Dualmic NR
                Bit 11: 1 0 :disable speech func 12 //Mic EQ
                Bit 12: 1 0 :disable speech func 13 //Spk EQ
                Bit 13: 1 0 :disable speech func 14
                Bit 14: 1 0 :disable speech func 15
        An example in A2DP is:
            bit [3:0]: runtime disable one of 16 different speech functions,
                Bit 0: 1 0 :disable speech func 1 //MB-DRC
                Bit 1: 1 0 :disable speech func 2 //Loudness
                Bit 2: 1 0 :disable speech func 3 //Virtual Bass
                Bit 3: 1 0 :disable speech func 4 //Surround
                Bit 4: 1 0 :disable speech func 5 //EQ
                Bit 5: 1 0 :disable speech func 6
                Bit 6: 1 0 :disable speech func 7
                Bit 7: 1 0 :disable speech func 8
                Bit 8: 1 0 :disable speech func 9
                Bit 9: 1 0 :disable speech func 10
                Bit 10: 1 0 :disable speech func 11
                Bit 11: 1 0 :disable speech func 12
                Bit 12: 1 0 :disable speech func 13
                Bit 13: 1 0 :disable speech func 14
                Bit 14: 1 0 :disable speech func 15
    */

    H2D_DSP_AUDIO_ACTION, //Command id: 0x15
    /*
    bit[3:0]
    4 0: DSP Audio stop
    This action informs DSP to stop current audio/speech application activity.
    DSP does the following thing
    1.  Start the fade-out process
    2.  At the end of fade-out process, clear queued packet buffer and intermediate FIFO.

    4 1: REC_START
    This action informs DSP to configure MIC path only for BT audio dongle or MIC applications.
    DSP does the following things:
    1.  Configure the FIFO
    2.  Configure SBC encoder
    3.  Configure signal processing function.
    4.  Configure the packet aggregation configuration.
    5.  Configure settings of analog audio codec
    6.  Configure I2S interface

    4 2: CMD_PLAY_START
    This action informs DSP to configure SPK path only for BT audio music playback applications.
    DSP does the following things:
    1.  Configure the FIFO
    2.  Configure audio decoder
    3.  Configure signal processing function.
    4.  Configure the packet aggregation configuration.
    5.  Configure settings of analog audio codec
    6.  Configure I2S interface

    4 3: CMD_REC_PLAY_SART
    This action informs DSP to configure SPK path only for BT speech/IoT applications.
    DSP does the following things:
    1.  Configure the FIFO
    2.  Configure decoder/encoder
    3.  Configure signal processing function.
    4.  Configure the packet aggregation configuration.
    5.  Configure settings of analog audio codec
    6.  Configure I2S interface
    */

    H2D_TX_PATH_RAMP_GAIN_CONTROL, //Command id: 0x16
    /*
    bit[7:0]
         TX Path Gain idx (0~15)
         [0  -2  -4  -6 -9 -12 -15 -18 -21 -27 -33 -39 -45 -70 -200]; Attenuation dB % 0:15
    bit[15:8] : Ramp Gain Time
        0: 50mSec
        1: 100mSec
        2: 150mSec
    */

    H2D_CMD_ACTION_CTRL, //Command id: 0x17
    /*
    bit[7:0]
    0: Ringtone Ctrl

    bit[15:8] : action
    0: stop ringtone
    */

    H2D_CMD_LINE_IN_START, //Command id: 0x18

    H2D_CMD_PROMPT_CONFIG, //Command id: 0x19
    /*
    total length bit[31:0]

    default: 0x331800 -> Mix disable
    default: 0x331810 -> Mix enable

    bit[3:0] : Prompt_Coder_type
    0: AAC Voice Prompt
    1: SBC Voice Prompt
    2: MP3 Voice Prompt
    other reserved

    bit[7:4] : Mix option
    0: Mix_disable
    1: Mix_enable
    other reserved

    bit[11:8]: Prompt file Frequency
    0: 96000;
    1: 88200;
    2: 64000;
    3: 48000;
    4: 44100;
    5: 32000;
    6: 24000;
    7: 22050;
    8: 16000;
    9: 12000;
    10:11025;
    11:8000;
    other reserved

    bit[15:12]: Channel number;
    0: ignore decode
    1: Mono;
    2: Stereo;
    other reserved

    bit[19:16]: Prompt attenuation gain;
    0: normal
    1: attenuation 1dB
    2: attenuation 2dB
    ...
    15: attenuation 15dB
    other reserved

    bit[23:20]: Path attenuation gain;
    0: normal
    1: attenuation 1dB
    2: attenuation 2dB
    ...
    15: attenuation 15dB
    other reserved
    */

    H2D_CMD_PROMPT_ACTION, //Command id: 0x1A
    /*
      Bit[15:0]: Frame Length byte N, and indicate Prompt active
         => If  Frame Length N = 0xFFFF, indicate Prompt File already End
      Bit[N+15: 16]: Raw data, max 800 byte
         => If  Frame Length N = 0xFFFF, Raw data =>empty
    */

    H2D_CMD_LOG_OUTPUT_SEL, //Command id: 0x1B
    /*
    Bit[7:0]: DSP Log output selection
            0: DSP Log disabled
            1: DSP Log output to UART directly
            2: DSP Log is forward to MCU to output

    Bit[15:8]: valid if Bit[7:0] == 0x0
            0: DSP send log to SHM, then use DMA output the log
            1: DSP send log to Uart Register directly

    Bit[23:16]: valid if Bit[15:8] == 0x1
            0: DSP use DMA to output log
            1: DSP write uart Register directly
    */

    H2D_CMD_DSP_MODULE_LOG_ENABLE_DISABLE, //Command id: 0x1C
    /*
    Word 0: Modules log disabled if corresponding related bits set
    Word 1: Modules enabled if corresponding related bit set
    */

    H2D_SILENCE_DETECT_CFG, //Command id: 0x1D
    /*
     * Bit[7:0]: Silence detect and suppress switch
     * Bit0: Silence detect switch: 1 for on
     * Bit1: Silence suppress switch: 1 for on
     *
     * Bit[15:8]: Silence detect threshold
     *
     * Bit[23:16]: Silence suppress detect threshold
     *
     * Bit[31:24] Silence suppress gain value
     */

    H2D_DSP_POWER_DOWN, //Command id: 0x1E
    /*
     * Null to ask DSP ack only
     */

    H2D_CMD_DSP_DAC_ADC_DATA_TO_MCU, //Command id: 0x1F
    /*
    Bit[ 7: 0]: DAC DATA TO MCU or Not
            0: DSP Don't Send DAC DATA to MCU
            1: DSP Send DAC DATA to MCU
    Bit[15: 8]: ADC DATA TO MCU or Not
            0: DSP Don't Send ADC DATA to MCU
            1: DSP Send ADC DATA to MCU
    */

    H2D_SAMPLE_NUMBER_QUERY, //Command id: 0x20
    /*
     * Null to ask DSP ack only
     */

    H2D_CALL_STATUS, //Command id: 0x21
    /*
    Byte 0: SCO call status
            0: Call not active
            1: Call active
     */

    H2D_DSP_FW_ROM_VERSION, //Command id: 0x22
    /*
     */

    H2D_DSP_FW_RAM_VERSION, //Command id: 0x23
    /*
     */

    H2D_DSP_FW_PATCH_VERSION, //Command id: 0x24
    /*
     */

    H2D_DSP_AUDIO_EFFECT, //Command id: 0x25
    /*
    Byte 0: Cmd mode = 0x01
    Byte 1: Audio effect mode
     */

    H2D_RWS_INIT, //Command id: 0x26
    /*
     * Reset RWS state and set Stamp index
        Word0:
        [3:0]: (0) Native
               (1) Piconet0
               (2) Piconet1
               (3) Piconet2
               (4) Piconet3
        Word1: [31:0] :I2S Sampling Rate idx(0~6)
           (0)8000,  (1)16000, (2)32000, (3)44100,  (4)48000, (5)88200, (6)96000,
     *
     */

    H2D_RWS_MASTER_STAMP, //Command id: 0x27
    /*
     * struct RwsStamp_T {
            unsigned int SerialNum          :5;
            unsigned int TimeStampClk       :27;

            unsigned int Queue              :13;
            int          AsrcOffset         :19;

            unsigned int I2sLen             :16; //u(16,2f)
            int          SubAsrcOffset      :16;

            unsigned int SubSerialNum       :3;
            unsigned int FrameSerialNum     :8;
            unsigned int TimeStampCnt       :10;
            unsigned int Reserved           :3;
            unsigned int                    :8;   //15byte

            //Structure 16byte  //content total 15 byte;
        };
    */

    H2D_RWS_SET_ASRC_RATIO, //Command id: 0x28
    /*  Word 0:
            Bit [31:0]: ASRC Dejitter ratio;
    */

    H2D_RWS_SET, //Command: 0x29
    /*
     * Set device is Master Speaker or Slave Speaker
        Word0:
        [0]: (1) Master Speaker enable; (0)Disable
        [4]: (1) Slave  Speaker enable; (0)Disable
    */

    H2D_SIG_PROCESS_MODE_SET, //Command: 0x2A
    /*
     * Default mode number is 0
     * Byte 0: Mode serial number
     */

    H2D_ASRC_INIT, //Command id: 0x2B
    /*
     *   Set ASRC BaseValue and Enable
     *   Word0:
     *      [0]: ASRC_SPK: (0) Disable (1) Enable
     *      [1]: ASRC_MIC: (0) Disable (1) Enable
     *   [31:2]: Reserved
     *   Word1:
     *      [31:0]: ASRC_SPK BaseValue1
     *      1.  General                 : 0x0800_0000 (Mandatory)
     *      2.  44.1 to 48(88.2to96)    : 0x0759_999A (Mandatory)
     *      3.  44.1 to 96              : 0x03AC_CCCD
     *      4.  48 to 96                : 0x0400_0000
     *      5.  16 to 48                : 0x02AA_AAAB
     *      6.   8 to 48                : 0x0155_5555
     *   Word2:
     *      [31:0]: ASRC_MIC BaseValue2
     *
     */

    H2D_MIX_MODE_ENCODER_CONFIG, //Command id: 0x2C
    /*
     *   Start Initialized encoder algorithm
     *   Bit0 : 0: Stop encode
     *          1: Start encode
     *
     */
    H2D_MIX_MODE_DECODER_CONFIG, //Command id: 0x2D
    /*
     *   Stop decoder algorithm
     *
     */

    H2D_SEND_BT_ADDR, //Command id: 0x2E
    /*
     *   Send BT Address to DSP
     *   Byte [5:0]: BT Address
     *
     */

    H2D_RWS_SEAMLESS, //Command id: 0x2F
    /*   Word0: [31:0] (0) disable Seamless
     *                   (1) enable  Seamless  Master
     *                   (2) enable  Seamless  Slave
     */

    H2D_ENCODER_CONFIG,//Command id: 0x30
    /*
        Word 0: Encoder and Rx ASRC configuration [default: 0x0078, default at mSBC mode]
          Bit [11:0]: Encoder frame size (unit : sample)
          Bit 12:rsvd
          Bit 13: channel mode
          Bit [20:16]: Encoder algorithm
            ALGORITHM_PROPRIETARY_VOICE,   //00
            ALGORITHM_G711_ALAW,           //01
            ALGORITHM_CVSD,                //02
            ALGORITHM_MSBC,                //03
            ALGORITHM_OPUS_VOICE,          //04
            ALGORITHM_UHQ_VOICE,           //05
            ALGORITHM_USB_SPEECH,          //06
            ALGORITHM_SBC,                 //07
            ALGORITHM_AAC,                 //08
            ALGORITHM_PROPRIETARY_AUDIO1,  //09
            ALGORITHM_PROPRIETARY_AUDIO2,  //10
            ALGORITHM_MP3,                 //11
            ALGORITHM_USB_AUDIO,           //12
            ALGORITHM_SUB_WOOFER,          //13
            ALGORITHM_LDAC,                //14 SONY
            ALGORITHM_UHQ,                 //15 Samsung
            ALGORITHM_LHDC,                //16 Savitech
            ALGORITHM_OPUS_AUDIO,          //17
            ALGORITHM_LINE_IN,             //18
            ALGORITHM_END                  //19

         Bit [22:21]: Bit width
                0x0: 16Bit
                0x1: 24Bit
                0x2: Reserved
                0x3: Reserved

         Bit [26:23]: encode sampling rate
         Bit [29:27]: AAC type
             0x0: AAC_TYPE_LATM_NORMAL
             0x1: AAC_TYPE_LATM_SIMPLE
             0x2: AAC_TYPE_ADTS
             0x3: AAC_TYPE_ADIF
             0x4: AAC_TYPE_RAW

         Bit 30: pass_through_enable
    */

    H2D_DECODER_CONFIG,//Command id: 0x31
    /*
        Word 0: Decoder and Tx ASRC configuration [default: 0x0178, default at mSBC mode]
          Bit[11:0] : Decoder frame size (unit : sample)
             Speech mode: CVSD: 30 or 60 samples, mSBC: 120 samples.
             Audio mode: SBC: 128 samples, LDAC 256 samples, AAC-LC : 1024 samples
          Bit 12: rsvd
          Bit 13:channel mode
          Bit [20:16]: Decoder algorithm
            ALGORITHM_PROPRIETARY_VOICE,   //00
            ALGORITHM_G711_ALAW,           //01
            ALGORITHM_CVSD,                //02
            ALGORITHM_MSBC,                //03
            ALGORITHM_OPUS_VOICE,          //04
            ALGORITHM_UHQ_VOICE,           //05
            ALGORITHM_USB_SPEECH,          //06
            ALGORITHM_SBC,                 //07
            ALGORITHM_AAC,                 //08
            ALGORITHM_PROPRIETARY_AUDIO1,  //09
            ALGORITHM_PROPRIETARY_AUDIO2,  //10
            ALGORITHM_MP3,                 //11
            ALGORITHM_USB_AUDIO,           //12
            ALGORITHM_SUB_WOOFER,          //13
            ALGORITHM_LDAC,                //14 SONY
            ALGORITHM_UHQ,                 //15 Samsung
            ALGORITHM_LHDC,                //16 Savitech
            ALGORITHM_OPUS_AUDIO,          //17
            ALGORITHM_LINE_IN,             //18
            ALGORITHM_END                  //19

         Bit [22:21]: Bit width
                0x0: 16Bit
                0x1: 24Bit
                0x2: Reserved
                0x3: Reserved

         Bit [26:23]: decode sampling rate
         Bit [29:27]: AAC type
             0x0: AAC_TYPE_LATM_NORMAL
             0x1: AAC_TYPE_LATM_SIMPLE
             0x2: AAC_TYPE_ADTS
             0x3: AAC_TYPE_ADIF
             0x4: AAC_TYPE_RAW
    */

    H2D_DAC_GAIN_STEREO, //Command id: 0x32
    /*
     * Byte 0: L channel DAC gain
     * Byte 1: R channel DAC gain
     */

    H2D_TONE_GAIN, //Command id: 0x33
    /* Command Packet
        MCU informs DSP to change the tone gain level. This action only to configure
        1 Word:
          Byte[0] : Codec gain
    */

    H2D_SEND_FLASH_DATA, //Command id: 0x34
    /*
     * MCU sends FLASH data request by DSP
     */

    H2D_FIXED_TONE_SET, //Command id: 0x35
    /*
     * MCU set fixed tone gain mode
     *
     * Byte[0] : 0  (No fixed tone gain)
     *           1  (Fixed tone gain)
     */

    H2D_CLEAR_DSP_INIT_FLAG, //Command id: 0x36
    /*
     * MCU clear DSP app init flag
     */

    H2D_BOOT_QUERY = 0xF0,

    //0x1000, Command for scenario 0 //A2DP
    H2D_SCENARIO0_PARA_PARSING = 0x1000,
    H2D_S0_GENERAL_EQ_PARA_MIC,
    H2D_S0_GENERAL_EQ_PARA_SPK,
    H2D_S0_CODEC_PARA_PARSING,
#if 1 // unused commands, only for ensuring compatibility
    H2D_S0_PARAMETRIC_EQ_MIC,
    H2D_S0_PARAMETRIC_EQ_SPK,
#endif
    H2D_S0_SYS_CONFIG, // command content defined in dsp_sys_scenario_config.h
    H2D_S0_PARAMETRIC_EQ_SPK_2,

    //0x1100, Command for scenario 1 //SCO
    H2D_SCENARIO1_PARA_PARSING = 0x1100,
    H2D_S1_GENERAL_EQ_PARA_MIC,
    H2D_S1_GENERAL_EQ_PARA_SPK,
    H2D_S1_CODEC_PARA_PARSING,
    H2D_S1_SYS_CONFIG, // command content defined in dsp_sys_scenario_config.h

    //0x1200, Command for scenario 2
    H2D_SCENARIO2_PARA_PARSING = 0x1200,
    H2D_S2_GENERAL_EQ_PARA_MIC,
    H2D_S2_GENERAL_EQ_PARA_SPK,
    H2D_S2_CODEC_PARA_PARSING,

    //0x1300, Command for scenario 3
    H2D_SCENARIO3_PARA_PARSING = 0x1300,

    //0x1F00, Command for common command
    H2D_SYSTEM_PARA_PARSING = 0x1F00,
    H2D_SYSTEM_PARA_SILENCE_DETECT,
} T_SHM_CMD_H2D;

typedef enum
{
    //D2H mailbox
    MAILBOX_D2H_SHARE_QUEUE_ADDRESS     = 0x01,
    MAILBOX_D2H_DSP_LOG_BUFFER          = 0x02,
    MAILBOX_D2H_DSP_ADCDAC_DATA0        = 0x04,
    MAILBOX_D2H_DSP_ADCDAC_DATA1        = 0X05,
    MAILBOX_D2H_CHECK                   = 0x06,
    MAILBOX_D2H_WATCHDOG_TIMEOUT        = 0xFE,
    MAILBOX_D2H_DSP_EXCEPTION           = 0xFF
} T_SHM_MAILBOX_D2H;

typedef enum
{
    //H2D mailbox
    //MAILBOX_H2D_XXX = 0x01,
    MAILBOX_H2D_SWITCH                  = 0x01,
} T_SHM_MAILBOX_H2D;

/**
  * @brief
  * @param
  * @retval
  */

#endif /* __SHM_COMMAND_H_ */
/******************* (C) COPYRIGHT 2015 Realtek Semiconductor Corporation *****END OF FILE****/
