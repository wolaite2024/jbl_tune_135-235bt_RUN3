#ifndef _ANC_DRIVER_H_
#define _ANC_DRIVER_H_

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include "flash_dspfw.h"

// use rtl876x.h macro to avoid non-volatile issue
#define ANC_REG_READ(offset)                 \
    HAL_READ32(ANC_REG_BASE, offset)
#define ANC_REG_WRITE(offset, value)         \
    HAL_WRITE32(ANC_REG_BASE, offset, value)
#define ANC_REG_UPDATE(offset, mask, value)  \
    HAL_UPDATE32((ANC_REG_BASE + offset), mask, value)

// content
#define ANC_DRV_DONT_CARE_8             0xFF
#define ANC_DRV_DONT_CARE_16            0xFFFF
#define ANC_DRV_DONT_CARE_32            0xFFFFFFFF
#define ANC_DRV_INVALID_DATA            0xDEADBEEF
#define ANC_DRV_MIC_SRC_TYPE_NUM        7
#define ANC_DRV_MIC_FMT_ANALOG          0x7531FDB9
#define ANC_DRV_MIC_FMT_DIGITAL         0x70000009
#define ANC_DRV_GAIN_COMP_DEFAULT       0x10000000
#define ANC_DRV_SUB_TYPE_MASK_ANC       0x1     // bit[0]: ANC
#define ANC_DRV_SUB_TYPE_MASK_APT       0xE     // bit[3:1]: idle/A2DP/SCO APT
#define ANC_DRV_ALU_OUT_MASK_SHIFT_OFS  5       // divide by sizeof(uint32_t)
#define ANC_DRV_ALU_OUT_MASK_FIELD      0x1F    // alu_out_mask[31:0]
#define ANC_DRV_LCH_ENABLE              0x1     // [0]: Lch
#define ANC_DRV_RCH_ENABLE              0x2     // [1]: Rch

// PALU traversal - MANUAL(0), BIQUAD_MOST(1)
#define ANC_DRV_PARA_IDX_0_1_MUL        1
#define ANC_DRV_PARA_IDX_0_1_MUL_STEREO 3       // offset
#define ANC_DRV_PARA_IDX_0_1_MUL_INT    2       // offset
#define ANC_DRV_PARA_IDX_0_1_VOL_SLEW   0
#define ANC_DRV_PARA_IDX_0_1_LIMITER    2

// PALU traversal - LL-APT/ANC Ambient Sound(2)
#define ANC_DRV_PARA_IDX_2_MUL          2
#define ANC_DRV_PARA_IDX_2_MUL_APT      4
#define ANC_DRV_PARA_IDX_2_VOL_SLEW     0
#define ANC_DRV_PARA_IDX_2_VOL_SLEW_APT 2
#define ANC_DRV_PARA_IDX_2_LIMITER      1
#define ANC_DRV_PARA_IDX_2_LIMITER_APT  3
#define ANC_DRV_PARA_IDX_2_APT_FILTER   5

// PALU traversal - Overload Protect(3)
#define ANC_DRV_PARA_IDX_3_EQ_MUL_R     8
#define ANC_DRV_PARA_IDX_3_EQ_MUL_L     10
#define ANC_DRV_PARA_IDX_3_EQ_BIQUAD    12
#define ANC_DRV_PARA_IDX_WNS_LIMITER_4  4
#define ANC_DRV_PARA_IDX_WNS_LIMITER_25 25

/*============================= Error Codes =============================*/
#define ANC_COMMAND_SUCCEEDED           0x00
#define ANC_COMMAND_PARAMETERS_ERROR    0xFD
#define ANC_REGISTER_ADDRESS_ERROR      0xFE
#define ANC_INVALID_DATA_ERROR          0xFF

