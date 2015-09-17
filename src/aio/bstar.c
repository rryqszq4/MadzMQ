#include "bstar.h"

typedef enum {
	STATE_PRIMARY = 1,
	STATE_BACKUP = 2,
	STATE_ACTIVE = 3,
	STATE_PASSIVE = 4
} state_t;

typedef enum {
	PEER_PRIMARY = 1,
	PEER_BACKUP = 2,
	PEER_ACTIVE = 3,
	PEER_PASSIVE = 4,
	CLIENT_REQUEST = 5
} event_t;

struct _bstar_t {
	zctx_t *ctx;
	zloop_t *loop;
	void *statepub;
	void *statesub;
	state_t state;
	event_t event;
	int64_t peer_expiry;
	zloop_fn *voter_fn;
	void *voter_arg;
	zloop_fn *active_fn;
	void *active_arg;
	zloop_fn *passive_fn;
	void *passive_arg;
};

#define BSTAR_HEARTBEAT 1000

static int
s_execute_fsm (bstar_t *self)
{
    int rc = 0;
    //  Primary server is waiting for peer to connect
    //  Accepts CLIENT_REQUEST events in this state
    if (self->state == STATE_PRIMARY) {
        if (self->event == PEER_BACKUP) {
            zclock_log ("I: connected to backup (passive), ready as active");
            self->state = STATE_ACTIVE;
            if (self->active_fn)
                (self->active_fn) (self->loop, NULL, self->active_arg);
        }
        else
        if (self->event == PEER_ACTIVE) {
            zclock_log ("I: connected to backup (active), ready as passive");
            self->state = STATE_PASSIVE;
            if (self->passive_fn)
                (self->passive_fn) (self->loop, NULL, self->passive_arg);
        }
        else
        if (self->event == CLIENT_REQUEST) {
            // Allow client requests to turn us into the active if we've
            // waited sufficiently long to believe the backup is not
            // currently acting as active (i.e., after a failover)
            assert (self->peer_expiry > 0);
            if (zclock_time () >= self->peer_expiry) {
                zclock_log ("I: request from client, ready as active");
                self->state = STATE_ACTIVE;
                if (self->active_fn)
                    (self->active_fn) (self->loop, NULL, self->active_arg);
            } else
                // Don't respond to clients yet - it's possible we're
                // performing a failback and the backup is currently active
                rc = -1;
        }
    }
    else
    //  Backup server is waiting for peer to connect
    //  Rejects CLIENT_REQUEST events in this state
    if (self->state == STATE_BACKUP) {
        if (self->event == PEER_ACTIVE) {
            zclock_log ("I: connected to primary (active), ready as passive");
            self->state = STATE_PASSIVE;
            if (self->passive_fn)
                (self->passive_fn) (self->loop, NULL, self->passive_arg);
        }
        else
        if (self->event == CLIENT_REQUEST)
            rc = -1;
    }
    else
    //  Server is active
    //  Accepts CLIENT_REQUEST events in this state
    //  The only way out of ACTIVE is death
    if (self->state == STATE_ACTIVE) {
        if (self->event == PEER_ACTIVE) {
            //  Two actives would mean split-brain
            zclock_log ("E: fatal error - dual actives, aborting");
            rc = -1;
        }
    }
    else
    //  Server is passive
    //  CLIENT_REQUEST events can trigger failover if peer looks dead
    if (self->state == STATE_PASSIVE) {
        if (self->event == PEER_PRIMARY) {
            //  Peer is restarting - become active, peer will go passive
            zclock_log ("I: primary (passive) is restarting, ready as active");
            self->state = STATE_ACTIVE;
        }
        else
        if (self->event == PEER_BACKUP) {
            //  Peer is restarting - become active, peer will go passive
            zclock_log ("I: backup (passive) is restarting, ready as active");
            self->state = STATE_ACTIVE;
        }
        else
        if (self->event == PEER_PASSIVE) {
            //  Two passives would mean cluster would be non-responsive
            zclock_log ("E: fatal error - dual passives, aborting");
            rc = -1;
        }
        else
        if (self->event == CLIENT_REQUEST) {
            //  Peer becomes active if timeout has passed
            //  It's the client request that triggers the failover
            assert (self->peer_expiry > 0);
            if (zclock_time () >= self->peer_expiry) {
                //  If peer is dead, switch to the active state
                zclock_log ("I: failover successful, ready as active");
                self->state = STATE_ACTIVE;
            }
            else
                //  If peer is alive, reject connections
                rc = -1;
        }
        //  Call state change handler if necessary
        if (self->state == STATE_ACTIVE && self->active_fn)
            (self->active_fn) (self->loop, NULL, self->active_arg);
    }
    return rc;
}

