#include <arpa/inet.h> /* ntop, pton */
#include <sys/socket.h> /* getaddrinfo */
#include <netdb.h>   /* getaddrinfo */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *getipstr(const struct sockaddr *sa, char *s, size_t maxlen) {
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;
        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }
    return s;
}

void printIP(struct sockaddr *a){
	char ip[255];
	printf("%s", getipstr(a, ip, 255));
}

/*  returns 0 on success, error on failure */
int parseIP(char * ip, struct sockaddr *a, char * port, int ipversion) {
	struct addrinfo hints, *tmpa;
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	switch(ipversion){
		case 0:
			hints.ai_family = AF_UNSPEC;     // don't care if it's IPv4 or IPv6
			break;
		case 4:
			hints.ai_family = AF_INET;     // great, why can't AF_INET == 4?
			break;
		case 6:
			hints.ai_family = AF_INET6;    
	}
	hints.ai_flags = AI_CANONNAME; /* Y U NO WORK?!? */
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	int res = getaddrinfo(ip, port, &hints, &tmpa);
	*a = *(tmpa->ai_addr); // Fuck, the lack of this line cost me hours!
	freeaddrinfo(tmpa);
	return res;
}

int getPort(struct sockaddr *a) {
	if (a->sa_family == AF_INET)
		return ntohs(((struct sockaddr_in *) a)->sin_port);
	else if (a->sa_family == AF_INET6)
		return ntohs(((struct sockaddr_in6 *) a)->sin6_port);
	else
		return -1;
}

int setPort(struct sockaddr *a, int port) {
	if (a->sa_family == AF_INET){
		((struct sockaddr_in *) a)->sin_port = htons(port);
		return 1;
	} else if (a->sa_family == AF_INET6){
		((struct sockaddr_in6 *) a)->sin6_port = htons(port);
		return 1;
	} else
		return -1;
}

/*  sets the ip in a to the in_addr or in6_addr in *ip */
int setIP(struct sockaddr *a, void * ip){
	if (a->sa_family == AF_INET){
		((struct sockaddr_in *) a)->sin_addr.s_addr  = ((struct in_addr *) ip)->s_addr;
		return 1;
	} else if (a->sa_family == AF_INET6) {
		memcpy(((struct sockaddr_in6 *) a)->sin6_addr.s6_addr,((struct in6_addr *) ip)->s6_addr, 15);
		return 1;
	}
	else
		return -1;
}
