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

/* do the obvious */
int createBuf(struct buffer *buf, unsigned int maxsize);
int copyBuf (struct buffer *dest, struct buffer *src);
void flushBuf(struct buffer *buf);
void freeBuf(struct buffer *buf);

ssize_t readToBuf (int fd, struct buffer *buf);
ssize_t writeBuf (int fd, struct buffer *buf);
ssize_t writeWrapper(int fd, const void *buf, size_t count);
ssize_t writef (int fd, char *fmt, ...);
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
/* works like [] for c arrays */
void *getArrayItem(struct array *a, unsigned long num);
/* an iterator, to be used like int i; while(iterateArray(a,&i)) foo */
void *iterateArray(struct array *a, unsigned long *i);


struct processChild {
	unsigned char type;
	pid_t pid;
};

struct array *addChildProcess(struct array *cpa, unsigned char type, pid_t pid);
int remChildProcess(struct array *cpa, pid_t pid);
int sendSignalToChildren(struct array *cpa, unsigned char type, int sig);

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
int semCreate(int num);
int semWait(int semid, int semnum);
int semSignal(int semid, int semnum);
int semVal(int semid, int semnum);
void semClose(int semgroupid);

/* struct flEntry
 * Represents a file in the network.
*/
struct flEntry {
	struct sockaddr ip;
	char filename[FILENAME_MAX];
	unsigned long size;
};

int setFdNonblock(int fd);

/***************** NETWORK STUFF *******************/
char *getipstr(const struct sockaddr *sa, char *s, size_t maxlen);
void printIP(struct sockaddr *a);
int parseIP(char * ip, struct sockaddr *a, char * port, int ipversion);
int getPort(struct sockaddr *a);
int setPort(struct sockaddr *a, int port);

/* Own def */
#define ETRAILING 177
int sperror(char * pref, char * buf, int buflen);
int my_strtol(char * buf);
int isDir(const char *dirname);
int isDirWritable(const char * dirname);
int isPowerOfTwo (unsigned int x);
char *path_join(const char * basedir, const char * subdir);
#endif
