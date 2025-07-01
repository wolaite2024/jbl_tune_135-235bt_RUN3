#ifndef VECTOR_TABLE_H
#define VECTOR_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

typedef void (*IRQ_Fun)(void);       /**< ISR Handler Prototype */

typedef enum
{
    InitialSP_VECTORn = 0,
    Reset_VECTORn,
    NMI_VECTORn,
    HardFault_VECTORn,
    MemMang_VECTORn,
    BusFault_VECTORn,
    UsageFault_VECTORn,
    RSVD0_VECTORn,
    RSVD1_VECTORn,
    RSVD2_VECTORn,
    RSVD3_VECTORn,
    SVC_VECTORn,
    DebugMonitor_VECTORn,
    RSVD4_VECTORn,
    PendSV_VECTORn,
    SysTick_VECTORn,

    System_VECTORn = 16,
    WDG_VECTORn,
    BTMAC_VECTORn,
    DSP_VECTORn,
    RXI300_VECTORn,
    SPI0_VECTORn,
    I2C0_VECTORn,
    ADC_VECTORn,
    SPORT0_RX_VECTORn,
    SPORT0_TX_VECTORn,
    Timer2_VECTORn,
    Timer3_VECTORn,
    Timer4_VECTORn,
    RTC_VECTORn,
    UART0_VECTORn,
    UART1_VECTORn,
    UART2_VECTORn,
    Peripheral_VECTORn,
    GPIO_A0_VECTORn,
    GPIO_A1_VECTORn,
    GPIO_A_2_7_VECTORn,
    GPIO_A_8_15_VECTORn,
    GPIO_A_16_23_VECTORn,
    GPIO_A_24_31_VECTORn,
    SPORT1_RX_VECTORn,
    SPORT1_TX_VECTORn,
    ADP_DET_NVIC_VECTORn,
    VBAT_DET_NVIC_VECTORn,
    GDMA0_Channel0_VECTORn,
    GDMA0_Channel1_VECTORn,
    GDMA0_Channel2_VECTORn,
    GDMA0_Channel3_VECTORn,
    GDMA0_Channel4_VECTORn,
    GDMA0_Channel5_VECTORn,
    GDMA0_Channel6_VECTORn,
    GDMA0_Channel7_VECTORn,
    GDMA0_Channel8_VECTORn,
    GPIO_B_0_7_VECTORn,
    GPIO_B_8_15_VECTORn,
    GPIO_B_16_23_VECTORn,
    GPIO_B_24_31_VECTORn,
    SPI1_VECTORn,
    SPI2_VECTORn,
    I2C1_VECTORn,
    I2C2_VECTORn,
    Keyscan_VECTORn,
    Qdecode_VECTORn,
    USB_VECTORn,
    USB_ISOC_VECTORn,
    SPIC0_VECTORn,
    SPIC1_VECTORn,
    SPIC2_VECTORn,
    SPIC3_VECTORn,
    Timer5_VECTORn,
    Timer6_VECTORn,
    Timer7_VECTORn,
    ASRC0_VECTORn,
    ASRC1_VECTORn,
    I8080_VECTORn,
    ISO7816_VECTORn,
    SDIO_HOST_VECTORn,
    SPORT2_RX_VECTORn,
    ANC_VECTORn,
    TOUCH_VECTORn,
    /* second level interrupt (Peripheral_VECTORn) */
    MFB_DET_L_VECTORn,
    PTA_Mailbox_VECTORn,
    USB_UTMI_SUSPEND_N_VECTORn,
    IR_VECTORn,
    TRNG_VECTORn,
    PSRAMC_VECTORn,
    LPCOMP_VECTORn,
    Timer5_Peri_VECTORn,
    Timer6_Peri_VECTORn,
    Timer7_Peri_VECTORn,
    Peri_10_VECTORn,
    Peri_11_VECTORn,
    VBAT_DET_VECTORn,
    ADP_DET_VECTORn,

    /* other function pointers, not real interrupts */
    ADP_IN_DET_VECTORn,
    ADP_OUT_DET_VECTORn,

    /* gpio sub interrupt */
    GPIOA2_VECTORn,
    GPIOA3_VECTORn,
    GPIOA4_VECTORn,
    GPIOA5_VECTORn,
    GPIOA6_VECTORn,
    GPIOA7_VECTORn,
    GPIOA8_VECTORn,
    GPIOA9_VECTORn,
    GPIOA10_VECTORn,
    GPIOA11_VECTORn,
    GPIOA12_VECTORn,
    GPIOA13_VECTORn,
    GPIOA14_VECTORn,
    GPIOA15_VECTORn,
    GPIOA16_VECTORn,
    GPIOA17_VECTORn,
    GPIOA18_VECTORn,
    GPIOA19_VECTORn,
    GPIOA20_VECTORn,
    GPIOA21_VECTORn,
    GPIOA22_VECTORn,
    GPIOA23_VECTORn,
    GPIOA24_VECTORn,
    GPIOA25_VECTORn,
    GPIOA26_VECTORn,
    GPIOA27_VECTORn,
    GPIOA28_VECTORn,
    GPIOA29_VECTORn,
    GPIOA30_VECTORn,
    GPIOA31_VECTORn,
    GPIOB0_VECTORn,
    GPIOB1_VECTORn,
    GPIOB2_VECTORn,
    GPIOB3_VECTORn,
    GPIOB4_VECTORn,
    GPIOB5_VECTORn,
    GPIOB6_VECTORn,
    GPIOB7_VECTORn,
    GPIOB8_VECTORn,
    GPIOB9_VECTORn,
    GPIOB10_VECTORn,
    GPIOB11_VECTORn,
    GPIOB12_VECTORn,
    GPIOB13_VECTORn,
    GPIOB14_VECTORn,
    GPIOB15_VECTORn,
    GPIOB16_VECTORn,
    GPIOB17_VECTORn,
    GPIOB18_VECTORn,
    GPIOB19_VECTORn,
    GPIOB20_VECTORn,
    GPIOB21_VECTORn,
    GPIOB22_VECTORn,
    GPIOB23_VECTORn,
    GPIOB24_VECTORn,
    GPIOB25_VECTORn,
    GPIOB26_VECTORn,
    GPIOB27_VECTORn,
    GPIOB28_VECTORn,
    GPIOB29_VECTORn,
    GPIOB30_VECTORn,
    GPIOB31_VECTORn,
} VECTORn_Type;

