#include "kvmsg.h"

static int m_snapshots(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_collector(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_flush_ttl(zloop_t *loop, int timer_id, void *args);

typedef struct {
	zctx_t *ctx;
	zhash_t *kvmap;
	zloop_t *loop;
	int port;
	int64_t sequence;
	void *snapshot;
	void *publisher;
	void *collector;
} mad_broker_t;



