#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <inttypes.h> /* uintX_t */
#include <stdio.h> 
#include <strings.h> 
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "connection.h"
#include "tokenizer.h"
#include "logger.h"

int createPassiveSocket(uint16_t *port){
	int sockfd;
	if ( (sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0)) < -1 ){
		perror("couldn't attach to socket, damn");
		return -1;
	}
	struct sockaddr_in servaddr;
	/* bind to wildcard ip (0.0.0.0), i.e. listen on all interfaces */
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY); 
	servaddr.sin_port = htons(*port);
	servaddr.sin_family = AF_INET;
	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1){
		perror("Couldn't bind to socket");
		return -1;
	}
	if (listen(sockfd, 50) == -1){
		perror("Failure to listen on socket");
		return -1;
	}
	socklen_t addrlen = sizeof(servaddr);
	if (getsockname(sockfd, (struct sockaddr *) &servaddr, &addrlen) == -1){
		perror("Couldn't find my own port number");
		return -1;
	}
	*port = ntohs(servaddr.sin_port);
	return sockfd;
}


int connectSocket(struct in_addr *ip, uint16_t port){
	int sockfd;

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < -1 ){
		perror("couldn't attach to socket, damn");
		return -1;
	}

	struct sockaddr_in peeraddr;
	peeraddr.sin_addr.s_addr = ip->s_addr;
	peeraddr.sin_port = htons(port);
	peeraddr.sin_family = AF_INET;

	if ( connect(sockfd, (struct sockaddr *) &peeraddr, sizeof(peeraddr)) == -1){
		fprintf(stderr, "errno: %d", errno);
		perror("Failure to connect to peer");
		return -1;
	}

	if (setFdNonblock(sockfd) == 1)
		return sockfd;
	else
		perror("(connectSocket) Couldn't set sockfd to nonBlocking");
		return -1;
}

int sendResult(int fd, struct actionParameters *ap, 
		struct serverActionParameters *sap){
	unsigned long i = 0;
	struct flEntry *fl;
	/* blatt7 is wrong about the comword alreay containing the search token */
	int res = getTokenFromBuffer(&ap->comline, &ap->comword, "\n", "\r\n",NULL );
	if (res == -1) return -3;

	while (( fl = iterateArray(sap->filelist, &i))){
		if (strcasecmp( (char *) ap->comword.buf, fl->filename)) continue;
		if (-1 == writef(fd, "%d\t%d\t%s\t%lu", fl->ip.s_addr, fl->port, 
				fl->filename, fl->size))
			return -3;

	}
	return -2;
}

int recvResult(int fd, struct actionParameters *ap,struct array * results){
	int counter, res;
	struct flEntry file;
	struct array * results2;
	/* get a line */
	while ( ( res = getTokenFromStream( ap->comfd, &ap->combuf, &ap->comline, "\n", "\r\n",NULL ) ) ){
		if (res ==  -1) return -3;
		/* get first word -> ip */
		res = getTokenFromBuffer( &(ap->combuf), &(ap->comword), " ", "\t",NULL );
		if (res ==  -1) return -3;;
		errno =0;
		if ( (file.ip.s_addr = strtol((char *)ap->comword.buf, NULL, 10)) < 0 || errno)  return -3;
		/* get second word -> port */
		res = getTokenFromBuffer(&ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		errno =0;
		if ( (file.port = strtol((char *)ap->comword.buf, NULL, 10)) < 0 || errno) return -3;
		/* get third word -> filename */
		res = getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		strcpy(file.filename, (char *)&ap->comline.buf);
		/* get fourth word -> size */
		res = getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		errno =0;
		if ( (file.size = strtol((char *)ap->comword.buf, NULL, 10)) < 0 || errno) return -3;

		results2 = addArrayItem(results, &file);
		if (results2) results = results2;
		else return -3;

		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "%d)\t%s\t(%d)", counter++, file.filename, file.size);
	}
	return 2;
}
