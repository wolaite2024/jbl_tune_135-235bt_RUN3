#include <stdbool.h>
#include <stdint.h>
#include "trace.h"
#include "cas.h"
#include "remote.h"
#include "app_cfg.h"
#include "app_csis.h"

#if F_APP_CSIS_SUPPORT
uint8_t csis_sirk[CSI_SIRK_LEN] = {0x11, 0x22, 0x33, 0xc6, 0xaf, 0xbb, 0x65, 0xa2, 0x5a, 0x41, 0xf1, 0x53, 0x05, 0x68, 0x8e, 0x83};

T_APP_RESULT app_csis_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_CSIS_READ_SIRK:
        {
            T_CSIS_READ_SIRK *p_data = (T_CSIS_READ_SIRK *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_CSIS_READ_SIRK: conn_handle 0x%x, service_id %d, sirk_type %d",
                            p_data->conn_handle, p_data->service_id,
                            p_data->sirk_type);
        }
        break;

    case LE_AUDIO_MSG_CSIS_LOCK_STATE:
        {
            T_CSIS_LOCK_STATE *p_data = (T_CSIS_LOCK_STATE *)buf;
            APP_PRINT_INFO4("LE_AUDIO_MSG_CSIS_LOCK_STATE: service_id %d, lock_state %d, lock_address_type %d, lock_address %s",
                            p_data->service_id, p_data->lock_state,
                            p_data->lock_address_type, TRACE_BDADDR(p_data->lock_address));
        }
        break;

    default:
        break;
    }
    return app_result;
}

void app_csis_init(void)
{
    uint8_t size = 2;
    uint8_t rank = 1;
    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        rank = 2;
    }

    T_CSIS_SIRK_TYPE sirk_type = CSIS_SIRK_PLN;
    uint8_t feature = (SET_MEMBER_LOCK_EXIST | SET_MEMBER_SIZE_EXIST | SET_MEMBER_RANK_EXIST);

    csis_init(1);
    cas_init((uint8_t *)csis_sirk, sirk_type, size, rank, feature);
}
#endif