/* CMD PARAM Maximum and Minimum Length */
#define ANC_DRV_MAX_CMD_PARAM_LEN       255
#define ANC_DRV_SET_PARA_BUF_LEN        256
#define ANC_DRV_REG_GRP_SIZE            21
#define ANC_DRV_REG_GRP_DATA_NUM        5
#define ANC_DRV_REG_WRITE_HEADER        5
#define ANC_DRV_REG_WRITE_BIT_SIZE      12
#define ANC_DRV_EAR_FIT_RESPONSE_NUM    32      // sample
#define ANC_DRV_PALU_PARA_LEN           20      // 5 words
#define ANC_DRV_VOL_SLEW_GAIN_OFS       0

/* ANC register address */
#define ANC_REG_START_ADDRESS           0x4002D000
#define ANC_REG_END_ADDRESS             0x4002DFFF
#define CODEC_REG_END_ADDRESS           0x4002C8FF

// gdma
#define ANC_DRV_GDMA_BUF_SIZE           64

// MCU play tone
#define ANC_DRV_SAMPLE_RATE             48000   // 48k
#define ANC_DRV_SAMPLE_INTERVAL         5
#define ANC_DRV_COS_TABLE_SAMPLE        (ANC_DRV_SAMPLE_RATE / ANC_DRV_SAMPLE_INTERVAL)
#define ANC_DRV_COS_TABLE_SIZE          (ANC_DRV_COS_TABLE_SAMPLE * 2)// each sample modify to 16 bits
#define ANC_DRV_COS_TEMP_BUFF_SIZE      ANC_DRV_GDMA_BUF_SIZE*4
#define ANC_DRV_AMP_DENOM               128
#define ANC_DRV_REALLOC_MEM_TIMEOUT     100
#define ANC_DRV_COS_TABLE_RETRY_MAX     50
#define ANC_DRV_TX_FREQ_NUM_MAX         64
#define ANC_DRV_COS_TABLE_MARGIN        128
#define ANC_DRV_COS_TABLE_BLK_SIZE      256     // bytes (128 samples)
#define ANC_DRV_COS_TABLE_BLK_NUM       (ANC_DRV_COS_TABLE_SIZE / ANC_DRV_COS_TABLE_BLK_SIZE)
#define ANC_DRV_COS_TABLE_BLK_SPL       (ANC_DRV_COS_TABLE_BLK_SIZE / 2)
#define ANC_DRV_COS_TABLE_BLK_SPL_DIV   7       // right shift bit refer to ANC_DRV_COS_TABLE_BLK_SPL
#define ANC_DRV_COS_TABLE_BLK_SPL_MASK  0x7F    // ANC_DRV_COS_TABLE_BLK_SPL - 1

extern uint8_t sinetone_gen_sport_gdma_tx_ch_num;
#define SINETONE_GEN_SPORT_GDMA_TX_CH_NUM        sinetone_gen_sport_gdma_tx_ch_num
#define SINETONE_GEN_SPORT_GDMA_TX_CH            DMA_CH_BASE(sinetone_gen_sport_gdma_tx_ch_num)
#define SINETONE_GEN_SPORT_GDMA_TX_CH_VECTOR     DMA_CH_VECTOR(sinetone_gen_sport_gdma_tx_ch_num)
#define SINETONE_GEN_SPORT_GDMA_TX_CH_IRQ        DMA_CH_IRQ(sinetone_gen_sport_gdma_tx_ch_num)

// Gain control
#define ANC_DRV_GAIN_DEFAULT            0x1000000   // BIT24
#define ANC_DRV_GAIN_MUTE               0x40        // BIT6
#define ANC_DRV_GAIN_FADE_IN            0x400       // BIT10
#define ANC_DRV_GAIN_FADE_OUT           0x8000      // BIT15
#define ANC_DRV_GAIN_SW_FADING_THD      0x80000     // BIT19
#define ANC_DRV_GAIN_SW_FADING_TO       20          // ms
#define ANC_DRV_GAIN_HW_FADING_TO       40          // us
#define ANC_DRV_GAIN_FADING_STEP        0x2000      // BIT13
#define ANC_DRV_GAIN_TIMEOUT_MAX        5           // processing rate
#define ANC_DRV_GAIN_TIMEOUT_MIN        5           // processing rate
#define ANC_DRV_GAIN_TABLE_CONSTANT     128
#define ANC_DRV_MS_TO_US                1000

