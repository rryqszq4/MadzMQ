#include <uuid/uuid.h>
#include "protocols/kvmsg.h"
#include "broker/mbroker.h"

int 
main (int argc, char *argv[])
{
	mbroker_t *mad_broker;
	//object mbroker construct
	mad_broker = mbroker_new();

	mbroker_loop_poller(mad_broker, client_route_recv_handle);
	//mbroker_loop_timer(mad_broker, 1000, 0, client_route_recv_handle);
	mbroker_looper(mad_broker);

	//object mbroker destruct
	mbroker_destroy(&mad_broker);
	return 0;
}

