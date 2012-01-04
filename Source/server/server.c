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
#include <sys/sem.h>
#include "config.h"
#include "logger.h"
#include "util.h"
#include "consoler.h"
#include "protocol.h"
#include "util_sem_defines.h"

/***** Setup CONFIG *****/
int initConf(char * conffilename, struct config *conf, char error[256]){
	int conffd = open(conffilename,O_RDONLY);
	if ( conffd == -1) {
		sperror("Error opening config file:\n", error, 256);
		return -1;
	}
	confDefaults(conf);
	if ( parseConfig(conffd, conf) == -1 ){
		close(conffd);
		strncpy(error, "Your config has errors, please fix them", 256);
		return -1;
	}
	close(conffd);
	writeConfig (STDOUT_FILENO, conf);
	return 1;
}

int initsap (struct serverActionParameters *sap, char error[256]){
	return 1;
}
void freesap(struct serverActionParameters *sap){
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

	if (optind == (argc-1))
		conffilename = argv[optind];
	
	if (initConf(conffilename, &conf, error) == -1){
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	/***** Setup Logging  *****/
	logfilefd = open ( conf.logfile, O_APPEND|O_CREAT,
		 	S_IRUSR|S_IWUSR|S_IRGRP );

	if ( logfilefd == -1 ) {
		fputs("error opening log file, i won't create a path for you",stderr);
		exit(EXIT_FAILURE);
	}

	if (initap(&ap, error, logfilefd, numsems) == -1) {
		close(logfilefd);
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}
	close(logfilefd);

	if (initsap(&sap, error) == -1) {
		fputs(error,stderr);
		exit(EXIT_FAILURE);
	}

	/* Main Client Loop */
	while (1){ 
		if ( -1 == processIncomingData(&ap, (union additionalActionParameters *)&sap )) 
			break; 
	}
	freeap(&ap);
	freesap(&sap);
	exit(EXIT_SUCCESS);
}
