
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

mad_except_frame_t *mad_except_stack = NULL;

void 
mad_except_raise(const mad_except_t *e, const char *file, int line)
{
	mad_except_frame_t *p = except_stack;
	
	assert(e);
	
	if (p == NULL){
		fprintf(stderr, "Uncaught exception");
		
		if (e->reason)
			fprintf(stderr, " %s", e->reason);
		else
			fprintf(stderr, " at 0x%p", e);

		if (file && line > 0)
			fprintf(stderr, " raised at %s:%d\n", file, line);

		fprintf(stderr, "aborting...\n");
		fflush(stderr);
		abort();
	}

	p->exception = e;
	p->file = file;
	p->line = line;
	except_stack = except_stack->prev;
	longjmp(p->env, except_stack);
}