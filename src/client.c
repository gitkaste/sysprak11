#define _GNU_SOURCE
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "config.h"
#include "connection.h"
#include "tokenizer.h"
#include "logger.h"
#include "util.h"
#include "protocol.h"
#include "stdin_protocol.h"
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

int initcap (struct clientActionParameters* cap, char error[256]){
	int toConsoler[2], fromConsoler[2];
	int size = conf->shm_size;

	if ( (cap->shmid_results = shmCreate(size)) == -1)
			return -1;
	cap->usedres = CAPRES_RESULTSSHMID;

	size -= sizeof(struct array);
	if (!(cap->results = initArray(sizeof(struct flEntry), size, cap->shmid_results)))
		goto error;
	cap->usedres |= CAPRES_RESULTS;

	if (!(cap->cpa = initArray(sizeof(struct processChild), 
					64*sizeof(struct processChild), -1)))
		goto error;
	cap->usedres |= CAPRES_CPA;

	if (pipe2(toConsoler, O_NONBLOCK) == -1 ||
		 	pipe2(fromConsoler, O_NONBLOCK) == -1) {
		sperror("Error creating pipe", error, 256);
		goto error;
	}

	/***** setup CONSOLER *****/
	switch(cap->conpid = fork()){
	case -1: 
		close(fromConsoler[0]);
		close(fromConsoler[1]);
		close(toConsoler[0]);
		close(toConsoler[1]);
		sperror("Error forking", error, 256);
		goto error;
	case 0: /* we are in the child - Sick */
		close(toConsoler[1]); /* writing end of pipe pushing to stdout*/
		close(fromConsoler[0]);  /* reading end of pipe fed from stdin*/
		cap->infd = fromConsoler[1];
		cap->outfd = toConsoler[0];
		cap->usedres |= CAPRES_OUTFD;
		/* Notation in the consoler function is reversed! */
		/* outfd: Prog->Consoler->STDOUT*/
		/* infd: STDIN->Consoler->Prog*/
		if (consoler(cap->outfd, cap->infd) == -1)
			goto error;
		freecap(cap);
		exit(EXIT_SUCCESS);
	default: /* we are in the parent - Hope for Total she is female :P */
		close(toConsoler[0]); /* reading end of the pipe going from client to stdout*/
		close(fromConsoler[1]); /* writing end of the pipe feeding us stdin */
		cap->outfd = toConsoler[1];
		cap->infd = fromConsoler[0];
		cap->usedres |= CAPRES_OUTFD;
		/* i for interaction */
		addChildProcess(cap->cpa, 'i', cap->conpid);
	}
	return 1;

error:
	freecap(cap);
	return -1;	
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
	int ret;
	switch (code){
		case REP_OK:
			return consolemsg(ap->semid, aap->cap->outfd, "OK");
		case REP_TEXT:
			return consolemsg(ap->semid, aap->cap->outfd, "%s\n", ap->comline.buf);
		case REP_COMMAND:
			initializeClientProtocol(ap);
			ret = processCommand(ap, aap);
			initializeStdinProtocol(ap);
			return ret;
		case REP_WARN:
			return consolemsg(ap->semid, aap->cap->outfd, "%s\n", ap->comline.buf);
		case REP_FATAL:
			return consolemsg(ap->semid, aap->cap->outfd, "%s\n", ap->comline.buf);
			return -1;
		default:
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "FATAL: "
			"Unknown class of Code found in protocol.\n");
			return -1;
	}
}

int processServerReply(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	int gtfsret, pcret;

	switch (readToBuf(aap->cap->serverfd, &(ap->combuf))){
		case -1:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"FATAL (processServerReply): read error.\n");
			return -1;
		case -2:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, 
					"WARNING (processServerReply): EINTR/EAGAIN.\n");
			return 1;
		case 0:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
					"(processServerReply) ap->serverfd closed.\n");
			return -1;
	}
	/* Split into lines */
	while((gtfsret = getTokenFromStreamBuffer( &ap->combuf,
			&ap->comline, "\r\n", "\n", (char *)NULL)) > 0) {
		//fprintf(stderr,"read: %s", ap->comline.buf);
		/* Split into tokens */
		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE,
		"SRVRECV: '%.*s'\n", ap->comline.buflen, ap->comline.buf);
		switch (getTokenFromBuffer( &ap->comline, &ap->comword, " ", "\t", NULL )) {
		case -1:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
				"FATAL: getTokenFromBuffer() failed in processServerReply.\n" );
			return -1;
		case 0:	
			return 2; /* command was empty */
		case 1:
			errno=0;
			/* Split into tokens */
			int code = my_strtol( (char *) ap->comword.buf );
			if ( errno || code<REP_OK || code>REP_FATAL || code % 100 ) return -1;
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
				"(procssServerReply) got command '%s'\n", ap->comline.buf);
			if ( (pcret = processCode(code, ap, aap)) <= 0 ) return pcret;
			break;
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
	char * conffilename="client.conf";
	char serveripstr[1024] = "";
	int server_port = 0;
	char portstr[8];
	const int numsems = 3;
	struct actionParameters ap;
	struct clientActionParameters cap;
	union additionalActionParameters aap;
	aap.cap = &cap;
	struct buffer msg;
	char error[256];
	uint16_t passport;
	int opt, passsock, s;