// Fading
#define ANC_FADING_POLLING_INTERVAL     20 * 1000   // ms

// EQ control
#define ANC_DRV_EQ_BIQUAD_NUM_MONO      8           // 8 stage biuqad for mono EQ
#define ANC_DRV_EQ_BIQUAD_NUM_STEREO    4           // 4 stage biquad for stereo EQ
#define ANC_DRV_EQ_ADJUST_INTERVAL      10          // ms
#define ANC_DRV_EQ_MUL_MAX              0x1000000   // BIT24
#define ANC_DRV_EQ_MUL_DEFAULT          0x800000    // BIT23
#define ANC_DRV_EQ_MUL_STEP             0x800       // BIT17
#define ANC_DRV_EQ_SEL_BASE             ANC_DRV_EQ_BIQUAD_SEL_L
#define ANC_DRV_EQ_MUL_DATA_LEN         8           // by words
#define ANC_DRV_EQ_TIMER_INTERVAL       25          // us

// tim_gdma
#define ANC_DRV_TIM_GDMA_SUB_BUF_WORD   30
#define ANC_DRV_TIM_GDMA_SUB_BUF_BYTE   (ANC_DRV_TIM_GDMA_SUB_BUF_WORD << 2)

typedef enum t_anc_drv_set_para
{
    ANC_DRV_SET_PARA_REG_WRITE,
    ANC_DRV_SET_PARA_REG_UPDATE,
    ANC_DRV_SET_PARA_PALU_TBL_WRITE,
    ANC_DRV_SET_PARA_REG_BIT_WRITE,
    ANC_DRV_SET_PARA_MAX,
} T_ANC_DRV_SET_PARA;

/*  mic_src_sel: Microphone source selection, [ext_L, ext_R, int_L, int_R]
    for each element:
    0(adcdata1), 1(adcdata2), 2(adcdata3),
    8(dmic1_data_l), 9(dmic1_data_r), 10(dmic2_data_l), 11(dmic2_data_r),
*/
typedef enum t_anc_drv_mic_src
{
    ANC_DRV_AMIC1_SRC,
    ANC_DRV_AMIC2_SRC,
    ANC_DRV_AMIC3_SRC,
    ANC_DRV_RSVD0,
    ANC_DRV_RSVD1,
    ANC_DRV_RSVD2,
    ANC_DRV_RSVD3,
    ANC_DRV_RSVD4,
    ANC_DRV_DMIC1_RAISING_SRC,          //HW latch data when raising edge
    ANC_DRV_DMIC1_FALLING_SRC,          //HW latch data when falling edge
    ANC_DRV_DMIC2_RAISING_SRC,          //HW latch data when raising edge
    ANC_DRV_DMIC2_FALLING_SRC,          //HW latch data when falling edge
} T_ANC_DRV_MIC_SRC;

typedef enum t_anc_drv_log_fmt
{
    ANC_DRV_LOG_FMT_0_32,               // src0: 0 bit, src1: 32 bits
    ANC_DRV_LOG_FMT_8_24,               // src0: 8 bit, src1: 24 bits
    ANC_DRV_LOG_FMT_12_20,              // src0: 12 bit, src1: 20 bits
    ANC_DRV_LOG_FMT_16_16,              // src0: 16 bit, src1: 16 bits
} T_ANC_DRV_LOG_FMT;

typedef enum t_anc_drv_log_src_sel
{
    ANC_DRV_LOG_SRC_EXT_L,              // input path
    ANC_DRV_LOG_SRC_EXT_R,              // input path
    ANC_DRV_LOG_SRC_INT_L,              // input path
    ANC_DRV_LOG_SRC_INT_R,              // input path
    ANC_DRV_LOG_SRC_ANC_L,              // output path
    ANC_DRV_LOG_SRC_ANC_R,              // output path
    ANC_DRV_LOG_SRC_MUSIC_L,            // output path
    ANC_DRV_LOG_SRC_MUSIC_R,            // output path
} T_ANC_DRV_LOG_SRC_SEL;

