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

/* ****IF I HAVE TO CHECK FOR close (?) errors USE LABELS AND GOTO! */
int main (int argc, char * argv[]){
	struct config conf;
	/* in and out fds are seen as from the clients view point */
	int logfd[2], consoleinfd[2], consoleoutfd[2];
	int consoleproc, logproc;
	struct buffer msg;

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
	/* check early - less rollback */
	int logfilefd = open (conf.logfile, 
			O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP);
	if ( logfilefd == -1) {
		perror("error opening log file");
		exit(EXIT_FAILURE);
	}
	if (pipe(consoleinfd) == -1) {
		close(logfilefd);
		perror("Error creating pipe");
		exit(EXIT_FAILURE);
	}
	if (pipe(consoleoutfd) == -1) {
		close(logfilefd);
		close(consoleinfd[0]);
		close(consoleinfd[1]);
		perror("Error creating pipe");
		exit(EXIT_FAILURE);
	}
	if (pipe(logfd) == -1) {
		close(logfilefd);
		close(consoleinfd[0]);
		close(consoleinfd[1]);
		close(consoleoutfd[0]);
		close(consoleoutfd[1]);
		perror("Error creating pipe");
		exit(EXIT_FAILURE);
	}


	/***** setup CONSOLER *****/
	switch(consoleproc = fork()){
	case -1: 
		perror("Error forking");
		close(logfilefd);
		close(logfd[0]);
		close(logfd[1]);
		close(consoleinfd[0]);
		close(consoleinfd[1]);
		close(consoleoutfd[0]);
		close(consoleoutfd[1]);
		perror("Error creating pipe");
		exit(EXIT_FAILURE);
		exit(2);
	case 0: /* we are in the child - Sick */
		close(logfd[0]);
		close(logfd[1]);
		close(consoleoutfd[1]); /* writing end of pipe pushing stdout*/
		close(consoleinfd[0]); /* reading end of pipe feeding stdin*/
		consoler(consoleoutfd[0], consoleinfd[1]);
		close(consoleoutfd[0]);
		close(consoleinfd[1]);
		exit(EXIT_SUCCESS);
	default: /* we are in the parent - Hope for Total she is female :P */
		close(consoleoutfd[0]); /* reading end of the pipe going from client to stdout*/
		close(consoleinfd[1]); /* writing end of the pipe feeding us stdin */
	}

	/***** setup LOGGER *****/
	switch(logproc = fork()){
	case -1: 
		close(logfilefd);
		close(logfd[0]);
		close(logfd[1]);
		close(consoleinfd[0]);
		close(consoleinfd[1]);
		close(consoleoutfd[0]);
		close(consoleoutfd[1]);
		perror("Error forking");
		exit(2);
	case 0: /* we are in the child */
		close(consoleoutfd[1]); 
		close(consoleinfd[0]); 
		close(logfd[1]); /* writing end */
		logger(logfd[0], logfilefd);
		close(logfd[0]); /* reading end */
		close(logfilefd);
		exit(EXIT_SUCCESS);
	default: /* we are in the parent */
		close(logfd[0]);  /* reading end */
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
	close(consoleoutfd[1]); 
	close(consoleinfd[0]); 
	close(logfd[1]); 
	if ( waitpid(consoleproc, NULL, 0) < 0)
		puts("Did Burpy: Unclean Shutdown - Sorry");
	exit(EXIT_SUCCESS);
}
