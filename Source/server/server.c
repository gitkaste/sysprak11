#include <stdio.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "config.h"
#include "logger.h"
#include "util.h"
#include "consoler.h"
#include "protocol.h"

/* ****IF I HAVE TO CHECK FOR close (?) errors USE LABELS AND GOTO! */
int main (int argc, char * argv[]){
	struct config conf;
	/* in and out fds are seen as from the clients view point */
	int logfd[2];
	int logproc;
	int usedres=0; /*needs to be at least 256 big */
	struct actionParameters ap;
	struct serverActionParameters sap;

	/***** Setup CONFIG *****/
	if (argc < 2) {
		puts("please provide the name of the config file");
		exit(EXIT_FAILURE);
	}
	int conffd = open (argv[1],O_RDONLY);
	if ( conffd == -1) {
		perror("error opening config file:");
		exit(EXIT_FAILURE);
	}
	confDefaults(&conf);
	if (parseConfig(conffd, &conf) ==-1){
		close(conffd);
		puts("Your config has errors, please fix them");
		exit(EXIT_FAILURE);
	}
	close(conffd);
	writeConfig (STDOUT_FILENO,&conf);

	/***** setup LOGGER *****/
	/* check early - less rollback */
	int logfilefd = open (conf.logfile, 
			O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP);
	if ( logfilefd == -1) {
		perror("error opening log file");
		exit(EXIT_FAILURE);
	}
	usedres |= APRES_LOGFD;
	if (pipe(logfd) == -1) {
		close(logfilefd);
		perror("Error creating pipe");
		exit(EXIT_FAILURE);
	}
	switch(logproc = fork()){
	case -1: 
		close(logfilefd);
		close(logfd[0]);
		close(logfd[1]);
		perror("Error forking");
		exit(2);
	case 0: /* we are in the child */
		close(logfd[1]); /* writing end */
		logger(logfd[0], logfilefd);
		close(logfd[0]); /* reading end */
		close(logfilefd);
		exit(EXIT_SUCCESS);
	default: /* we are in the parent */
		close(logfd[0]);  /* reading end */
	}

	/* Main Client Loop */
	/* BUGBUG these could all fail!  */
	createBuf(&(ap.combuf),4096);
	createBuf(&(ap.comline),4096);
	createBuf(&(ap.comword),4096);

//	int ap->c2s = mkfifo("tmp/syprac2s",S_IROTH|S_IWOTH);
//	int ap->s2c = mkfifo("tmp/sypras2c",S_IROTH|S_IWOTH);

	while (1){ 
		processIncomingData(&ap, (union additionalActionParameters *)&sap);
	}
	close(logfd[1]); 
	exit(EXIT_SUCCESS);
}
