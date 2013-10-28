#include "net.h"
#include "cmd.h"
#include "fwd.h"
#include "test.pb.h" 

#include <assert.h>

static void forward(conn* c, connector *cr, msg_head *h, unsigned char *msg, size_t sz)
{
    if (NULL == c->user) {
        merror("no associate user");
        /* close connection */
        /* return; */
    }

    msg_head nh;
    message_head(msg, sz, &nh);
    minfo("client_cb magic:%d len:%d cmd:%d flags:%d", nh.magic, nh.len, nh.cmd, nh.flags);

    if (h->flags & FLAG_HAS_UID) {
        merror("should no uid connection %s", c->addrtext);
        /* close connection */
        return;
    }

    sz += sizeof(uint64_t);

    if (cr->state != STATE_CONNECTED || !cr->c || !cr->c->bev) {
        merror("forward connection %s's cmd:%d failed!", c->addrtext, h->cmd);
        return;
    }

    evbuffer *output = bufferevent_get_output(cr->c->bev);
    if (!output) {
        merror("forward connection %s's cmd:%d failed, bufferevent_get_output!", c->addrtext, h->cmd);
        return;
    }

    unsigned char *nmsg = (unsigned char *)malloc(sz);
    memcpy(nmsg, msg, sz);
    message_head(nmsg, sz, &nh);
    minfo("client_cb magic:%d len:%d cmd:%d flags:%d", nh.magic, nh.len, nh.cmd, nh.flags);
    evbuffer_add(output, nmsg, sz);
    bufferevent_enable(cr->c->bev, EV_WRITE);

    /*
    evbuffer_lock(output);
    struct evbuffer_iovec v[1];
    if (0 >= evbuffer_reserve_space(output, sz, v, 1)) {
        evbuffer_unlock(output);
        merror("forward connection %s's cmd:%d failed, evbuffer_reserve_space!", c->addrtext, h->cmd);
        return;
    }

    unsigned short *cur = (unsigned short *)(v[0].iov_base);
    assert(cur);
    *cur++ = htons(h->magic);
    *cur++ = htons(h->len);
    *cur++ = htons(h->cmd);
    *cur++ = htons(h->flags);
    *(uint64_t *)cur = htons(1);
    cur += sizeof(uint64_t);
    memcpy(cur, msg - MSG_HEAD_SIZE, h->len);

    if (0 > evbuffer_commit_space(output, v, 1)) {
        evbuffer_unlock(output);
        merror("forward connection %s's cmd:%d failed, evbuffer_add!", c->addrtext, h->cmd);
        return;
    }
    evbuffer_unlock(output);

    bufferevent_enable(cr->c->bev, EV_WRITE);
    */
    mdebug("forward cmd:%d for connection %s", h->cmd, c->addrtext);
}

static void cli_cb(conn *c, msg_head *h, unsigned char *msg, size_t sz)
{

}

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        /* close connection */
        return;
    }
    minfo("client_cb magic:%d len:%d cmd:%d flags:%d", h.magic, h.len, h.cmd, h.flags);

    if (h.cmd >= CG_BEGIN && h.cmd < CS_END) {
        /* client -> gate */
        if (h.cmd >= CG_BEGIN && h.cmd < CG_END) {
            cli_cb(c, &h, msg, sz);
        /* client -> center */
        } else if (h.cmd >= CE_BEGIN && h.cmd < CE_END) {
            forward(c, center, &h, msg, sz);
        /* client -> cache */
        } else if (h.cmd >= CA_BEGIN && h.cmd < CA_END) {
            forward(c, cache, &h, msg, sz);
        /* client -> game */
        } else if (h.cmd >= CM_BEGIN && h.cmd < CM_END) {
            forward(c, cache, &h, msg, sz);
        } else {
            merror("invalid cmd:%d connection %s", h.cmd, c->addrtext);
            /* close connection */
        }
    } else {
        merror("invalid cmd:%d connection %s", h.cmd, c->addrtext);
        /* close connection */
    }
}

void client_connect_cb(conn *c)
{

}

void client_disconnect_cb(conn *c)
{

}
