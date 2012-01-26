#include <stdio.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include "config.h"
#include "logger.h"
#include "util.h"
#include "consoler.h"
#include "server_protocol.h"
#include "protocol.h"
#include "connection.h"
#include "signalfd.h"

int initsap (struct serverActionParameters *sap, char error[256], struct config * conf){
	int size = conf->shm_size;
	if ( (sap->shmid_filelist = shmCreate(size)) == -1)
			return -1;
	//fprintf(stderr, "shmid %d\n", sap->shmid_filelist);
	sap->usedres = SAPRES_FILELISTSHMID;

	size -= sizeof(struct array);
	if (!(sap->filelist = initArray(sizeof(struct flEntry), size, sap->shmid_filelist))){
		shmctl(sap->shmid_filelist, IPC_RMID, NULL);
		return -1;	
	}
	sap->usedres |= SAPRES_FILELIST;

	return 1;
}
void freesap(struct serverActionParameters *sap){
  if (sap->usedres & SAPRES_FILELIST)
		freeArray(sap->filelist);
  if (sap->usedres & SAPRES_FILELISTSHMID)
		shmctl(sap->shmid_filelist, IPC_RMID, NULL);
}

void print_usage(char * prog){
	fprintf(stderr, 
		"Usage: %s [[-c] config filename]\n"
"Config file is assumed to be ./server.conf if the value isn't given.\n",
		prog);
}

/* function called when forking off after accepting */
int comfork(struct actionParameters *ap, 
		union additionalActionParameters *aap){

	struct pollfd pollfds[2];
  struct signalfd_siginfo fdsi;
  ssize_t SRret;

	if (ap->comfd == -1){
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "Error accepting a connection");
		return -1;
	} else {

		/* Setting up stuff for Polling */
		pollfds[0].fd = ap->comfd;    /* data incoming from client */
		pollfds[0].events = POLLIN;
		pollfds[0].revents = 0;
		pollfds[1].fd = ap->sigfd; /* communication with signalfd */
		pollfds[1].events = POLLIN;
		pollfds[1].revents = 0;

		while (1) {
			
			if ( poll(pollfds, 2, -1) <= 0) {
				if (errno == EINTR || errno == EAGAIN ) continue; /* Signals */
				logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "POLLING Error:%d - %s.\n", 
						errno, strerror(errno));
				return -1;
			}

			if(pollfds[0].revents & POLLIN) {  /* client calls */
				switch(processIncomingData(ap, aap)){
					case -1:
						logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL,
							"FATAL (comfork): processIncomingData errored.\n");
						return -1;
					case -2:
						logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
							"VERBOSE (comfork): child %d quit with success.\n");
						break;
					case -3:
						logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN,
							"WARNING (comfork): client %d gave an error.\n");
						break;
					case 0:
						logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
							"VERBOSE (comfork): client hung up\n");
						return 1;
				}
			} else if(pollfds[1].revents & POLLIN) { /* incoming signal */
				SRret = read(ap->sigfd, &fdsi, sizeof(struct signalfd_siginfo));
				if (SRret != sizeof(struct signalfd_siginfo)) {
					logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, 
							"signalfd returned something broken");
					return -3;
				}
				switch(fdsi.ssi_signo){
					case SIGINT:
					case SIGQUIT:
						return -2; /*?*/
				}
			} else {
				logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN,
					 "Dunno what to do with this poll");
				return -3;
			}
		}
	}
}

