
// Copyright (c) rryqszq4

#ifndef _MAD_EXCEPT_H_INCLUDED_
#define _MAD_EXCEPT_H_INCLUDED_

typedef struct _mad_except_t mad_except_t;

typedef struct _mad_except_frame_t mad_except_frame_t;

enum {
	mad_except_entered=0,
	mad_except_raised,
	mad_except_handled,
	mad_except_finalized
};

extern mad_except_frame_t *mad_except_stack;
extern const mad_except_t mad_assert_failed;

void mad_except_raise(const mad_except_t *e, const char *file, int line);

#define RAISE(e) mad_except_raise(&(e), __FILE__, __LINE__)

#define RERAISE mad_except_raise(except_frame.exception, \
	except_frame.file, except_frame.line)

#define RETURN switch (mad_except_stack = mad_except_stack->prev, 0) default; return

#define TRY do { 	\
	volatile int mad_except_flag;	\
	mad_except_frame_t except_frame;	\
	except_frame.prev = mad_except_stack;	\
	mad_except_stack = &except_frame;	\
	mad_except_flag = setjmp(except_frame.env);	\
	if (mad_except_flag == mad_except_entered) {

#define EXCEPT(e)	\
		if (mad_except_flag == mad_except_entered) mad_except_stack = mad_except_stack->prev;	\
	}else if (except_frame.exception == &(e)){	\
		mad_except_flag = mad_except_handled;

#define ELSE \
		if (mad_except_flag == mad_except_entered) mad_except_stack = mad_except_stack->prev;	\
	}else {	\
		mad_except_flag = mad_except_handled;

#define FINALLY \
		if (mad_except_flag == mad_except_entered) mad_except_stack = mad_except_stack->prev;	\
	} {	\
		if (mad_except_flag == mad_except_entered)	\
			mad_except_flag = mad_except_finalized;

#define END_TRY \
		if (mad_except_flag == mad_except_entered) mad_except_stack = mad_except_stack->prev;	\
	} if (mad_except_flag == mad_except_raised) RERAISE;	\
} while (0)

void mad_except_test();

#endif