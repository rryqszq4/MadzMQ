#include "czmq.h"
#include "protocols/mdp.h"

typedef struct _mad_client_t mad_client_t;

struct _mad_client_t{
	zctx_t *ctx;
	char *broker;
	void *client;
	int verbose;
	int timeout;
};

static void m_connect_to_broker(mad_client_t *self);

mad_client_t *
mad_client_new(char *broker, int verbose)
{
	assert(broker);

	mad_client_t *self = (mad_client_t *)zmalloc(sizeof(mad_client_t));
	self->ctx = zctx_new();
	self->broker = strdup(broker);
	self->verbose = verbose;
	self->timeout = 2500;

	m_connect_to_broker(self);

	return self;
}

void 
mad_client_destroy(mad_client_t **self_p)
{
	assert(self_p);
	if (*self_p){
		mad_client_t *self = *self_p;
		zctx_destroy(&self->ctx);
		free(self->broker);
		free(self);
		*self_p = NULL;
	}
}












