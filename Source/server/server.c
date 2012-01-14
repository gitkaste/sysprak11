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

	int pollret;
	struct pollfd pollfds[2];
  struct signalfd_siginfo fdsi;
  ssize_t SRret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if (ap->comfd == -1){
		perror("Error accepting a connection");
		return -3;
	}else{
		char buf[16];
		memset (&ap->comip, 0, addrlen);
		if (!getpeername (ap->comfd, (struct sockaddr *)&ap->comip, &addrlen) || addrlen != sizeof(struct sockaddr_in))
			if (inet_ntop(AF_INET, &ap->comip, buf, sizeof(buf)))
				fprintf(stderr, "Connection accepted from %s:%d\n", buf, ntohs(ap->comport));
		/* BUGBUG: Hier muss gepolled werden. */

		/* Setting up stuff for Polling */
		pollfds[0].fd = ap->comfd;    /* data incoming from client */
		pollfds[0].events = POLLIN;
		pollfds[0].revents = 0;
		pollfds[1].fd = ap->sigfd; /* communication with signalfd */
		pollfds[1].events = POLLIN;
		pollfds[1].revents = 0;

		while (1){
			pollret = poll(pollfds, 2, -1);

			if (pollret < 0 || pollret == 0) {
				if (errno == EINTR) continue; /* Signals */
				fprintf(stderr, "POLLING Error.\n");
				return -3;
			}

			if(pollfds[0].revents & POLLIN) {    /* incoming client */
				return (processIncomingData(ap, aap));
			} else if(pollfds[1].revents & POLLIN) { /* incoming signal */
				SRret = read(ap->sigfd, &fdsi, sizeof(struct signalfd_siginfo));
				if (SRret != sizeof(struct signalfd_siginfo)){
					fprintf(stderr, "signalfd returned something broken");
					return -3;
				}
				switch(fdsi.ssi_signo){
					case SIGINT:
					case SIGQUIT:
						return -2; /*?*/
				}
			} else {
				fprintf(stderr, "Dunno what to do with this poll");
				return -3;
			}
		}
	}
		return -2;
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
	sigset_t mask;
  struct signalfd_siginfo fdsi;
  ssize_t SRret;
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
	
	if ( confDefault(conffilename, &conf, error) == -1 ){
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	if (initap(&ap, error, &conf, numsems) == -1) {
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	if (initsap(&sap, error, &conf) == -1){
		freeap(&ap);
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	int sockfd;
	if ( (sockfd = createPassiveSocket(&(conf.port))) == -1){
		freeap(&ap);
		freesap(&sap);
		fputs("error setting up network", stderr);
		exit(EXIT_FAILURE);
	}

  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGQUIT);
  sigaddset(&mask, SIGCHLD);
  if ( (ap.sigfd = getSigfd(&mask)) < 0 ){
		fputs("error getting a signal fd", stderr);
		close(sockfd);
		freeap(&ap);
		freesap(&sap);
		exit(EXIT_FAILURE);
	}
	/* delete this, it's not needed in the forked off child */
	sigdelset(&mask, SIGCHLD);

	struct pollfd pollfds[2];
	/* Setting up stuff for Polling */
	pollfds[0].fd = sockfd;    /* incoming new connections */
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	pollfds[1].fd = ap.sigfd; /* communication with signalfd */
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;
	
	/* Main Client Loop */
	while (1){ 
		int pollret = poll(pollfds, 2, -1);

		if (pollret < 0 || pollret == 0) {
			if (errno == EINTR) continue; /* Signals */
			fprintf(stderr, "POLLING Error.\n");
			shellReturn = EXIT_FAILURE;
			break;
		}

		fputs("another poll round\t", stderr);
		if(pollfds[0].revents & POLLIN) {    /* incoming client */
			fputs("new client\n", stderr);
			ap.comport = sizeof(ap.comip);
			ap.comfd = accept(sockfd, (struct sockaddr *) &(ap.comip), (socklen_t *)&(ap.comport));
			switch ((forkret = fork())){
				case -1:
					close(ap.comfd);
					fputs("error forking off a new client connection", stderr);
					break;
				case 0: /* I'm in the child */
					close(sockfd);
					close(ap.sigfd);
					if ( (ap.sigfd = getSigfd(&mask)) <= 0 ){
						fputs("error getting a new signal fd for the fork\n", stderr);
						_exit(EXIT_FAILURE);
					} else {
						aap.sap = &sap;
						comret = comfork(&ap, &aap);
						/* Do i need to do more */
						_exit(comret);
					}
				default: /* Parent branch */
					close(ap.comfd);
			}
		} else if(pollfds[1].revents & POLLIN) { /* incoming signal */
			fputs("incoming signal\n", stderr);
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
							"Child %d quit with status %d", fdsi.ssi_pid, fdsi.ssi_status);
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