typedef enum t_anc_drv_mic_sel
{
    ANC_DRV_MIC_SEL_EXT_L,
    ANC_DRV_MIC_SEL_EXT_R,
    ANC_DRV_MIC_SEL_INT_L,
    ANC_DRV_MIC_SEL_INT_R,
    ANC_DRV_MIC_SEL_MAX,
} T_ANC_DRV_MIC_SEL;

typedef enum t_anc_drv_lr_sel
{
    ANC_DRV_LR_SEL_L,
    ANC_DRV_LR_SEL_R,
    ANC_DRV_LR_SEL_MAX,
} T_ANC_DRV_LR_SEL;

typedef enum t_anc_drv_vol_slew_sel
{
    ANC_DRV_VOL_SLEW_SEL_ALL,
    ANC_DRV_VOL_SLEW_SEL_ALL_L = ANC_DRV_VOL_SLEW_SEL_ALL,
    ANC_DRV_VOL_SLEW_SEL_ALL_R,
    ANC_DRV_VOL_SLEW_SEL_APT,
    ANC_DRV_VOL_SLEW_SEL_APT_L = ANC_DRV_VOL_SLEW_SEL_APT,
    ANC_DRV_VOL_SLEW_SEL_APT_R,
    ANC_DRV_VOL_SLEW_SEL_MAX,
} T_ANC_DRV_VOL_SLEW_SEL;

typedef enum t_anc_drv_limiter_sel
{
    ANC_DRV_LIMITER_SEL_NOISE,
    ANC_DRV_LIMITER_SEL_APT,
    ANC_DRV_LIMITER_SEL_MAX,
} T_ANC_DRV_LIMITER_SEL;

typedef enum t_anc_drv_instr_type
{
    ANC_DRV_INSTR_TYPE_MANUAL,              // ANC image v0
    ANC_DRV_INSTR_TYPE_BIQUAD_MOST,         // ANC image v1
    ANC_DRV_INSTR_TYPE_APT,                 // ANC image v2
    ANC_DRV_INSTR_TYPE_OVERLOAD_PROTECT,    // ANC image v3
    ANC_DRV_INSTR_TYPE_MAX,
} T_ANC_DRV_INSTR_TYPE;

typedef enum t_anc_drv_para_type
{
    ANC_DRV_PARA_TYPE_MUL_EXT,
    ANC_DRV_PARA_TYPE_MUL_INT,
    ANC_DRV_PARA_TYPE_APT_FILTER,
    ANC_DRV_PARA_TYPE_MUL_ADD_EXT,      // not defined
    ANC_DRV_PARA_TYPE_MUL_ADD_INT,      // not defined
    ANC_DRV_PARA_TYPE_VOL_SLEW,
    ANC_DRV_PARA_TYPE_NOISE_GATE,       // not defined
    ANC_DRV_PARA_TYPE_PEAK_DET,         // not defined
    ANC_DRV_PARA_TYPE_LIMITER,
    ANC_DRV_PARA_TYPE_EQ_MUL,
    ANC_DRV_PARA_TYPE_EQ_BIQUAD,
    ANC_DRV_PARA_TYPE_WNS_LIMITER,
    ANC_DRV_PARA_TYPE_MAX,
} T_ANC_DRV_PARA_TYPE;