static void
s_update_peer_expiry (bstar_t *self)
{
    self->peer_expiry = zclock_time () + 2 * BSTAR_HEARTBEAT;
}

//  Reactor event handlers...

//  Publish our state to peer
int s_send_state (zloop_t *loop, int timer_id, void *arg)
{
    bstar_t *self = (bstar_t *) arg;
    zstr_sendf (self->statepub, "%d", self->state);
    return 0;
}

//  Receive state from peer, execute finite state machine
int s_recv_state (zloop_t *loop, zmq_pollitem_t *poller, void *arg)
{
    bstar_t *self = (bstar_t *) arg;
    char *state = zstr_recv (poller->socket);
    if (state) {
        self->event = atoi (state);
        s_update_peer_expiry (self);
        free (state);
    }
    return s_execute_fsm (self);
}

//  Application wants to speak to us, see if it's possible
int s_voter_ready (zloop_t *loop, zmq_pollitem_t *poller, void *arg)
{
    bstar_t *self = (bstar_t *) arg;
    //  If server can accept input now, call appl handler
    self->event = CLIENT_REQUEST;
    if (s_execute_fsm (self) == 0)
        (self->voter_fn) (self->loop, poller, self->voter_arg);
    else {
        //  Destroy waiting message, no-one to read it
        zmsg_t *msg = zmsg_recv (poller->socket);
        zmsg_destroy (&msg);
    }
    return 0;
}

bstar_t *
bstar_new(int primary, char *local, char *remote)
{
	bstar_t *self;

	self = (bstar_t *)zmalloc(sizeof(bstar_t));

	self->ctx = zctx_new();
	self->loop = zloop_new();
	self->state = primary ? STATE_PRIMARY : STATE_BACKUP;

	self->statepub = zsocket_new(self->ctx, ZMQ_PUB);
	zsocket_bind(self->statepub, local);

	self->statesub = zsocket_new(self->ctx, ZMQ_SUB);
	zsocket_set_subscribe(self->statesub, "");
	zsocket_connect(self->statesub, remote);

	zloop_timer(self->loop, BSTAR_HEARTBEAT, 0, s_send_state, self);
	zmq_pollitem_t poller = {self->statesub, 0, ZMQ_POLLIN};
	zloop_poller(self->loop, &poller, s_recv_state, self);
	return self;
}

void 
bstar_destroy(bstar_t **self_p)
{
	assert(self_p);
	if (*self_p){
		bstar_t *self = *self_p;
		zloop_destroy(&self->loop);
		zctx_destroy(&self->ctx);
		free(self);
		*self_p = NULL;
	}
}

zloop_t *
bstar_zloop(bstar_t *self)
{
	return self->loop;
}

int 
bstar_voter(bstar_t *self, char *endpoint, int type, zloop_fn handler, void *arg)
{
	void *socket = zsocket_new(self->ctx, type);
	zsocket_bind(socket, endpoint);
	assert(!self->voter_fn);
	self->voter_fn = handler;
	self->voter_arg = arg;
	zmq_pollitem_t poller = {socket, 0, ZMQ_POLLIN};
	return zloop_poller(self->loop, &poller, s_voter_ready, self);
}

void 
bstar_new_active(bstar_t *self, zloop_fn handler, void *arg)
{
	assert(!self->active_fn);
	self->active_fn = handler;
	self->active_arg = arg;
}

void 
bstar_new_passive(bstar_t *self, zloop_fn handler, void *arg)
{
	assert(!self->passive_fn);
	self->passive_fn = handler;
	self->passive_arg = arg;
}

void 
bstar_set_verbose(bstar_t *self, bool verbose)
{
	zloop_set_verbose(self->loop, verbose);
}

int
bstar_start(bstar_t *self)
{
	assert(self->voter_fn);
	s_update_peer_expiry(self);
	return zloop_start(self->loop);
}














