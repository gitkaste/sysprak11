#ifndef _util_h
#define _util_h
#include <sys/types.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdarg.h>
#include "signalfd.h"

/***************** BUFFERS *******************/
struct buffer {
	unsigned char *buf;
	unsigned int bufmax;
	unsigned int buflen;
};

/* initializes the buf structure, with a buffer of maxsize
 * returns -1 on failure, 1 on success */
int createBuf(struct buffer *buf, unsigned int maxsize);
/* copies the contents of one buffer over to another
 * dest needs not be initialized before (unsure if this is correct */
int copyBuf (struct buffer *dest, struct buffer *src);
/* sets the buffer to zero and adjusts lengths */
void flushBuf(struct buffer *buf);
/* returns the memory */ 
void freeBuf(struct buffer *buf);

/* reads the maximum amount of previously unwritten bytes in buf from fd conserving 
 * had been written there before 
 * returns the number of bytes read */
ssize_t readToBuf (int fd, struct buffer *buf);
/* writes buf's content to the file descriptor fd, flushing buf afterwards 
 * returns the amount of bytes written */
ssize_t writeBuf (int fd, struct buffer *buf);
/* internal use only */
ssize_t writeWrapper(int fd, const void *buf, size_t count);
/* works like fprintf but writes directly to an fd 
 * returns amount of bytes printed */
ssize_t writef (int fd, char *fmt, ...);
/* both used internally, build a formatted string from fmt and .../ap 
 * returning an allocated string on success or Null on failure
 * user is responsible for freeing! */
char *stringBuilder (const char *fmt, ...);
char *vStringBuilder (const char *fmt, va_list ap);


/***************** ARRAYS *******************/
/* Unified structure for mem in shared mem segments as well as process local mem
 */
struct array { 
	size_t memsize; /* total memsize */ 
	size_t itemsize;
	unsigned long itemcount;
	/* BUG, Direct quote from the shmat man page: 
	* Be aware that the shared memory segment attached in this way may be attached
	* at different addresses in different processes.  There‐ fore, any pointers
	* maintained within the shared memory must be made relative (typically to the
	* starting address of the segment), rather than absolute.
	*/
	void *mem;
	int shmid; /* indicates the shared mem segment, -1 for normal arrays */ 
};

/* initialize the array, giving you a continous stretch of mem, with the describing struct in the beginning
 * initial_size is exclusive of the size for the struct
 * shmid is the shared mem id or -1 for malloc
 * returns NULL for failure */
struct array *initArray(size_t itemsize, size_t initial_size, int shmid);
/* free the mem associated with the array */
void freeArray(struct array *a);
/* add a new item to the array, possibly resizing in the process, 
 * returns the new (or old) array with the item attached
 * be sure to not overwrite the old variable with the return so you can free still free it */
struct array *addArrayItem(struct array *a, void *item);
/* deletes an item (doesn't resize) */
int remArrayItem(struct array *a, unsigned long num);
void clearArray(struct array *a);
/* works like [] for c arrays */
void *getArrayItem(struct array *a, unsigned long num);
/* an iterator, to be used like int i; while(iterateArray(a,&i)) foo */
void *iterateArray(struct array *a, unsigned long *i);

/* internal representation of child processes */
struct processChild {
	unsigned char type;
	pid_t pid;
};
/* add a child process to cpa, possibly resizing it, returning the new cpa on success
 * Null of failure, the caller is responsible for saving cpa to be able to free it! */
struct array *addChildProcess(struct array *cpa, unsigned char type, pid_t pid);
/* deleted child with pid from cpa, returns 1 or -1 */
int remChildProcess(struct array *cpa, pid_t pid);
/* send signal to all members of cpa of type type, returns 1, -1 */
int sendSignalToChildren(struct array *cpa, unsigned char type, int signal);

/***************** SHARED MEM *******************/
/* functions for creating and deleting shared mem */
int shmCreate(int id);
int shmDelete(int size);
/* find the system max and min for the gross combined shared mem */
int getShmMin();
int getShmMax();
#define IPC_KEY 39471


/***************** SEMAPHORES *******************/
/* functions for creating and deleting shared mem */
#define SEM_LOGGER 0
#define SEM_FILELIST 1
#define SEM_CONSOLER 1
#define SEM_RESULTS 2
/* creates a semaphore group with num semaphores
 * returns a semaphore id on success, -1 otherwise  */
int semCreate(int num);
/* decrement/increment semnum'th semaphore in group with id semid
 * return 0 on succes, -1 otherwise */
int semWait(int semid, int semnum);
int semSignal(int semid, int semnum);
/* get the value of semnum'th semaphore in semid or -1 on error */
int semVal(int semid, int semnum);
/* loose the semaphore group identified by semgroupid */
void semClose(int semgroupid);

/* struct flEntry
 * Represents a file in the network.  */
struct flEntry {
	/* sockaddr contains port information as well */
	char filename[FILENAME_MAX];
	unsigned long size;
	struct sockaddr_storage ip;
};

/* set a file descriptor fd non blocking
 * returns -1 for failure, 1 on success*/ 
int setFdNonblock(int fd);
int setFdBlocking(int fd);

/***************** NETWORK STUFF *******************/
/* prints a string representation of sa into s (upto maxlen)
 * returns s */
char * getipstr(const struct sockaddr *sa, char *s, size_t maxlen);
/* returns a string representation of a, return must not be freed! */
char * putIP (struct sockaddr *a);
/* fills a with ip, port and other information avaiable, 
 * returns 1 on success, otherwise -1 */
int parseIP(char * ip, struct sockaddr *a, char * port, int ipversion);
/* returns the port from a */
int getPort(struct sockaddr *a);
/* sets a's port, returns 1 or -1 */
int setPort(struct sockaddr *a, int port);
/* sets a's ip to ip, ip being a struct in_addr * or a in6_addr *,
 * a->sa_family needs to be set correctly
 * returns 1 or -1 */
int setIP(struct sockaddr *a, void * ip);

/* Own def */
#define ETRAILING 177
/* prints a description of errno into buf, prefixd by pref 
 * returns 1 or -1 */
int sperror(char * pref, char * buf, int buflen);
/* wrapper for strtol using base 10 and setting errno for any error */
int my_strtol(char * buf);
/*  is a string completely made up of space, tabs and newlines */
int iswhitespace(char * buf);
/* is dirname a valid dir? returns 1 for success, 0 otherwise */
int isDir(const char *dirname);
/* is it possible to write to dirname? returns 1 for success, 0 otherwise */
int isDirWritable(const char * dirname);
/* returns 1 if x is a power of 2 0 otherwise */
int isPowerOfTwo (unsigned int x);
/* returns a malloced string, containing a concatenation of basedir and subdir, 
 * or NULL on return */
char *path_join(const char * basedir, const char * subdir);

#endif
