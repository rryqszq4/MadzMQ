#include "bstar.h"
#include "kvmsg.h"


static int m_snapshots(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_collector(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_flush_ttl(zloop_t *loop, int timer_id, void *args);
static int m_send_hugz(zloop_t *loop, int timer_id, void *args);
static int m_new_active(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_new_passive(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_subscriber(zloop_t *loop, zmq_pollitem_t *poller, void *args);

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

typedef struct {
	void *socket;
	zframe_t *identity;
	char *subtree;
} kvroute_t;

static int
m_send_single(const char *key, void *data, void *args)
{
	kvroute_t *kvroute = (kvroute_t *)args;
	kvmsg_t kvmsg = (kvmsg_t *)data;
	if (strlen(kvroute->subtree) <= strlen(kvmsg_key(kvmsg)) 
		&& memcmp(kvroute->subtree, kvmsg_key(kvmsg), strlen(kvroute->subtree)) == 0){
		zframe_send(&kvroute->identity, kvroute->socket, ZFRAME_MORE + ZFRAME_REUSE);
		kvmsg_send(kvmsg, kvroute->socket);
	}
	return 0;
}

static int
m_snapshots(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	zframe_t *identity = zframe_recv(poller->socket);
	if (identity){
		char *request = zstr_recv(poller->socket);
		char *subtree = NULL;
		if (streq(request, "ICANHAZ?")){
			free(request);
			subtree = zstr_recv(poller->socket);
		}else {
			printf("E: bad request, aborting\n");
		}

		if (subtree){
			kvroute_t routing = {poller->socket, identity, subtree};
			zhash_foreach(self->kvmap, m_send_single, &routing);

			zclock_log("I: sending snapshot=%d", (int)self->sequence);
			zframe_send(&identity, poller->socket, ZFRAME_MORE);
			kvmsg_t *kvmsg = kvmsg_new(self->sequence);
			kvmsg_set_key(kvmsg, "KTHXBAI");
			kvmsg_set_body(kvmsg, (byte *)subtree, 0);
			kvmsg_send(kvmsg, poller->socket);
			kvmsg_destroy(&kvmsg);
			free(subtree);
		}
		zframe_destroy(&identity);
	}
	return 0;
}

static int 
m_collector(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;
	if (kvmsg){
		kvmsg_set_sequence(kvmsg, ++self->sequence);
		kvmsg_send(kvmsg, self->publisher);
		int ttl = atoi(kvmsg_get_prop(kvmsg, "ttl"));
		if (ttl)
			kvmsg_set_prop(kvmsg, "ttl", "%" PRID64, zclock_time() + ttl * 1000);
		kvmsg_store(&kvmsg, self->kvmap);
		zclock_log("I: publishing update=%d", (int)self->sequence);
	}
	return 0;
}

static int 
m_flush_single(const char *key, void *data, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	kvmsg_t *kvmsg = (kvmsg_t *)data;
	int64_t ttl;
	sscanf(kvmsg_get_prop(kvmsg, "ttl"), "%" PRID64, &ttl);
	if (ttl && zclock_time() >= ttl){
		kvmsg_set_sequence(kvmsg, ++self->sequence);
		kvmsg_set_body(kvmsg, (byte *) "", 0);
		kvmsg_send(kvmsg, self->publisher);
		kvmsg_store(&kvmsg, self->kvmap);
		zclock_log("I: publishing delete=%d", (int)self->sequence);
	}
	return 0;
}

static int
m_flush_ttl(zloop_t *loop, int timer_id, void *args)
{
	mad_broker_t * self = (mad_broker_t *) args;
	if (self->kvmap)
		zhash_foreach(self->kvmap, m_flush_single, args);
	return 0;
}














