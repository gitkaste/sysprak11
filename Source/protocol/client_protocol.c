#ifndef _CLIENT_PROTOCOL_H
#define _CLIENT_PROTOCOL_H

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include "client_protocol.h"
#include "logger.h"
#include "connection.h"
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
	logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
		"Command \"%s\" not understood.\n", ap->comword.buf);
	return -1;
}

int client_resultAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	pid_t child;
	int gtfsret;
	switch(child = fork()){
		case -1:
			return -1;
		case 0:
			/* Command is RESULT (already stripped off) SOCKET port */
			gtfsret = getTokenFromStreamBuffer( &ap->combuf, &ap->comword, " ", "\t", NULL);
			if (gtfsret != 1 || strcasecmp ( (char *)&ap->comword, "SOCKET"))
				_exit(EXIT_FAILURE);
			/* Dies sollte port sein */
			gtfsret = getTokenFromStreamBuffer( &ap->combuf, &ap->comword, " ", "\t", NULL);
			if (gtfsret != 1)
				_exit(EXIT_FAILURE);
			errno =0;
			int port = strtol( (char *) ap->comword.buf, NULL, 10);
			if ( errno || port<0|| port>65536 ) _exit(EXIT_FAILURE);
			int sockfd = connectSocket(&ap->comip, port);
			if (sockfd == -1) _exit(EXIT_FAILURE);
			int ret = recvResult(sockfd, ap->conf->share, "");
			if (ret == 1){
				// Bugbug here log to console!
			}
			return (ret == 1)? -2 : -3;
		default:
			return 1;
	}
}

int client_sendlistAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	pid_t child;
	int gtfsret;
	switch(child = fork()){
		case -1:
			return -1;
		case 0:
			/* Command is SENDLIST (already stripped off) SOCKET port */
			gtfsret = getTokenFromStreamBuffer( &ap->combuf, &ap->comword, " ", "\t", NULL);
			if (gtfsret != 1 || strcasecmp ( (char *)&ap->comword, "SOCKET"))
				_exit(EXIT_FAILURE);
			/* Dies sollte port sein */
			gtfsret = getTokenFromStreamBuffer( &ap->combuf, &ap->comword, " ", "\t", NULL);
			if (gtfsret != 1)
				_exit(EXIT_FAILURE);
			errno =0;
			int port = strtol( (char *) ap->comword.buf, NULL, 10);
			if ( errno || port<0|| port>65536 ) _exit(EXIT_FAILURE);
			int sockfd = connectSocket(&ap->comip, port);
			if (sockfd == -1) _exit(EXIT_FAILURE);
			int ret = parseDirToFD(sockfd, ap->conf->share, "");
			return (ret == 1)? -2 : -3;
		default:
			return 1;
	}
}

#endif
