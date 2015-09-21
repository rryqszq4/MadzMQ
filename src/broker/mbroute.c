#include "mbroute.h"

struct _mbroute_t {
	zctx_t *ctx;
	char *host;
	int port;
	void *socket;
	int64_t recv_hits;
	int64_t recv_misses;
};

mbroute_t *
mbroute_new(zctx_t *ctx, char *host, port)
{
	mbroute_t *this;
	this = (mbroute_t *) zmalloc(sizeof(mbroute_t));
	this->ctx = ctx;
	this->host = host;
	this->port = port;
	this->socket = zsocket_new(this->ctx, ZMQ_ROUTER);
	this->recv_hits = 0;

	return this;
}

void 
mbroute_destroy(mbroute_t *this)
{
	if (this){
		free(this);
		this = NULL;
	}
}

int
mbroute_bind(mbroute_t *this)
{
	return zsocket_bind(this, "tcp://%s:%d", this->host, this->port);
}

zmsg_t *
mbroute_recv(mbroute_t *this)
{	
	zmsg_t *msg = zmsg_recv(this->socket);
	this->recv_hits++;
	return msg;
}

int
mbroute_send(mbroute_t *this, zmsg_t *msg)
{
	int rc = 0;
	rc = zmsg_send(&msg, this->socket);

	return rc;
}






