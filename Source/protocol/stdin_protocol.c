#include "stdin_protocol.h"
#include "connection.h"
#include "util.h"

struct protocol stdin_protocol = {
	&stdin_passOnAction,
	2,
	{
		{"EXIT", "Server wants a file list from us.\n", &stdin_exitAction},
		{"HELP", "HELP prints this help.\n", &stdin_helpAction}
	}
};

int initializeStdinProtocol(struct actionParameters *ap){
	ap->prot = &stdin_protocol;
	return 1;
}

int passOnAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	if ( writeBuf(ap.comfd, msg) == -1 )
		return -2;

}

int stdin_resultsAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	char * msg;
	for (int i =0; i < ap->; i++){
	;
}

int stdin_exitAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	;
}

int stdin_helpAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	char * msg;
	struct protocol * p = ap->prot;
	for (int i =0; i < p->actionCount; i++){
		msg = stringBuilder("%s: %s\n", p->actions[i].actionName, p->actions[i].description);
// don't use reply!
		reply(ap->comfd, ap->logfd, ap->semid, REP_TEXT,msg);
		free(msg);
	}
	return (writef(ap.comfd, "300 HELP", ) == -1)? -1: 1;
}

int stdin_downloadAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	;
}
