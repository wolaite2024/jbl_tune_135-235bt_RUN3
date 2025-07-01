/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    gap_vendor.h
  * @brief
  * @details
  * @author  ranhui_xia
  * @date    2017-08-02
  * @version v1.0
  ******************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
  ******************************************************************************
  */
#ifndef GAP_VNR_H
#define GAP_VNR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup GAP GAP Module
  * @{
  */

/** @addtogroup GAP_VENDOR GAP Vendor
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup Gap_Vendor_Exported_Macros GAP Vendor Exported Macros
  * @{
  */

/** @defgroup GAP_LE_MSG_Types GAP LE Msg Types
  * @{
  */
#define GAP_MSG_LE_VENDOR_SET_PRIORITY              0xA8

/**
  * @}
  */

/** End of GAP_Vendor_Exported_Macros
  * @}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup GAP_Vendor_Exported_Types GAP Vendor Exported Types
  * @{
  */

typedef struct
{
    uint16_t cause;
} T_GAP_VENDOR_CAUSE;

typedef union
{
    T_GAP_VENDOR_CAUSE gap_vendor_cause;
} T_GAP_VENDOR_CB_DATA;

/** End of GAP_Vendor_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup GAP_VENDOR_Exported_Functions GAP vendor command Exported Functions
  * @brief GAP vendor command Exported Functions
  * @{
  */

/**
 * @brief  Register callback to vendor command, when messages in @ref GAP_VENDOR_MSG_TYPE happens, it will callback to app.
 * @param[in] vendor_callback Callback function provided by the APP to handle @ref GAP_VENDOR_MSG_TYPE messages sent from the GAP.
 *              @arg NULL -> Not send @ref GAP_VENDOR_MSG_TYPE messages to APP.
 *              @arg Other -> Use application defined callback function.
 * @return void
 *
 * <b>Example usage</b>
 * \code{.c}
   void app_le_gap_init(void)
    {
        ......
        gap_register_vendor_cb(app_gap_vendor_callback);
    }
    void app_gap_vendor_callback(uint8_t cb_type, void *p_cb_data)
    {
        T_GAP_VENDOR_CB_DATA cb_data;
        memcpy(&cb_data, p_cb_data, sizeof(T_GAP_VENDOR_CB_DATA));
        APP_PRINT_INFO1("app_gap_vendor_callback: cb_type is %d", cb_type);
        switch (cb_type)
        {
        case GAP_MSG_GAP_VENDOR_SET_ANT_CTRL:
            APP_PRINT_INFO1("GAP_MSG_GAP_VENDOR_SET_ANT_CTRL: cause 0x%x",
                            cb_data.gap_vendor_cause.cause);
            break;
        default:
            break;
        }
        return;
    }
   \endcode
 */
void gap_register_vendor_cb(P_FUN_GAP_APP_CB vendor_callback);

/** End of GAP_VENDOR_Exported_Functions
  * @}
  */

/** End of GAP_VENDOR
  * @}
  */

/** End of GAP
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* GAP_VNR_H */
