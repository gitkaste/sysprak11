#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include "stdin_protocol.h"
#include "connection.h"
#include "consoler.h"
#include "util.h"

struct protocol stdin_protocol = {
	&passOnAction,
	5,
	{
		{"EXIT", "\t\tQuitting the client.\n", &stdin_exitAction},
		{"SHOW", "\t\tShow download progress.\n", &stdin_showAction},
		{"DOWNLOAD", "\tDownload the nth. result in the result list.\n", 
			&stdin_exitAction},
		{"RESULTS", "\tPrint out the results of a search to the user.\n", 
			&stdin_resultsAction},
		{"HELP", "\t\tHELP prints this help and requests help from the server.\n", 
			&stdin_helpAction}
	}
};

int initializeStdinProtocol(struct actionParameters *ap) {
	ap->prot = &stdin_protocol;
	return 1;
}

int passOnAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	return (consolemsg(ap->semid, aap->cap->outfd, "%s %s",
				ap->comword.buf,ap->comline.buf)==-1)?-1:1;
}

int stdin_showAction(struct actionParameters *ap, 
		union additionalActionParameters *aap) {
	return sendSignalToChildren( aap->cap->cpa, 'd', SIGUSR1 );
}

int stdin_resultsAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	struct flEntry *f;
	long unsigned int i;
	char * size;
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
		if (consolemsg(ap->semid, aap->cap->outfd, "%s: %s, %s %d\n", f->filename, 
					size, putIP(&f->ip), getPort(&f->ip)) == -1 ){
			free(size);
			return -1;
		}
	}
	free(size);
	return 1;
}

int stdin_exitAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	consolemsg(ap->semid, aap->cap->outfd, "closing from within");
	return 0;
}

int stdin_helpAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	struct protocol * p = ap->prot;
	if ( -1 == writef(aap->cap->serverfd, "HELP\n") )
			return -1;
	for (int i =0; i < p->actionCount; i++) {
		if ( -1 == consolemsg(ap->semid, aap->cap->outfd,  "client: %s %s",
					p->actions[i].actionName, p->actions[i].description) )
			return -1;
	}
	return 1;
}

int stdin_downloadAction(struct actionParameters *ap, 
		union additionalActionParameters *aap) {
	/* i bail on anything but 'download <nr> :whitespace:', too pedantic? */
	int nr = my_strtol( (char *) ap->comline.buf);
	if (errno) return -1;
	return nr;
//	advFileCopy(nr);
}
