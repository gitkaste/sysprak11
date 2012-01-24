#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h> /* ntop, pton */
#include <sys/types.h>
#include <sys/stat.h> /* stat */
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h> /* getaddrinfo */
#include <netdb.h>   /* getaddrinfo */
#include <unistd.h>
#include "tokenizer.h"
#include "config.h"
#include "util.h"
#include "logger.h"

/* This is a gnu extension but it's safer 
#define Min(X, Y) do { 
	typeof (X) x_ = (X);\
	typeof (Y) y_ = (Y);\
	(x_ < y_) ? x_ : y_; } while(0)
*/

/* unsafe macro, if you need safety use a function! */
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define LOG(x) do {puts(x); fflush(stdout);} while(0)

//Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
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

void printIP(struct addrinfo *a){
	char ip[255];
	printf("%s", get_ip_str(a->ai_addr, ip, 255));
}

int parseIP(char * ip, struct addrinfo *a, char * port, int ipversion) {
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
	printf("%s - %d\n", ip, ipversion);
	hints.ai_flags = AI_CANONNAME; /* Y U NO WORK?!? */
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	int res = getaddrinfo(ip, port, &hints, &tmpa);
	*a = *tmpa; // Fuck the lack of this line cost me hours
	return res;
}

/* supposed to init the defaults */
void confDefaults(struct config *conf){
	/* This shall not fail */
	inet_pton(AF_INET, "0.0.0.0", (void *)&(conf->ip));
	printf("PARSEIP%d\n", parseIP("0.0.0.0", &(conf->ipa), NULL, 4));
	printf("confDef %p\n", (void *)&conf->ipa);
	conf->port = 4444;
	strcpy(conf->logfile, "tmp/sysprak/server.log");
	conf->loglevel = 1;
	strcpy(conf->share, "tmp/sysprak/share");
	/* read from /proc/sys/kernel/shmmax */
	conf->shm_size = 33554432; 
	conf->forceIpVersion = 0;
	conf->logMask = 1;
	strcpy(conf->workdir, "tmp/sysprak");
	strcpy(conf->networkDumpLogFile, "tmp/sysprak/serverDump.log");
	/* BUGBUG read from eth0 or something this is crap */
	inet_pton(AF_INET, "127.0.0.1", (void *)&(conf->bc_ip));
	parseIP("127.0.0.1", &(conf->bc_ipa), NULL, 4);
	conf->bc_port = 4445;
	conf->scheduler = SCHEDULER_RR;
	parseIP("127.0.0.1", &(conf->bc_broadcasta), NULL, 4);
	conf->bc_interval = 10;
	conf->schedTimeSlice = 3;
}

