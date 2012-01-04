#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include "tokenizer.h"
#include "config.h"
#include "writef.h"

/* This is a gnu extension but it's safer 
#define Min(X, Y) do { 
	typeof (X) x_ = (X);\
	typeof (Y) y_ = (Y);\
	(x_ < y_) ? x_ : y_; } while(0)
*/

/* unsafe macro, if you need safety use a function! */
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define LOG(x) do {puts(x); fflush(stdout);} while(0)

/* supposed to init the defaults */
void confDefaults(struct config *conf){
	/* This shall not fail */
	inet_pton(AF_INET, "0.0.0.0", (void *)&(conf->ip));
	conf->port = 4444;
	strcpy(conf->logfile, "tmp/sysprak");
	conf->loglevel = 1;
	strcpy(conf->share, "share");
	conf->shm_size = 0; 
}

/* fills the conf with the values from the conf file */
int parseConfig (int conffd, struct config *conf){
	struct buffer key;
	struct buffer value;
	struct buffer line;
	struct buffer buf_tmp;
	char * commentstart;
	createBuf(&line,1024);
	createBuf(&buf_tmp,1024);
	createBuf(&key,256);
	createBuf(&value,256);
	int res;
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

			/* BUGBUG Theoretically errno needs to be checked after every strtol */
			/* BUGBUG ERROR checking! */
			/*** IP ***/
			if (!strncmp((char *)key.buf, "ip", key.buflen)){
				inet_pton(AF_INET, (char *)value.buf,(void *) &(conf->ip));
			/*** Port ***/
			}else if (!strncmp((char *)key.buf, "port", key.buflen)){
				conf->port = (uint16_t) strtol((char *)value.buf, NULL, 10);
			/*** Loglevel ***/
			}else if (!strncmp((char *)key.buf, "loglevel", key.buflen)){
				conf->loglevel = (uint8_t) strtol((char *)value.buf, NULL, 10);
			/*** shm_size ***/
			}else if (!strncmp((char *)key.buf, "shm_size", key.buflen)){
				conf->shm_size = (uint32_t) strtol((char *)value.buf, NULL, 10);
			/*** logfile ***/
			}else if (!strncmp((char *)key.buf, "logfile", key.buflen)){
				strncpy(conf->logfile, (char *)value.buf, FILENAME_MAX);
			}else if (!strncmp((char *)key.buf, "share", key.buflen)){
				strncpy(conf->share, (char *)value.buf, FILENAME_MAX);
			}else{
				printf("crap unknown token: %s\n", key.buf);
			} 
			/* ... */
		}
	}
	freeBuf(&line);
	freeBuf(&buf_tmp);
	freeBuf(&key);
	freeBuf(&value);
	return 1;
}
int writeConfig (int fd, struct config *conf){
	char ip[127];
	/* There is no way to enter a flawed ip in our system */
	inet_ntop(AF_INET, &conf->ip, ip, 126);
	if (writef(fd, "Using config:\n\tip:\t\t%s\n\tport:\t\t%d\n",ip, conf->port) == -1) return -1;
	if (writef(fd, "\tlogfile:\t%s\n\tloglevel:\t%d\n", conf->logfile, conf->loglevel) == -1) return -1;
	return writef(fd, "\tshare:\t\t%s\n\tshm_size:\t%d\n", conf->share, conf->shm_size);
}
