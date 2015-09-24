#include <uuid/uuid.h>
#include "protocols/kvmsg.h"
#include "broker/mbroker.h"

static void
s_test()
{
	printf("123\n");
}

int 
main (int argc, char *argv[])
{
	mbroker_t *mad_broker;
	mad_broker = mbroker_new();

	//mbroker_loop_poller(mad_broker, client_route_recv_handle);
	mbroker_loop_timer(mad_broker, 1000, 0, s_test);
	mbroker_looper(mad_broker);

	mbroker_destroy(&mad_broker);
	return 0;
}

