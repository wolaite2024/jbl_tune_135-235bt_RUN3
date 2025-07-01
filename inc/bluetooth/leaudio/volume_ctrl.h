#ifndef _VOLUME_CTRL_H_
#define _VOLUME_CTRL_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

typedef struct
{
    uint16_t       conn_handle;
} T_VCS_VCP_INIT_DONE;

bool volume_ctrl_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
