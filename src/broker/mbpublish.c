#include "mbpublish.h"


mbpublish_t *
mbpublish_new(zctx_t *ctx, char *host, int port)
{
	mbpublish_t *this;
	this = (mbpublish_t *)zmalloc(sizeof(mbpublish_t));
	this->ctx = ctx;
	this->host = host;
	this->port = port;
	this->socket = zsocket_new(this->ctx, ZMQ_PUB);
	this->send_hits;
	this->send_misses;

	return this;
}

void 
mbpublish_destroy(mbpublish_t *this)
{
	if (this){
		free(this);
		this = NULL;
	}
}

void 
mbpublish_set_topic(mbpublish_t *this, char *topic)
{
	zsocket_set_subscribe(this->socket, topic);
}

int 
mbpublish_bind(mbpublish_t *this)
{
	return zsocket_bind(this->socket, "tcp://127.0.0.1:%d", this->port);
}

kvmsg_t *
mbpublish_recv(mbpublish_t *this)
{
	kvmsg_t *kvmsg = kvmsg_recv(this->socket);
	return kvmsg;
}

int 
mbpublish_send(mbpublish_t *this, kvmsg_t *kvmsg)
{
	int rc = 0;
	rc = kvmsg_send(kvmsg, this->publish);
	return rc;
}









