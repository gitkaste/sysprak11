#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include "util.h"

struct array *addChildProcess(struct array *cpa, unsigned char type, pid_t pid){
	struct processChild * c = malloc(sizeof(struct processChild));
	c->type = type;
	c->pid = pid;
	/* I don't need to be careful here because the caller needs to take care of not losing cpa */
	cpa = addArrayItem(cpa, c);
	free(c);
	return cpa;
}

int remChildProcess(struct array *cpa, pid_t pid){
	struct processChild *c;
	long unsigned int i = 0;
	while ( (c = iterateArray(cpa, &i)) ){
		if (pid == c->pid){
			if ( -1 == remArrayItem(cpa, i-1))
				return -1;
		}
	}
	return 1;
}

/* Why don't we take advantage of process groups */
int sendSignalToChildren(struct array *cpa, unsigned char type, int sig){
	struct processChild *c;
	long unsigned int i = 0;
	while ( (c = iterateArray(cpa, &i)) ){
		if (c->type == type){
			if ( kill (c->pid, sig) )
				return -1;
		}
	}
	return 1;
}
