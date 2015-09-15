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

int
main (void)
{
	mad_broker_t *self = (mad_broker_t *) zmalloc(sizeof(mad_broker_t));

	self->port = 5556;
	self->ctx = zctx_new();
	self->kvmap = zhash_new();
	self->loop = zloop_new();
	zloop_set_verbose(self->loop, false);

	self->snapshot = zsocket_new(self->ctx, ZMQ_ROUTER);
	zsocket_bind(self->snapshot, "tcp://127.0.0.1:%d", self->port);
	self->publisher = zsocket_new(self->ctx, ZMQ_PUB);
	zsocket_bind(self->publisher, "tcp://127.0.0.1:%d", self->port + 1);
	self->collector = zsocket_new(self->ctx, ZMQ_PULL);
	zsocket_bind(self->collector, "tcp://127.0.0.1:%d", self->port + 2);

	zmq_pollitem_t poller = {0, 0, ZMQ_POLLIN};
	poller.socket = self->snapshot;
	zloop_poller(self->loop, &poller, m_snapshots, self);
	poller.socket = self->collector;
	zloop_poller(self->loop, &poller, m_collector, self);
	zloop_time(self->loop, 1000, 0, m_flush_ttl, self);

	zloop_start(self->loop);

	zloop_destroy(&self->loop);
	zhash_destroy(&self->kvmap);
	zctx_destroy(&self->ctx);
	free(self);
	return 0;
}



