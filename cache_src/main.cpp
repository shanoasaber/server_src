
#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define WORKER_NUM 8

static void signal_cb(evutil_socket_t, short, void *);

/* gate_cb */
static user_callback gate_cb;
void gate_cb_init(user_callback *cb);

/* game_cb */
static user_callback game_cb;
void game_cb_init(user_callback *cb);

int main(int argc, char **argv)
{
    /* open log */
    if (0 != LOG_OPEN("./center", LOG_LEVEL_DEBUG, -1)) {
        fprintf(stderr, "open center log failed!\n");
        return 1;
    }

    if (0 != check_cmd()) {
        return 1;
    }

    /* protobuf verify version */
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    /* net init */
    if (0 > net_init()) {
        mfatal("net_init failed!");
        return 1;
    }

    struct event_base *main_base = event_base_new();
    if (NULL == main_base) {
        mfatal("main_base = event_base_new() failed!");
        return 1;
    }

    conn_init();

    /* thread */
    pthread_t worker[WORKER_NUM];
    thread_init(main_base, WORKER_NUM, worker);

    /* signal */
    struct event *signal_event;
    signal_event = evsignal_new(main_base, SIGINT, signal_cb, (void *)main_base);
    if (NULL == signal_event || 0 != event_add(signal_event, NULL)) {
        mfatal("create/add a signal event failed!");
        return 1;
    }

    /* listener for gate */
    struct sockaddr_in sa;
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(45000);

    listener *lg = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa), &gate_cb);
    if (NULL == lg) {
        mfatal("create client listener failed!");
        return 1;
    }

    /* listener for game */
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(45001);

    listener *lm = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa), &game_cb);
    if (NULL == lm) {
        mfatal("create client listener failed!");
        return 1;
    }
    event_base_dispatch(main_base);

    for (int i = 0; i < WORKER_NUM; i++)
        pthread_join(worker[i], NULL);

    listener_free(lm);
    listener_free(lg);
    event_free(signal_event);
    event_base_free(main_base);

    /* shutdown protobuf */
    google::protobuf::ShutdownProtobufLibrary();

    /* close log */
    LOG_CLOSE();

    return 0;
}

void signal_cb(evutil_socket_t fd, short what, void *arg)
{
    mdebug("signal_cb");
    struct event_base *base = (struct event_base *)arg;
    event_base_loopbreak(base);
    dispatch_conn_new(-1, 'k', NULL);
}