int main (int argc, char * argv[]){
	char error[256];
	/* in and out fds are seen as from the clients view point */
	struct config conf;
	char * conffilename = "server.conf";
	struct actionParameters ap;
	struct serverActionParameters sap;
	union additionalActionParameters aap;
	const int numsems = 2;
	int opt, forkret,comret;
  struct signalfd_siginfo fdsi;
  ssize_t SRret;
	sigset_t mask;

#define EXIT_NO (EXIT_FAILURE * 2+3)
	char shellReturn = EXIT_NO;
  while ((opt = getopt(argc, argv, "ipc")) != -1) {
		switch (opt) {
			case 'c':
				conffilename = optarg;
        break;
      default: /*  '?' */
				print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

	if ( optind == (argc-1) )
		conffilename = argv[optind];
	
	if ( initConf(conffilename, &conf, error) == -1 ){
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	if (initap(&ap, error, &conf, numsems) == -1) {
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}
	initializeServerProtocol(&ap); 

	if (initsap(&sap, error, &conf) == -1){
		freeap(&ap);
		logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "%s.\n", error);
		exit(EXIT_FAILURE);
	}

	int sockfd;
	if ( (sockfd = createPassiveSocket(&(conf.port))) == -1){
		freeap(&ap);
		freesap(&sap);
		logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "Error setting up network.\n");
		exit(EXIT_FAILURE);
	}

	struct pollfd pollfds[2];
	/* Setting up stuff for Polling */
	pollfds[0].fd = sockfd;    /* incoming new connections */
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	pollfds[1].fd = ap.sigfd; /* communication with signalfd */
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;
	
	/* Main Server Loop */
	while (1){ 
		int pollret = poll(pollfds, 2, -1);

		if (pollret < 0 || pollret == 0) {
			if (errno == EINTR) continue; /* Signals */
			logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "Polling Error.\n");
			shellReturn = EXIT_FAILURE;
			break;
		}

		if(pollfds[0].revents & POLLIN) {    /* incoming client */
			logmsg(ap.semid, ap.logfd, LOGLEVEL_VERBOSE, "New client.\n");

			socklen_t iplen = sizeof (ap.comip);
			ap.comfd = accept( sockfd, &(ap.comip), &iplen );
			ap.comport = getPort(&ap.comip);

			switch ((forkret = fork())){
				/* BUGBUG error handling? */
				case -1:
					close(ap.comfd);
					logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL,
							"Error forking off a new client connection\n");
					shellReturn = EXIT_FAILURE;
					break;
				case 0: /* I'm in the child */
					close(sockfd);
					close(ap.sigfd);
					logmsg(ap.semid, ap.logfd, LOGLEVEL_WARN, "Child\n");
					/* fetch old mask, cf. initap!*/
					if (sigprocmask(0, NULL, &mask) || 
							(ap.sigfd = getSigfd(&mask)) <= 0 ){
						fputs("error getting a new signal fd for the fork\n", stderr);
						_exit(EXIT_FAILURE);
					} else {
						aap.sap = &sap;
						comret = comfork(&ap, &aap);
						/* Do i need to do more */
						_exit(comret);
					}
				default: /* Parent branch */
					logmsg(ap.semid, ap.logfd, LOGLEVEL_WARN, "Parent\n");
					close(ap.comfd);
			}
		} else if(pollfds[1].revents & POLLIN) { /* incoming signal */
			SRret = read(ap.sigfd, &fdsi, sizeof(struct signalfd_siginfo));
			if (SRret != sizeof(struct signalfd_siginfo)){
				fprintf(stderr, "signalfd returned something broken");
				shellReturn = EXIT_FAILURE;
			}
			switch(fdsi.ssi_signo){
				case SIGINT:
				case SIGQUIT:
					fprintf(stderr,"Yeah i found %s",(fdsi.ssi_signo==SIGINT) ?"SIGINT": "SIGQUIT");
					shellReturn = EXIT_SUCCESS;
					break;
				case SIGCHLD:
					logmsg(ap.semid, ap.logfd, LOGLEVEL_VERBOSE, 
							"Child %d quit with status %d\n", fdsi.ssi_pid, fdsi.ssi_status);
					break;
				default:
					logmsg(ap.semid, ap.logfd, LOGLEVEL_WARN,
							"Encountered unknown signal on sigfd %d", fdsi.ssi_signo);
			}
		} else { /* Somethings broken with poll */
			fprintf(stderr, "Dunno what to do with this poll");
			shellReturn = EXIT_FAILURE;
		}
		if (shellReturn !=EXIT_NO) break;
	}

	fputs("Closing down",stderr);
	close(sockfd);
	freeap(&ap);
	freesap(&sap);
	exit(shellReturn);
}
