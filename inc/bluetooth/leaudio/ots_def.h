/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     ots_def.h
  * @brief    Head file for Object Transfer Service.
  * @details  mcs data structs and external functions declaration.
  * @author   cheng_cai
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _OTS_DEF_H_
#define _OTS_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#define OTS_OBJ_ID_BASE                 0x0100
#define OTS_DIR_LIST_OBJ_ID             0x0000

#define OBJECT_TRANS_SERVICE_UUID       0x1825

#define OTS_FEATURE_CHAR_UUID           0x2ABD
#define OTS_OBJ_NAME_CHAR_UUID          0x2ABE
#define OTS_OBJ_TYPE_CHAR_UUID          0x2ABF
#define OTS_OBJ_SIZE_CHAR_UUID          0x2AC0
#define OTS_OBJ_FIRST_CR_CHAR_UUID      0x2AC1
#define OTS_OBJ_LAST_MOD_CHAR_UUID      0x2AC2
#define OTS_OBJ_ID_CHAR_UUID            0x2AC3
#define OTS_OBJ_PROPS_CHAR_UUID         0x2AC4
#define OTS_OACP_CHAR_UUID              0x2AC5
#define OTS_OLCP_CHAR_UUID              0x2AC6
#define OTS_OLF_CHAR_UUID               0x2AC7
#define OTS_OBJ_CHANGE_CHAR_UUID        0x2AC8



#define OTS_DIRECT_LIST_OBJ_UUID        0x2ACB
#define OTS_PSM                         0x0025


//Object action control point opcde
#define OTS_OACP_CREATE_OP            0x01
#define OTS_OACP_DELETE_OP            0x02
#define OTS_OACP_CAL_CHECKSUM_OP      0x03
#define OTS_OACP_EXECUTE_OP           0x04
#define OTS_OACP_READ_OP              0x05
#define OTS_OACP_WRITE_OP             0x06
#define OTS_OACP_ABORT_OP             0x07
#define OTS_OACP_RSP_CODE_OP          0x60

//Object action control point result code
#define OACP_SUCCESS_RES_CODE            0x01
#define OACP_OPCODE_NOTSUP_RES_CODE      0x02
#define OACP_INV_PARAM_RES_CODE          0x03
#define OACP_INSF_RESC_RES_CODE          0x04
#define OACP_INV_OBJ_RES_CODE            0x05
#define OACP_CHAN_UNAV_RES_CODE          0x06
#define OACP_UNSUP_TYPE_RES_CODE         0x07
#define OACP_PROCE_NOT_PERM_RES_CODE     0x08
#define OACP_OBJ_LOCKD_RES_CODE          0x09
#define OACP_OPER_FAIL_RES_CODE          0x0A

//object properties
#define OBJ_PROP_DELETE_BIT_NUM               0
#define OBJ_PROP_EXECUTE_BIT_NUM              1
#define OBJ_PROP_READ_BIT_NUM                 2
#define OBJ_PROP_WRITE_BIT_NUM                3
#define OBJ_PROP_APPEDND_BIT_NUM              4
#define OBJ_PROP_TRUNCATE_BIT_NUM             5
#define OBJ_PROP_PATCH_BIT_NUM                6
#define OBJ_PROP_MARK_BIT_NUM                 7

//Object feature bit
#define OTS_OACP_CREATE_FEAT_MSK            0
#define OTS_OACP_DELETE_FEAT_MSK            1
#define OTS_OACP_CAL_CHECKSUM_FEAT_MSK      2
#define OTS_OACP_EXECUTE_FEAT_MSK           3
#define OTS_OACP_READ_FEAT_MSK              4
#define OTS_OACP_WRITE_FEAT_MSK             5
#define OTS_OACP_APPEND_FEAT_MSK            6
#define OTS_OACP_TRUNCATE_FEAT_MSK          7
#define OTS_OACP_PATCH_FEAT_MSK             8
#define OTS_OACP_ABORT_FEAT_MSK             9

#define OTS_OLCP_GOTO_FEAT_MSK              0
#define OTS_OLCP_ORDER_FEAT_MSK             1
#define OTS_OLCP_REQ_NUM_OBJ_FEAT_MSK       2
#define OTS_OLCP_CLR_MARK_FEAT_MSK          3

#define OTS_WRITE_REJECTED              0x80
#define OTS_OBJ_NOT_SELECTED            0x81
#define OTS_CONCURR_LIMIT_EXCEED        0x82
#define OTS_OBJ_NAME_ALREADY_EXIST      0x83


//List Sort Order Parameter
#define OBJ_NAME_ASCEND_SORT_ORDER              0x01
#define OBJ_TYPE_ASCEND_SORT_ORDER              0x02
#define OBJ_CUR_SIZE_ASCEND_SORT_ORDER          0x03
#define OBJ_FSCR_TS_ASCEND_SORT_ORDER           0x04
#define OBJ_LAST_MODIY_ASCEND_SORT_ORDER        0x05

#define OBJ_NAME_DESCEND_SORT_ORDER             0x11
#define OBJ_TYPE_DESCEND_SORT_ORDER             0x12
#define OBJ_CUR_SIZE_DESCEND_SORT_ORDER         0x13
#define OBJ_FSCR_TS_DESCEND_SORT_ORDER          0x14
#define OBJ_LAST_MODIY_DESCEND_SORT_ORDER       0x15

//Object list control point result code
#define OLCP_SUCCESS_RES_CODE            0x01
#define OLCP_OPCODE_NOTSUP_RES_CODE      0x02
#define OLCP_INV_PARAM_RES_CODE          0x03
#define OLCP_OPER_FAIL_RES_CODE          0x04
#define OLCP_OOB_RES_CODE                0x05
#define OLCP_TOO_MANY_OBJS_RES_CODE      0x06
#define OLCP_NO_OBJ_RES_CODE             0x07
#define OLCP_NO_OBJID_RES_CODE           0x08

//Object list control point opcode
#define OTS_OLCP_FIRST_OP             0x01
#define OTS_OLCP_LAST_OP              0x02
#define OTS_OLCP_PREVIOUS_OP          0x03
#define OTS_OLCP_NEXT_OP              0x04
#define OTS_OLCP_GOTO_OP              0x05
#define OTS_OLCP_ORDER_OP             0x06
#define OTS_OLCP_REQ_NUM_OBJS_OP      0x07
#define OTS_OLCP_CLR_MARK_OP          0x08
#define OTS_OLCP_RSP_CODE_OP          0x70

//Object change char
#define OTS_SRC_OF_CHG_SERVER         0x00
#define OTS_SRC_OF_CHG_CLIENT         0x01
#define OTS_OBJ_CONT_CHG_TRUE         0x02
#define OTS_OBJ_META_CHG_TRUE         0x04
#define OTS_OBJ_CREATE                0x08
#define OTS_OBJ_DEL                   0x10

typedef enum
{
    OTS_NO_FILTER                      = 0x00,
    OTS_NAME_STARTS_WITH,
    OTS_NAME_ENDS_WITH,
    OTS_NAME_CONTAINS,
    OTS_NAME_IS_EXACTLY,
    OTS_OBJ_TYPE,
    OTS_CREATE_BETWEEN_TMS,
    OTS_MOD_BETWEEN_TMS,
    OTS_CUR_SIZE_BETWEEN,
    OTS_ALLOC_SIZE_BETWEEN,
    OTS_MARKED_OBJS,
    OTS_RFU
} T_OTS_FILTER;

#define OTS_OBJ_NAME_LEN_MAX              60
#define OTS_OBJ_ID_LEN                    6


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _OTS_COMM_H_ */
