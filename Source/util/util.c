#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "util.h"

int sperror(char * pref, char * buf, int buflen){
		strncpy(buf, pref, buflen);
		return strerror_r(errno, buf + strlen(pref), buflen - strlen(pref));
}

int mystrtol(char * buf){
	char * endptr;
	errno = 0;
	int nr = strtol(buf, &endptr, 10);
	while (*endptr != '\0') {
		if (!(isspace(endptr++))) {
			errno = ETRAILING;
			break;
		}
	}
	return nr;
}
