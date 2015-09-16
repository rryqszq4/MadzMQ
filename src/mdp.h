#ifndef _MDP_H_INCLUDED_
#define _MDP_H_INCLUDED_

#define MDPC_CLIENT		"MDPC01"

#define MDPW_WORKER		"MDPW01"

#define MDPW_READY		"\001"
#define MDPW_REQUEST	"\002"
#define MDPW_REPLY		"\003"
#define MDPW_HEARTBEAT	"\004"
#define MDPW_DISCONNECT "\005"

static char *mdps_commands [] = {
	NULL, "READY", "REQUEST", "REPLY", "HEARTBEAT", "DISCONNECT"
};

#endif