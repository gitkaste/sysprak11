#ifndef _util_h
#define _util_h
#include <sys/types.h>

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

ssize_t writeWrapper (int fd, const void *buf, size_t count);
ssize_t readToBuf (int fd, struct buffer *buf);
ssize_t writeBuf (int fd, struct buffer *buf);

/* Unified structure for mem in shared mem segments as well as process local mem
 */
struct array { 
	size_t memsize; /* total memsize */ 
	size_t itemsize;
	unsigned long itemcount;
	/* BUG, Direct quote from the shmat man page: 
	*
	* Be aware that the shared memory segment attached in this way may be attached
	* at different addresses in different processes.  There‐ fore, any pointers
	* maintained within the shared memory must be made relative (typically to the
	* starting address of the segment), rather than absolute.  
	*/
	void *mem;
	int shmid; /* indicates the shared mem segment, -1 for normal arrays */ 
};

struct array *initArray(size_t itemsize, size_t initial_size, int shmid);
void freeArray(struct array *a);
struct array *addArrayItem(struct array *a, void *item);
int remArrayItem(struct array *a, unsigned long num);
void *getArrayItem(struct array *a, unsigned long num);
void *iterateArray(struct array *a, unsigned long *i);

#endif
