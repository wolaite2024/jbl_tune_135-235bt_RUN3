/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     mcs_obj_str.h
  * @brief    Head file for Media Control Service.
  * @details  create object structure
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _MCS_OBJ_STR_H_
#define _MCS_OBJ_STR_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if LE_AUDIO_MCS_SERV_SUPPORT
#include "mcs_server.h"
typedef struct t_tk_elem
{
    struct t_tk_elem   *p_next;
    uint64_t            gp_obj_id;
    uint64_t            obj_id;
    uint64_t            seg_obj_id;
    T_MCS_TK_ELEM       tk_elem;
} T_TK_ELEM;

typedef struct t_gp_elem
{
    struct t_gp_elem   *p_next;
    struct t_gp_elem   *parent_gp;
    T_OS_QUEUE          tr_queue;
    uint64_t            obj_id;
    T_MCS_GP_ELEM       gp_elem;
} T_GP_ELEM;

void mcs_update_gp_value(T_MCS_CB *p_entry, uint64_t gp_obj_id);
void mcs_create_objs_complt(T_MCS_CB *p_entry);
T_TK_ELEM *mcs_find_tk_by_tk_id(T_MCS_CB *p_entry, uint64_t tk_obj_id);
uint64_t mcs_find_parent_id_by_gp_id(T_MCS_CB *p_entry, uint64_t gp_obj_id);
void mcs_delete_track(T_MCS_CB *p_entry, uint64_t gp_obj_id, uint64_t tk_obj_id);
void mcs_delete_gp(T_MCS_CB *p_entry, uint64_t gp_obj_id);
uint64_t mcs_create_track(T_MCS_CB *p_entry, T_MCS_TK_ELEM *p_tk_elem, uint64_t gp_obj_id,
                          uint8_t *p_id3v2_fmt, uint16_t fmt_len);
uint64_t mcs_create_spec_gp(T_MCS_CB *p_entry, T_MCS_GP_ELEM *p_gp_elem, uint64_t parent_obj_id);
uint64_t mcs_create_root_gp(T_MCS_CB *p_entry, T_MCS_GP_ELEM *p_gp_elem);

#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _MCS_OBJ_STR_H_ */
