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

	void *publisher = zsocket_new(ctx, ZMQ_PUSH);
	zsocket_connect(publisher, "tcp://127.0.0.1:5558");

	zhash_t *kvmap = zhash_new();
	srandom((unsigned)time(NULL));

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

	int64_t alarm = zclock_time() + 1000;

	while (!zctx_interrupted) {
		/*zmq_pollitem_t items[] = {
			{subscriber, 0, ZMQ_POLLIN, 0}
		};
		int tickless = (int)((alarm - zclock_time()));
		if (tickless < 0)
			tickless = 0;
		int rc = zmq_poll(items, 1, tickless * ZMQ_POLL_MSEC);
		if (rc == -1)
			break;

		if (items[0].revents & ZMQ_POLLIN){
			kvmsg_t *kvmsg = kvmsg_recv(subscriber);
			kvmsg_dump(kvmsg);
			if (!kvmsg)
				break;

			if (kvmsg_sequence (kvmsg) > sequence) {
            	sequence = kvmsg_sequence (kvmsg);
            	kvmsg_store (&kvmsg, kvmap);
            	printf("I: received update=%d\n", (int)sequence);
        	}else {
        		kvmsg_destroy (&kvmsg);
        	}			
		}

        if (zclock_time () >= alarm) {
            kvmsg_t *kvmsg = kvmsg_new (0);
            kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, randof (10000));
            kvmsg_fmt_body (kvmsg, "%d", randof (1000000));
            kvmsg_set_prop (kvmsg, "ttl", "%d", randof (30));
            kvmsg_send     (kvmsg, publisher);
            kvmsg_destroy (&kvmsg);
            alarm = zclock_time () + 1000;
        }*/
        kvmsg_t *kvmsg = kvmsg_recv (subscriber);
        kvmsg_dump(kvmsg);
        if (!kvmsg)
            break;          //  Interrupted
        if (kvmsg_sequence (kvmsg) > sequence) {
            sequence = kvmsg_sequence (kvmsg);
            kvmsg_store (&kvmsg, kvmap);
        }
        else
        	kvmsg_destroy(&kvmsg);
    }

	printf("Interrupted\n%d messages in\n", (int)sequence);
	zhash_destroy(&kvmap);
	zctx_destroy(&ctx);
	return 0;
}