typedef enum t_au_anc_gain_src
{
    ANC_DRV_GAIN_COMP_SRC_EXT,
    ANC_DRV_GAIN_COMP_SRC_EXT_L = ANC_DRV_GAIN_COMP_SRC_EXT,
    ANC_DRV_GAIN_COMP_SRC_EXT_R,
    ANC_DRV_GAIN_COMP_SRC_INT,
    ANC_DRV_GAIN_COMP_SRC_INT_L = ANC_DRV_GAIN_COMP_SRC_INT,
    ANC_DRV_GAIN_COMP_SRC_INT_R,
    ANC_DRV_GAIN_COMP_SRC_APT,
    ANC_DRV_GAIN_COMP_SRC_APT_L = ANC_DRV_GAIN_COMP_SRC_APT,
    ANC_DRV_GAIN_COMP_SRC_APT_R,
    ANC_DRV_GAIN_COMP_SRC_MAX,
} T_ANC_DRV_GAIN_COMP_SRC;

typedef enum t_anc_drv_grp_comp_src
{
    ANC_DRV_GRP_COMP_SRC_ANC,
    ANC_DRV_GRP_COMP_SRC_LLAPT,
    ANC_DRV_GRP_COMP_SRC_MAX,
} T_ANC_DRV_GRP_COMP_SRC;

typedef enum t_anc_drv_use_mode
{
    ANC_DRV_MODE_RESET,
    ANC_DRV_MODE_ANC,
    ANC_DRV_MODE_APT,
} T_ANC_DRV_MODE;

typedef enum t_anc_drv_en_code_fmt
{
    ANC_DRV_EN_CODE_FMT_ANC_ONLY,
    ANC_DRV_EN_CODE_FMT_APT,
    ANC_DRV_EN_CODE_FMT_OLD_ANC,
} T_ANC_DRV_EN_CODE_FMT;

typedef enum t_anc_info
{
    ANC_INFO_ANC_INITIAL,
    ANC_INFO_EN_CODE_FMT,
    ANC_INFO_GAIN_ADDR,
} T_ANC_INFO;

typedef enum t_anc_gain_mask_idx
{
    ANC_GAIN_MASK_LSB,
    ANC_GAIN_MASK_MSB,
    ANC_GAIN_MASK_MAX,
} T_ANC_GAIN_MASK_IDX;

typedef enum t_anc_cb_event
{
    ANC_CB_EVENT_FADE_IN_COMPLETE,
    ANC_CB_EVENT_FADE_OUT_COMPLETE,
    ANC_CB_EVENT_GAIN_ADJUST_COMPLETE,
    ANC_CB_EVENT_EQ_ADJUST_COMPLETE,
    ANC_CB_EVENT_LOAD_CONFIGURATION_COMPLETE,
    ANC_CB_EVENT_LOAD_SCENARIO_COMPLETE,
    ANC_CB_EVENT_NONE,
} T_ANC_CB_EVENT;

typedef enum t_anc_fading_status
{
    ANC_FADING_STATUS_FADE_IN,
    ANC_FADING_STATUS_FADE_OUT,
    ANC_FADING_STATUS_ADJUST,
    ANC_FADING_STATUS_NONE = 0xFF,
} T_ANC_FADING_STATUS;

typedef enum t_anc_msg
{
    ANC_MSG_ANC_FADING,
    ANC_MSG_EQ_CTL_DONE,
    ANC_MSG_TOTAL,
} T_ANC_MSG;

typedef enum t_anc_drv_eq_biquad_sel
{
    ANC_DRV_EQ_BIQUAD_SEL_L,
    ANC_DRV_EQ_BIQUAD_SEL_L_0 = ANC_DRV_EQ_BIQUAD_SEL_L,
    ANC_DRV_EQ_BIQUAD_SEL_L_1,
    ANC_DRV_EQ_BIQUAD_SEL_R,
    ANC_DRV_EQ_BIQUAD_SEL_R_0 = ANC_DRV_EQ_BIQUAD_SEL_R,
    ANC_DRV_EQ_BIQUAD_SEL_R_1,
    ANC_DRV_EQ_BIQUAD_SEL_MAX,
} T_ANC_DRV_EQ_BIQUAD_SEL;

typedef enum t_anc_drv_eq_coef_sel
{
    ANC_DRV_EQ_COEF_SEL_0,
    ANC_DRV_EQ_COEF_SEL_1,
    ANC_DRV_EQ_COEF_SEL_MAX,
} T_ANC_DRV_EQ_COEF_SEL;

