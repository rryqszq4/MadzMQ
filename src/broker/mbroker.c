
#include "mbroker.h"

mbroker_t *
mbroker_new()
{
	mbroker_t *this;
	this = (mbroker_t *) zmalloc(sizeof(mbroker_t));

	this->verbose = true;
	this->ctx = zctx_new();
	this->kvmap = zhash_new();
	this->loop = zloop_new();
	this->sequence = 1;

	//与客户端通信对象
	this->client_route = mbroute_new(this->ctx, MBROUTE_CLIENT_HOST, MBROUTE_CLIENT_PORT);
	//快照对象
	this->snapshot = mbroute_new(this->ctx, SNAPSHOT_HOST, SNAPSHOT_PORT);
	//发布者对象
	this->publish = mbpublish_new(this->ctx, MBPUBLISH_HOST, MBPUBLISH_PORT);
	
	mbroute_bind(this->client_route);

	mbroute_bind(this->snapshot);

	int port = mbpublish_bind(this->publish);
	//printf("socket:%p\n", this->publish->socket);
	//printf("port:%d\n", port);
	return this;
}

void
mbroker_loop_poller(mbroker_t *this, zloop_fn handler, int mode)
{
	zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN};
	if (mode == CLIENT_ROUTE_MODE)
		poller.socket = this->client_route->socket;
	else if (mode == SNAPSHOT_MODE)
		poller.socket = this->snapshot->socket;
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
		mbroute_destroy(this->snapshot);
		mbpublish_destroy(this->publish);
		zloop_destroy(&this->loop);
		zhash_destroy(&this->kvmap);
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

int 
publish_send_handle(zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
	mbroker_t *this = (mbroker_t *)args;
	int rc = 0;


	kvmsg_t *kvmsg = kvmsg_new(1);
	kvmsg_set_key(kvmsg, "key");
	rc = mbpublish_send(this->publish, kvmsg);
	kvmsg_dump(kvmsg);
	return rc;
}

int
s_send_single(const char *key, void *data, void *args)
{
	kvroute_t *kvroute = (kvroute_t *)args;
	kvmsg_t *kvmsg = (kvmsg_t *)data;
	if (strlen(kvroute->subtree) <= strlen(kvmsg_key(kvmsg)) && memcmp(kvroute->subtree, kvmsg_key(kvmsg), strlen(kvroute->subtree)) == 0)
	{
		zframe_send(&kvroute->identity, kvroute->socket, ZFRAME_MORE + ZFRAME_REUSE);
		kvmsg_send(kvmsg, kvroute->socket);
	}
	return 0;
}

int 
snapshot_handle(zloop_t *loop, zmq_pollitem_t *pooler, void *args)
{

	mbroker_t *this = (mbroker_t *)args;
	zframe_t *identity = zframe_recv(this->snapshot->socket);
	zframe_print(identity, "");

	if (identity){
		char *request = zstr_recv(this->snapshot->socket);
		printf("%s\n", request);
		char *subtree = NULL;
		if (streq(request, "ICANHAZ?")){
			free(request);
			subtree = zstr_recv(this->snapshot->socket);
		}else {
			printf("E: ban request, aborting\n");
		}

		if (subtree){
			kvroute_t routing = {this->snapshot->socket, identity, subtree};
			zhash_foreach(this->kvmap, s_send_single, &routing);

			//现在发送带有序号的END消息
			zclock_log("I: sending snapshot=%d", (int)this->sequence);
			zframe_send(&identity, this->snapshot->socket, ZFRAME_MORE);

			kvmsg_t *kvmsg = kvmsg_new(this->sequence);
			kvmsg_set_key(kvmsg, "KTHXBAI");
			kvmsg_set_body(kvmsg, (byte *)"body", 4);
			//kvmsg_dump();
			kvmsg_send(kvmsg, this->snapshot->socket);
			kvmsg_destroy(&kvmsg);
			free(subtree);
		}
		zframe_destroy(&identity);
	}
	return 0;
}




















