#ifndef _MBROKER_H_INCLUDED_
#define _MBROKER_H_INCLUDED_

#include "czmq.h"

typedef struct _mbroker_t mbroker_t;

mbroker_t *mdbroker_new();
void mdbroker_destroy(mbroker_t **mbroker_p);

#endif