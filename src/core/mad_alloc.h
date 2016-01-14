
// Copyright (c) rryqszq4

#ifndef _MAD_ALLOC_H_INCLUDED_
#define _MAD_ALLOC_H_INCLUDED_

#include "except.h"

extern const mad_except_t mad_alloc_failed;

extern void *mad_alloc(long nbytes, const char *file, int line);

extern void *mad_calloc(long count, long nbytes, const char *file, int line);

extern void mad_free(void *ptr, const char *file, int line);

extern void *mad_resize(void *ptr, long nbytes, const char *file, int line);

#define ALLOC(nbytes)	\
	mad_alloc((nbytes), __FILE__, __LINE__)

#define CALLOC(count, nbytes)	\
	mad_calloc((count), (nbytes), __FILE__, __LINE__)

#define NEW(p)	((p) = ALLOC((long)sizeof *(p)))

#define NEW0(p)	((p) = CALLOC(1, (long)sizeof *(p)))

#define FREE(ptr)	((void)(mad_free((ptr), \
	__FILE__, __LINE__), (ptr) = 0))

#define RESIZE(ptr, nbytes)		((ptr) = mad_resize((ptr),	\
	(nbytes), __FILE__, __LINE__))

#endif