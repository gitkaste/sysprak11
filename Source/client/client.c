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

int initcap (struct clientActionParameters* cap, char error[256]){
	int consoleinfd[2], consoleoutfd[2];

	if (pipe(consoleinfd) == -1 || pipe(consoleoutfd) == -1) {
		sperror("Error creating pipe", error, 256);
		return -1;
	}

	/***** setup CONSOLER *****/
	switch(cap->conpid = fork()){
	case -1: 
		close(consoleoutfd[0]);
		close(consoleoutfd[1]);
		close(consoleinfd[0]);
		close(consoleinfd[1]);
		sperror("Error forking", error, 256);
		return -1;
	case 0: /* we are in the child - Sick */
		close(consoleoutfd[1]); /* writing end of pipe pushing to stdout*/
		close(consoleinfd[0]);  /* reading end of pipe fed from stdin*/
		cap->infd = consoleinfd[1];
		cap->outfd = consoleoutfd[0];
		cap->usedres &= APRES;
		/* Notation in the consoler function is reversed! */
		/* outfd: Prog->Consoler->STDOUT*/
		/* infd: STDIN->Consoler->Prog*/
		consoler(cap->outfd, cap->infd);
		freecap(cap);
		exit(EXIT_SUCCESS);
	default: /* we are in the parent - Hope for Total she is female :P */
		close(consoleoutfd[0]); /* reading end of the pipe going from client to stdout*/
		close(consoleinfd[1]); /* writing end of the pipe feeding us stdin */
		cap->infd = consoleinfd[0];
		cap->outfd = consoleoutfd[1];
		cap->usedres &= APRES;
	}
}

void freecap(struct clientActionParameters* cap){
	if (usedres & CAPRES_OUTFD)        close(cap->outfd);
	if (usedres & CAPRES_RESULTSSHMID);
	if (usedres & CAPRES_RESULTS)      freeArray(cap->results);
	if (usedres & CAPRES_SERVERFD)     close (cap->serverfd);
	if (usedres & CAPRES_CPA)          freeArray(cap->cpa);)
}

int main (int argc, char * argv[]){
	struct config conf;
	const int numsems = 3;
	struct buffer msg;

	/***** Setup CONFIG *****/
	if (argc < 2) {
		puts("please provide the name of the config file");
		exit(EXIT_FAILURE);
	}

	/***** Setup Logging  *****/
	logfilefd = open ( conf.logfile, O_APPEND|O_CREAT,
		 	S_IRUSR|S_IWUSR|S_IRGRP );

	if ( logfilefd == -1 ) {
		puts("error opening log file, i won't create a path for you");
		exit(EXIT_FAILURE);
	}

	if (initap(&ap, error, logfilefd, numsems) == -1) {
		close(logfilefd);
		puts(error);
		exit(EXIT_FAILURE);
	}
	close(logfilefd);

	if (initcap(&cap, error) == -1){
		puts(error);
		exit(EXIT_FAILURE);
	}
		/* Main Client Loop */
	createBuf(&msg,4096);

	ssize_t s;
	while (1){ 
		s = readToBuf(consoleinfd[0], &msg);
 	 	if (s  == -2 ) continue;
		if (s == -1 ){
			puts("arg");
			break;
		}
		logmsg(0, logfd[1], LOGLEVEL_VERBOSE, "%s", msg);
		/* subtract one from the strlen because newlines probably shouldn't count */
		consolemsg(0, consoleoutfd[1], "wc: %d\n", strlen((char *)msg.buf)-1);
		puts("end of loop");
	}

	freeBuf(&msg);
	freeap(&ap);
	freecap(&cap);
	if ( waitpid(consoleproc, NULL, 0) < 0)
		puts("Did Burpy: Unclean Shutdown - Sorry");
	exit(EXIT_SUCCESS);

error1:
	freeap(&ap);
	exit(EXIT_FAILURE);
}
