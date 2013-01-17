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
#include <strings.h>
#include "protocol.h"
#include "server_protocol.h"
#include "util.h"
#include "logger.h"
#include "tokenizer.h"
#include "connection.h"
#include "directoryParser.h"
#include "util.h"

/*#include "../util/signalfd.h"*/

struct protocol server_protocol = {
	&unknownCommandAction,
	7,
	{
		{"FILELIST", "\tsend my filelist to the Server.\n", &filelistAction},
		{"STATUS", "\t\tSTATUS returns Server-Status.\n", &statusAction},
		{"QUIT", "\t\tQUIT closes the connection cleanly (probably).\n", &quitAction},
		{"SEARCH", "\t\tSEARCH file to crawl the filelist).\n", &searchAction},
		{"PORT", "\t\tset passive Port of the client.\n", &portAction},
		{"BROWSE", "\t\tBrowse users files list.\n", &browseAction},
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
	int ret = 1;
	if (strcasecmp((char *)ap->comline.buf, "")){
		msg = stringBuilder("(server) Command \"%s\" not understood.\n",
			ap->comline.buf);
		logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, 
			"fuck unknown command \"%s\"", ap->comword.buf);
		ret =  reply(ap->comfd, ap->logfd, ap->semid, REP_WARN, msg);
		free(msg);
		return ret;
	}
	return 1;
}

int browseAction(struct actionParameters *ap, 
		union additionalActionParameters *aap) {
	unsigned long i =0;
	struct flEntry *f; 

	logmsg(ap->semid, ap->logfd, LOGLEVEL_DEBUG, 
			"(browseAction) invoked with %s\n", ap->comline.buf);
	reply(ap->comfd, ap->logfd, ap->semid, REP_TEXT, "share");
	while ((f = iterateArray(aap->cap->results, &i) )){
		char *msg = stringBuilder("%d: %d \t - %s\n",i,  f->size, f->filename);
		reply(ap->comfd, ap->logfd, ap->semid, REP_TEXT, msg);
		free(msg);
	}
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
	return 0;
}

int searchAction(struct actionParameters *ap,
		union additionalActionParameters *aap) {
	uint16_t port = 0;
	pid_t pid;
	int sockfd = 0;

	if (iswhitespace((char *)ap->comline.buf)) return 1;
	switch (pid = fork()) {

		case -1:
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
					"(searchAction) Problem forking\n");
			return -3;

		case 0:
			sockfd = createPassiveSocket(&port);
			if (sockfd<=0) return -3;
			if ( -1 == getTokenFromBuffer(&ap->comline, &ap->comword, "\n","\r\n",NULL))
				return -3;
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
					"(searchAction) called upon to search for %s\n", ap->comword.buf);
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
					"created port %d to send search results\n", port);
			char *msg = stringBuilder("RESULT SOCKET %d\n", port);
			if ( -1 == reply(ap->comfd, ap->logfd, ap->semid, REP_COMMAND, msg)){
				free(msg);
				return -3;
			}
			free(msg);

			setFdBlocking(sockfd);
			socklen_t addrlen = sizeof(ap->comip);
			if ((ap->comfd = accept(sockfd, &ap->comip, &addrlen)) == -1 ) {
				logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
						"(searchAction) Problem accepting connection\n");
				return -3;
			}
			int ret = (sendResult(ap->comfd, ap, aap->sap));
			close (ap->comfd);
			return (ret == -1)? -3: -2;

		default:
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
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
					"(filelistAction) waiting for a customer\n");
			ap->comfd = accept( sockfd, &(ap->comip), &iplen );
			close(sockfd);
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
					"(filelistAction) got one!\n");
			ap->comport = getPort(&ap->comip);
			logmsg(ap->semid, ap->logfd, LOGLEVEL_DEBUG, 
					"(filelistAction) going into recvFileList!\n");
			int	ret = recvFileList(ap->comfd, ap, aap->sap);
			close(ap->comfd);
			return  ( ret == 1 ) ? -2 : -3;
		default:
			close(sockfd);
			if (writef(ap->comfd, "%d SENDLIST SOCKET %d\n", REP_COMMAND, port) < 0) return -1;
			else 
				logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, 
						"created filelisten port on %d\n", port);
			return 1;
	}
}

int portAction(struct actionParameters *ap, 
		union additionalActionParameters *aap){
	ap->comport = my_strtol( (char *) ap->comline.buf);
	return (errno) ? -1 : 1;
}
