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
#include "config.h"
#include "logger.h"
#include "util.h"
#include "consoler.h"
#include "protocol.h"
#include "connection.h"

int initsap (struct serverActionParameters *sap, char error[256], struct config * conf){
	int size = conf->shm_size;
	if ( (sap->shmid_filelist = shmget(IPC_PRIVATE, size, 0)) == -1)
			return -1;
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

int main (int argc, char * argv[]){
	char error[256];
	/* in and out fds are seen as from the clients view point */
	int logfilefd;
	struct config conf;
	char * conffilename = "server.conf";
	struct actionParameters ap;
	struct serverActionParameters sap;
	const int numsems = 2;
	int opt;

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

	/***** Setup Logging  *****/
	logfilefd = open ( conf.logfile, O_APPEND|O_CREAT|O_NONBLOCK,
		 	S_IRUSR|S_IWUSR|S_IRGRP );

	if ( logfilefd == -1 ) {
		fputs("error opening log file, i won't create a path for you", stderr);
		exit(EXIT_FAILURE);
	}

	if (initap(&ap, error, logfilefd, numsems) == -1) {
		close(logfilefd);
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}
	close(logfilefd);

	if (initsap(&sap, error, &conf) == -1){
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	int sockfd;
	if ( (sockfd = createPassiveSocket(&(conf.port))) == -1){
		fputs("error setting up network", stderr);
		exit(EXIT_FAILURE);
	}
	
	/* Main Client Loop */
	while (1){ 
		ap.comport = sizeof(ap.comip);
		ap.comfd = accept(sockfd, (struct sockaddr *) &(ap.comip), (socklen_t *)&(ap.comport));
		if (ap.comfd == -1)
			perror("Error accepting a connection");
		else{
			char buf[16];
			fprintf(stderr, "Connection accepted from %s:%d\n",
					inet_ntop(AF_INET, &ap.comip, buf, sizeof(buf)), 
					ntohs(ap.comport));
			if ( -1 == processIncomingData(&ap, (union additionalActionParameters *)&sap )){
				close(ap.comfd);
				break; 
			}
		}
	}
	close(sockfd);
	freeap(&ap);
	freesap(&sap);
	exit(EXIT_SUCCESS);
}
