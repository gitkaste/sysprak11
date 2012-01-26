#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "util.h"

int g_loglevel = 1;

int logger(int pipefd, int filefd){
	ssize_t s, t;
	struct buffer buf;
	if ( createBuf(&buf,4096) == -1) return -1;
	while (1) { 
		
		switch( (s = readToBuf(pipefd, &buf)) ){
		case -2 :
			continue;
		case -1 :
			perror("logger had trouble reading from fd");
			freeBuf(&buf);
			return -1;
		case 0:
			freeBuf(&buf);
			return 1;
		default: // Means we got something decent - do nothing
			if (s != ( t = writeBuf(filefd, &buf))) {
				if (t ==-1) perror("logger had trouble writing");
				freeBuf(&buf);
				return -1;
			}
		}
	}
}

int logmsg(int semid, int pipefd, int loglevel, const char *fmt, ...){
	char *ret;
	struct tm *t;
	char s[256];
	time_t u;
	struct buffer buf;

	va_list argp;
	/* BUGBUG: maybe this needs to be (loglevel & g_loglevel) cf. logger.h */
	if (loglevel > g_loglevel) return 1;
	u = time(NULL);
	if ( (t = localtime(&u)) == 0 ) return -1;
	if ( !(strftime(s, 255, "%c", t)) ) return -1;
	if ( !(fmt=stringBuilder("%s - sev: %d %d: %s", s, loglevel, getpid(), fmt))) 
		return -1;

	va_start (argp, fmt);
	ret = vStringBuilder(fmt, argp);
	free((void *)fmt);
	va_end(argp);
	if ( !(ret) )  return -1;

	/* mean hack - bypassing createbuf */
	buf.buf = (unsigned char *) ret;	
	buf.bufmax = strlen(ret) +1;
	buf.buflen = strlen(ret);

	if (semWait(semid, SEM_LOGGER) == -1) {
		freeBuf(&buf);
		fprintf(stderr, "trouble getting pid");
		return -1;
	}
	ssize_t written = writeBuf(pipefd, &buf);
	freeBuf(&buf);
	if (semSignal(semid, SEM_LOGGER) ==-1) return -1;
	return (written > 0) ? 1 : -1;
}
