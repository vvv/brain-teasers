#ifndef _UTIL_H
#define _UTIL_H

#include <error.h>
#include <errno.h>
#include <stdlib.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define die(format, ...)  error(1, 0, format, ##__VA_ARGS__)
#define die_errno(format, ...)  error(1, errno, format, ##__VA_ARGS__)

static inline void *
xmalloc(size_t size)
{
	void *p = malloc(size);
	if (p == NULL)
		die("Out of memory, malloc failed");
	return p;
}

#endif /* _UTIL_H */
