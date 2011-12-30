#ifndef _consoler_h
#define _consoler_h


/* consoler
 * consoler wraps around the console, providing a prompt to enter commands and
 * a secure way for multiple processes to print messages.
 * It expects two pipes (infd for reading, outfd for writing) which ties it to
 * other processes - data on infd is printed to the console line-wise (that#
 * means only full lines are printed), data on STDIN_FILENO is written to outfd
 * line-wise. As the writing-side of the infd may be used by multiple processes
 * it needs to be protected by a semaphore. */
int consoler(int infd, int outfd);


/* consolemsg
 * consolemsg just writes a message to consoler via pipefd (the writing-side of
 * consoler's infd) protected by a semaphore. */
int consolemsg(int semid, int pipefd, const char *fmt, ...);



#endif
