#ifndef _BSTAR_H_INCLUDED_
#define _BSTAR_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "czmq.h"

#define BSTAR_PRIMARY 1
#define BSTAR_BACKUP  0

typedef struct _bstar_t bstar_t;

bstar_t *bstar_new(int primary, char *local, char *remote);
void bstar_destroy(bstar_t **self_p);

zloop_t *bstar_zloop(bstar_t *self);

int bstar_voter(bstar_t *self, char *endpoint, int type, zloop_fn handler, void *arg);

void bstar_new_master(bstar_t *self, zloop_fn handler, void *arg);
vodi bstar_new_slave(bstar_t *self, zloop_fn handler, void *arg);

void bstar_set_verbose(bstar_t *self, bool verbose);

int bstar_start(bstar_t *self);

#ifdef __cplusplus
}
#endif

#endif