#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"

int sperror(char * pref, char * buf, int buflen) {
		strncpy(buf, pref, buflen);
		return strerror_r(errno, buf + strlen(pref), buflen - strlen(pref));
}

int my_strtol(char * buf) {
	char * endptr;
	errno = 0;
	int nr = strtol(buf, &endptr, 10);
	while (*endptr != '\0') {
		if (!(isspace(*endptr++))) {
			errno = ETRAILING;
			break;
		}
	}
	return nr;
}

/* Tests an int x to be a power of two (and that alone) by computing 2's complement. */
int isPowerOfTwo (unsigned int x) {
  return ((x != 0) && ((x & (~x + 1)) == x));
}

int isDir(const char *dirname){
	struct stat st;
	return !!(!stat(dirname,&st) && S_ISDIR(st.st_mode));
}

int isDirWritable(const char * dirname){
	char * tmpfile = path_join(dirname, "test");
	FILE * f = fopen(tmpfile, "a+");
	if (!f){
		free(tmpfile);
		return 0;
	}
	free(tmpfile);
	fclose(f);
	return 1;
}

char *path_join(const char * basedir, const char * subdir){
	char * fulldirname = malloc(strlen(basedir) + strlen(subdir) +2);
	if (!fulldirname) return NULL;
	return strcat(strcat(strcpy(fulldirname,basedir),"/"), subdir);
}

