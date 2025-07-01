#ifndef __ADAPTER_H_
#define __ADAPTER_H_

typedef enum
{
    ADP_DET_ING = 0,
    ADP_DET_IN,
    ADP_DET_OUT,
} ADP_DET_Type;

typedef struct _ADAPTER_CONFIG
{
    uint32_t interrupt_enable:          1; // 1: enable 0: disable
    uint32_t fw_control_m1:             1; // 1: enable 0: disable
    uint32_t vl2h:                      3; // MBIAS_SEL_DPD_ADPIN_DET_H
    uint32_t vh2l:                      3; // MBIAS_SEL_DPD_ADPIN_DET_L
    uint32_t in_det_debounce_time:      8; // ADPIN detection debouncing timeout (unit: 10ms)
    uint32_t out_det_debounce_time:     8; // ADPOUT detection debouncing timeout (unit: 10ms)
    uint32_t io_in_debounce_time:       4;
    uint32_t io_out_debounce_time:      4;
} ADAPTER_CONFIG;

extern ADAPTER_CONFIG adapter_cfg;
extern ADP_DET_Type ADP_DET_status;

extern void (*create_adp_det_debounce_timer)(void);
extern void (*adapter_init)(void);

void create_adp_det_debounce_timer_rom(void);
void adapter_init_rom(void);

#endif //__ADAPTER_H_
