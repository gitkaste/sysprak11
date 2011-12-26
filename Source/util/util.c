#include "util.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

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
ssize_t writeWrapper (int fd, const void *buf, size_t count){
	int countwritten;
	int totalcount = 0;
	errno = 0;
	while ((countwritten = write(fd,buf,count)) < count){
		if (errno == EINTR || errno == EAGAIN){
			errno =0;
			continue;
		}
		if (countwritten > -1){
			totalcount += countwritten;
			count -= countwritten;
			buf += countwritten;
			continue;
		} else {
			return -1;
		}
	}
	return totalcount;
}

ssize_t readToBuf (int fd, struct buffer *buf){
	ssize_t count = 64;
	ssize_t countwritten;

 while ( (count = read (fd, buf->buf, buf->bufmax)) <0){
	 if (count == 0) return 
 }
}
ssize_t writeBuf (int fd, struct buffer *buf){
	ssize_t written = writeWrapper(fd, buf->buf, buf->buflen);
	flushBuf(buf);
	return written;
}

