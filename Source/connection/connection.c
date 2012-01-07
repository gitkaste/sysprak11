#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <inttypes.h> /* uintX_t */
#include <stdio.h> 
#include "connection.h"

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
	if ( (sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0)) < -1 ){
		perror("couldn't attach to socket, damn");
		return -1;
	}
	struct sockaddr_in peeraddr;
	peeraddr.sin_addr.s_addr = ip->s_addr;
	peeraddr.sin_port = htons(port);
	peeraddr.sin_family = AF_INET;
	if ( connect(sockfd, (struct sockaddr *) &peeraddr, sizeof(peeraddr)) == -1){
		perror("Failure to connect to peer");
		return -1;
	}
	return sockfd;
}

int sendResult(int fd, struct actionParameters *ap, 
		struct serverActionParameters *sap){
	unsigned long i = 0;
	struct flEntry *fl;
	/* blatt7 is wrong about the comword alreay containing the search token */
	res = getTokenFromBuffer(ap->comline, ap->comword, "\n", "\r\n",NULL );
	if (res == -1) return -3;

	while (( fl = iterateArray(sap->filelist, &i))){
		if (strcasecmp(ap->comword, fl->filename)) continue;
		if (-1 == writef(fd, "%d\t%d\t%s\t%lu", fl->ip->s_addr, fl->port, 
				fl->filename, fl->size))
			return -3;

	}
	return -2;
}

int recvResult(int fd, struct actionParameters *ap,
		struct serverActionParameters *sap){
	int res;
	struct flEntry file;
	/* get a line */
	while ( ( res = getTokenFromStream( conffd, &ap->combuf, &ap->comline, "\n", "\r\n",NULL ) ) ){
		if (res ==  -1) return -3;
		/* get first word -> ip */
		res = getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		errno =0;
		if ( (file.ip.s_addr = strtol(ap->comword.buf, NULL, 10)) < 0 || errno)  return -3;
		/* get second word -> port */
		res = getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		errno =0;
		if ( (file.port = strtol(ap->comword.buf, NULL, 10)) < 0 || errno)  return -3;
		/* get third word -> filename */
		res = getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		strcpy(file.filename, &ap->comline.buf);
		/* get fourth word -> size */
		res = getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL );
		if (res ==  -1) return -3;;
		errno =0;
		if ( (file.size = strtol(ap->comword.buf, NULL, 10)) < 0 || errno)  return -3;

		if (semWait(cap->results, SEM_FILELIST)) return -3;
		fl2 = addArrayItem(cap->results, file);
		if (semSignal(cap->results, SEM_FILELIST)) return -3;

		if (fl2) cap->filelist = fl2
		else return -3;
	}
	return 2;
}

cap->results 
