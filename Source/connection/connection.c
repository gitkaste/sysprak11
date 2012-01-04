#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <inttypes.h> /* uintX_t */
#include <stdio.h> 
#include "connection.h"

int createPassiveSocket(uint16_t *port){
	int sockfd;
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < -1 ){
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
	/* bind to wildcard ip (0.0.0.0), i.e. listen on all interfaces */
	peeraddr.sin_addr.s_addr = ip->s_addr;
	peeraddr.sin_port = htons(port);
	peeraddr.sin_family = AF_INET;
	if ( connect(sockfd, (struct sockaddr *) &peeraddr, sizeof(peeraddr)) == -1){
		perror("Failure to connect to peer");
		return -1;
	}
	return sockfd;
}
