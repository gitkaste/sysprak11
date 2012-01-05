#include <string.h>
#include <errno.h>

int sperror(char * pref, char * buf, int buflen){
		strncpy(buf, pref, buflen);
		return strerror_r(errno, buf + strlen(pref), buflen - strlen(pref));
}
