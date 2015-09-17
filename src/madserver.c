#include "protocols/kvmsg.h"
#define SUBTREE "/madserver/"

int main (void)
{
	zctx_t *ctx = zctx_new();
	void *snapshot = zsocket_new(ctx, ZMQ_DEALER);
	zsocket_connect(snapshot, "tcp://127.0.0.1:5556");

	void *subscriber = zsocket_new(ctx, ZMQ_SUB);
	zsocket_set_subscribe(subscriber, "");
	zsocket_connect(subscriber, "tcp://127.0.0.1:5557");
	zsocket_set_subscribe(subscriber, SUBTREE);

	zhash_t *kvmap = zhash_new();

	int64_t sequence = 0;
	zstr_sendm(snapshot, "ICANHAZ?");
	zstr_send(snapshot, SUBTREE);
	while (true){
		kvmsg_t *kvmsg = kvmsg_recv(snapshot);
		kvmsg_dump(kvmsg);
		if (!kvmsg)
			break;
		if (streq(kvmsg_key(kvmsg), "KTHXBAI")){
			sequence = kvmsg_sequence(kvmsg);
			printf("I: received snapshot=%d\n", (int)sequence);
			kvmsg_destroy(&kvmsg);
			break;
		}
		kvmsg_store(&kvmsg, kvmap);
	}

	while (!zctx_interrupted) {
        kvmsg_t *kvmsg = kvmsg_recv (subscriber);
        kvmsg_dump(kvmsg);
        if (!kvmsg)
            break;          //  Interrupted
        if (kvmsg_sequence (kvmsg) > sequence) {
            sequence = kvmsg_sequence (kvmsg);
            kvmsg_store (&kvmsg, kvmap);
        }
        else
            kvmsg_destroy (&kvmsg);
    }

	printf("Interrupted\n%d messages in\n", (int)sequence);
	zhash_destroy(&kvmap);
	zctx_destroy(&ctx);
	return 0;
}