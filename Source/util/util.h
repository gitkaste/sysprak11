#ifndef _util_h
#define _util_h


struct buffer {
	unsigned char *buf;
	unsigned int bufmax;
	unsigned int buflen;
};




/* Buffer Functions
 * createBuf creates a new buffer (including malloc)
 * copyBuf copies the contents from src to dest (overwriting dest)
 * flushBuf empties a buffer (sets buflen to 0)
 * freeBuf frees the malloced memory
 * 
 * returns -1 on error, 1 on success (for non-void functions)
*/
int createBuf(struct buffer *buf, unsigned int maxsize);
int copyBuf (struct buffer *dest, struct buffer *src);
void flushBuf(struct buffer *buf);
void freeBuf(struct buffer *buf);


ssize t writeWrapper (int fd, const void *buf, size t count);
ssize t readToBuf (int fd, struct buffer *buf);
ssize t writeBuf (int fd, struct buffer *buf);

#endif
