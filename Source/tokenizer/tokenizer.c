#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include "tokenizer.h"

struct separator{
	char * buf; 
	struct separator *next;
};
typedef struct separator * ptsep;

#define NOMEM if(1){fprintf(stderr,"ALARM: Malloc failed"); exit(1);}

/* cf. man qsort, sorting function for aforementioned function */
int strlencmp (const void * a, const void * b) {return ( strlen(*(char **)b) - strlen(*(char **)a) ); }

int searchToken(int *tokenstart,int *tokenlength,int *fulllength, struct buffer *buffer_temp, va_list ap) {
	/* the return value: 1 if a separator was found, else 0 */
	int return_value = 0;
	/* define an array of strings for the separators and a total number of separators */
	char **separator_array;
	int separator_number_total=0;
	char led_seppelin; /* used to signal weather the last token was a separator */
	ptsep separators, current_sep;

	*fulllength = 0;
	*tokenlength = 0;
	*tokenstart = 0; 

	/* fill linked list with all the separators, this is more inefficient than 
	 * realloc but way easier and safer than using a fixed buffer */
	separators = current_sep = malloc(sizeof(struct separator));
	while( ( current_sep->buf = va_arg(ap, char *)) != NULL) {
		current_sep->next = malloc(sizeof(struct separator));
		if (!current_sep->next) { NOMEM };
		current_sep = current_sep->next;
		separator_number_total++;
		current_sep->next = NULL;
	}

	/* preprare the memory for the array! */
	separator_array = malloc(separator_number_total*sizeof(char *));
	if (!separator_array){ NOMEM }

	/* parse the linked list into the array and clean up afterwards */
	for ( int i = 0; i < separator_number_total; i++){
		current_sep = separators;
	  separators = separators->next;
	  separator_array[i] = current_sep->buf;
		free(current_sep);
	}
	free(separators);

	/* Sort by length of the keywords to avoid problems with shorter strings,
	 * that are total subsets of bigger ones shadowing the latter */
	qsort (separator_array, separator_number_total, sizeof(char *), strlencmp);
	//for( int m = 0; m < separator_number_total; ++m) printf("%d: '%s'", m, separator_array[m]);

	/* search for leading separators and set *tokenstart correctly; if no leading
	 * separators are found, set *tokenstart to 0	*/
	for( unsigned int i = 0; i < buffer_temp->buflen; ++i) {
		led_seppelin=0;
		/* test for ALL the separators */
		for( int m = 0; m < separator_number_total; ++m) {
			/* separator is found */
			if(strncmp((char *) &buffer_temp->buf[i], separator_array[m], strlen(separator_array[m])) == 0) {
				return_value = 1;
				*tokenstart += strlen(separator_array[m]);
				led_seppelin=1;
				/* skip the next strlen(separator_array[m]) chars */
				i += strlen(separator_array[m])-1;
				/* skip the testing of the rest of the separators */
				break;
			}
		}
		if (!led_seppelin) break;
	}
	*fulllength = *tokenstart;
	/* overwritten later if a separator was found */
	*tokenlength = buffer_temp->buflen - *tokenstart;


	/* find the next separator and set *tokenlength */
	for( unsigned int j = *tokenstart; j < buffer_temp->buflen; ++j) {
		for(int n = 0; n < separator_number_total; ++n) {
			//printf("j: %d/%d, n: %d/%d\t", j, buffer_temp->buflen, n, separator_number_total);
			/* separator is found */
			if(strncmp((char *) &buffer_temp->buf[j], separator_array[n], strlen(separator_array[n])) == 0) {
				return_value = 1;
				/* add the length of the found seperator to fulllength */
				*fulllength += strlen(separator_array[n]);
				*tokenlength = j - *tokenstart;
			//	printf("found %s", buffer_temp->buf + *tokenstart);
				/* let the outer loop exit */
				j = buffer_temp->buflen;
				/* let the inner loop exit */
				break;
			}
		}
	}
	*fulllength += *tokenlength;

	/* in case no separator was found */
	if(return_value == 0) {
		*fulllength = buffer_temp->buflen;
	}
	free(separator_array);
	return return_value;
}


/* extracts a token from a given buffer */
int vaToken(struct buffer *buffer_temp, int *tokenstart, int *tokenlength, int *fulllength, ...){
	va_list ap;
	va_start(ap,fulllength);
	return searchToken(tokenstart, tokenlength, fulllength, buffer_temp, ap);
	va_end(ap);
}

int extractToken(struct buffer *buffer_temp, struct buffer *token,
		int tokenstart, int tokenlength, int fulllength){
	/* errors that may occur */
	if(tokenlength > token->bufmax || fulllength > buffer_temp->buflen || 
			tokenlength + tokenstart > fulllength || tokenstart > buffer_temp->buflen){
		return -1;
	}
	/* copy the token from *buffer_temp to *token */
	memcpy((char *)token->buf, &buffer_temp->buf[tokenstart], tokenlength);
	token->buflen = tokenlength;
	token->buf[tokenlength] = '\0';
	/* move the values in buffer_temp so the next token can be searched for */
	memmove(buffer_temp->buf, &buffer_temp->buf[fulllength], buffer_temp->buflen - fulllength+1);
	/* set the length of the buffer lower (fulllength chars were taken away) */
	buffer_temp->buflen -= fulllength;

	return 1;
}

int getTokenFromStream(int fd, struct buffer *buf, struct buffer *token, ...) {
	va_list ap, ap_copy;
	int r, t, retval = 1;
	int ts, tl, fl;
	struct pollfd pollfds[1];
	int pollret;
	
	pollfds[0].fd = fd;
	pollfds[0].events = POLLIN;
	
	va_start(ap, token);
	while(1) {
		va_copy(ap_copy, ap);
		t = searchToken(&ts, &tl, &fl, buf, ap_copy);
		va_end(ap_copy);

		//printf("buf: '%s' token found?=%d\n",buf->buf, t);
		if(t == 1) break;
		/* there won't be a token in this buffer anymore as it's full */
		if(buf->buflen >= buf->bufmax) {
			retval = -1;
			break;
		}
		
		if((pollret = poll(pollfds, 1, -1)) < 0) {
			if(errno == EINTR) continue;
			else {
				retval = -1;
				break;
			}
		} else if (pollfds[0].revents & POLLIN) {
			if( (r = readToBuf(fd, buf)) == -1) {
				retval = -1;
				break;
			} else if (r == 0) {
				retval = 0;
				break;
			}
		} else {
			/* unknown error */
			retval = -1;
			break;
		}
	}
	va_end(ap);
	if(t == 1) if(extractToken(buf, token, ts, tl, fl) == -1) return -1;
	return retval;
}

int getTokenFromBuffer(struct buffer *buf, struct buffer *token, ...) {
	va_list ap;
	int ts, tl, fl;

	/* we just don't care if searchToken thinks it found something or not */
	va_start(ap, token);
	searchToken(&ts, &tl, &fl, buf, ap);
	va_end(ap);
	
	if(tl == 0) return 0;
	else {
		if(extractToken(buf, token, ts, tl, fl) == -1) return -1;
	}
	
	return 1;
}

int getTokenFromStreamBuffer(struct buffer *buf, struct buffer *token, ...) {
	va_list ap;
	int t;
	
	int ts, tl, fl;
	
	va_start(ap, token);
	t = searchToken(&ts, &tl, &fl, buf, ap);
	va_end(ap);
	
	
	if(t == 1) if(extractToken(buf, token, ts, tl, fl) == -1) return -1;
	
	return t;
}

