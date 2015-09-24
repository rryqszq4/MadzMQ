
#include "mbroker.h"

mbroker_t *
mbroker_new()
{
	mbroker_t *this;
	this = (mbroker_t *) zmalloc(sizeof(mbroker_t));

	this->verbose = true;
	this->ctx = zctx_new();
	this->loop = zloop_new();
	this->client_route = mbroute_new(this->ctx, MBROUTE_CLIENT_HOST, MBROUTE_CLIENT_PORT);
	
	int port = mbroute_bind(this->client_route);
	//printf("port:%d\n", port);
	return this;
}

void
mbroker_loop_poller(mbroker_t *this, zloop_fn handler)
{
	zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN};
	poller.socket = this->client_route->socket;
	zloop_poller(this->loop, &poller, handler, this);
}

void 
mbroker_loop_timer(mbroker_t *this, size_t delay, size_t time,  zloop_timer_fn handler)
{
	zloop_timer(this->loop, delay, time, handler, this);
}

void
mbroker_looper(mbroker_t *this)
{
	zloop_start(this->loop);
}

void
mbroker_destroy(mbroker_t **this_p)
{
	assert(this_p);
	if (*this_p){
		mbroker_t *this = *this_p;
		mbroute_destroy(this->client_route);
		zloop_destroy(&this->loop);
		zctx_destroy(&this->ctx);
		free(this);
		*this_p = NULL;
	}
}

int
client_route_recv_handle(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mbroker_t *this = (mbroker_t *)args;
	zmsg_t *msg = mbroute_recv(this->client_route);

	zmsg_dump(msg);

	return 0;
}

