#ifndef _CLIENT_PROTOCOL_H
#define _CLIENT_PROTOCOL_H

#include "client_protocol.h"
#include "logger.h"

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
	return 1;
}

int client_sendlistAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	return 1;
}

#endif
