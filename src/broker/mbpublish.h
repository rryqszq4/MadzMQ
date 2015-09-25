#ifndef _MBPUBLISH_H_INCLUDED_
#define _MBPUBLISH_H_INCLUDED_

#include "czmq.h"
#include "../protocols/kvmsg.h"

typedef struct _mbpublish mbpublish_t;

struct _mbpublish {
	zctx_t *ctx;
	char *host;
	int port;
	void *socket;
	kvmsg_t *kvmsg;
	int64_t send_hits;
	int64_t send_misses;
};

mbpublish_t *mbpublish_new(zctx_t *ctx, char *host, int port);

void mbpublish_destroy(mbpublish_t *this);

int mbpublish_bind(mbpublish_t *this);

kvmsg_t *mbpublish_recv(mbpublish_t *this); 

int mbpublish_send(mbpublish_t *this, kvmsg_t *kvmsg);

#endif