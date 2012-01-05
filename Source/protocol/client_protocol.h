#include "protocol.h"

int initializeClientProtocol(struct actionParameters *ap);

int client_resultAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

int client_sendlistAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);

int client_unknownCommandAction(struct actionParameters *ap, 
		union additionalActionParameters *aap);
