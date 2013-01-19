#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "stdin_protocol.h"
#include "connection.h"
#include "consoler.h"
#include "util.h"
#include "logger.h"

struct protocol stdin_protocol = {
	&passOnAction,
	5,
	{
		{"EXIT", "\t\tQuitting the client.\n", &stdin_exitAction},
		{"SHOW", "\t\tShow download progress.\n", &stdin_showAction},
		{"DOWNLOAD", "\tDownload the nth. result in the result list.\n", 
			&stdin_downloadAction},
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
	int ret = 0;
	logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "passing through %s %s\n", (char *)ap->comword.buf, (char *)ap->comline.buf);
	ret = writef(aap->cap->serverfd, "%s %s\n",  ap->comword.buf, ap->comline.buf);
	flushBuf(&ap->comline);
	flushBuf(&ap->comword);
	return (ret <1)? -1:1; 
}

int stdin_showAction(struct actionParameters *ap, 
		union additionalActionParameters *aap) {
  (void) ap;
	return sendSignalToChildren( aap->cap->cpa, 'd', SIGUSR1 );
}

int stdin_resultsAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	struct flEntry *f;
	long unsigned int i = 0;
	char * size;
	while ( (f = iterateArray(aap->cap->results, &i)) ){
		if (!(f->size / 1024)){
			size = stringBuilder("%dB" , f->size);
		} else if (!(f->size / 1024 / 1024)){
			fprintf(stderr, "1\n\r");
			size = stringBuilder("%dKB" , f->size/1024);
		} else if (!(f->size / 1024 / 1024 / 1024)){
			fprintf(stderr, "1\n\r");
			size = stringBuilder("%dMB" , f->size/1024 /1024);
		} else if (!(f->size / 1024 / 1024 / 1024 / 1024)){
			fprintf(stderr, "1\n\r");
			size = stringBuilder("%dGB" , f->size/1014/1024/1024);
		}
		if (consolemsg(ap->semid, aap->cap->outfd, "%s: %s, %s %d\n", f->filename, 
					size, putIP((struct sockaddr *)&f->ip), 
					getPort((struct sockaddr *)&f->ip)) == -1 ){
			free(size);
			return -1;
		}
		free(size);
	}
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
  pid_t dlpid;
  int sockfd;
	/*BUGBUG i bail on anything but 'download <nr> :whitespace:', too pedantic? */
	int nr = my_strtol( (char *) ap->comline.buf);
  struct flEntry *fl;
  switch ((dlpid = fork())){
  case -1:
    logmsg(ap->semid,ap->logfd,LOGLEVEL_FATAL,
        "(downloadAction)Forking failed when downloading\n");
    return -1;
  case 0:
    if (!(fl = getArrayItem(aap->cap->results, nr))){
      logmsg(ap->semid,ap->logfd,LOGLEVEL_VERBOSE,
        "(downloadAction)Wrong download number %d\n", nr);
      consolemsg(ap->semid,aap->cap->outfd, "<%d> doesn't exist\n" , nr);
      _exit(0);
    }

    char * filename = path_join(conf->workdir, fl->filename);
    if (!filename){
      logmsg(ap->semid,ap->logfd,LOGLEVEL_FATAL,"(downloadAction)out of mem in\n");
      _exit(0);
    }

    int filefd = open( filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP );
    if ( filefd == -1 ) {
      logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "Can't open %s", filename);
      free(filename);
      _exit(0);
    }

		if ( -1 == (sockfd = connectSocket(&fl->ip,
      getPort((struct sockaddr *)&fl->ip)))){
      logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
        "(downloadAction) can't connect to %s %d\n", putIP(&fl->ip),getPort(&fl->ip));
      logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
        "(downloadAction) can't connect to %s %d\n", putIP(&fl->ip),getPort(&fl->ip));
      if(close(filefd) < 0)
        logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
          "(downloadAction) can't close opened file %s\n", filename);
      free(filename);
      _exit(0);
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGUSR1);
    if ( (ap->sigfd = getSigfd(&mask)) < 0 )
      logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, "can't adjust signal mask");

    logmsg(ap->semid,ap->logfd,LOGLEVEL_VERBOSE,"Starting download of %d\n",nr);
    int ret = advFileCopy( filefd, sockfd, fl->size, filename, ap->semid, 
        ap->logfd, ap->sigfd, aap->cap->outfd );
  	if (close(filefd)<0)
      logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, "Error closing file %s", 
          filename);

    if(close(sockfd) < 0)
      logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN,
        "(downloadAction) can't close socket\n");
    free(filename);
    _exit(ret);
  default:
    addChildProcess(aap->cap->cpa, 'd', dlpid);
    return 1;
  }
}
