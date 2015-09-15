#ifndef _KVMSG_H_INCLUDED_
#define _KVMSG_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

kvmsg_t *kvmsg_new(int64_t sequence);
void kvmsg_destroy(kvmsg_t **self_p);

kvmsg_t * kvmsg_dup(kvmsg_t *self);

kvmsg_t *kvmsg_recv(void *socket);
void kvmsg_send(kvmsg_t *self, void *socket);

char *kvmsg_key(kvmsg_t *self);
int64_t kvmsg_sequence(kvmsg_t *self);
byte *kvmsg_uuid(kvmsg_t *self);
byte *kvmsg_body(kvmsg_t *self);
size_t kvmsg_size(kvmsg_t *self);

void kvmsg_set_key(kvmsg_t *self, char *key);
void kvmsg_set_sequence(kvmsg_t *self, int64_t sequence);
void kvmsg_set_uuid(kvmsg_t *self);
void kvmsg_set_body(kvmsg_t *self, byte *body, size_t size);

void kvmsg_fmt_key(kvmsg_t *self, char *format, ...);
void kvmsg_fmt_body(kvmsg_t *self, char *format, ...);

char *kvmsg_get_prop(kvmsg_t *self, char *name);
void kvmsg_set_prop(kvmsg_t *self, char *name, char *format, ...);

void kvmsg_store(kvmsg_t **self_p, zhash_t *hash);
void kvmsg_dump(kvmsg_t *self);
int kvmsg_test(int verbose);

#ifdef __cplusplus
}
#endif

#endif