#define DebugMon_Handler        HardFault_Handler
#define WDT_Handler             HardFault_Handler
#define RXI300_Handler          HardFault_Handler


extern void *RomVectorTable[];
extern void *RamVectorTable[];

/**
 * @brief  Initialize RAM vector table to a given RAM address.
 * @param  ram_vector_addr: RAM Vector Address.
 * @retval true: Success
 *         false: Fail
 * @note   When using vector table relocation, the base address of the new vector
 *         table must be aligned to the size of the vector table extended to the
 *         next larger power of 2. In RTL8763, the base address is aligned at 0x100.
 */
bool RamVectorTableInit(uint32_t ram_vector_addr);

/**
 * @brief  Update ISR Handler in RAM Vector Table.
 * @param  v_num: Vector number(index)
 * @param  isr_handler: User defined ISR Handler.
 * @retval true: Success
 *         false: Fail
 */
extern bool RamVectorTableUpdate(VECTORn_Type v_num, IRQ_Fun isr_handler);

void peripheral_interrupt_configuration(void);

void HardFault_Handler(void);

bool RamVectorTableUpdate_rom(VECTORn_Type v_num, IRQ_Fun isr_handler);

/************************************************************************** used for vector table */
__weak void Reset_Handler(void);
__weak void NMI_Handler(void);
//__weak void HardFault_Handler(void);
__weak void MemManage_Handler(void);
__weak void BusFault_Handler(void);
__weak void UsageFault_Handler(void);
__weak void SVC_Handler(void);
__weak void DebugMon_Handler(void);
__weak void PendSV_Handler(void);
__weak void SysTick_Handler(void);
__weak void System_Handler(void);
//__weak void WDT_Handler(void);
__weak void BTMAC_Handler(void);
__weak void DSP_Handler(void);
__weak void RXI300_Handler(void);
__weak void SPI0_Handler(void);
__weak void I2C0_Handler(void);
__weak void ADC_Handler(void);
__weak void SPORT0_TX_Handler(void);
__weak void SPORT0_RX_Handler(void);
__weak void TIM2_Handler(void);
__weak void TIM3_Handler(void);
__weak void TIM4_Handler(void);
__weak void RTC_Handler(void);
__weak void UART0_Handler(void);
__weak void UART1_Handler(void);
__weak void UART2_Handler(void);
__weak void Peri_Handler(void);
__weak void GPIO_A0_Handler(void);
__weak void GPIO_A1_Handler(void);
__weak void GPIO_A_2_7_Handler(void);
__weak void GPIO_A_8_15_Handler(void);
__weak void GPIO_A_16_23_Handler(void);
__weak void GPIO_A_24_31_Handler(void);
__weak void GDMA0_Channel0_Handler(void);
__weak void GDMA0_Channel1_Handler(void);
__weak void GDMA0_Channel2_Handler(void);
__weak void GDMA0_Channel3_Handler(void);
__weak void GDMA0_Channel4_Handler(void);
__weak void GDMA0_Channel5_Handler(void);
__weak void GDMA0_Channel6_Handler(void);
__weak void GDMA0_Channel7_Handler(void);
__weak void GDMA0_Channel8_Handler(void);
__weak void GPIO_B_0_7_Handler(void);
__weak void GPIO_B_8_15_Handler(void);
__weak void GPIO_B_16_23_Handler(void);
__weak void GPIO_B_24_31_Handler(void);
__weak void SPORT1_RX_Handler(void);
__weak void SPORT1_TX_Handler(void);
__weak void SPORT2_RX_Handler(void);
__weak void SPI1_Handler(void);
__weak void SPI2_Handler(void);
__weak void I2C1_Handler(void);
__weak void I2C2_Handler(void);
__weak void IR_Handler(void);
__weak void KeyScan_Handler(void);
__weak void QDEC_Handler(void);
__weak void USB_Handler(void);
__weak void USB_ISOC_Handler(void);
__weak void Utmi_Suspend_N_Handler(void);
__weak void SPIC0_Handler(void);
__weak void SPIC1_Handler(void);
__weak void SPIC2_Handler(void);
__weak void SPIC3_Handler(void);
__weak void PSRAMC_Handler(void);
__weak void LPCOMP_Handler(void);
__weak void TIM5_Handler(void);
__weak void TIM6_Handler(void);
__weak void TIM7_Handler(void);
__weak void Default_Handler(void);
__weak void SPDIF_TX_Handler(void);
__weak void ASRC0_Handler(void);
__weak void ASRC1_Handler(void);
__weak void I8080_Handler(void);
__weak void ISO7816_Handler(void);
__weak void SDIO0_Handler(void);
__weak void ANC_Handler(void);
__weak void TOUCH_Handler(void);
__weak void MFB_DET_L_Handler(void);
__weak void VBAT_DET_Handler(void);
__weak void ADP_DET_Handler(void);
__weak void PTA_Mailbox_Handler(void);
__weak void TRNG_Handler(void);
__weak void ADP_IN_DET_Handler(void);
__weak void ADP_OUT_DET_Handler(void);
__weak void GPIOA0_Handler(void);
__weak void GPIOA1_Handler(void);
__weak void GPIOA2_Handler(void);
__weak void GPIOA3_Handler(void);
__weak void GPIOA4_Handler(void);
__weak void GPIOA5_Handler(void);
__weak void GPIOA6_Handler(void);
__weak void GPIOA7_Handler(void);
__weak void GPIOA8_Handler(void);
__weak void GPIOA9_Handler(void);
__weak void GPIOA10_Handler(void);
__weak void GPIOA11_Handler(void);
__weak void GPIOA12_Handler(void);
__weak void GPIOA13_Handler(void);
__weak void GPIOA14_Handler(void);
__weak void GPIOA15_Handler(void);
__weak void GPIOA16_Handler(void);
__weak void GPIOA17_Handler(void);
__weak void GPIOA18_Handler(void);
__weak void GPIOA19_Handler(void);
__weak void GPIOA20_Handler(void);
__weak void GPIOA21_Handler(void);
__weak void GPIOA22_Handler(void);
__weak void GPIOA23_Handler(void);
__weak void GPIOA24_Handler(void);
__weak void GPIOA25_Handler(void);
__weak void GPIOA26_Handler(void);
__weak void GPIOA27_Handler(void);
__weak void GPIOA28_Handler(void);
__weak void GPIOA29_Handler(void);
__weak void GPIOA30_Handler(void);
__weak void GPIOA31_Handler(void);
__weak void GPIOB0_Handler(void);
__weak void GPIOB1_Handler(void);
__weak void GPIOB2_Handler(void);
__weak void GPIOB3_Handler(void);
__weak void GPIOB4_Handler(void);
__weak void GPIOB5_Handler(void);
__weak void GPIOB6_Handler(void);
__weak void GPIOB7_Handler(void);
__weak void GPIOB8_Handler(void);
__weak void GPIOB9_Handler(void);
__weak void GPIOB10_Handler(void);
__weak void GPIOB11_Handler(void);
__weak void GPIOB12_Handler(void);
__weak void GPIOB13_Handler(void);
__weak void GPIOB14_Handler(void);
__weak void GPIOB15_Handler(void);
__weak void GPIOB16_Handler(void);
__weak void GPIOB17_Handler(void);
__weak void GPIOB18_Handler(void);
__weak void GPIOB19_Handler(void);
__weak void GPIOB20_Handler(void);
__weak void GPIOB21_Handler(void);
__weak void GPIOB22_Handler(void);
__weak void GPIOB23_Handler(void);
__weak void GPIOB24_Handler(void);
__weak void GPIOB25_Handler(void);
__weak void GPIOB26_Handler(void);
__weak void GPIOB27_Handler(void);
__weak void GPIOB28_Handler(void);
__weak void GPIOB29_Handler(void);
__weak void GPIOB30_Handler(void);
__weak void GPIOB31_Handler(void);
__weak void GPIO_A_6_31_Handler(void);
__weak void DSP_WDG_Handler(void);


