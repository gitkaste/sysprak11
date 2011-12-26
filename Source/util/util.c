#include "util.h"
#include <string.h>
#include <stdlib.h>

int createBuf(struct buffer *buf, unsigned int maxsize){
	buf->buf = malloc(maxsize);
	if (!buf->buf) return -1;
	buf->bufmax = maxsize;
	buf->buflen = maxsize;
	return 1;
}

int copyBuf (struct buffer *dest, struct buffer *src){
	dest->buf = (unsigned char *) strndup((char *)src->buf, (ssize_t) src->buflen);
	if (!dest->buf) return -1;
	dest->bufmax = src->bufmax;
	dest->buflen = src->buflen;
	return 1;
}

void flushBuf(struct buffer *buf){
	buf->buf[0] = '\0';
	buf->buflen = 0;
}
void freeBuf(struct buffer *buf){
	buf->bufmax = 0;
	buf->buflen = 0;
	free(buf->buf);
}
