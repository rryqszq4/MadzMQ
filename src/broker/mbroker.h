#ifndef _MBROKER_H_INCLUDED_
#define _MBROKER_H_INCLUDED_

#include "czmq.h"

typedef struct _mbroker_t mbroker_t;

mbroker_t *mbroker_new();
void mbroker_destroy(mbroker_t **this_p);

#endif