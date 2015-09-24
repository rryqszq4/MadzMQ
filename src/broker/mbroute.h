#ifndef _MBROUTE_H_INCLUDED_
#define _MBROUTER_H_INCLUDED_

#include "czmq.h"

typedef struct _mbroute_t mbroute_t;

struct _mbroute_t {
	zctx_t *ctx;
	char *host;
	int port;
	void *socket;
	int64_t recv_hits;
	int64_t recv_misses;
};

mbroute_t *mbroute_new(zctx_t *ctx, char *host, int port);

void mbroute_destroy(mbroute_t *this);

int mbroute_bind(mbroute_t *this);

zmsg_t *mbroute_recv(mbroute_t *this);

int mbroute_send(mbroute_t *this, zmsg_t *msg);

#endif