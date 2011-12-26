#ifndef logger_h
#define logger_h


#define LOGLEVEL_FATAL 1
#define LOGLEVEL_WARN  2
#define LOGLEVEL_VERBOSE 4
#define LOGLEVEL_DEBUG 8


/* logger
 * this is the forked process for logging. It recieves data on pipefd, 
 * and writes them into filefd. It terminates when all writing ends of the pipe
 * are closed. Return values are -1 on failure case and 1 on success.
*/
int logger(int pipefd, int filefd);

/* logmsg
 * This function is used to send a message to the logger process. The pipe 
 * (pipefd) used for this is protected by a semaphore (semid). Messages are
 * filtered by loglevel (see loglevels above) and are assembled in the same
 * way as printf() does. It also attaches a timestamp and the PID of the
 * process. Return values are -1 on failure case and 1 on success.
*/
int logmsg(int semid, int pipefd, int loglevel, const char *fmt, ...);



#endif
