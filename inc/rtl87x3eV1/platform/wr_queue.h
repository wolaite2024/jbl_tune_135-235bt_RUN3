/////////////////////////////////////////////////
//
// WR_Queue, the communication channel between MCU and DSP
// Realtek CN3-BT, Raven Su
//
// Version: 0.0.1 , Date: 2015/10/28
//   first version
// Version: 0.0.2 , Date: 2015/10/29
//   refine/clean code
// Version: 0.0.3 , Date: 2016/01/12
//   more log
//
/////////////////////////////////////////////////

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/

#ifndef __WR_QUEUE_H
#define __WR_QUEUE_H

/*============================================================================*
 *                               Header Files
 *============================================================================*/

#include "rtl876x.h"

/** @addtogroup 87x3e_WR_QUEUE WR Queue
  * @brief DSP Read/Write Queue
  * @{
  */


/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup DSP_WR_QUEUE_Exported_Types DSP Read/Write Queue Exported Types
  * @{
  */

//#define ASSERT(x) if(!(x)) {DBG_DIRECT("ASSERT"); while(1);}

#define __IO volatile

/////////////////////////////////////////////////

typedef struct
{
    __IO uint32_t w_index;
    __IO uint32_t w_dummy;      // dummy word for DSP RAM prefetch issue
    __IO uint32_t r_index;
    __IO uint32_t r_dummy;      // dummy word for DSP RAM prefetch issue

    uint32_t *p_buffer_dsp;
    uint32_t *p_buffer;
    uint32_t length;

    __IO uint32_t user_data;
    __IO uint32_t user_tag;

    uint32_t full_cnt;
    uint32_t send_cnt;
    uint32_t recv_cnt;
    uint32_t empty_cnt;
} T_WR_QUEUE;

typedef struct
{
    union
    {
        uint16_t raw16;
        struct
        {
            uint16_t sequence           : 8;
            uint16_t length_minus_ofs   : 2;
            uint16_t rsvd               : 6;
        } cmd;

        struct
        {
            uint16_t sequence           : 8;
            uint16_t length_minus_ofs   : 2;
            uint16_t rsvd               : 6;
        } data;
    };
} T_WR_HEADER;

typedef struct
{
    T_WR_HEADER header;
    uint16_t length;
    uint32_t *p_buffer;
} T_WR_ITEAM;

typedef struct
{
    uint32_t *p_buffer_dsp;
    uint32_t *p_buffer;
} T_CHECK_QUEUE;

/** @} */ /* End of group DSP_WR_QUEUE_Exported_Types */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup DSP_WR_QUEUE_Exported_Variables DSP Read/Write Queue Exported Variables
  * @{
  */

/////////////////////////////////////////////////
// uint32_t wr_queue_rcv(T_WR_QUEUE* p_wr_queue, Item_T* p_item)
// will modify p_wr_queue->r_index
// return value
//   0 means OK, output p_item
//   1 means empty
//   2 means data corrupt
//   3 means p_buffer is too small
//uint32_t wr_queue_rcv(T_WR_QUEUE *p_wr_queue, T_WR_ITEAM *p_item,
//                      uint32_t buff_len); // buff_len is p_item->p_buffer , "4bytes" per unit
uint32_t wr_queue_rcv(T_WR_QUEUE *p_wr_queue, T_WR_ITEAM *p_item, uint32_t buff_len);
// uint32_t wr_queue_send(T_WR_QUEUE* p_wr_queue, Item_T* p_item)
// will modify p_wr_queue->w_index
// return value
//   0 means OK
//   1 means full
//uint32_t wr_queue_send(T_WR_QUEUE *p_wr_queue, T_WR_ITEAM *p_item);
uint32_t wr_queue_send(T_WR_QUEUE *p_wr_queue, T_WR_ITEAM *p_item);

/** @} */ /* End of group DSP_WR_QUEUE_Exported_Variables */

/** @} End of 87x3e_WR_QUEUE */

#endif  // __WR_QUEUE_H
