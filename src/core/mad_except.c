
// Copyright (c) rryqszq4

#include "mad_core.h"

struct _mad_except_t
{
	char *reason;
};

struct _mad_except_frame_t
{
	mad_except_frame_t *prev;
	jmp_buf env;
	const char *file;
	int line;
	const mad_except_t *exception;
};

mad_except_frame_t *mad_except_stack = NULL;

const mad_except_t mad_assert_failed = { "Assertion failed" };

void 
mad_except_raise(const mad_except_t *e, const char *file, int line)
{
	mad_except_frame_t *p = mad_except_stack;
	
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
	mad_except_stack = mad_except_stack->prev;
	longjmp(p->env, mad_except_raised);
}


void
mad_except_test()
{
	char *p;
	TRY
		p = (char *)malloc(-1);
		if (p != NULL){
		}else {
			RAISE(mad_assert_failed);
			assert(0);
		}
	EXCEPT(mad_assert_failed)
		printf("catch exception :%s\n", mad_assert_failed.reason);
		;
	END_TRY;

	if (p){
		free(p);
	}

	printf("%s() end\n", __FUNCTION__);
	return ;

}




