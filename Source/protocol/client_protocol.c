#ifndef _CLIENT_PROTOCOL_H
#define _CLIENT_PROTOCOL_H

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include "client_protocol.h"
#include "logger.h"
#include "connection.h"
#include "consoler.h"
#include "tokenizer.h"
#include "util.h"

struct protocol client_protocol = {
	&client_unknownCommandAction,
	2,
	{
		{"RESULT", "Server has search results for us.\n", 
			&client_resultAction},
		{"SENDLIST", "Server wants a file list from us.\n",
			&client_sendlistAction}
	}
};

int initializeClientProtocol(struct actionParameters *ap){
	ap->prot = &client_protocol;
	return 1;
}

int client_unknownCommandAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	consolemsg(ap->semid, aap->cap->outfd, 
		"(client) Command '\"%s\"' not understood.\n", ap->comword.buf);
	logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN,
		"Command \"%s\" not understood.\n", ap->comword.buf);
	return 1;
}

int client_resultAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	pid_t child;
	int sockfd, num;

	logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
			"(client_resultAction) started %s\n", ap->comword.buf);
	switch(child = fork()){
		case -1:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
				"(client_resultAction) can't fork.\n");
			return -1;
		case 0:
			close(aap->cap->serverfd);

			logmsg(ap->semid, ap->logfd, LOGLEVEL_DEBUG, "print %s\n", ap->comline);
			/* Command is RESULT (already stripped off) SOCKET port */
			if ( ( 1 != getTokenFromBuffer( &ap->comline, &ap->comword,
					"SOCKET", " ", "\t", NULL) )) {
				logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
					"(client_resultAction) trouble parsing token %s.\n", ap->comline.buf);
				return -3;
			}
			int port = my_strtol( (char *) ap->comword.buf);
			if ( errno || port<0 || port > 65536 ) {
			 	logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
				"(client_resultAction) failed to get port. %s %d\n", ap->comline.buf);
				return -3;
				}

			if ( -1 == (sockfd = connectSocket(&ap->comip, port))){
			 	logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
				"(client_resultAction) can't connect. %s %d\n", putIP(&ap->comip), port);
				return -3;
				}
			clearArray(aap->cap->results);
			switch( (num = recvResult(sockfd, ap,aap->cap->results) )){
				case 0:
					consolemsg(ap->semid, aap->cap->outfd, "Didn't find any results.\n");
					break;
				case -3:
					consolemsg(ap->semid, aap->cap->outfd, "Search failed.\n");
					logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
						"(client_resultAction) recvResult failed\n");
					return -3;
				default:
					consolemsg(ap->semid, aap->cap->outfd, "Found %d results.\n", num);
			}
			return -2;
		default:
			/* r for resultAction */
			addChildProcess(aap->cap->cpa, 'r', child);
			return 1;
	}
}

int client_sendlistAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	pid_t child;

	switch(child = fork()){
		case -1:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"(client_sendlistAction) forking failed\n");
			return -1;

		case 0:
			/* Command is SENDLIST (already stripped off) SOCKET port */
			if ( 1 != getTokenFromBuffer( &ap->comline, &ap->comword, " ", "\t", 
					"SOCKET", NULL)){
				logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"(client_sendlistAction) forking failed\n");
				return -3;
			}

			int port = my_strtol( (char *) ap->comword.buf);
			if ( errno || port<0|| port>65536 ) {
				logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"(client_sendlistAction) forking failed\n");
				return -3;
			}

			socklen_t iplen = sizeof(aap->cap->serverfd);
			if (getpeername (aap->cap->serverfd, &ap->comip, &iplen)) 
				perror("getpeername");

			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "(client_sendlistAction) "
				"connecting to %s:%d via %d %d\n", putIP(&ap->comip), port,ap->comfd, aap->cap->serverfd);

			int sockfd = connectSocket(&ap->comip, port);
			if (sockfd == -1) {
				logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"(client_sendlistAction) connecting failed\n");
				return -3;
			}

			int ret = parseDirToFD(sockfd, ap->conf->share, "");
			close (sockfd);
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
				"(client_sendlistAction) finished\n");
			return (ret == 1)? -2 : -3;

		default:
			/* s for sendlist */
			addChildProcess(aap->cap->cpa, 'u', aap->cap->conpid);
			return 1;
	}
}

#endif
