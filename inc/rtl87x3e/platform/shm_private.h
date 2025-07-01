/////////////////////////////////////////////////
//
// SHM, the Share Memory between MCU and DSP
// Realtek CN3-BT, Raven Su
//
/////////////////////////////////////////////////

#ifndef SHM_PRIVATE_H
#define SHM_PRIVATE_H

/////////////////////////////////////////////////

//#define ASSERT(...)
#define ASSERT(x) PLATFORM_ASSERT(x) //if(!(x)) {DBG_DIRECT("Assert(%s) fail in %s,%d\n", #x,__FILE__, __LINE__, 3); while(1) {platform_delay_ms(100);}}

//#define REG_PEON_SYS_CLK_SEL_2 0x0208
#define REG_PEON_SYS_CLK_SEL_2_R_DSP_CLK_SRC_EN             BIT25

//#define REG_SOC_FUNC_EN 0x0210
#define REG_SOC_PERI_FUNC0_EN_BIT_SOC_GDMA0_EN              BIT13

//#define REG_SOC_PERI_FUNC0_EN 0x218
#define REG_SOC_PERI_FUNC0_EN__BIT_DSP_WDT_EN               BIT29
#define REG_SOC_PERI_FUNC0_EN__BIT_ASRC_EN                  BIT28
#define REG_SOC_PERI_FUNC0_EN__BIT_DSP_MEM                  BIT27
#define REG_SOC_PERI_FUNC0_EN__BIT_DSP_H2D_D2H              BIT26
#define REG_SOC_PERI_FUNC0_EN__BIT_DSP_CORE_EN              BIT25

//#define REG_PESOC_CLK_CTRL 0x0230
#define REG_PESOC_CLK_CTRL_BIT_SOC_SLPCK_GDMA0_EN           BIT17
#define REG_PESOC_CLK_CTRL_BIT_SOC_ACTCK_GDMA0_EN           BIT16

//#define REG_PESOC_PERI_CLK_CTRL1 0x238
#define REG_PESOC_PERI_CLK_CTRL1__BIT_CKE_DSP_WDT           BIT30
#define REG_PESOC_PERI_CLK_CTRL1__BIT_SLPCKE_DSP            BIT29
#define REG_PESOC_PERI_CLK_CTRL1__BIT_ACTCKE_DSP            BIT28
#define REG_PESOC_PERI_CLK_CTRL1__BIT_SLPCKE_H2D_D2H        BIT27
#define REG_PESOC_PERI_CLK_CTRL1__BIT_ACTCKE_H2D_D2H        BIT26
#define REG_PESOC_PERI_CLK_CTRL1__BIT_SLPCKE_DSP_MEM        BIT23
#define REG_PESOC_PERI_CLK_CTRL1__BIT_ACTCKE_DSP_MEM        BIT22
#define REG_PESOC_PERI_CLK_CTRL1__BIT_SLPCKE_ASRC           BIT21
#define REG_PESOC_PERI_CLK_CTRL1__BIT_ACTCKE_ASRC           BIT20

#define H2D_D2H_BASE_HOST_TX_REQ_INT_OFS        0x04
#define H2D_D2H_BASE_HOST_RX_ACK_INT_OFS        0x08
#define H2D_D2H_BASE_DSP_EVT_INT_OFS            0x10
#define H2D_D2H_BASE_CLR_DSP_INT_OFS            0x1C
#define H2D_D2H_BASE_CLR_DSP_EVT_INT_BIT        0x01
#define H2D_D2H_BASE_CLR_DSP_RX_REQ_INT_BIT     0x02
#define H2D_D2H_BASE_CLR_DSP_TX_ACK_INT_BIT     0x04
#define H2D_D2H_BASE_DSP_RX_REQ_INT_OFS         0x14
#define H2D_D2H_BASE_DSP_TX_ACK_INT_OFS         0x18

//D2H Mailbox
//0xA0: group index, 0xA4: value
#define H2D_D2H_BASE_REG_DSP_INIT_0_OFS         0xA0
#define H2D_D2H_BASE_REG_DSP_INIT_1_OFS         0xA4
//H2D Mailbox
//0xA8: group index, 0xAC: value
#define H2D_D2H_BASE_REG_DSP_INIT_2_OFS         0xA8
#define H2D_D2H_BASE_REG_DSP_INIT_3_OFS         0xAC

/////////////////////////////////////////////////

#define DSP_STALL_OFS 0x254
#define DSP_STALL_OFS__BIT_STALL 1

typedef enum
{
    SHM_STATE_RESET,
    SHM_STATE_SHARE_MEM_READY,
    SHM_STATE_INIT_FINISH,

    SHN_STATE_TOTAL
} T_SHM_STATE;

typedef void (*shm_irq_cb)(void);
typedef void (*shm_mailbox_cb)(uint32_t group, uint32_t value);

void shm_dsp_handler(void);

#endif // SHM_PRIVATE_H
