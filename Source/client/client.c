#define _GNU_SOURCE
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "config.h"
#include "connection.h"
#include "logger.h"
#include "util.h"
#include "protocol.h"
#include "consoler.h"

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

	if (pipe2(consoleinfd, O_NONBLOCK) == -1 ||
		 	pipe2(consoleoutfd, O_NONBLOCK) == -1) {
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
	logfilefd = open ( conf.logfile, O_APPEND|O_CREAT|O_NONBLOCK,
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

	ap.comip = conf.ip;
	ap.comport = conf.port;
	if ( (ap.comfd = connectSocket(&(conf.ip), conf.port))  == -1 ){
		freeap(&ap);
		freecap(&cap);
		fputs("error setting up network connection", stderr);
		exit(EXIT_FAILURE);
	}
	ap.usedres |= APRES_COMFD;

		/* Main Client Loop */
	createBuf(&msg,4096);

	struct pollfd pollfds[2];
	/* Setting up stuff for Polling */
	pollfds[0].fd = ap.comfd; /* communication with server */
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	pollfds[1].fd = cap.infd; /* communication with user */
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;
		
	ssize_t s;
	while (1){ 
		/* should i poll infinitely or a discrete time? */
		int pollret = poll(pollfds, 2, -1);

		if(pollret < 0 || pollret == 0) {
			if(errno == EINTR) continue; /* Signals */
			fprintf(stderr, "POLLING Error.\n");
			break;
		}

		if(pollfds[0].revents & POLLIN) {
			s = readToBuf(ap.comfd, &msg);
			if (s  == -2) continue;
			if (s == -1){
				puts("arg, shouldn't happen");
				break;
			}
			consolemsg(0, cap.outfd, "wc: %d\n", strlen((char *)msg.buf)-1);
		} else if(pollfds[1].revents & POLLIN) {
			s = readToBuf(cap.infd, &msg);
			if (s  == -2) continue;
			if (s == -1){
				puts("arg, shouldn't happen");
				break;
			}
		} else if (pollfds[0].revents & POLLHUP) {
			/*  server closed connection? */
			break;
		} else if (pollfds[1].revents & POLLHUP) {
			/*  user closed connection - how? */
			break;
		} else {
			fprintf(stderr, "Dunno what to do with this poll");
			break;
		}
		/*logmsg(0, ap.logfd, LOGLEVEL_VERBOSE, "%s", msg);*/
	}
	puts("end of loop");

	freeBuf(&msg);
	freeap(&ap);
	freecap(&cap);
	if ( waitpid(cap.conpid, NULL, 0) < 0)
		puts("Did Burpy: Unclean Shutdown - Sorry");
	exit(EXIT_SUCCESS);
}
