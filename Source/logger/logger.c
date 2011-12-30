#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "util.h"
#include "writef.h"

int logger(int pipefd, int filefd){
	ssize_t s, t;
	struct buffer buf;
	if ( createBuf(&buf,4096) == -1) return -1;
	while (1){ 
		s = readToBuf(pipefd, &buf);
	 	if (s  == -2 ) continue;
		if (s == -1 ){
			puts("arg");
			freeBuf(&buf);
			return -1;
		}
		if (s != ( t = writeBuf(filefd, &buf))) {
			printf("%d - t %d",(int) s, (int) t);
			freeBuf(&buf);
			return -1;
		}
	}
	freeBuf(&buf);
	return 1;
}

int logmsg(int semid, int pipefd, int loglevel, const char *fmt, ...){
	char *ret;
	struct tm *t;
	char s[256];
	time_t u;
	struct buffer buf;

	va_list ap;
	/* BUGBUG insert code here to check for loglevel once it's clear how to get it out of config */
	u = time(NULL);
	if ( (t = localtime(&u)) == 0 ) return -1;
	if ( !(strftime(s, 255, "%c", t)) ) return -1;
	if ( !(fmt = stringBuilder ("%s - %d: %s", s, getpid(), fmt)) ) return -1;

	va_start (ap, fmt);
	ret = vStringBuilder(fmt, ap);
	va_end(ap);
	if ( !(ret) )  return -1;

	/* mean hack - bypassing createbuf */
	buf.buf = (unsigned char *) ret;	
	buf.bufmax = strlen(ret) +1;
	buf.buflen = strlen(ret);

	ssize_t written = writeBuf(pipefd, &buf);
	freeBuf(&buf);
	return (written > 0) ? 1 : -1;
}
