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
		sperror("Error opening config file", error, 256);
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

int main (int argc, char * argv[]){
	struct config conf;
	char error[256];
	/* in and out fds are seen as from the clients view point */
	int logfilefd;
	struct actionParameters ap;
	struct serverActionParameters sap;

	if (argc < 2) {
		puts("please provide the name of the config file");
		exit(EXIT_FAILURE);
	}
	if (initConf(argv[1], &conf, error) == -1){
		puts(error);
		exit(EXIT_FAILURE);
	}
	/***** Setup Logging  *****/
	logfilefd = open ( conf.logfile, O_APPEND|O_CREAT,
		 	S_IRUSR|S_IWUSR|S_IRGRP );

	if ( logfilefd == -1 ) {
		puts("error opening log file, i won't create a path for you");
		exit(EXIT_FAILURE);
	}

	initap(&ap, error, logfilefd, 2);

	/* Main Client Loop */
	while (1){ if ( -1 == processIncomingData(&ap, (union additionalActionParameters *)&sap )) break; }
	freeap(&ap);
	exit(EXIT_SUCCESS);
}
