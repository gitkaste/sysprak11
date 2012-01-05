#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

ssize_t writef (int fd, char *fmt, ...){
	int ret;
	char *p;
	va_list ap;
	
	va_start(ap, fmt);
	p = vStringBuilder(fmt, ap);
	va_end(ap);
	
	ret = writeWrapper(fd, p, strlen(p));
	free(p);
	return ret;
}

char *stringBuilder (const char *fmt, ...){
	char *ret;
	va_list ap;
	
	va_start(ap, fmt);
	ret = vStringBuilder(fmt, ap);
	va_end(ap);
	return ret;
}

char *vStringBuilder (const char *fmt, va_list ap){
	/* Code taken from vsnprintf manpage */
	int n, size = 128;
	char *p, *np;
	va_list ap_copy; /* NOTE: IMPORTANT ! Without va_copy we can't restart parsing varargs ! */

	if ((p = malloc(size)) == NULL)
		return NULL;

	while (1) {
		/* Try to print in the allocated space. */
		va_copy(ap_copy, ap);
		n = vsnprintf(p, size, fmt, ap_copy);
		va_end(ap_copy);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			return p;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = realloc (p, size)) == NULL) {
			free(p);
			return NULL;
		} else {
			p = np;
		}
	}
}
