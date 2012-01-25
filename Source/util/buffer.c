/* needs to be defined before anything else! */
#define _XOPEN_SOURCE 701
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "util.h"

int createBuf(struct buffer *buf, unsigned int maxsize){
	buf->buf = malloc(maxsize);
	if (!buf->buf) return -1;
	memset(buf->buf, '\0', maxsize);
	buf->bufmax = maxsize;
	buf->buflen = 0;
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
	buf->buf = NULL;
}
ssize_t writeWrapper (int fd, const void *bufv, size_t count){
	char * buf = (char *) bufv;
	int countwritten;
	errno = 0;
	while ( ( countwritten = write(fd,buf,count)) < 0){
		if (errno == EINTR || errno == EAGAIN){
			errno =0;
			continue;
		} else return -1;
	}
	return countwritten;
}

ssize_t writeBuf (int fd, struct buffer *buf){
	ssize_t written = writeWrapper(fd, buf->buf, buf->buflen);
	flushBuf(buf);
	return written;
}

ssize_t readToBuf (int fd, struct buffer *buf){
	/* Apparently (reverse engineered - not in Spec) this function is 
	 * assumed to conserve what's written in the buffer and append to
	 * instead of overwrite this content! */
	ssize_t count = buf->bufmax-buf->buflen-1;
	errno=0;
	/* try to read count many bytes */
	count = read (fd, buf->buf + buf->buflen, count);
	if (count ==-1) {
		if (errno == EINTR || errno == EAGAIN) {
			return -2;
		/* shouldn't this be continue instead? */
		} else {perror("Read error in ReadToBuf");return -1;}
	}
	/* see how many are left */
	buf->buflen += count;
	/* Can't overflow because i subtract 1 above */
	buf->buf[buf->buflen] = '\0';
	return count;
}
