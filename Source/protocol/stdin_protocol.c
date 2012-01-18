#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include "stdin_protocol.h"
#include "connection.h"
#include "util.h"

struct protocol stdin_protocol = {
	&passOnAction,
	5,
	{
		{"EXIT", "Quitting the client.\n", &stdin_exitAction},
		{"SHOW", "Show download progress.\n", &stdin_showAction},
		{"DOWNLOAD", "Download the nth. result in the result list.\n", 
			&stdin_exitAction},
		{"RESULTS", "Print out the results of a search to the user.\n", 
			&stdin_resultsAction},
		{"HELP", "HELP prints this help.\n", &stdin_helpAction}
	}
};

int initializeStdinProtocol(struct actionParameters *ap) {
	ap->prot = &stdin_protocol;
	return 1;
}

int passOnAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	return ( writef(aap->cap->serverfd, "%s %s", ap->comword, ap->comline) == -1 )? -2: -3; }

int stdin_showAction(struct actionParameters *ap, 
		union additionalActionParameters *aap) {
	return sendSignalToChildren( aap->cap->cpa, 'd', SIGUSR1 );
}

int stdin_resultsAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	struct flEntry *f;
	long unsigned int i;
	char * size;
	char ip[127];
	while ( (f = iterateArray(aap->cap->results, &i)) ){
		if (!(f->size / 1024)){
			size = stringBuilder("%dB" , f->size);
		} else if (!(f->size / 1024 * 1024)){
			size = stringBuilder("%dKB" , f->size);
		} else if (!(f->size / 1024 * 1024 * 1024)){
			size = stringBuilder("%dMB" , f->size);
		} else if (!(f->size / 1024 * 1024 * 1024 * 1024)){
			size = stringBuilder("%dGB" , f->size);
		}
		inet_ntop(AF_INET, &f->ip, ip, 126);
		if ( writef(aap->cap->outfd, "%s: %s, %s %d\n",f->filename,size,ip,f->port)
			 == -1 ){
			free(size);
			return -1;
		}
	}
	free(size);
	return 1;
}

/* This should start a cascade killing the whole client */
int stdin_exitAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	return close(ap->comfd);
}

int stdin_helpAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	struct protocol * p = ap->prot;
	for (int i =0; i < p->actionCount; i++) {
		if ( -1 == writef(ap->comfd, "%s: %s\n", p->actions[i].actionName, 
					p->actions[i].description) ) {
			perror("(stdin_helpAction) trouble writing to socket");
			return -1;
		}
	}
	return 1;
}

int stdin_downloadAction(struct actionParameters *ap, 
		union additionalActionParameters *aap) {
	/* i bail on anything but 'download <nr> :whitespace:', too pedantic? */
	int nr = mystrtol( (char *) ap->comline.buf);
	if (errno) return -1;
	return nr;
//	advFileCopy(nr);
}