typedef enum t_anc_drv_tim_gdma_ctl
{
    ANC_DRV_TIM_GDMA_CTL_NONE,
    ANC_DRV_TIM_GDMA_CTL_GDMA,
    ANC_DRV_TIM_GDMA_CTL_TIM,
    ANC_DRV_TIM_GDMA_CTL_ALL,
} T_ANC_DRV_TIM_GDMA_CTL;

typedef struct
{
    uint8_t     msg_type;
    uint16_t    data_len;
    void        *p_data;
} T_ANC_TO_APP_MSG;

typedef struct t_anc_cmd_pkt
{
    uint8_t param_total_length;
    uint8_t cmd_parameter[ANC_DRV_MAX_CMD_PARAM_LEN];
} T_ANC_CMD_PKT;

typedef struct __attribute__((packed)) t_anc_drv_reg_grp
{
    uint8_t index;
    uint32_t para[ANC_DRV_REG_GRP_DATA_NUM];
} T_ANC_DRV_REG_GRP;

typedef struct __attribute__((packed)) t_anc_drv_reg_write
{
    uint32_t reg_addr;
    uint8_t data_len;
    uint32_t reg_data;
} T_ANC_DRV_REG_WRITE;

typedef struct __attribute__((packed)) t_anc_drv_reg_bit_write
{
    uint32_t reg_addr;
    uint32_t data;
    uint32_t mask;
} T_ANC_DRV_REG_BIT_WRITE;

/* The Structure of ear fit response */
typedef struct t_anc_drv_ear_fit_response
{
    union
    {
        uint32_t configure;
        struct
        {
            uint32_t l_gain_mismatch_valid: 1;      /* bit[0] */
            uint32_t l_response_valid:      1;      /* bit[1] */
            uint32_t l_reserved:            14;     /* bit[15:2] */
            uint32_t r_gain_mismatch_valid: 1;      /* bit[16] */
            uint32_t r_response_valid:      1;      /* bit[17] */
            uint32_t r_reserved:            14;     /* bit[31:18] */
        };
    };

    uint32_t l_gain_mismatch;
    uint32_t l_response[ANC_DRV_EAR_FIT_RESPONSE_NUM];
    uint32_t r_gain_mismatch;
    uint32_t r_response[ANC_DRV_EAR_FIT_RESPONSE_NUM];
} T_ANC_DRV_EAR_FIT_RESPONSE;


/* The Structure of ANC scenario info */
typedef union t_anc_drv_scenario_info
{
    uint8_t d8[10];
    struct __attribute__((packed))
    {
        uint8_t scenario_num;
        uint8_t sub_type;
        uint8_t *scenario_mode;
        uint8_t *scenario_apt_effect;
    };
} T_ANC_DRV_SCENARIO_INFO;

/* The Structure of MP extend data */
typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t rsvd1: 3;               /* bit[2:0], reserved */
        uint32_t grp_info: 12;           /* bit[14:3] */
        uint32_t rsvd2: 17;              /* bit[31:15], reserved */
    } data;
} T_ANC_DRV_MP_EXT_DATA;

/* The Structure of ANC_REG_IO_ENABLE_ADDR Register */
typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t ext_L_en: 1;           /* bit[0] */
        uint32_t ext_R_en: 1;           /* bit[1] */
        uint32_t int_L_en: 1;           /* bit[2] */
        uint32_t int_R_en: 1;           /* bit[3] */
        uint32_t music_L_en: 1;         /* bit[4] */
        uint32_t music_R_en: 1;         /* bit[5] */
        uint32_t anc_out_L_en: 1;       /* bit[6] */
        uint32_t anc_out_R_en: 1;       /* bit[7] */
        uint32_t alu_en: 1;             /* bit[8] */
        uint32_t rsvd: 23;              /* bit[31:9] */
    };
} T_ANC_DRV_IO_EN;

