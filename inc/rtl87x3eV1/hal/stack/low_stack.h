#ifndef LOW_STACK_H_
#define LOW_STACK_H_

#include <stdint.h>
#include <stdbool.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define LOWER_STACK_EN 1

void low_stack_init(void);
bool low_stack_task_is_running(void);
void low_stack_wake(void);

/**
 * @brief Encrypt a 16-byte data.
 * @param key  Encryption key.
 * @param[in] data  Input data to encrypt.
 * @param[out] out  Output data encrypted.
 */
void ll_driver_encrypt_16bytes(void *key, void *data, void *out);

/**
 * @brief Generate a 32bit random value.
 * @return A 32bit random value.
 */
uint32_t ll_driver_random(void);

/**
 * @brief Read bluetooth piconet clock for application
 * @param conn_handle   identify which connection to be used
 * @param bb_clk_timer  piconet clk[27:0] (unit : half slot)
 * @param bb_clk_us     count up timer (range 0~624 us) \n
 * \p if bb_clk_timer[0] = 1, 312 <= bb_clk_us <= 624; \n
 * \p if bb_clk_timer[0] = 0, 0 <= bb_clk_us <= 312;
 */
uint8_t rws_read_bt_piconet_clk(uint16_t conn_handle, uint32_t *bb_clk_timer, uint16_t *bb_clk_us);

uint8_t read_bt_rssi_callback(uint16_t conn_handle, int8_t *last_rssi, int8_t *avg_rssi);

#define EXT_OP_LOWERSTACK_READ_BT_CLK_INDEX     0x01

extern uint8_t (*rcp_bb_write_baseband_register_func)(void *buf, ...);
#define lowerstack_SystemCall(opcode, param0, param1, param2) rcp_bb_write_baseband_register_func((void *)opcode, param0, param1, param2)

static inline bool bt_clk_index_read(uint16_t conn_handle, uint8_t *clk_index, uint8_t *role)
{
    uint32_t local_index;
    uint32_t local_role;

    if (lowerstack_SystemCall(EXT_OP_LOWERSTACK_READ_BT_CLK_INDEX,
                              conn_handle,
                              (uint32_t)&local_index,
                              (uint32_t)&local_role) == 0)
    {
        *clk_index = (uint8_t)local_index;
        *role      = (uint8_t)local_role;
        return true;
    }
    else
    {
        *clk_index = 0xFF;
        *role      = 0x00;
        return false;
    }
}

/**
 * @brief Return expected A2DP sequence number of the recovery link
 * @param conn_handle  Connection handle of the recovery link which must be
 * primary role
 * @return 0x0000~0xFFFF: valid expected A2DP sequence number of the recovery
 * link; negative number is error code
 */
int32_t recovery_link_get_expected_a2dp_seq(uint16_t conn_handle);

#if defined (__cplusplus)
}
#endif

#endif /* ! LOW_STACK_H_ */
