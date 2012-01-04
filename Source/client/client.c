#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "config.h"
#include "logger.h"
#include "util.h"
#include "protocol.h"
#include "consoler.h"

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
		strncpy(error, "Your config has errors, please fix them\n", 256);
		return -1;
	}
	close(conffd);
	writeConfig (STDOUT_FILENO, conf);
	return 1;
}

void freecap(struct clientActionParameters* cap){
	if (cap->usedres & CAPRES_OUTFD){
		close(cap->outfd);
		close(cap->infd);
	}
	if (cap->usedres & CAPRES_RESULTSSHMID);
	if (cap->usedres & CAPRES_RESULTS)      freeArray(cap->results);
	if (cap->usedres & CAPRES_SERVERFD)     close (cap->serverfd);
	if (cap->usedres & CAPRES_CPA)          freeArray(cap->cpa);
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
		cap->usedres &= CAPRES_OUTFD;
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
		cap->usedres &= CAPRES_OUTFD;
	}
	return 1;
}

void print_usage(char * prog){
	fprintf(stderr, 
		"Usage: %s [-i server ip] [-p port] [[-c] config filename]\n"
"Config file is assumed to be ./server.conf if the value isn't given.\n"
"Server IP and port constitute the address of the server to connect to.\n",
		prog);
}

int main (int argc, char * argv[]){
	int logfilefd;
	struct config conf;
	char * conffilename="client.conf";
  struct in_addr server_ip;
	int server_port = 0;
	const int numsems = 3;
	struct actionParameters ap;
	struct clientActionParameters cap;
	struct buffer msg;
	char error[256];
	int opt;

	server_ip.s_addr = 0;

	/*  Option Processing */
  while ((opt = getopt(argc, argv, "i:p:c:")) != -1) {
		switch (opt) {
			case 'c':
				conffilename = optarg;
        break;
      case 'p':
				errno =0;
				server_port = strtol(optarg,NULL,10);
				if (errno == EINVAL || server_port <= 0 || server_port > 65536){
					fprintf(stderr, "%s is not a valid port, use 0-65536\n",optarg);
				  exit(EXIT_FAILURE);
				}
        break;
			case 'i':
				if (!inet_aton(optarg, &server_ip)){
				fprintf(stderr, "%s is not a valid ipv4\n",optarg);
			  exit(EXIT_FAILURE);
			}
				break;
      default: /*  '?' */
				print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

	if (optind == (argc -1))
		conffilename = argv[optind];

	/* config Parser */
	if (initConf(conffilename, &conf, error) == -1){
		fputs(error, stderr);
		exit(EXIT_FAILURE);
	}
	/* Possibly override with values from command line */
	if (server_port)
		conf.port = server_port;
	if (server_ip.s_addr)
		conf.ip.s_addr = server_ip.s_addr;

	/***** Setup Logging *****/
	logfilefd = open ( conf.logfile, O_APPEND|O_CREAT,
		 	S_IRUSR|S_IWUSR|S_IRGRP );

	if ( logfilefd == -1 ) {
		fputs("error opening log file, i won't create a path for you", stderr);
		exit(EXIT_FAILURE);
	}

	if (initap(&ap, error, logfilefd, numsems) == -1) {
		close(logfilefd);
		fputs(error, stderr);
		exit(EXIT_FAILURE);
	}
	close(logfilefd);

	if (initcap(&cap, error) == -1){
		freeap(&ap);
		fputs(error, stderr);
		exit(EXIT_FAILURE);
	}
		/* Main Client Loop */
	createBuf(&msg,4096);

	ssize_t s;
	while (1){ 
		s = readToBuf(cap.infd, &msg);
 	 	if (s  == -2 ) continue;
		if (s == -1 ){
			puts("arg");
			break;
		}
		logmsg(0, ap.logfd, LOGLEVEL_VERBOSE, "%s", msg);
		/* subtract one from the strlen because newlines probably shouldn't count */
		consolemsg(0, cap.outfd, "wc: %d\n", strlen((char *)msg.buf)-1);
		puts("end of loop");
	}

	freeBuf(&msg);
	freeap(&ap);
	freecap(&cap);
	if ( waitpid(cap.conpid, NULL, 0) < 0)
		puts("Did Burpy: Unclean Shutdown - Sorry");
	exit(EXIT_SUCCESS);
	return 0; /* Make the compiler happy */
}
