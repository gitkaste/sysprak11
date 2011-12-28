#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include "tokenizer.h"

/* supposed to init the defaults */
void confDefaults(struct config *conf){
	/* This shall not fail */
	inet_pton(AF_INET, "0.0.0.0", conf->ip);
	conf->port = 4444;
	conf->logfile = "tmp/sysprak";
	conf->loglevel = 1;
	conf->share = "share";
	conf->shm_size = 0; 
}
void strip_comments(struct buffer *buf, ...){
	int commenstart; 
	int tokenlength;
	int fulllength;
	va_list ap;
	char *  comment_char;
	va_start(ap, buf);
	while (1){ 
		if ( !(comment_char =  va_arg(ap_copy, char *)) ) break;
		if (!searchToken( &commentstart, &tokenlength, &fulllength, buf, comment_char)) continue;
		/* we found a comment marker, so kill everything after the marker  */
		buf->len = commentstart;
		buf->buf[buf->len]='\0';
	}
	va_end(ap);
}

/* fills the conf with the values from the conf file */
int parseConfig (int conffd, struct config *conf){
	struct buffer *key;
	struct buffer *value;
	struct buffer *line;
	struct buffer *buf_tmp;
	createBuf(line,1024);
	createBuf(buf_tmp,1024);
	createBuf(key,256);
	createBuf(value,256);
	int res;
	while ( ( res = getTokenFromStream( conffd, buf_tmp, line, "\n", "\r\n" ) ) ){
		/* crap error */
		if (res ==  -1) return -1;
		/* you can't use searchToken to strip comments because then you have a problem with lines starting with comments*/
		strip_comments(line, "#", ";");
		while ( getTokenFromBuffer(line, key, "\t", " ") == 1){
			/* break out inner loop if we don't have an argument to key */
			if (getTokenFromBuffer(line, value, "\t", " ") != 1) break;

			if (!strcmp(key, "ip")){
				/* weird shit that doesn't match the signature */
			}else if (!strcmp(key, "port")){
				conf->port = (uint16_t) strtol(value->buf, NULL, 10);
			}else if (!strcmp(key, "loglevel")){
				conf->loglevel = (uint8_t) strtol(value->buf, NULL, 10);
			}else if (!strcmp(key, "shm_size")){
				conf->shm_size = (uint32_t) strtol(value->buf, NULL, 10);
			} else if (!strcmp(key, "logfile")){
				strncpy(conf->logfile, value->buf, min(value->buflen, FILENAME_MAX));
			} else if (!strcmp(key, "share")){
				strncpy(conf->share, value->buf, min(value->buflen, FILENAME_MAX));
			} 
				/* ... */
		}
	}
	return 1;
}
