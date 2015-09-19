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