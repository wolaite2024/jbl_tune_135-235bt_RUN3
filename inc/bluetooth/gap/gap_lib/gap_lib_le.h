/*
* Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
*/

#ifndef _GAP_LIB_LE_H_
#define _GAP_LIB_LE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <gap_le.h>

/** @addtogroup GAP GAP Module
  * @{
  */

/** @addtogroup GAP_LIB_LE GAP LIB LE Module
  * @{
  */
/** @brief Mode of setting priority. */
typedef enum
{
    GAP_VENDOR_UPDATE_PRIORITY, //!< Set priority without operation of resetting priority
    GAP_VENDOR_SET_PRIORITY,    //!< Set priority after operation of resetting priority
    GAP_VENDOR_RESET_PRIORITY,  //!< Reset priority
} T_GAP_VENDOR_SET_PRIORITY_MODE;

/** @brief Definition of priority level. */
typedef enum
{
    GAP_VENDOR_RESERVED_PRIORITY,
    GAP_VENDOR_PRIORITY_LEVEL_1,
    GAP_VENDOR_PRIORITY_LEVEL_2,
    GAP_VENDOR_PRIORITY_LEVEL_3,
    GAP_VENDOR_PRIORITY_LEVEL_4,
    GAP_VENDOR_PRIORITY_LEVEL_5,
    GAP_VENDOR_PRIORITY_LEVEL_6,
    GAP_VENDOR_PRIORITY_LEVEL_7,
    GAP_VENDOR_PRIORITY_LEVEL_8,
    GAP_VENDOR_PRIORITY_LEVEL_9,
    GAP_VENDOR_PRIORITY_LEVEL_10,  //!< Highest priority
} T_GAP_VENDOR_PRIORITY_LEVEL;

/** @brief Mode of setting link priority. */
typedef enum
{
    GAP_VENDOR_NOT_SET_LINK_PRIORITY,       //!< Not set priority of link
    GAP_VENDOR_SET_SPECIFIC_LINK_PRIORITY,  //!< Set priority of specific links
    GAP_VENDOR_SET_ALL_LINK_PRIORITY,       //!< Set priority of all link
} T_GAP_VENDOR_SET_LINK_PRIORITY_MODE;

/** @brief Definition of common priority. */
typedef struct
{
    bool set_priority_flag;
    T_GAP_VENDOR_PRIORITY_LEVEL priority_level; /**< Priority is valid if set_priority_flag is true. */
} T_GAP_VENDOR_COMMON_PRIORITY;

/** @brief Definition of connection priority. */
typedef struct
{
    uint8_t conn_id;
    T_GAP_VENDOR_PRIORITY_LEVEL conn_priority_level;/**< Priority of specific connection. */
} T_GAP_VENDOR_CONN_PRIORITY;

/** @brief  Parameters of setting priority.*/
typedef struct
{
    T_GAP_VENDOR_SET_PRIORITY_MODE set_priority_mode;/**< Mode of setting priority.
                                                            (@ref T_GAP_VENDOR_SET_PRIORITY_MODE). */
    T_GAP_VENDOR_COMMON_PRIORITY adv_priority;/**< Priority of advertising.
                                                     (@ref T_GAP_VENDOR_COMMON_PRIORITY). */
    T_GAP_VENDOR_COMMON_PRIORITY scan_priority;/**< Priority of scan.
                                                      (@ref T_GAP_VENDOR_COMMON_PRIORITY). */
    T_GAP_VENDOR_COMMON_PRIORITY initiate_priority;/**< Priority of initiating.
                                                          (@ref T_GAP_VENDOR_COMMON_PRIORITY). */
    T_GAP_VENDOR_SET_LINK_PRIORITY_MODE link_priority_mode;/**< Mode of setting link priority.
                                                                  (@ref T_GAP_VENDOR_SET_LINK_PRIORITY_MODE). */
    T_GAP_VENDOR_PRIORITY_LEVEL link_priority_level;/**< Priority of all link is valid
                                                           if link_priority_mode is GAP_VENDOR_SET_ALL_LINK_PRIORITY. */
    uint8_t num_conn_ids;/**< Number of specific links is valid if link_priority_mode is GAP_VENDOR_SET_SPECIFIC_LINK_PRIORITY. */
    T_GAP_VENDOR_CONN_PRIORITY p_conn_id_list[1];/**< List of connection priority is valid
                                                        if link_priority_mode is GAP_VENDOR_SET_SPECIFIC_LINK_PRIORITY.
                                                        (@ref T_GAP_VENDOR_CONN_PRIORITY). */
} T_GAP_VENDOR_PRIORITY_PARAM;

/** End of GAP_LIB_LE
  * @}
  */

/** End of GAP
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* _GAP_LIB_LE_H_ */