/* fills the conf with the values from the conf file */
int parseConfig (int conffd, struct config *conf){
	struct buffer key;
	struct buffer value;
	struct buffer line;
	struct buffer buf_tmp;
	char ipstr[1024]; /* BUGBUG How long can DNS names be? */
	char bc_ipstr[1024];
	char bc_broadcaststr[1024];
	char * commentstart;
	char portstr[12];
	int forceIpVersion, res, retval = 1, s;
	createBuf(&line, 1024);
	createBuf(&buf_tmp, 1024);
	createBuf(&key, 256);
	createBuf(&value, 256);
	while ( ( res = getTokenFromStream( conffd, &buf_tmp, &line, "\n", "\r\n",NULL ) ) ){
		/* crap error */
		if (res ==  -1) return -1;
		if  ( (commentstart = strchr( (char *)line.buf, '#')) ){
			commentstart[0]='\0';
			line.buflen = commentstart-(char *)line.buf; 
		}
		while ( getTokenFromBuffer(&line, &key, "\t", " ", NULL) == 1){
			/* break out inner loop if we don't have an argument to key */
			if (getTokenFromBuffer(&line, &value, "\t", " ", NULL) != 1) break;

			/* BUGBUG ERROR checking! */
			/*** IP ***/
			if (!strncmp((char *)key.buf, "ip", key.buflen)) {
				strncpy(ipstr, (char *) value.buf, 1024);
			/*** Port ***/
			} else if (!strncmp((char *)key.buf, "port", key.buflen)) {
				conf->port = (uint16_t) my_strtol((char *)value.buf);
				if (errno || conf->port > 645536){
					fprintf(stderr,"(config parser) port isn't a valid port number or trailing chars\n");
					retval = -1;
					break;
				}
			/*** Loglevel ***/
			} else if (!strncmp((char *)key.buf, "loglevel", key.buflen)) {
				conf->loglevel = (uint8_t) my_strtol((char *)value.buf);
				if (errno || !isPowerOfTwo(conf->loglevel)){
					fprintf(stderr,"(config parser) loglevel isn't a valid level or trailing chars\n");
					retval = -1;
					break;
				}	
				g_loglevel = conf->loglevel;
			/*** shm_size ***/
			} else if (!strncmp((char *)key.buf, "shm_size", key.buflen)){
				conf->shm_size = (uint32_t) my_strtol((char *)value.buf);
				if (errno || conf->shm_size > getShmMax() || conf->shm_size < getShmMin()){
					fprintf(stderr,"(config parser) shm_size isn't valid or trailing chars\n");
					retval = -1;
					break;
				}
			/*** logfile ***/
			} else if (!strncmp((char *)key.buf, "logfile", key.buflen)){
				strncpy(conf->logfile, (char *)value.buf, FILENAME_MAX);
			/*** share ***/
			} else if (!strncmp((char *)key.buf, "share", key.buflen)){
				strncpy(conf->share, (char *)value.buf, FILENAME_MAX);
				if (!isDir(conf->share)){
					fprintf(stderr,"(config parser) share isn't a valid dir (maybe unreadable?)\n");
					retval = -1;
				}
			/*** logMask ***/
			} else if (!strncmp((char *)key.buf, "logMask", key.buflen)){
				conf->logMask = (uint16_t) my_strtol((char *)value.buf);
				if (errno) {
					fprintf(stderr,"(config parser) shm_size isn't valid or trailing chars\n");
					retval = -1;
				}
			/*** forceIpVersion ***/
			} else if (!strncmp((char *)key.buf, "forceIpVersion", key.buflen)){
				forceIpVersion = (uint8_t) strtol((char *)value.buf, NULL, 10);
				switch(forceIpVersion){
					case 0:
					case 4:
					case 6:
						conf->forceIpVersion = forceIpVersion;
						break;
					default:
						fprintf(stderr,"(config parser) forceIpVersion can only be 0,4 or 6\n");
				}
			/*** bc_ip ***/
			} else if (!strncmp((char *)key.buf, "bc_ip", key.buflen)){
				strncpy(bc_ipstr, (char *) value.buf, 1024);
			/*** bc_port ***/
			} else if (!strncmp((char *)key.buf, "bc_port", key.buflen)){
				conf->bc_port = (uint16_t) my_strtol((char *)value.buf);
				if (errno || conf->bc_port > 65536) {
					fprintf(stderr,"(config parser) bc_port isn't valid\n");
					retval = -1;
				}
			/*** bc_broadcast ***/
			} else if (!strncmp((char *)key.buf, "bc_broadcast", key.buflen)){
				strncpy(bc_broadcaststr, (char *) value.buf, 1024);
			/*** bc_interval ***/
			} else if (!strncmp((char *)key.buf, "bc_interval", key.buflen)){
				conf->bc_interval = (uint16_t) my_strtol((char *)value.buf);
				if (errno || conf->bc_interval <= 0) {
					fprintf(stderr,"(config parser) bc_interval isn't sensible\n");
					retval = -1;
				}
			/*** workdir ***/
			} else if (!strncmp((char *)key.buf, "workdir", key.buflen)){
				strncpy(conf->workdir, (char *)value.buf, FILENAME_MAX);
				if (!isDirWritable(conf->workdir)){
					fprintf(stderr,"(config parser) workdir isn't writable or doesn't exist\n");
					retval = -1;
				}
			/*** scheduler***/
			} else if (!strncmp((char *)key.buf, "scheduler", key.buflen)){
				if (strcasecmp((char *)value.buf, "rr"))
					conf->scheduler = SCHEDULER_RR;
				else if (strcasecmp((char *)value.buf, "pri"))
					conf->scheduler =SCHEDULER_PRI;
				else {
					fprintf(stderr,"(config parser) unknown scheduler: %s\n", value.buf);
					retval = -1;
				}
			/*** schedTimeSlice ***/
			} else if (!strncmp((char *)key.buf, "schedTimeSlice", key.buflen)){
				conf->schedTimeSlice = (uint16_t) my_strtol((char *)value.buf);
				if (errno || conf->bc_interval <= 0) {
					fprintf(stderr,"(config parser) schedTimeSlice isn't sensible\n");
					retval = -1;
				}
			/*** networkDumpLogFile ***/
			} else if (!strncmp((char *)key.buf, "networkDumpLogFile", key.buflen)){
				strncpy(conf->networkDumpLogFile, (char *)value.buf, FILENAME_MAX);
			} else{
				fprintf(stderr,"(config parser) unknown token: %s\n", key.buf);
			} 
		}
	}

	/*** Lookup IPs for ip bc_ip and bc_broadcast ***/
	if (retval != -1){
		snprintf(portstr, 11, "%d", conf->bc_port);
		if ( (s = parseIP(ipstr, &conf->ipa, NULL, conf->forceIpVersion)) ){
			fprintf(stderr,"(config parser) ip isn't a valid ip address%s\n", gai_strerror(s));
			retval = -1;
		} else if (parseIP(bc_ipstr, &conf->bc_ipa, portstr, conf->forceIpVersion) == -1){
			fprintf(stderr,"(config parser) bc_ip isn't a valid ip address%s\n", gai_strerror(s));
			retval = -1;
		} else if (parseIP(bc_broadcaststr, &conf->bc_broadcasta, NULL, conf->forceIpVersion) == -1){
			fprintf(stderr,"(config parser) bc_broadcast isn't a valid ip address%s\n", gai_strerror(s));
			retval = -1;
		}
	}
	freeBuf(&line);
	freeBuf(&buf_tmp);
	freeBuf(&key);
	freeBuf(&value);
	return retval;
}

