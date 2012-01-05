#ifndef _SERVER_PROTOCOL_H
#define _SERVER_PROTOCOL_H

#include "protocol.h"

int initializeServerProtocol(struct actionParameters *ap);

/* standard action */
int unknownCommandAction(struct actionParameters *ap,
	union additionalActionParameters *aap);

/* send some status-info to the client (great for testing) */
int statusAction(struct actionParameters *ap,
	union additionalActionParameters *aap);

/* quit and disconnect and stuff (server side closing of connection) */
int quitAction(struct actionParameters *ap,
	union additionalActionParameters *aap);

/* Print some help (available commands) */
int helpAction(struct actionParameters *ap,
	union additionalActionParameters *aap);
#endif
