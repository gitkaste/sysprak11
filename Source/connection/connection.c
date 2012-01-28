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
#include "consoler.h"
#include "tokenizer.h"
#include "logger.h"
#include "util.h"

int createPassiveSocket4(uint16_t *port){
	int sockfd;
	int yes = 1;
	if ( (sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0)) < -1 ){
		perror("couldn't attach to socket, damn");
		return -1;
	}
	/* either this works or it doesn't, if it doesn't then it will work if no 
	 * socket is lingering in the kernel on that port or it won't in which case 
	 * we just fail further down below */
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
	if ( conf->forceIpVersion == 6) {
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

	if (bind(sockfd, (struct sockaddr *) &servaddr6, sizeof(servaddr6)) == -1 ){
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
	if (conf->forceIpVersion == 4) 
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

	if ( connect(sockfd, ip, (ip->sa_family == AF_INET)? sizeof (struct sockaddr_in):
				sizeof(struct sockaddr_in6)) == -1){
		perror("Failure to connect to peer");
		return -1;
	}

//	if (setFdNonblock(sockfd) == 1)
		return sockfd;
//	else
//		perror("(connectSocket) Couldn't set sockfd to nonBlocking");
//		return -1;
}

int sendResult(int fd, struct actionParameters *ap, 
		struct serverActionParameters *sap){
	unsigned long i = 0;
	struct flEntry *fl;

	fprintf(stderr, "mah\n");
	while (( fl = iterateArray(sap->filelist, &i))){
		fprintf(stderr, "%s\n", fl->filename);
		if ( -1 == searchString((char *) ap->comword.buf, fl->filename) ) continue;
		if ( -1 == writef(fd, "%s\n%d\n%s\n%lu\n", putIP((struct sockaddr *)&fl->ip), 
					getPort((struct sockaddr *) &fl->ip), fl->filename, fl->size))
		return -3;
	}

	return -2;
}

int recvResult(int fd, struct actionParameters *ap,struct array * results){
	int counter, res;
	struct flEntry file;
	struct array * results2;
	int num = 0;
	char port[7];
	char ipstr[56];
	/* get a line */

	fprintf(stderr, "muh\n");
	logmsg(ap->semid, ap->logfd, LOGLEVEL_DEBUG, "print %s\n", ap->comline);
	while ( ( res = getTokenFromStream( ap->comfd, &ap->combuf, &ap->comline, 
					"\n", "\r\n",NULL ) ) ){
		if (res ==  -1) return -3;

		/* get first word -> ip */
		if (getTokenFromBuffer( &(ap->combuf), &(ap->comword), " ", "\n",NULL )==-1 )
			return -3;
		strncpy(ipstr, (char *)ap->combuf.buf, 56);

		/* get second word -> port */
		if (getTokenFromBuffer(&ap->combuf, &ap->comword, " ", "\n",NULL ) == -1)
			return -3;
		strncpy(port, (char *)ap->combuf.buf, 7);

		if (parseIP(ipstr, (struct sockaddr *)&file.ip, port, 0) == -1)
			return -3;

		/* get third word -> filename */
		if (getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\n",NULL ) == -1)
			return -3;
		strcpy(file.filename, (char *)&ap->comline.buf);

		fprintf(stderr, "%s\n", file.filename);

		/* get fourth word -> size */
		if (getTokenFromBuffer( &ap->combuf, &ap->comword, " ", "\n",NULL ) == -1)
			return -3;
		if ( (file.size = my_strtol((char *)ap->comword.buf)) < 0 || errno) return -3;

		results2 = addArrayItem(results, &file);
		if (results2) results = results2;
		else {
			logmsg(ap->semid, ap->logfd, LOGLEVEL_FATAL, "couldn't resize Resultsarray");
			return -3;
		}

		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "%d)\t%s\t(%d)", counter++,
			 	file.filename, file.size);
		fprintf(stderr,"%d ", num);
		if (!(num %100)) fprintf(stderr,"\n");
		num++;
	}
	return num;
}

int recvFileList(int sfd, struct actionParameters *ap,
		struct serverActionParameters *sap){
	int res;
	struct flEntry file;
	struct array *fl2; 
	memcpy(&file.ip,&ap->comip, sizeof(struct flEntry));

	while (( res = getTokenFromStream( sfd, &ap->combuf, &ap->comline, "\n", 
			"\r\n",NULL ))){
		if (res ==  -1) return -1;
		strcpy(file.filename, (char *)ap->comline.buf);
		flushBuf(&ap->comline);
		if (getTokenFromStream( sfd, &ap->combuf, &ap->comline, "\n", "\r\n",NULL ) == -1 )
			return -1;
		file.size = my_strtol((char *)ap->comline.buf);
		if ( errno || file.size < 0 ) {
			logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, "problem converting file size for %s - %s\n", file.filename, ap->comline.buf);
			continue;
		}
		logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "%s - %lu\n", file.filename, file.size);

		if ( (res = semWait(ap->semid, SEM_FILELIST)))
			logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, "(recvfile) can't get semaphore, %d", res);
		fl2 = addArrayItem(sap->filelist, &file);
		if ( (res = semSignal(ap->semid, SEM_FILELIST)))
			logmsg(ap->semid, ap->logfd, LOGLEVEL_WARN, "(recvfile) can't release semaphore, %d", res);
		if (fl2) sap->filelist = fl2;
		else {
			logmsg(ap->semid, ap->logfd, LOGLEVEL_VERBOSE, "leaving the functions because realloc failed %d", fl2);
			return -1;
		}
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