/***** Setup CONFIG *****/
int initConf(char * conffilename, struct config *conf, char error[256]){
	int conffd = open(conffilename, O_RDONLY);
	if ( conffd == -1) {
		sperror("Error opening config file:\n", error, 256);
		return -1;
	}
	confDefaults(conf);
	if ( parseConfig(conffd, conf) == -1 ){
		close(conffd);
		strncpy(error, "Your config has errors, please fix them", 256);
		return -1;
	}
	close(conffd);
//	writeConfig (STDOUT_FILENO, conf);
	return 1;
}

void writeConfig (int fd, struct config *conf){
	char ip[127];
	/* There is no way to enter a flawed ip in our system */
	inet_ntop(AF_INET, &conf->ip, ip, 126);
	writef(fd, "Using config:\n\tip:\t\t\t%s\n\tport:\t\t\t%d\n",ip, conf->port);
	if (!get_ip_str(conf->ipa.ai_addr, ip, 127))
		writef(fd, "problem converting ip conf->ip");
	else
		writef(fd, "\tipa:\t\t\t%s\n",ip);
	writef(fd, "\tlogfile:\t\t%s\n\tloglevel:\t\t%d\n", conf->logfile, conf->loglevel);
	writef(fd, "\tlogMask:\t\t%d\n", conf->logMask); 
	writef(fd, "\tnetworkDumpLogFile:\t%s\n", conf->networkDumpLogFile);
	writef(fd, "\tshare:\t\t\t%s\n\tshm_size:\t\t%d\n", conf->share, conf->shm_size);
	writef(fd, "\tworkdir:\t\t%s\n", conf->workdir);
	writef(fd, "\tforceIpVersion:\t\t%d\n", conf->forceIpVersion);
	if (!get_ip_str(conf->bc_ipa.ai_addr, ip, 127))
		writef(fd, "problem converting ip conf->ip");
	else
		writef(fd, "\tbc_ip:\t\t\t%s\n\tbc_port:\t\t%d\n", ip, conf->bc_port);
	if (!get_ip_str(conf->bc_broadcasta.ai_addr, ip, 127))
		writef(fd, "problem converting ip conf->ip");
	else
		writef(fd, "\tbc_broadcast:\t\t%s\n", ip);
}
