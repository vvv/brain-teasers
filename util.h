#ifndef _UTIL_H
#define _UTIL_H

#include <stdlib.h>

static inline void *
xmalloc(size_t size)
{
	void *p = malloc(size);
	if (p == NULL)
		die("Out of memory, malloc failed");
	return p;
}

#endif /* _UTIL_H */
