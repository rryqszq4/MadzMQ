#include "mbqueue.h"

struct _mbqueue_t
{
	zlist_t *list;
	int64_t total_push;
	int64_t total_pop;
};



