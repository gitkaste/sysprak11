#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <inttypes.h> /* uintX_t */
#include <stdio.h> 
#include <strings.h> 
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "connection.h"
#include "tokenizer.h"
#include "logger.h"
#include "util.h"

int g_forceipversion = 4;

int createPassiveSocket4(uint16_t *port){
	int sockfd;
	int yes = 1;
	if ( (sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0)) < -1 ){
		perror("couldn't attach to socket, damn");
		return -1;
	}
	/* either this works or it doesn't, if it doesn't then it will work if no 
	 * socket is lingering in the kernel on that port or it won't in which case 
	 * we just fail down below */
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
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

int createPassiveSocket6(uint16_t *port){
	int sockfd;
	int yes = 1;
	if ( g_forceipversion == 6) {
		if ((sockfd = socket(AF_INET6,SOCK_STREAM|SOCK_NONBLOCK|IPV6_V6ONLY,0))<-1){
			perror("couldn't attach to socket, damn");
			return -1;
		}
	} else {
		if ( (sockfd = socket(AF_INET6, SOCK_STREAM|SOCK_NONBLOCK, 0)) < -1 ){
			perror("couldn't attach to socket, damn");
			return -1;
		}
	}
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	struct sockaddr_in6 servaddr6;
	memset(&servaddr6, 0, sizeof(servaddr6));

	servaddr6.sin6_addr = in6addr_any; // use my IPv6 address
	servaddr6.sin6_port = htons(*port);
	servaddr6.sin6_family = AF_INET6;

	if (bind(sockfd, (struct sockaddr *) &servaddr6, sizeof(servaddr6)) == -1){
		perror("Couldn't bind to socket");
		return -1;
	}
	if (listen(sockfd, 50) == -1){
		perror("Failure to listen on socket");
		return -1;
	}
	socklen_t addrlen = sizeof(servaddr6);
	if (getsockname(sockfd, (struct sockaddr *) &servaddr6, &addrlen) == -1){
		perror("Couldn't find my own port number");
		return -1;
	}
	*port = ntohs(servaddr6.sin6_port);
	return sockfd;
}

int createPassiveSocket(uint16_t *port){
	if (g_forceipversion == 4) 
		return createPassiveSocket4(port);
	else
		return createPassiveSocket6(port);
}

int connectSocket(struct sockaddr *ip, uint16_t port){
	int sockfd;

	if ( (sockfd = socket(ip->sa_family, SOCK_STREAM, 0)) < -1 ){
		perror("couldn't attach to socket, damn");
		return -1;
	}

	if (ip->sa_family == AF_INET)
		((struct sockaddr_in *) ip)->sin_port = htons(port);
	else
		((struct sockaddr_in6 *) ip)->sin6_port = htons(port);
	printIP(ip);
	puts("");
	if ( connect(sockfd, ip, (ip->sa_family == AF_INET)? sizeof (struct sockaddr_in):
				sizeof(struct sockaddr_in6)) == -1){
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
	char ip[127];
	/* blatt7 is wrong about the comword alreay containing the search token */
	int res = getTokenFromBuffer(&ap->comline, &ap->comword, "\n", "\r\n",NULL );
	if (res == -1) return -3;

	while (( fl = iterateArray(sap->filelist, &i))){
		if ( strcasecmp( (char *) ap->comword.buf, fl->filename) ) continue;
		if ( -1 == writef(fd, "%s\t%d\t%s\t%lu", getipstr(&fl->ip,ip,127), 
					getPort(&fl->ip), fl->filename, fl->size))
			return -3;

	}
	return -2;
}

int recvResult(int fd, struct actionParameters *ap,struct array * results){
	int counter, res;
	struct flEntry file;
	struct array * results2;
	char port[7];
	char ipstr[56];
	/* get a line */
	while ( ( res = getTokenFromStream( ap->comfd, &ap->combuf, &ap->comline, "\n", "\r\n",NULL ) ) ){
		if (res ==  -1) return -3;

		/* get first word -> ip */
		if (getTokenFromBuffer( &(ap->combuf), &(ap->comword), " ", "\t",NULL ) == -1)
			return -1;
		strncpy(ipstr, (char *)ap->combuf.buf, 56);

		/* get second word -> port */
		if (getTokenFromBuffer(&ap->combuf, &ap->comword, " ", "\t",NULL ) == -1)
			return -1;
		strncpy(port, (char *)ap->combuf.buf, 7);

		if (parseIP(ipstr, &(file.ip), port, 0) == -1)
			return -1;

		/* get third word -> filename */
		if (getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL ) == -1)
			return -1;
		strcpy(file.filename, (char *)&ap->comline.buf);

		/* get fourth word -> size */
		if (getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\t",NULL ) == -1)
			return -1;
		if ( (file.size = my_strtol((char *)ap->comword.buf)) < 0 || errno) return -3;

		results2 = addArrayItem(results, &file);
		if (results2) results = results2;
		else return -3;

		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "%d)\t%s\t(%d)", counter++, file.filename, file.size);
	}
	return 2;
}

int recvFileList(int sfd, struct actionParameters *ap,
		struct serverActionParameters *sap){
	int res;
	struct flEntry file;
//	char port[7];
//	struct sockaddr a;
	struct array *fl2; 

///	snprintf( port, 7, "%d", ap->comport);
///	if (parseIP(ap->comip, &a, port, 0) == -1)
///		return -1;
///
	while ( ( res = getTokenFromStream( sfd, &ap->combuf, &ap->comline, "\n", "\r\n",NULL ) ) ){
		if (res ==  -1) return -1;
		strcpy(file.filename, (char *)&ap->comline.buf);
		res = getTokenFromStream( sfd, &ap->combuf, &ap->comword, "\n", "\r\n",NULL );
		if (res ==  -1) return -1;
		errno =0;
		if ( (file.size = strtol((char *)ap->comword.buf, NULL, 10)) < 0 || errno)  return -1;

		if (semWait(sap->shmid_filelist, SEM_FILELIST)) return -1;
		fl2 = addArrayItem(sap->filelist, &file);
		if (semSignal(sap->shmid_filelist, SEM_FILELIST)) return -1;

		if (fl2) sap->filelist = fl2;
		else return -1;
	}
	return 1;
}

int handleUpload(int upfd, int confd, struct actionParameters *ap){
	int res = getTokenFromStream( upfd, &ap->combuf, &ap->comline, 
			"\n", "\r\n",NULL );
	if (res == -1){
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, 
				"Error getting Filenname to upload");
		return -2;
	}
	int filefd = open( (char *) ap->comline.buf, O_RDONLY);
	if ( filefd == -1 ) {
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "Can't open %s", ap->comline.buf);
		return -2;
	}
	struct stat statbuf;
	if ( !fstat(filefd, &statbuf)){
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "Couldn't stat %s", ap->comline.buf);
		return -2;
	}
	int ret = advFileCopy( upfd, filefd, statbuf.st_size, (char *)ap->comline.buf, ap->semid, 
			ap->logfd, ap->sigfd, confd);
	/* readonly-> no EIO, signalfd-> no EINTR and EBADF unrealistic, necessary? */
	if (-1 == close(filefd)) 
		logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "Error closing file %s", 
				ap->comline.buf);
	return ret;
}

int advFileCopy(int destfd, int srcfd, unsigned long size, const char *name, 
		int semid, int logfd, int sigfd, int confd){
	return 1;
}
