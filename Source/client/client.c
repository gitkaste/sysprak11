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
#include "tokenizer.h"
#include "logger.h"
#include "util.h"
#include "protocol.h"
#include "client_protocol.h"
#include "consoler.h"

void freecap(struct clientActionParameters* cap){
	if (cap->usedres & CAPRES_OUTFD){
		close(cap->outfd);
		close(cap->infd);
	}
	if (cap->usedres & CAPRES_RESULTSSHMID) shmDelete(cap->shmid_results);
	if (cap->usedres & CAPRES_RESULTS)      freeArray(cap->results);
	if (cap->usedres & CAPRES_SERVERFD)     close (cap->serverfd);
	if (cap->usedres & CAPRES_CPA)          freeArray(cap->cpa);
}

int initcap (struct clientActionParameters* cap, char error[256], struct config * conf){
	int consoleinfd[2], consoleoutfd[2];
	int size = conf->shm_size;
	if ( (cap->shmid_results = shmCreate(size)) == -1)
			return -1;
	//fprintf(stderr, "shmid %d\n", cap->shmid_results);
	cap->usedres = CAPRES_RESULTSSHMID;

	size -= sizeof(struct array);
	if (!(cap->results = initArray(sizeof(struct flEntry), size, cap->shmid_results))){
		shmDelete(cap->shmid_results);
		return -1;	
	}
	cap->usedres |= CAPRES_RESULTS;

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
		fprintf(stderr,"consoler shutting down\n");
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

int processCode(int code, struct actionParameters *ap, 
		union additionalActionParameters *aap){
	switch (code){
		case REP_OK: 
			return consolemsg(ap->semid, aap->cap->outfd, "OK");
		case REP_TEXT: 
			return consolemsg(ap->semid, aap->cap->outfd, "%s", ap->comline.buf, 
					ap->comline.buflen);
		case REP_COMMAND: 
			return processCommand(ap, aap);
		case REP_WARN: 
			return consolemsg(ap->semid, aap->cap->outfd, "%s", ap->comline.buf, 
					ap->comline.buflen);
		case REP_FATAL: 
			return consolemsg(ap->semid, aap->cap->outfd, "%s", ap->comline.buf, 
					ap->comline.buflen);
			return -1;
		default:
			fprintf(stderr,"Unknown class of Code found in protocol");
			return -1;
	}
}

int processServerReply(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	int gtfsret, pcret;

	switch (readToBuf(ap->comfd, &(ap->combuf))){
		case -2:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"FATAL (processServerReply): read error.\n");
			return -1;
		case -1:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, 
					"WARNING (processServerReply): EINTR/EAGAIN.\n");
			return 1;
		case 0:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
					"(processIncomingData) ap->comfd closed.\n");
			return 1;
	}
	/* Split into lines */
	while((gtfsret = getTokenFromStreamBuffer( &ap->combuf,
			&ap->comline, "\r\n", "\n", (char *)NULL)) > 0) {
		fprintf(stderr,"read: %s", ap->comline.buf);
		/* Split into tokens */
		switch (getTokenFromBuffer( &ap->comline, &ap->comword, " ", "\t", NULL )) {
		case -1:
			logmsg( ap->semid, ap->logfd, LOGLEVEL_FATAL, 
				"FATAL: getTokenFromBuffer() failed in processServerReply.\n" );
			return -1;
		case 0:	
			return 2; /* command was empty */
		case 1:
			errno=0;
			/* Split into tokens */
			int code = strtol( (char *) ap->comword.buf, NULL, 10);
			if ( errno || code<REP_OK || code>REP_FATAL || code % 100 ) return -1;
			if ( (pcret = processCode(code, ap, aap)) <= 0 ) return pcret;
		default:
			logmsg( ap->semid, ap->logfd, LOGLEVEL_WARN,
				"WARN: getTokenFromBuffer() gave unknown return.\n" );
			return -1;
		}
	}
	if(gtfsret < 0) { /* token buffer too small */
		return gtfsret;
	}
	/* this is the gtfsret=0 branch */
	return 1;
}

int main (int argc, char * argv[]){
	struct config conf;
	char * conffilename="client.conf";
  struct in_addr server_ip;
	int server_port = 0;
	const int numsems = 3;
	struct actionParameters ap;
	struct clientActionParameters cap;
	union additionalActionParameters aap;
	aap.cap = &cap;
	struct buffer msg;
	char error[256];
	int opt;
#define EXIT_NO (EXIT_FAILURE * 2+3)
	int pSRret, shellReturn=EXIT_NO;

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

	if (initap(&ap, error, &conf, numsems) == -1) {
		fputs(error, stderr);
		exit(EXIT_FAILURE);
	}
	initializeClientProtocol(&ap);

	if (initcap(&cap, error, &conf) == -1){
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

		if (pollret < 0 || pollret == 0) {
			if (errno == EINTR) continue; /* Signals */
			fprintf(stderr, "POLLING Error.\n");
			shellReturn = EXIT_FAILURE;
			break;
		}

		if(pollfds[0].revents & POLLIN) {    /* Server greets us */
		 	switch (pSRret = processServerReply(&ap,&aap)){
				case -3:
				case -1:
					shellReturn = EXIT_FAILURE;
					break;
				case -2:
				case 0: 
					shellReturn = EXIT_SUCCESS;
					break;
				default: 
					if (pSRret ==1) continue;
					fprintf(stderr, "WTF did processServerReply just return: %d?\n",pSRret);
					shellReturn = EXIT_FAILURE;
			}
		} else if(pollfds[1].revents & POLLIN) { /* User commands us */
			s = readToBuf(cap.infd, &msg);
			if (s  == -2) continue;
			if (s == -1){
				puts("arg, shouldn't happen");
				break;
			}
		} else if (pollfds[0].revents & POLLHUP) {
			/*  server closed connection? */
			shellReturn = EXIT_SUCCESS;
		} else if (pollfds[1].revents & POLLHUP) {
			/*  user closed connection - how? */
			shellReturn = EXIT_SUCCESS;
		} else {
			fprintf(stderr, "Dunno what to do with this poll\n");
			shellReturn = EXIT_FAILURE;
		}
		if (shellReturn != EXIT_NO) break;
		/*logmsg(0, ap.logfd, LOGLEVEL_VERBOSE, "%s", msg);*/
	}
	puts("end of loop");

	freeBuf(&msg);
	freeap(&ap);
	freecap(&cap);
	if ( waitpid(cap.conpid, NULL, 0) < 0){
		puts("Did Burpy: Unclean Shutdown - Sorry\n");
		exit(shellReturn);
	}

	exit(shellReturn);
}
