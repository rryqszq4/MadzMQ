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

void
mad_client_set_timeout(mad_client_t *self, int timeout)
{
	assert(self);
	self->timeout = timeout;
}

int
mad_client_send(mad_client_t *self, char *service, zmsg_t **request_p)
{
	assert(self);
	assert(request_p);
	zmsg_t *request = *request_p;

	zmsg_pushstr(request, service);
	zmsg_pushstr(request, MDPC_CLIENT);
	zmsg_pushstr(request, "");
	if (self->verbose){
		zclock_log("I: send request to '%s' service:", service);
		zmsg_dump(request);
	}
	zmsg_send(&request, self->client);
	return 0;
}

static void 
m_connect_to_broker(mad_client_t *self) 
{
	if (self->client)
		zsocket_destroy(self->ctx, self->client);
	self->client = zsocket_new(self->ctx, ZMQ_DEALER);
	zmq_connect(self->client, self->broker);
	if (self->verbose)
		zclock_log("I: connecting to madbroker at %s...", self->broker);
}

int main (int argc, char *argv[])
{
	int verbose = (argc > 1 && streq(argv[1], "-v"));
	mad_client_t *session = mad_client_new("tcp://127.0.0.1:5555", verbose);

	zmsg_t *request = zmsg_new();
	zmsg_pushstr(request, "Hello world");
	mad_client_send(session, "echo", &request);

	printf("send receive\n");
	mad_client_destroy(&session);
	return 0;
}













