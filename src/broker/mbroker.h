#ifndef _MBROKER_H_INCLUDED_
#define _MBROKER_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "czmq.h"
#include "mbroute.h"
#include "mbqueue.h"

typedef struct _mbroker_t mbroker_t;

struct _mbroker_t
{
	zctx_t *ctx;
	mbroute_t *client_route;
	mbqueue_t *queue;
	//mbpublish_t *publish;
	void *snapshot;
	mbroute_t *service_route;
	void *service;
	//mbstats_t *stats;
	zloop_t *loop;
	bool verbose;
};

#define MBROKER_HOST 			"127.0.0.1"
#define MBROUTE_CLIENT_HOST		"127.0.0.1"
#define MBROUTE_CLIENT_PORT		5555

mbroker_t *mbroker_new();
void mbroker_destroy(mbroker_t **this_p);

void mbroker_loop_poller(mbroker_t *this, zloop_fn handler);
void mbroker_loop_timer(mbroker_t *this, size_t delay, size_t time,  zloop_timer_fn handler);
void mbroker_looper(mbroker_t *this);

int client_route_recv_handle(mbroker_t *this);

#ifdef __cplusplus
}
#endif

#endif