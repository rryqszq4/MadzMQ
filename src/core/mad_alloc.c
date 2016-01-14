
// Copyright (c) rryqszq4

#include "mad_core.h"

const mad_except_t mad_alloc_failed = { "Allocation Failed" };

void *
mad_alloc(long nbytes, const char *file, int line)
{
	void *ptr;

	assert(nbytes > 0);

	ptr = malloc(nbytes);
	if (ptr == NULL)
	{
		if (file == NULL)
			RAISE(mad_alloc_failed);
		else
			mad_except_raise(&mad_alloc_failed, file, line);
	}
	
	return ptr;
}

void *
mad_calloc(long count, long nbytes, const char *file, int line)
{
	void *ptr;
	assert(count > 0);
	assert(nbytes > 0);
	ptr = calloc(count, nbytes);
	if (ptr == NULL)
	{
		if (file == NULL)
			RAISE(mad_alloc_failed);
		else 
			mad_except_raise(&mad_alloc_failed, file, line);
	}

	return ptr;
}

void 
mad_free(void *ptr, const char *file, int line)
{
	if (ptr)
		free(ptr);
}

void *
mad_resize(void *ptr, long nbytes, const char *file, int line)
{
	assert(ptr);
	assert(nbytes > 0);

	ptr = realloc(ptr, nbytes);
	if (ptr == NULL)
	{
		if (file == NULL)
			RAISE(mad_alloc_failed);
		else 
			mad_except_raise(&mad_alloc_failed, file ,line);
	}

	return ptr;
}




