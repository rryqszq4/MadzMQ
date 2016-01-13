
// Copyright (c) rryqszq4

#include "mad_core.h"

struct _mad_except_t
{
	char *reason;
}

struct _mad_except_frame_t
{
	mad_except_frame_t *prev;
	jmp_buf env;
	const char *file;
	int line;
	const mad_except_t *exception;
}