#define EXIT_NO (EXIT_FAILURE * 2+3)
	int huRet, pSRret, SRret, shellReturn=EXIT_NO, gtfsRet, rtbRet;
	pid_t uploadpid;
	socklen_t socklen = sizeof(struct sockaddr_storage);
	struct sockaddr_storage peer_addr;
  struct signalfd_siginfo fdsi;
  struct pollfd pollfds[3];

	/*  Option Processing */
  while ((opt = getopt(argc, argv, "i:p:c:")) != -1) {
		switch (opt) {
			case 'c':
				conffilename = strdup(optarg);
        break;
      case 'p':
				server_port = (uint16_t) my_strtol(optarg);
				if (errno || server_port > 645536){
					fprintf(stderr, "%s is not a valid port, use 0-65536\n",optarg);
				  exit(EXIT_FAILURE);
				}
        break;
			case 'i':
				strncpy(serveripstr, optarg, 1024);
				break;
      default: /*  '?' */
				print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

	if (optind == (argc -1))
		conffilename = argv[optind];

	/* config Parser */
	if (initConf(conffilename,  error) == -1){
		fputs(error, stderr);
		exit(EXIT_FAILURE);
	}
	//writeConfig(1);
	/* Possibly override with values from command line */
	if ( *serveripstr != '\0' ){
		snprintf(portstr, 8, "%d", conf->port);
		if ( (s = parseIP(serveripstr, (struct sockaddr *)&conf->ip, portstr, 
						conf->forceIpVersion)) ){
			fprintf(stderr,"(main) serverip isn't a valid ip address%s\n",
		gai_strerror(s));
			goto error;
		} 
	if (server_port)
		conf->port = server_port;
	  setPort((struct sockaddr *)&conf->ip, conf->port);
	}

	if (initap(&ap, error, numsems) == -1) {
		fputs(error, stderr);
		shellReturn = EXIT_FAILURE;
		goto error;
	}
	initializeStdinProtocol(&ap);

	if (initcap(&cap, error) == -1){
		logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "%s", error);
		shellReturn = EXIT_FAILURE;
		goto error;
	}

	/* connecting to server. */
	if ( ( ap.comfd = cap.serverfd = connectSocket((struct sockaddr *)&conf->ip, conf->port))  == -1 ){
		logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "error connecting to server\n");
		shellReturn = EXIT_FAILURE;
		goto error;
	}

	socklen = sizeof(ap.comip);
	if (getpeername(ap.comfd, (struct sockaddr *) &ap.comip, &socklen) == -1){
		logmsg(ap.semid, ap.logfd, LOGLEVEL_WARN,
				"Unable to retrieve information on incoming client connection.\n");
	}
	ap.usedres |= APRES_COMFD;

	/* create a passive port to accept client connections. */
	passport =  0;
	if ( ( passsock = createPassiveSocket(&passport)) == -1){
		logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, 
				"Error setting up network connection\n");
		shellReturn = EXIT_FAILURE;
		goto error;
	}

	/* inform the server of our passive port */
	//consolemsg(ap.semid, cap.outfd, "sending PORT %d\n", passport);
	if (-1 == writef( cap.serverfd, "PORT %d\n", passport)){
		logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "Couldn't send my port to server\n");
		perror("");
		shellReturn = EXIT_FAILURE;
		goto error;
	}	

	/* send our file list */
	if (-1 == writef( cap.serverfd, "FILELIST\n" )){
		perror("Coulnd't send my filelist to server");
		shellReturn = EXIT_FAILURE;
		goto error;
	}
  //consolemsg(ap.semid, aap.cap->outfd, "successfully connected to server\n");
	logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL, "successfully started up\n");
		/* Main Client Loop */
	createBuf(&msg,4096);

	/* Setting up stuff for Polling */
	pollfds[0].fd = cap.serverfd; /* communication with server */
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	pollfds[1].fd = cap.infd; /* communication with user */

	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;
	pollfds[2].fd = passsock; /* communication with other client */
	pollfds[2].events = POLLIN;
	pollfds[2].revents = 0;
	pollfds[3].fd = ap.sigfd; /* signal handling */
	pollfds[3].events = POLLIN;
	pollfds[3].revents = 0;

	while (1) { 
		/* should i poll infinitely or a discrete time? */
		if (	poll(pollfds, 2, -1) <= 0) {
			if ( errno == EINTR ) continue; /* Signals */

			fprintf(stderr, "POLLING Error.\n");
			shellReturn = EXIT_FAILURE;
			break;
		}

		if(pollfds[0].revents & POLLIN) { /* Server greets us */
			pSRret = processServerReply(&ap,&aap);
		 	switch (pSRret){
				case 0: 
					continue;
				case -1:
					shellReturn = EXIT_FAILURE;
					break;
				case -2:
					_exit(EXIT_SUCCESS);
				case -3:
					_exit(EXIT_FAILURE);
				default: 
					if (pSRret ==1) continue;
					consolemsg(ap.semid, cap.outfd,
							"WTF did processServerReply just return:%d?\n",pSRret);
					shellReturn = EXIT_FAILURE;
			}
			break;
		} else if(pollfds[1].revents & POLLIN) { /* User commands us */
			if ( (rtbRet = readToBuf(cap.infd, &ap.combuf)) == -1 ){
					logmsg(ap.semid, ap.logfd, LOGLEVEL_FATAL,
						"FATAL (main loop, user incoming): read()-error from fd.\n");
					shellReturn = EXIT_FAILURE;
					break;
			} else if(rtbRet == 0){
					shellReturn = EXIT_SUCCESS;
					logmsg(ap.semid, ap.logfd, LOGLEVEL_VERBOSE,
						"FATAL (main loop, user incoming): fd closed from other side");
					break;
			}
			while ((gtfsRet = getTokenFromStreamBuffer(&ap.combuf,
				&ap.comline, "\r\n", "\n", (char *)NULL)) > 0) {
				switch(rtbRet = processCommand(&ap, &aap)){
					case -1:
						shellReturn = EXIT_FAILURE;
						break;
					case 0:
						shellReturn = EXIT_SUCCESS;
						break;
					case 1:
					case 2:
						continue;
						break;
					default:
						consolemsg(ap.semid, aap.cap->outfd, "command $%s$ not understood, %d\n",
							ap.comline.buf, rtbRet);
				}
			}
		} else if(pollfds[2].revents & POLLIN) { /* Client connection incoming */
			int uploadfd = accept(passsock, (struct sockaddr *) &peer_addr, &socklen);
			if (!uploadfd) return -1;
			logmsg(ap.semid, ap.logfd, LOGLEVEL_WARN, "%d %d\n", uploadfd, passsock);
			switch(uploadpid = fork()){
			case -1:
				close(uploadfd);
				perror("(client main) Problem forking when accepting client conn");
				goto error;
			case 0:
				close(passsock);
				huRet =  handleUpload(uploadfd, cap.outfd, &ap);
				huRet = (huRet == -2)? EXIT_SUCCESS: EXIT_FAILURE;
				close(uploadfd);
				_exit(huRet);
			default:
				/* u for upload */
				addChildProcess(cap.cpa, 'u', cap.conpid);
				close( uploadfd );
			}
		} else if(pollfds[3].revents & POLLIN) { /* incoming signal */
			SRret = read(ap.sigfd, &fdsi, sizeof(struct signalfd_siginfo));
			if (SRret != sizeof(struct signalfd_siginfo)){
				fprintf(stderr, "signalfd returned something broken");
				shellReturn = EXIT_FAILURE;
			}
			switch(fdsi.ssi_signo){
				case SIGINT:
				case SIGQUIT:
					fprintf(stderr,"Yeah i found %s",(fdsi.ssi_signo==SIGINT) ?
						"SIGINT": "SIGQUIT");
					shellReturn = EXIT_SUCCESS;
					break;
				//Pase SIGCHLD:
					logmsg(ap.semid, ap.logfd, LOGLEVEL_VERBOSE, 
							"Child %d quit with status %d\n", fdsi.ssi_pid, fdsi.ssi_status);
					break;
				default:
					logmsg(ap.semid, ap.logfd, LOGLEVEL_WARN,
							"Encountered unknown signal on sigfd %d", fdsi.ssi_signo);
			}
		} else if (pollfds[0].revents & POLLHUP) {
			/*  server closed connection? */
			fprintf(stderr, "server closed on us\n");
			shellReturn = EXIT_SUCCESS;
			break;
		} else if (pollfds[1].revents & POLLHUP) {
			/*  user closed connection - possible? */
			shellReturn = EXIT_SUCCESS;
			break;
		} else {
			fprintf(stderr, "Dunno what to do with this poll\n");
			shellReturn = EXIT_FAILURE;
			break;
		}
		/* logmsg(0, ap.logfd, LOGLEVEL_VERBOSE, "%s", msg); */
		if (shellReturn != EXIT_NO) break;
	}

error:
	freeBuf(&msg);
	close(passport);
	freeap(&ap);
	freecap(&cap); /* closes all open file handles with the consoler */
	if ( waitpid(cap.conpid, NULL, 0) < 0 ||  waitpid(ap.logpid, NULL, 0) < 0) {
		fputs("Did Burpy: Unclean Shutdown - Sorry\n", stderr);
		exit(shellReturn);
	}
	exit(shellReturn);
}
