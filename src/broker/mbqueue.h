#ifndef _MBROUTE_H_INCLUDED_
#define _MBROUTER_H_INCLUDED_

#include "czmq.h"

typedef struct _mbqueue_t mbqueue_t;

mbqueue_t *mbqueue_new();

void mbqueue_destroy(mbqueue_t **this_p);

int mbqueue_push(mbqueue_t *this, void *item);

void *mbqueue_pop(mbqueue_t *this);

#endif