// for BBPRO compatible
#define GPIO0_Handler      GPIOA0_Handler
#define GPIO1_Handler      GPIOA1_Handler
#define GPIO2_Handler      GPIOA2_Handler
#define GPIO3_Handler      GPIOA3_Handler
#define GPIO4_Handler      GPIOA4_Handler
#define GPIO5_Handler      GPIOA5_Handler
#define GPIO6_Handler      GPIOA6_Handler
#define GPIO7_Handler      GPIOA7_Handler
#define GPIO8_Handler      GPIOA8_Handler
#define GPIO9_Handler      GPIOA9_Handler
#define GPIO10_Handler     GPIOA10_Handler
#define GPIO11_Handler     GPIOA11_Handler
#define GPIO12_Handler     GPIOA12_Handler
#define GPIO13_Handler     GPIOA13_Handler
#define GPIO14_Handler     GPIOA14_Handler
#define GPIO15_Handler     GPIOA15_Handler
#define GPIO16_Handler     GPIOA16_Handler
#define GPIO17_Handler     GPIOA17_Handler
#define GPIO18_Handler     GPIOA18_Handler
#define GPIO19_Handler     GPIOA19_Handler
#define GPIO20_Handler     GPIOA20_Handler
#define GPIO21_Handler     GPIOA21_Handler
#define GPIO22_Handler     GPIOA22_Handler
#define GPIO23_Handler     GPIOA23_Handler
#define GPIO24_Handler     GPIOA24_Handler
#define GPIO25_Handler     GPIOA25_Handler
#define GPIO26_Handler     GPIOA26_Handler
#define GPIO27_Handler     GPIOA27_Handler
#define GPIO28_Handler     GPIOA28_Handler
#define GPIO29_Handler     GPIOA29_Handler
#define GPIO30_Handler     GPIOA30_Handler
#define GPIO31_Handler     GPIOA31_Handler

