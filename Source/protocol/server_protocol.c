/*******************************************************************************
*                                                                              *
*        A  C  T  I  O  N  S                                                   *
*                                                                              *
*******************************************************************************/

/* Note on Return Value of Actions:
 *        1 - everything okay, continue
 *        0 - clean exit (hangup, quit)
 *       -1 - abort and exit
 *       -2 - child (ok): cleanup and exit
 *       -3 - child (error): cleanup and exit
 */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE /* signal.h sigset_t */
#endif
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h> /* for signalfd (Signalhandler) */
#include <signal.h>
#include <poll.h> /* poll() */
/* for inet_ntoa(). Probably this is just for some very verbose log-messages. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include "protocol.h"
#include "server_protocol.h"
#include "util.h"
#include "logger.h"
#include "tokenizer.h"
#include "connection.h"
#include "directoryParser.h"

/*#include "../util/signalfd.h"*/

struct protocol server_protocol = {
	&unknownCommandAction,
	6,
	{
		{"FILELIST", "\tsend my filelist to the Server.\n", &filelistAction},
		{"STATUS", "\t\tSTATUS returns Server-Status.\n", &statusAction},
		{"QUIT", "\t\tQUIT closes the connection cleanly (probably).\n", &quitAction},
		{"SEARCH", "\t\tSEARCH file to crawl the filelist).\n", &searchAction},
		{"PORT", "\t\tset passive Port of the client.\n", &portAction},
		{"HELP", "\t\tHELP prints this help.\n", &helpAction}
	}
};

int initializeServerProtocol(struct actionParameters *ap) {
	ap->prot = &server_protocol;
	return 1;
}

int unknownCommandAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	char *msg;
	msg = stringBuilder("Command \"%s\" not understood.\n",
		ap->comword.buf);
	reply(ap->comfd, ap->logfd, ap->semid, REP_WARN, msg);
	logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, 
			"fuck unknown command \"%s\"", ap->comword.buf);
	free(msg);
	return 1;
}

int statusAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	char *msg;
	msg = stringBuilder("WTF %s\n", ap->comword.buf);
	reply(ap->comfd, ap->logfd, ap->semid, REP_WARN, msg);
	free(msg);
	return 1;
}

int quitAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	reply(ap->comfd, ap->logfd, ap->semid, REP_TEXT,
		"Closing connection. Have a nice day ;)\n"); 
	/* Returning 0: main-loop shall break and shouldn't accept any further commands */
	close(ap->comfd);
	return 1;
}

int searchAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	uint16_t port = 0;
	pid_t pid;
	int sockfd = createPassiveSocket(&port); ;
	if (!sockfd) return -1;
	switch (pid = fork()) {
		case -1:
			close(sockfd);
			perror("(searchAction) Problem forking");
			return -1;
		case 0:
			if (writef(sockfd, "%d RESULT SOCKET %d\n", REP_COMMAND, port) < 0) return -2;
			return  (sendResult(sockfd, ap, aap->sap) == 1) ? -2 : -3;
		default:
			close(sockfd);
			return 1;
	}
}

int helpAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	char * msg;
	struct protocol * p = ap->prot;
	for (int i =0; i < p->actionCount; i++){
		msg = stringBuilder("server: %s %s\n", p->actions[i].actionName, 
				p->actions[i].description);
		if ( -1 == reply(ap->comfd, ap->logfd, ap->semid, REP_TEXT,msg)){
			free(msg);
		  return -1;
		}
		free(msg);
	}
	return 1;
}

int filelistAction(struct actionParameters *ap,
	union additionalActionParameters *aap){
	uint16_t port = 0;
	pid_t pid;

	int sockfd = createPassiveSocket(&port); ;
	if (!sockfd) return -1;
	
	switch( pid = fork()){
		case -1:
			close(sockfd);
			perror("(filelistAction) Problem forking");
			return -1;
		case 0:
			setFdBlocking(sockfd);
			socklen_t iplen = sizeof (ap->comip);
			ap->comfd = accept( sockfd, &(ap->comip), &iplen );
			ap->comport = getPort(&ap->comip);
			return  (recvFileList(sockfd, ap, aap->sap) == 1) ? -2 : -3;
		default:
			close(sockfd);
			if (writef(ap->comfd, "%d SENDLIST SOCKET %d\n", REP_COMMAND, port) < 0) return -1;
			else 
				logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "filelist connect successful");
			return 1;
	}
}

int portAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	ap->comport = my_strtol( (char *) ap->comline.buf);
	return (errno) ? -1 : 1;
}
