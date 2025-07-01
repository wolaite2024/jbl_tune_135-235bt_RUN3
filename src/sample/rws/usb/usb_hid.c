#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "os_sync.h"
#include "hid_driver.h"
#include "usb_hid_desc.h"
#include "trace.h"

typedef void (*VOID_FUNC)();

static const char report_descs[] =
{
    HID_REPORT_DESCS
};

typedef struct _t_hid_req
{
    T_HID_DRIVER_DATA data;
    VOID_FUNC cb;
} T_HID_REQ;

typedef struct _t_hid_req_node
{
    struct _t_hid_req *p_next;
    T_HID_REQ *req;
} T_HID_REQ_NODE;

typedef struct _t_hid_db
{
    void *handle;
    void *mutex;
    T_HID_REQ *cur_req;
    T_OS_QUEUE pending_reqs;
} T_HID_DB;

T_HID_DB   hid_db;
static void usb_hid_data_send_complete(void *param);

static void usb_hid_handle_pending_req(void)
{
    bool send_now = false;

    os_mutex_take(hid_db.mutex, 0xFFFFFFFF);
    if (hid_db.cur_req == NULL && hid_db.pending_reqs.count > 0)
    {
        T_HID_REQ_NODE  *cur_req_node = os_queue_out(&(hid_db.pending_reqs));
        hid_db.cur_req = cur_req_node->req;
        send_now = true;
    }
    os_mutex_give(hid_db.mutex);

    if (send_now)
    {
        hid_driver_data_send(NULL, hid_db.cur_req->data.buf, hid_db.cur_req->data.len,
                             usb_hid_data_send_complete);
    }
}

static void usb_hid_data_send_complete(void *param)
{
    uint32_t actual_len = *((uint32_t *)param);

    if (hid_db.cur_req->cb != NULL)
    {
        hid_db.cur_req->cb(hid_db.handle, actual_len);
    }

    if (hid_db.cur_req->data.buf)
    {
        free(hid_db.cur_req->data.buf);
    }

    free(hid_db.cur_req);

    os_mutex_take(hid_db.mutex, 0xFFFFFFFF);
    hid_db.cur_req = NULL;
    os_mutex_give(hid_db.mutex);

    usb_hid_handle_pending_req();
}

bool usb_hid_data_send(void *handle, void *buf, uint32_t len, void(*cb)())
{
    bool send_now = true;
    T_HID_REQ  *cur_req = NULL;
    T_HID_REQ_NODE *cur_req_node = NULL;

    cur_req_node = (T_HID_REQ_NODE *)malloc(sizeof(T_HID_REQ_NODE));
    if (!cur_req_node)
    {
        return false;
    }

    cur_req = (T_HID_REQ *)malloc(sizeof(T_HID_REQ));
    if (!cur_req)
    {
        free(cur_req_node);
        return false;
    }

    cur_req_node->req = cur_req;

    cur_req->data.buf = malloc(len);
    if (!(cur_req->data.buf))
    {
        free(cur_req);
        free(cur_req_node);
        return false;
    }

    memcpy(cur_req->data.buf, buf, len);
    cur_req->data.len = len;
    cur_req->cb = cb;

    os_mutex_take(hid_db.mutex, 0xFFFFFFFF);
    if (hid_db.pending_reqs.count > 0 || hid_db.cur_req != NULL)
    {
        os_queue_in(&(hid_db.pending_reqs), cur_req_node);
        send_now = false;
    }
    else
    {
        hid_db.cur_req = cur_req;
        send_now = true;

    }
    os_mutex_give(hid_db.mutex);

    if (send_now)
    {
        free(cur_req_node);
        hid_driver_data_send(NULL, cur_req->data.buf, cur_req->data.len, usb_hid_data_send_complete);
    }

    return true;
}

void usb_hid_init(void)
{
    hid_driver_report_desc_register((void *)report_descs, sizeof(report_descs));

    memset(&hid_db, 0, sizeof(hid_db));
    os_queue_init(&(hid_db.pending_reqs));
    os_mutex_create(&(hid_db.mutex));
}
