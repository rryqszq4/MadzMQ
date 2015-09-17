#include <uuid/uuid.h>
#include "aio/bstar.h"
#include "protocols/kvmsg.h"

static int m_client(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_snapshots(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_collector(zloop_t *loop, zmq_pollitem_t *poller, void *args);
static int m_flush_ttl(zloop_t *loop, int timer_id, void *args);
static int m_send_hugz(zloop_t *loop, int timer_id, void *args);
static int m_publish_message(zloop_t *loop, int timer_id, void *args);
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
	void *client;
	zlist_t *pending;
	bool primary;
	bool active;
	bool passive;
} mad_broker_t;

int
main (int argc, char *argv[])
{
	mad_broker_t *self = (mad_broker_t *) zmalloc(sizeof(mad_broker_t));

	if (argc == 2 && streq(argv[1], "-p")){
		zclock_log("I: primary active, waiting for backup (passive)");
		self->bstar = bstar_new(BSTAR_PRIMARY, "tcp://127.0.0.1:5003", "tcp://127.0.0.1:5004");
		bstar_voter(self->bstar, "tcp://127.0.0.1:5556", ZMQ_ROUTER, m_snapshots, self);
		self->port = 5556;
		self->peer = 5566;
		self->primary = true;
	}else if (argc == 2 && streq(argv[1],"-b")){
		zclock_log("I: backup passive, waiting for primary (active)");
		self->bstar = bstar_new(BSTAR_BACKUP, "tcp://127.0.0.1:5004", "tcp://127.0.0.1:5003");
		bstar_voter(self->bstar, "tcp://127.0.0.1:5566", ZMQ_ROUTER, m_snapshots, self);
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
	bstar_set_verbose(self->bstar, false);

	self->publisher = zsocket_new(self->ctx, ZMQ_PUB);
	self->collector = zsocket_new(self->ctx, ZMQ_SUB);
	self->client = zsocket_new(self->ctx, ZMQ_ROUTER);

	zsocket_set_subscribe(self->collector, "");
	zsocket_bind(self->publisher, "tcp://127.0.0.1:%d", self->port + 1);
	zsocket_bind(self->collector, "tcp://127.0.0.1:%d", self->port + 2);
	zsocket_bind(self->client, "tcp://127.0.0.1:%d", 5555);

	self->subscriber = zsocket_new(self->ctx, ZMQ_SUB);
	zsocket_set_subscribe(self->subscriber, "");
	zsocket_connect(self->subscriber, "tcp://127.0.0.1:%d", self->peer + 1);

	bstar_new_active(self->bstar, m_new_active, self);
	bstar_new_passive(self->bstar, m_new_passive, self);

	zmq_pollitem_t poller = {self->collector, 0, ZMQ_POLLIN};
	zloop_poller(bstar_zloop(self->bstar), &poller, m_collector, self);
	zmq_pollitem_t client_poller = {self->client, 0, ZMQ_POLLIN};
	zloop_poller(bstar_zloop(self->bstar), &client_poller, m_client, self);
	zloop_timer(bstar_zloop(self->bstar), 1000, 0, m_flush_ttl, self);
	zloop_timer(bstar_zloop(self->bstar), 1000, 0, m_send_hugz, self);
	//zloop_timer(bstar_zloop(self->bstar), 1000, 0, m_publish_message, self);

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
m_client(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	zmsg_t *msg = zmsg_recv(self->client);
	if (!msg)
		return 1;
	zclock_log("I: received client message:");
	zmsg_dump(msg);

	char *body_1 = zmsg_popstr(msg);
	//zmsg_dump(msg);
	char *body_2 = zmsg_popstr(msg);
	//zmsg_dump(msg);
	char *body_3 = zmsg_popstr(msg);
	//zmsg_dump(msg);
	char *body_4 = zmsg_popstr(msg);
	//zmsg_dump(msg);
	char *body_5 = zmsg_popstr(msg);
	//zmsg_dump(msg);
	//printf("%s %s %s %s %s\n", body_1, body_2, body_3, body_4, body_5);
	kvmsg_t *kvmsg = kvmsg_new(++self->sequence);
	kvmsg_fmt_key(kvmsg, "%s", body_4);
	kvmsg_fmt_body(kvmsg, "%s", body_5);
	kvmsg_set_prop(kvmsg, "msg", body_5);
	kvmsg_send(kvmsg, self->publisher);
	kvmsg_destroy(&kvmsg);

	return 0;
}

static int
m_send_single(const char *key, void *data, void *args)
{
	kvroute_t *kvroute = (kvroute_t *)args;
	kvmsg_t *kvmsg = (kvmsg_t *)data;
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
	zframe_print(identity, "");
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
m_was_pending(mad_broker_t *self, kvmsg_t *kvmsg)
{
	kvmsg_t *held = (kvmsg_t *)zlist_first(self->pending);
	while (held){
		if (memcmp(kvmsg_uuid(kvmsg), kvmsg_uuid(held), sizeof(uuid_t)) == 0){
			zlist_remove(self->pending, held);
			return true;
		}
		held = (kvmsg_t *)zlist_next(self->pending);
	}
	return false;
}

static int 
m_collector(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	kvmsg_t *kvmsg = kvmsg_recv(poller->socket);
	if (kvmsg){
		if (self->active){
			kvmsg_set_sequence(kvmsg, ++self->sequence);
			kvmsg_send(kvmsg, self->publisher);
			int ttl = atoi(kvmsg_get_prop(kvmsg, "ttl"));
			if (ttl)
				kvmsg_set_prop(kvmsg, "ttl", "%" PRId64, zclock_time() + ttl * 1000);
			kvmsg_store(&kvmsg, self->kvmap);
			zclock_log("I: publishing update=%d", (int)self->sequence);
		}else {
			if (m_was_pending(self, kvmsg))
				kvmsg_destroy(&kvmsg);
			else 
				zlist_append(self->pending, kvmsg);
		}
	}
	return 0;
}

static int 
m_flush_single(const char *key, void *data, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	kvmsg_t *kvmsg = (kvmsg_t *)data;
	int64_t ttl;
	sscanf(kvmsg_get_prop(kvmsg, "ttl"), "%" PRId64, &ttl);
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

static int 
m_send_hugz(zloop_t *loop, int timer_id, void *args)
{
	mad_broker_t *self = (mad_broker_t *) args;

	kvmsg_t *kvmsg = kvmsg_new(self->sequence);
	kvmsg_set_key(kvmsg, "HUGZ");
	kvmsg_set_body(kvmsg, (byte *)"", 0);
	kvmsg_send(kvmsg, self->publisher);
	kvmsg_destroy(&kvmsg);

	return 0;
}

static int 
m_publish_message(zloop_t *loop, int timer_id, void *args)
{
	mad_broker_t *self = (mad_broker_t *) args;
	kvmsg_t *kvmsg = kvmsg_new(++self->sequence);
	kvmsg_fmt_key(kvmsg, "%d", randof(10000));
	kvmsg_fmt_body(kvmsg, "%d", randof(1000000));
	kvmsg_send(kvmsg, self->publisher);
	kvmsg_destroy(&kvmsg);

	return 0;
}

static int 
m_new_active(zloop_t *loop, zmq_pollitem_t *unused, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	self->active = true;
	self->passive = false;

	zmq_pollitem_t poller = {self->subscriber, 0, ZMQ_POLLIN};
	zloop_poller_end(bstar_zloop(self->bstar), &poller);

	while (zlist_size(self->pending)){
		kvmsg_t *kvmsg = (kvmsg_t *)zlist_pop(self->pending);
		kvmsg_set_sequence(kvmsg, ++self->sequence);
		kvmsg_send(kvmsg, self->publisher);
		kvmsg_store(&kvmsg, self->kvmap);
		zclock_log("I: publishing pending=%d", (int)self->sequence);
	}
	return 0;
}

static int 
m_new_passive(zloop_t *loop, zmq_pollitem_t *unused, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	zhash_destroy(&self->kvmap);
	self->active = false;
	self->passive = true;

	zmq_pollitem_t poller = {self->subscriber, 0, ZMQ_POLLIN};
	zloop_poller(bstar_zloop(self->bstar), &poller, m_subscriber, self);

	return 0;
}

static int 
m_subscriber(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mad_broker_t *self = (mad_broker_t *)args;

	if (self->kvmap == NULL){
		self->kvmap = zhash_new();
		void *snapshot = zsocket_new(self->ctx, ZMQ_DEALER);
		zsocket_connect(snapshot, "tcp://127.0.0.1:%d", self->peer);
		zclock_log("I: asking for snapshot from: tcp://127.0.0.1:%d", self->peer);
		zstr_sendm(snapshot, "ICANHAZ?");
		zstr_send(snapshot, "");
		while (true){
			kvmsg_t *kvmsg = kvmsg_recv(snapshot);
			if (!kvmsg)
				break;
			if (streq(kvmsg_key(kvmsg), "KTHXBAI")){
				self->sequence = kvmsg_sequence(kvmsg);
				kvmsg_destroy(&kvmsg);
				break;
			}
			kvmsg_store(&kvmsg, self->kvmap);
		}
		zclock_log("I: received snapshot=%d", (int)self->sequence);
		zsocket_destroy(self->ctx, snapshot);
	}

	kvmsg_t *kvmsg = kvmsg_recv(poller->socket);
	if (!kvmsg)
		return 0;

	if (streq(kvmsg_key(kvmsg), "HUGZ")){
		if (!m_was_pending(self, kvmsg)){
			zlist_append(self->pending, kvmsg_dup(kvmsg));
		}
		if (kvmsg_sequence(kvmsg) > self->sequence){
			self->sequence = kvmsg_sequence(kvmsg);
			kvmsg_store(&kvmsg, self->kvmap);
			zclock_log("I: received update=%d", (int)self->sequence);
		}else {
			kvmsg_destroy(&kvmsg);
		}
	}else {
		kvmsg_destroy(&kvmsg);
	}

	return 0;
}













