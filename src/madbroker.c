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
	bstar_t *bstar;
	zloop_t *loop;
	int port;
	int peer;
	int64_t sequence;
	void *snapshot;
	void *publisher;
	void *collector;
	void *subscriber;
	zlist_t *pending;
	bool primary;
	bool active;
	bool passive;
} mad_broker_t;

int
main (void)
{
	mad_broker_t *self = (mad_broker_t *) zmalloc(sizeof(mad_broker_t));

	if (argc == 2 && streq(argv[1], "-p")){
		zclock_log("I: primary active, waiting for backup (passive)");
		self->bstar = bstar_new(BSTAR_PRIMARY, "tcp://127.0.0.1:5003", "tcp://127.0.0.1:5004");
		bstar_vote(self->bstar, "tcp://127.0.0.1:5556", ZMQ_ROUTER, m_snapshots, self);
		self->port = 5556;
		self->peer = 5566;
		self->primary = true;
	}else if (argc == 2 && streq(argv[1],"-b")){
		zclock_log("I: backup passive, waiting for primary (active)");
		self->bstar = bstar_new(BSTAR_BACKUP, "tcp://127.0.0.1:5004", "tcp://127.0.0.1:5003");
		bstar_vote(self->bstar, "tcp://127.0.0.1:5566", ZMQ_ROUTER, m_snapshots, self);
		self->port = 5566;
		self->peer = 5556;
		self->primary = false;
	}else {
		printf("Usage: madbroker {-p | -b}\n");
		free(self);
		exit(0);
	}

	if (self->primary)
		self->kvmap = zhash_new();

	self->ctx = zctx_new();
	self->pending = zlist_new();
	bstar_set_verbose(self->bstar, true);

	self->publisher = zsocket_new(self->ctx, ZMQ_PUB);
	self->collector = zsocket_new(self->ctx, ZMQ_PUB);
	zsocket_set_subscribe(self->collector, "");
	zsocket_bind(self->publisher, "tcp://127.0.0.1:%d", self->port + 1);
	zsocket_bind(self->collector, "tcp://127.0.0.1:%d", self->port + 2);

	self->subscriber = zsocket_new(self->ctx, ZMQ_SUB);
	zsocket_set_subscribe(self->subscriber, "");
	zsocket_connect(self->subscriber, "tcp://127.0.0.1:%d", self->peer + 1);

	bstar_new_active(self->bstar, m_new_active, self);
	bstar_new_passive(self->bstar, m_new_passive, self);

	zmq_pollitem_t poller = {self->collector, 0, ZMQ_POLLIN};
	zloop_poller(bstar_zloop(self->star), &poller, m_collector, self);
	zloop_timer(bstar_zloop(self->star), 1000, 0, m_flush_ttl, self);
	zloop_timer(bstar_zloop(self->star), 1000, 0, m_send_hugz, self);

	bstar_start(self->bstar);

	while (zlist_size(self->pending)){
		kvmsg_t *kvmsg = (kvmsg_t *)zlist_pop(self->pending);
		kvmsg_destroy(&kvmsg);
	}

	zlist_destroy(&self->pending);
	bstar_destroy(&self->bstar);
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














