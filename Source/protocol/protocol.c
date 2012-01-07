#define _GNU_SOURCE /* sigset_t, POLLIN, POLL*, SIG*, esp. POLLRDHUP */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h> /* strcasecmp() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include "protocol.h"
#include "util.h"
#include "config.h"
#include "logger.h"
#include "tokenizer.h"

/* processIncomingData shall be called when there is Data on an fd which is
 * processed by a protocol.
 * It does read from the fd, tries to get (all) tokens (that means lines here)
 * out of the data and processes them through processCommand.
 * Return Values are:
 *    -3: returning child process (error)
 *    -2: returning child process (success)
 *    -1: Fatal error. (abort and exit)
 *     0: ap->comfd hung up / closed from the other side (clean exit)
 *    +x: everything okay, continue
 * Note: Return Values blend with the return values of actions (see note below).
 *       Therefore it's okay to return the value returned by processCommand.
 * Note2: gtfsret can never be < 0 ...
 */
int processIncomingData(struct actionParameters *ap,
			 union additionalActionParameters *aap) {
	int gtfsret, pcret, readret;
	
	/* read once - poll told us there is data */
	if((readret = readToBuf(ap->comfd, &ap->combuf)) == -1) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
			"FATAL (processIncomingData): read()-error from fd.\n");
		return -1;
	} else if (readret == -2) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN,
			"WARNING (processIncomingData): couldn't "
			"read (EINTR or EAGAIN). This shouldn't happen "
			"if signalfd() is used.\n");
		return 1;
	} else if (readret == 0) { /* might also throw a POLLHUP */
		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
			"(processIncomingData) ap->comfd "
			"closed from other side.\n");
		return 0;
	}
	
	/* tokenize all lines recieved and process them */
	while((gtfsret = getTokenFromStreamBuffer(&ap->combuf,
			&ap->comline, "\r\n", "\n", (char *)NULL)) > 0) {
		printf("read: %s", ap->comline.buf);
		if((pcret = processCommand(ap, aap)) <= 0) return pcret;
		/* NOTE: Remaining content in comline will be overwritten
		 * by getTokenFrom*(). */
	}
	if(gtfsret < 0) { /* token buffer too small */
		return gtfsret;
	}
	
	return 1;
}

/* processCommand shall be called, when a new command (line/token) came through
 * ap->comfd. It tries to find the first "word" in the line, feeds it to
 * validateToken and run the action returned by validateToken.
 * Return Value:
 *    2: if no "wordh was found inside the line (means line was empty)
 *       this is positive, because we just want to ignore it.
 *    otherwise: return value of the action called
 *               (see note on return values of actions below)
 */
int processCommand(struct actionParameters *ap,
		    union additionalActionParameters *aap) {
	int gtfbret;
	logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
		"COMRECV: '%.*s'\n", ap->comline.buflen, ap->comline.buf);
	/* get the first word of the line */
	gtfbret = getTokenFromBuffer(&ap->comline,
					&ap->comword, " ", "\t", (char *)NULL);
	if(gtfbret < 0) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "FATAL: "
			"getTokenFromBuffer() failed in processCommand.\n");
		return gtfbret;
	} else if (gtfbret == 0) return 2; /* command was empty */
	/* find (validate) and run the action and return its return value */
	return (*validateToken(&ap->comword, ap->prot)) (ap, aap);
}

int reply(int comfd, int logfd, int semid, int code, const char *msg) {
	logmsg(semid, logfd, LOGLEVEL_DEBUG, "SEND: %3d %s", code, msg);
	return writef(comfd, "%3d %s", code, msg);
}

action validateToken(struct buffer *token, struct protocol *prot) {
	int i;
	for(i = 0; i < prot->actionCount; i++) {
		if(strcasecmp(prot->actions[i].actionName, (char *) token->buf) == 0)
			return prot->actions[i].actionPtr;
	}
	return prot->defaultAction;
}

/* This function needs to return gracefully! Either it succeeds completely
 * or rolls back any and every open ressource */
int initap(struct actionParameters *ap, char error[256], struct config *conf, int semcount){
	int logfds[2];
	int logfilefd;
	/***** Setup BUFFERS *****/
	strncpy(error, "Couldn't create buffers, out of mem", 256);
	if (createBuf(&(ap->combuf),4096) == -1 )
		return -1;
	else ap->usedres = APRES_COMBUF;
	if ( createBuf(&(ap->comline),4096) == -1 )
		goto error;
	else ap->usedres |= APRES_COMLINE;
	
	if (createBuf(&(ap->comword),4096) == -1 )
		goto error;
	else ap->usedres |= APRES_COMWORD;

	/***** Setup Semaphores *****/
	if ( (ap->semid = semCreate(semcount)) == -1 ){
		sperror("Can't create semaphores", error, 256);
		goto error;
	} else
		ap->usedres |= APRES_SEMID;

	ap->comfd = 0;
	ap->sigfd = 0;
	ap->conf = conf;

	/***** Setup Logging  *****/
	logfilefd = open ( conf->logfile, O_APPEND|O_CREAT|O_NONBLOCK,
		 	S_IRUSR|S_IWUSR|S_IRGRP );
	if ( logfilefd == -1 ) {
		sperror("error opening log file, i won't create a path for you", error, 256);
		goto error;
	}

	if (pipe2(logfds, O_NONBLOCK) == -1) {
		sperror("Error creating pipe", error, 256);
		goto error;
	}

	switch(ap->logpid = fork()){
	case -1: 
		sperror("Error forking", error, 256);
		close(logfilefd);
		close(logfds[0]);
		close(logfds[1]);
		goto error;
	case 0: /* we are in the child */
		/* Setting up  */
		ap->logfd = logfds[0];
		close(logfds[1]); /* writing end */
		ap->usedres |= APRES_LOGFD;
		/* Working */
		logger(ap->logfd, logfilefd);
		close(logfilefd);
		/* Quitting */
		freeap(ap);
		fputs("child shutting down",stderr);
		_exit(EXIT_SUCCESS);
	default: /* we are in the parent */
		close(logfilefd);
		close(logfds[0]);  /* reading end */
		ap->logfd = logfds[1];
		ap->usedres |= APRES_LOGFD;
	}
	return 1;
	fprintf(stderr, "I really shouldn't be here alone after dark");

error:
	freeap(ap);
	return -1;
}

void freeap(struct actionParameters *ap){
	if (ap->usedres & APRES_COMBUF)  freeBuf(&(ap->combuf));
	if (ap->usedres & APRES_COMWORD) freeBuf(&(ap->comword));
	if (ap->usedres & APRES_COMLINE) freeBuf(&(ap->comline));
	if (ap->usedres & APRES_SEMID)   semClose(ap->semid);
	if (ap->usedres & APRES_LOGFD)   close(ap->logfd);
	if (ap->usedres & APRES_SIGFD)   close(ap->sigfd);
}