#define GPIO32_Handler     GPIOB0_Handler
#define GPIO33_Handler     GPIOB1_Handler
#define GPIO34_Handler     GPIOB2_Handler
#define GPIO35_Handler     GPIOB3_Handler
#define GPIO36_Handler     GPIOB4_Handler
#define GPIO37_Handler     GPIOB5_Handler
#define GPIO38_Handler     GPIOB6_Handler
#define GPIO39_Handler     GPIOB7_Handler
#define GPIO40_Handler     GPIOB8_Handler
#define GPIO41_Handler     GPIOB9_Handler
#define GPIO42_Handler     GPIOB10_Handler
#define GPIO43_Handler     GPIOB11_Handler
#define GPIO44_Handler     GPIOB12_Handler
#define GPIO45_Handler     GPIOB13_Handler
#define GPIO46_Handler     GPIOB14_Handler
#define GPIO47_Handler     GPIOB15_Handler
#define GPIO48_Handler     GPIOB16_Handler
#define GPIO49_Handler     GPIOB17_Handler
#define GPIO50_Handler     GPIOB18_Handler
#define GPIO51_Handler     GPIOB19_Handler
#define GPIO52_Handler     GPIOB20_Handler
#define GPIO53_Handler     GPIOB21_Handler
#define GPIO54_Handler     GPIOB22_Handler
#define GPIO55_Handler     GPIOB23_Handler
#define GPIO56_Handler     GPIOB24_Handler
#define GPIO57_Handler     GPIOB25_Handler
#define GPIO58_Handler     GPIOB26_Handler
#define GPIO59_Handler     GPIOB27_Handler
#define GPIO60_Handler     GPIOB28_Handler
#define GPIO61_Handler     GPIOB29_Handler
#define GPIO62_Handler     GPIOB30_Handler
#define GPIO63_Handler     GPIOB31_Handler
// for BBPRO compatible
#define GPIO0_VECTORn      GPIOA0_VECTORn
#define GPIO1_VECTORn      GPIOA1_VECTORn
#define GPIO2_VECTORn      GPIOA2_VECTORn
#define GPIO3_VECTORn      GPIOA3_VECTORn
#define GPIO4_VECTORn      GPIOA4_VECTORn
#define GPIO5_VECTORn      GPIOA5_VECTORn
#define GPIO6_VECTORn      GPIOA6_VECTORn
#define GPIO7_VECTORn      GPIOA7_VECTORn
#define GPIO8_VECTORn      GPIOA8_VECTORn
#define GPIO9_VECTORn      GPIOA9_VECTORn
#define GPIO10_VECTORn     GPIOA10_VECTORn
#define GPIO11_VECTORn     GPIOA11_VECTORn
#define GPIO12_VECTORn     GPIOA12_VECTORn
#define GPIO13_VECTORn     GPIOA13_VECTORn
#define GPIO14_VECTORn     GPIOA14_VECTORn
#define GPIO15_VECTORn     GPIOA15_VECTORn
#define GPIO16_VECTORn     GPIOA16_VECTORn
#define GPIO17_VECTORn     GPIOA17_VECTORn
#define GPIO18_VECTORn     GPIOA18_VECTORn
#define GPIO19_VECTORn     GPIOA19_VECTORn
#define GPIO20_VECTORn     GPIOA20_VECTORn
#define GPIO21_VECTORn     GPIOA21_VECTORn
#define GPIO22_VECTORn     GPIOA22_VECTORn
#define GPIO23_VECTORn     GPIOA23_VECTORn
#define GPIO24_VECTORn     GPIOA24_VECTORn
#define GPIO25_VECTORn     GPIOA25_VECTORn
#define GPIO26_VECTORn     GPIOA26_VECTORn
#define GPIO27_VECTORn     GPIOA27_VECTORn
#define GPIO28_VECTORn     GPIOA28_VECTORn
#define GPIO29_VECTORn     GPIOA29_VECTORn
#define GPIO30_VECTORn     GPIOA30_VECTORn
#define GPIO31_VECTORn     GPIOA31_VECTORn
/**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // VECTOR_TABLE_H
