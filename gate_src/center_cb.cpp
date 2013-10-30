#include "net.h"
#include "cmd.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[EG_END - EG_BEGIN];

static void center_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("center_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > EG_BEGIN && h.cmd < EG_END) {
        if (cbs[h.cmd - EG_BEGIN]) {
            (*(cbs[h.cmd - EG_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        return;
    }
}

static void center_connect_cb(conn *c)
{
    mdebug("center_connect_cb");
}

static void center_disconnect_cb(conn *c)
{
    mdebug("center_disconnect_cb");
}

void center_cb_init(user_callback *cb)
{
    cb->rpc = center_rpc_cb;
    cb->connect = center_connect_cb;
    cb->disconnect = center_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (EG_END - EG_BEGIN));
}
