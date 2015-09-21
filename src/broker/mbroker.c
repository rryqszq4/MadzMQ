#include "mbroker.h"

struct _mbroker_t
{
	zctx_t *ctx;
	mbroute_t *client_route;
	mbqueue_t *queue;
	mbpublish_t *publish;
	void *snapshot;
	mbroute_t *service_route;
	void *service;
	mbstats_t *stats;
	zloop_t *loop;
};

mbroker_t *
mbroker_new()
{
	mbroker_t *this;
	this = (mbroker_t) zmalloc(sizeof(mbroker_t));

	this->ctx = zctx_new();
	this->loop = zloop_new();

	return this;
}

void
mbroker_destroy(mbroker_t **this_p)
{
	assert(this_p);
	if (*this_p){
		mbroker_t *this = *this_p;
		zloop_destroy(&this->loop);
		zctx_destroy(&this->ctx);
		free(this);
		*this_p = NULL;
	}
}