/* The Structure of ANC_REG_SRC_SEL_ADDR Register */
typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t ext_src_sel_L: 4;      /* bit[3:0] */
        uint32_t ext_src_sel_R: 4;      /* bit[7:4] */
        uint32_t int_src_sel_L: 4;      /* bit[11:8] */
        uint32_t int_src_sel_R: 4;      /* bit[15:12] */
        uint32_t rsvd: 16;              /* bit[31:16] */
    };
} T_ANC_DRV_SRC_SEL;

/* The Structure of ANC_REG_LOG_ENABLE_ADDR Register */
typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t log_en: 1;             /* bit[0] */
        uint32_t log_ds_en: 1;          /* bit[1] */
        uint32_t log_rst: 1;            /* bit[2] */
        uint32_t rsvd_1: 13;            /* bit[15:3] */
        uint32_t log0_src_sel: 3;       /* bit[18:16] */
        uint32_t log1_src_sel: 3;       /* bit[21:19] */
        uint32_t log_format: 2;         /* bit[23:22] */
        uint32_t log_dest_sel: 1;       /* bit[24] */
        uint32_t rsvd_2: 7;             /* bit[31:25] */
    };
} T_ANC_DRV_LOG_ENABLE;

/* The Structure of ANC_DRV_FEATURE_MAP */
typedef union t_anc_drv_feature_map
{
    uint32_t d32;
    struct
    {
        uint32_t set_regs: 1;           /* bit[0], 0: bypass ANC register setting script in driver */
        uint32_t enable_fading: 1;      /* bit[1], 0: bypass ANC fade in/out */
        uint32_t user_mode: 1;          /* bit[2], 0: set tool mode to block ANC user mode operation */
        uint32_t mic_path: 2;           /* bit[4:3], 0: none, 1: ANC mic path, 2: APT mic path */
        uint32_t enable_dehowling: 1;   /* bit[5], 0: bypass llapt dehowling, deprecated */
        uint32_t enable_wns: 1;         /* bit[6], 0: disable WNS limiter, deprecated */
        uint32_t rsvd: 24;              /* bit[30:7] */
        uint32_t read_regs_dbg: 1;      /* bit[31], 0: disable ANC tool read register command for debug */
    };
} T_ANC_DRV_FEATURE_MAP;


typedef union t_anc_drv_llapt_vol_slew
{
    uint32_t d32[7];
    struct
    {
        uint32_t is_binaural;
        uint32_t max_target_gain_l;
        uint32_t max_target_gain_r;
        uint32_t vol_slew_l_addr;
        uint32_t vol_slew_r_addr;
        uint32_t update_done_mask_lsb;
        uint32_t update_done_mask_msb;
    };
} T_ANC_DRV_LLAPT_VOL_SLEW;

typedef union t_anc_drv_mp_data
{
    uint32_t d32[9];
    struct
    {
        uint32_t gain_mismatch[ANC_DRV_GAIN_COMP_SRC_MAX];
        uint32_t ext_data[ANC_DRV_GRP_COMP_SRC_MAX];
        uint32_t end_pattern;
    };
} T_ANC_DRV_MP_DATA;

// TODO: using ANC FIR
typedef union t_anc_drv_eq_biquad
{
    uint32_t d32[5];
} T_ANC_DRV_EQ_BIQUAD;

typedef bool (*P_ANC_CBACK)(T_ANC_CB_EVENT event);

/* Design/MP Tool usage */
uint8_t anc_drv_tool_set_para(T_ANC_CMD_PKT *anc_cmd_ptr);
uint32_t anc_drv_tool_reg_read(uint32_t anc_reg_addr);
void anc_drv_set_gain_mismatch(uint8_t gain_src, uint32_t l_gain, uint32_t r_gain);
bool anc_drv_read_gain_mismatch(uint8_t gain_src, uint8_t read_flash,
                                uint32_t *l_gain, uint32_t *r_gain);
