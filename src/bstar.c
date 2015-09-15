#include "bstar.h"

typedef enum {
	STATE_PRIMARY = 1,
	STATE_BACKUP = 2,
	STATE_ACTIVE = 3,
	STATE_PASSIVE = 4
} state_t;

typedef enum {
	PEER_PRIMARY = 1,
	PEER_BACKUP = 2,
	PEER_ACTIVE = 3,
	PEER_PASSIVE = 4,
	CLIENT_REQUEST = 5
} event_t;

struct _bstar_t {
	zctx_t *ctx;
	zloop_t *loop;
	void *statepub;
	void *statesub;
	state_t state;
	event_t event;
	int64_t peer_expiry;
	zloop_fn *voter_fn;
	void *voter_arg;
	zloop_fn *active_fn;
	void *active_arg;
	zloop_fn *passive_fn;
	void *passive_arg;
};

#define BSTAR_HEARTBEAT 1000

bstar_t *
bstar_new(int primary, char *local, char *remote)
{
	bstar_t *self;

	self = (bstar_t *)zmalloc(sizeof(bstar_t));

	self->ctx = zctx_new();
	self->loop = zloop_new();
	self->state = primary ? STATE_PRIMARY : STATE_BACKUP;

	self->statepub = zsocket_new(self->ctx, ZMQ_PUB);
	zsocket_bind(self->statepub, local);

	self->statesub = zsocket_new(self->ctx, ZMQ_SUB);
	zsocket_set_subscribe(self->statesub, "");
	zsocket_connect(self->statesub, remote);

	zloop_timer(self->loop, BSTAR_HEARTBEAT, 0, s_send_state, self);
	zmq_pollitem_t poller = {self->statesub, 0, ZMQ_POLLIN};
	zloop_poller(self->loop, &poller, s_recv_state, self);
	return self;
}

void 
bstar_destroy(bstar_t **self_p)
{
	assert(self_p);
	if (*self_p){
		bstar_t *self = *self_p;
		zloop_destroy(&self->loop);
		zctx_destroy(&self->ctx);
		free(self);
		*self_p = NULL;
	}
}

zloop_t *
bstar_zloop(bstar_t *self)
{
	return self->loop;
}

int 
bstar_vote(bstar_t *self, char *endpoint, int type, zloop_fn handler, void *arg)
{
	void *socket = zsocket_new(self->ctx, type);
	zsocket_bind(socket, endpoint);
	assert(!self->voter_fn);
	self->voter_fn = handler;
	self->voter_arg = arg;
	zmq_pollitem_t poller = {socket, 0, ZMQ_POLLIN};
	return zloop_poller(self->loop, &poller, s_voter_ready, self);
}

void 
bstar_new_active(bstar_t *self, zloop_fn handler, void *arg)
{
	assert(!self->active_fn);
	self->active_fn = handler;
	self->active_arg = arg;
}

void 
bstar_new_passive(bstar_t *self, zloop_fn handler, void *arg)
{
	assert(!self->passive_fn);
	self->passive_fn = handler;
	self->passive_arg = arg;
}

void 
bstar_set_verbose(bstar_t *self, bool verbose)
{
	zloop_set_verbose(self->loop, verbose);
}

int
bstar_start(bstar_t *self)
{
	assert(self->voter_fn);
	s_update_peer_expiry(self);
	return zloop_start(self->loop);
}