bool anc_drv_read_mp_ext_data(T_ANC_DRV_MP_EXT_DATA *mp_ext_data);
bool anc_drv_burn_gain_mismatch(T_ANC_DRV_MP_EXT_DATA mp_ext_data);
void anc_drv_set_llapt_gain_mismatch(uint32_t l_gain, uint32_t r_gain);
bool anc_drv_read_llapt_gain_mismatch(uint8_t read_flash, uint32_t *l_gain, uint32_t *r_gain);
bool anc_drv_read_llapt_ext_data(T_ANC_DRV_MP_EXT_DATA *llapt_ext_data);
bool anc_drv_burn_llapt_gain_mismatch(T_ANC_DRV_MP_EXT_DATA llapt_ext_data);
void anc_drv_set_ear_fit_buffer(uint32_t *buffer);
void anc_drv_get_ear_fit_response(uint32_t *buffer);
void anc_drv_tool_set_feature_map(T_ANC_DRV_FEATURE_MAP feature_map);
uint32_t anc_drv_tool_get_feature_map(void);
void anc_drv_limiter_wns_switch(void);

/* Response Measurement */
bool anc_drv_play_tone_init(uint8_t enable);
bool anc_drv_cos_table_allocate(void);
void anc_drv_config_data_log(T_ANC_DRV_LOG_SRC_SEL src0_sel,
                             T_ANC_DRV_LOG_SRC_SEL src1_sel,
                             uint16_t log_len);
void anc_drv_load_data_log(void);
uint32_t anc_drv_get_data_log_length(void);
void anc_drv_response_measure_start(void);
bool anc_drv_response_measure_enable(uint8_t enable, uint32_t *tx_freq, uint8_t freq_num,
                                     uint8_t amp_ratio);
void anc_drv_preload_cos_table(void);
void anc_drv_trigger_wdg_reset(uint8_t enter_resp_meas_mode);
uint8_t anc_drv_check_resp_meas_mode(void);

/* Fade in/out */
void anc_msg_handler(void);
void anc_drv_fade_in(void);
void anc_drv_fade_out(void);
void anc_drv_set_gain(double l_gain, double r_gain);

/* EQ contorl */
void anc_drv_eq_set(double strength);

/* Normal usage */
void anc_drv_mic_src_setting(uint8_t ch_sel, uint8_t mic_sel, uint8_t mic_type);
void anc_drv_llapt_mic_src_sel(uint8_t ch_sel, uint8_t mic_sel, uint8_t mic_type);
bool anc_drv_param_init(P_ANC_CBACK cback);
void anc_drv_dsp_cfg(void);
void anc_drv_enable(void);
void anc_drv_disable(void);
void anc_drv_get_info(void *buf, T_ANC_INFO info_id);

/* ANC image parsing */
uint8_t anc_drv_get_scenario_info(uint8_t *scenario_mode, uint8_t sub_type,
                                  uint8_t *scenario_apt_effect);
void anc_drv_image_parser(void *anc_header, uint32_t *anc_image);

void anc_drv_set_img_load_param(uint8_t sub_type, uint8_t scenario_id);

/* PALU customer usage */
bool anc_drv_set_ds_rate(uint8_t ds_rate);

void anc_drv_anc_mp_bak_buf_config(uint32_t param1, uint32_t param2);

void anc_drv_ramp_data_write(uint32_t wdata);

void anc_drv_get_scenario_addr(uint32_t param);

uint8_t anc_drv_query_para(uint8_t *adsp_info);

uint8_t anc_drv_config_adsp_para(uint16_t crc_check, uint32_t adsp_para_len, uint8_t segment_total);

uint8_t anc_drv_turn_on_adsp(uint8_t enable);

uint8_t anc_drv_enable_adaptive_anc(uint8_t enable, uint8_t grp_idx);

uint8_t anc_drv_load_adsp_para(uint8_t *para_ptr, uint8_t segment_index, uint8_t mode);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif  /* _ANC_DRIVER_H_ */
